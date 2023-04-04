/*
 * This file is part of nope.media.
 *
 * Copyright (c) 2016 Stupeflix
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

#ifndef LOG_H
#define LOG_H

#include <libavutil/log.h>

#include "nopemd.h"

#ifndef ENABLE_DBG
# define ENABLE_DBG 0
#endif

#if ENABLE_DBG
# define LOG_LEVEL AV_LOG_DEBUG
#else
/* The following will affect the default usage (aka no user logging callback specified) */
# define LOG_LEVEL AV_LOG_ERROR  // will log only errors
//# define LOG_LEVEL AV_LOG_WARNING // will log errors, warnings
//# define LOG_LEVEL AV_LOG_INFO   // will log little information such as file opening and decoder in use
//# define LOG_LEVEL AV_LOG_DEBUG  // will log most of the important actions (get/ret frame)
#endif

#define DO_LOG(c, log_level, ...) nmdi_log_print((c)->log_ctx, log_level, \
                                                 __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG(c, level, ...) DO_LOG(c, NMD_LOG_##level, __VA_ARGS__)

#if ENABLE_DBG
#define TRACE(c, ...) do { DO_LOG(c, NMD_LOG_VERBOSE, __VA_ARGS__); fflush(stdout); } while (0)
#else
/* Note: this could be replaced by a "while(0)" but it wouldn't test the
 * compilation of the printf format, so we use this more complex form. */
#define TRACE(c, ...) do { if (0) DO_LOG(c, NMD_LOG_VERBOSE, __VA_ARGS__); } while (0)
#endif

struct log_ctx;

struct log_ctx *nmdi_log_alloc(void);

int nmdi_log_init(struct log_ctx *ctx, void *avlog);

void nmdi_log_set_callback(struct log_ctx *ctx, void *arg,
                           nmd_log_callback_type callback);

void nmdi_log_print(void *log_ctx, int log_level, const char *filename,
                    int ln, const char *fn, const char *fmt, ...) av_printf_format(6, 7);

void nmdi_log_free(struct log_ctx **ctxp);

#endif /* LOG_H */
