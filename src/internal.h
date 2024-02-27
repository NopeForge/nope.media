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

#ifndef NOPE_MEDIA_INTERNAL_H
#define NOPE_MEDIA_INTERNAL_H

#include <stdio.h>
#include <libavcodec/version.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>

#include "nopemd.h"

#ifdef __ANDROID__
#define HAVE_MEDIACODEC_HWACCEL 1
#else
#define HAVE_MEDIACODEC_HWACCEL 0
#endif

#ifndef HAVE_VAAPI_HWACCEL
#define HAVE_VAAPI_HWACCEL 0
#endif

enum AVPixelFormat nmdi_pix_fmts_nmd2ff(enum nmd_pixel_format pix_fmt);
enum nmd_pixel_format nmdi_pix_fmts_ff2nmd(enum AVPixelFormat pix_fmt);
enum nmd_pixel_format nmdi_smp_fmts_ff2nmd(enum AVSampleFormat smp_fmt);
void nmdi_set_thread_name(const char *name);
void nmdi_update_dimensions(int *width, int *height, int max_pixels);

#define TIME2INT64(d) llrint((d) * av_q2d(av_inv_q(AV_TIME_BASE_Q)))
#define PTS2TIMESTR(t64) av_ts2timestr(t64, &AV_TIME_BASE_Q)

#define NMDI_STATIC_ASSERT(id, c) _Static_assert(c, #id)

#endif
