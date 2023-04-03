/*
 * This file is part of nope.media.
 *
 * Copyright (c) 2015 Stupeflix
 *
 * nope.media is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * nope.media is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with nope.media; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdarg.h>
#include <libavutil/mem.h>
#include <libavutil/time.h>

#include "log.h"
#include "pthread_compat.h"

struct log_ctx {
    int64_t last_time;
    pthread_mutex_t lock;
    void *avlog;
    void *user_arg;
    nmd_log_callback_type callback;
};

void nmdi_log_set_callback(struct log_ctx *ctx, void *arg,
                           nmd_log_callback_type callback)
{
    ctx->user_arg = arg;
    ctx->callback = callback;
}

struct log_ctx *nmdi_log_alloc(void)
{
    struct log_ctx *ctx = av_mallocz(sizeof(*ctx));
    if (!ctx)
        return NULL;
    return ctx;
}

static void default_callback(void *arg, int level, const char *filename, int ln,
                             const char *fn, const char *fmt, va_list vl)
{
    char logline[128];
    char *logbuf = NULL;
    const char *logp = logline;
    struct log_ctx *ctx = arg;
    static const int avlog_levels[] = {
        [NMD_LOG_VERBOSE] = AV_LOG_VERBOSE,
        [NMD_LOG_DEBUG]   = AV_LOG_DEBUG,
        [NMD_LOG_INFO]    = AV_LOG_INFO,
        [NMD_LOG_WARNING] = AV_LOG_WARNING,
        [NMD_LOG_ERROR]   = AV_LOG_ERROR,
    };
    const int avlog_level = avlog_levels[level];
    void *avlog = ctx->avlog;

    /* we need a copy because it may be re-used a 2nd time */
    va_list vl_copy;
    va_copy(vl_copy, vl);

    int len = vsnprintf(logline, sizeof(logline), fmt, vl);

    /* handle the case where the line doesn't fit the stack buffer */
    if (len >= sizeof(logline)) {
        logbuf = av_malloc(len + 1);
        if (!logbuf) {
            va_end(vl_copy);
            return;
        }
        vsnprintf(logbuf, len + 1, fmt, vl_copy);
        logp = logbuf;
    }

    if (ENABLE_DBG) {
        int64_t t;
        pthread_mutex_lock(&ctx->lock);
        t = av_gettime();
        if (!ctx->last_time)
            ctx->last_time = t;
        av_log(avlog, avlog_level, "[%f] %s:%d %s: %s\n",
               (t - ctx->last_time) / 1000000.,
               filename, ln, fn, logp);
        ctx->last_time = t;
        pthread_mutex_unlock(&ctx->lock);
    } else {
        av_log(avlog, avlog_level, "%s:%d %s: %s\n",
               filename, ln, fn, logp);
    }

    av_free(logbuf);
    va_end(vl_copy);
}

int nmdi_log_init(struct log_ctx *ctx, void *avlog)
{
    ctx->avlog = avlog;
    nmdi_log_set_callback(ctx, ctx, default_callback);
    return AVERROR(pthread_mutex_init(&ctx->lock, NULL));
}

void nmdi_log_free(struct log_ctx **ctxp)
{
    struct log_ctx *ctx = *ctxp;
    if (!ctx)
        return;
    pthread_mutex_destroy(&ctx->lock);
    av_freep(ctxp);
}

void nmdi_log_print(void *log_ctx, int log_level, const char *filename,
                    int ln, const char *fn, const char *fmt, ...)
{
    va_list vl;
    struct log_ctx *ctx = log_ctx;

    va_start(vl, fmt);
    ctx->callback(ctx->user_arg, log_level, filename, ln, fn, fmt, vl);
    va_end(vl);
}
