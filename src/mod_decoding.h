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

#ifndef MOD_DECODING_H
#define MOD_DECODING_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/threadmessage.h>

#include "opts.h"

struct decoding_ctx *nmdi_decoding_alloc(void);

int nmdi_decoding_init(void *log_ctx,
                       struct decoding_ctx *ctx,
                       AVThreadMessageQueue *pkt_queue,
                       AVThreadMessageQueue *frames_queue,
                       int is_image,
                       const AVStream *stream,
                       const struct nmdi_opts *opts);

const AVCodecContext *nmdi_decoding_get_avctx(struct decoding_ctx *ctx);

int nmdi_decoding_queue_frame(struct decoding_ctx *ctx, AVFrame *frame);

void nmdi_decoding_run(struct decoding_ctx *ctx);

void nmdi_decoding_free(struct decoding_ctx **ctxp);

#endif
