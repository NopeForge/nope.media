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

#ifndef ASYNC_H
#define ASYNC_H

#include <stdint.h>
#include <libavutil/frame.h>

#include "nopemd.h"
#include "opts.h"
#include "msg.h"

const char *nmdi_async_get_msg_type_string(enum msg_type type);

void nmdi_msg_free_data(void *arg);

struct async_context *nmdi_async_alloc_context(void);

int nmdi_async_init(struct async_context *actx, void *log_ctx,
                    const char *filename, const struct nmdi_opts *o);

int nmdi_async_start(struct async_context *actx);

int nmdi_async_fetch_info(struct async_context *actx, struct nmd_info *info);

int nmdi_async_seek(struct async_context *actx, int64_t ts);

int nmdi_async_pop_frame(struct async_context *actx, AVFrame **framep);

int nmdi_async_stop(struct async_context *actx);

int nmdi_nmdi_async_started(struct async_context *actx);

void nmdi_async_free(struct async_context **actxp);

#endif /* ASYNC_H */
