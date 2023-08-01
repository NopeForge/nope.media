/*
 * This file is part of nope.media.
 *
 * Copyright (c) 2023 Matthieu Bouron <matthieu.bouron@gmail.com>
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

#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>

#include "mod_decoding.h"
#include "decoders.h"
#include "internal.h"
#include "log.h"

struct ffdec_context {
    int extra_hw_frames;
};

#if HAVE_MEDIACODEC_HWACCEL
#include <libavcodec/mediacodec.h>
#include <libavutil/hwcontext_mediacodec.h>

static int init_mediacodec(struct decoder_ctx *ctx)
{
    AVCodecContext *avctx = ctx->avctx;

    if (avctx->codec_id != AV_CODEC_ID_H264  &&
        avctx->codec_id != AV_CODEC_ID_HEVC  &&
        avctx->codec_id != AV_CODEC_ID_MPEG4 &&
        avctx->codec_id != AV_CODEC_ID_VP8   &&
        avctx->codec_id != AV_CODEC_ID_VP9)
        return AVERROR_DECODER_NOT_FOUND;

    const char *codec_name = NULL;

    switch (avctx->codec_id) {
    case AV_CODEC_ID_H264:
        codec_name = "h264_mediacodec";
        break;
    case AV_CODEC_ID_HEVC:
        codec_name = "hevc_mediacodec";
        break;
    case AV_CODEC_ID_MPEG4:
        codec_name = "mpeg4_mediacodec";
        break;
    case AV_CODEC_ID_VP8:
        codec_name = "vp8_mediacodec";
        break;
    case AV_CODEC_ID_VP9:
        codec_name = "vp9_mediacodec";
        break;
    default:
        av_assert0(0);
    }

    const AVCodec *codec = avcodec_find_decoder_by_name(codec_name);
    if (!codec)
        return AVERROR_DECODER_NOT_FOUND;

    AVBufferRef *hw_device_ctx_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_MEDIACODEC);
    if (!hw_device_ctx_ref)
        return -1;

    AVHWDeviceContext *hw_device_ctx = (AVHWDeviceContext *)hw_device_ctx_ref->data;
    AVMediaCodecDeviceContext *hw_ctx = hw_device_ctx->hwctx;
    hw_ctx->surface = ctx->opaque;

    int ret = av_hwdevice_ctx_init(hw_device_ctx_ref);
    if (ret < 0) {
        av_buffer_unref(&hw_device_ctx_ref);
        return ret;
    }

    avctx->hw_device_ctx = hw_device_ctx_ref;
    avctx->thread_count = 0;

    AVDictionary *opts = NULL;
    av_dict_set_int(&opts, "delay_flush", 1, 0);

    ret = avcodec_open2(avctx, codec, &opts);
    if (ret < 0) {
        av_buffer_unref(&avctx->hw_device_ctx);
    }
    av_dict_free(&opts);
    return ret;
}
#endif

#if HAVE_VAAPI_HWACCEL
#include <libavutil/hwcontext_vaapi.h>

static enum AVPixelFormat get_format_vaapi(struct AVCodecContext *avctx,
                                           const enum AVPixelFormat *pix_fmts)
{
    struct decoder_ctx *ctx = avctx->opaque;
    struct ffdec_context *ffdec_ctx = ctx->priv_data;

    const enum AVPixelFormat *p = NULL;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(*p);
        if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) {
            break;
        }

        if (*p != AV_PIX_FMT_VAAPI)
            continue;

        AVBufferRef *fctx_ref = NULL;
        int ret = avcodec_get_hw_frames_parameters(ctx->avctx, avctx->hw_device_ctx, *p, &fctx_ref);
        if (ret < 0)
            continue;

        AVHWFramesContext *fctx = (void *)fctx_ref->data;
        if (fctx->initial_pool_size)
            fctx->initial_pool_size += ffdec_ctx->extra_hw_frames;

        ret = av_hwframe_ctx_init(fctx_ref);
        if (ret < 0) {
            av_buffer_unref(&fctx_ref);
            return ret;
        }

        avctx->hw_frames_ctx = fctx_ref;

        break;
    }

    return *p;
}

static int get_extra_hw_frames(const struct nmdi_opts *opts)
{
    /* The maximum number of extra hw frames (in the worst case scenario) is
     * computed from:
     * - The maximum number of frames on flight in the decoding module: max_nb_frames + 1
     * - The maximum number of frames on flight in the filtering module: max_nb_sink + 1
     * - The maximum number of frames on flight in the API module: 2 (one cached, one candidate)
     *   The maximum number of retained frames by the user: 3 (should be enough)
     */
    return opts->max_nb_frames + 1 + opts->max_nb_sink + 1 + 2 + 3;
}

static const struct {
    int profile;
    enum AVCodecID codec_id;
    VAProfile va_profile;
} va_profile_map[] = {
    {FF_PROFILE_H264_CONSTRAINED_BASELINE, AV_CODEC_ID_H264, VAProfileH264ConstrainedBaseline},
    {FF_PROFILE_H264_MAIN,                 AV_CODEC_ID_H264, VAProfileH264Main},
    {FF_PROFILE_H264_HIGH,                 AV_CODEC_ID_H264, VAProfileH264High},
    {FF_PROFILE_H264_HIGH_10,              AV_CODEC_ID_H264, VAProfileH264High10},

    {FF_PROFILE_HEVC_MAIN,                 AV_CODEC_ID_HEVC, VAProfileHEVCMain},
    {FF_PROFILE_HEVC_MAIN_10,              AV_CODEC_ID_HEVC, VAProfileHEVCMain10},
    {FF_PROFILE_HEVC_MAIN_STILL_PICTURE,   AV_CODEC_ID_HEVC, VAProfileHEVCMain},
};

static int is_entrypoint_supported(const VAEntrypoint *entrypoint_list, int num_entrypoints, VAEntrypoint entrypoint)
{
    for (int i = 0; i < num_entrypoints; i++) {
        if (entrypoint_list[i] == entrypoint)
            return 1;
    }
    return 0;
}

static VAProfile av_codec_to_va_profile(enum AVCodecID codec_id, int profile)
{
    for (int i = 0; i < FF_ARRAY_ELEMS(va_profile_map); i++) {
        if (va_profile_map[i].codec_id == codec_id &&
            va_profile_map[i].profile  == profile)
            return va_profile_map[i].va_profile;
    }
    return VAProfileNone;
}

static int is_codec_supported(struct decoder_ctx *ctx)
{
    int ret = 0;
    AVCodecContext *avctx = ctx->avctx;
    AVCodecParameters *codecpar = NULL;
    VADisplay display = ctx->opaque;
    const int max_num_profiles = vaMaxNumProfiles(display);
    const int max_num_entrypoints = vaMaxNumEntrypoints(display);

    VAProfile *profile_list = av_mallocz(max_num_profiles * sizeof(*profile_list));
    VAEntrypoint *entrypoint_list = av_mallocz(max_num_entrypoints * sizeof(*entrypoint_list));

    if (!profile_list || !entrypoint_list) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    int num_profiles = 0;
    VAStatus status = vaQueryConfigProfiles(display, profile_list, &num_profiles);
    if (status != VA_STATUS_SUCCESS) {
        ret = AVERROR_EXTERNAL;
        goto end;
    }

    codecpar = avcodec_parameters_alloc();
    if (!codecpar) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ret = avcodec_parameters_from_context(codecpar, avctx);
    if (ret < 0)
        goto end;

    VAProfile profile = av_codec_to_va_profile(codecpar->codec_id, codecpar->profile);
    if (profile == VAProfileNone) {
        ret = 0;
        goto end;
    }

    int num_entrypoints = 0;
    status = vaQueryConfigEntrypoints(display, profile, entrypoint_list, &num_entrypoints);
    if (status == VA_STATUS_ERROR_UNSUPPORTED_PROFILE) {
        ret = 0;
        goto end;
    }

    if (status != VA_STATUS_SUCCESS) {
        ret = AVERROR_EXTERNAL;
        goto end;
    }

    if (!is_entrypoint_supported(entrypoint_list, num_entrypoints, VAEntrypointVLD)) {
        ret = 0;
        goto end;
    }

    VAConfigID config = VA_INVALID_ID;
    status = vaCreateConfig(display, profile, VAEntrypointVLD, NULL, 0, &config);
    if (status != VA_STATUS_SUCCESS) {
        ret = AVERROR_EXTERNAL;
        goto end;
    }
    vaDestroyConfig(display, config);

    ret = 1;
end:
    av_freep(&profile_list);
    av_freep(&entrypoint_list);
    avcodec_parameters_free(&codecpar);
    return ret;
}

static int init_vaapi(struct decoder_ctx *ctx, const struct nmdi_opts *opts)
{
    AVCodecContext *avctx = ctx->avctx;
    struct ffdec_context *ffdec_ctx = ctx->priv_data;

    if (avctx->codec_id != AV_CODEC_ID_H264 &&
        avctx->codec_id != AV_CODEC_ID_HEVC)
        return AVERROR_DECODER_NOT_FOUND;

    if (!ctx->opaque)
        return AVERROR_DECODER_NOT_FOUND;

    int ret = is_codec_supported(ctx);
    if (ret < 0)
        return ret;
    else if (ret == 0)
        return AVERROR_DECODER_NOT_FOUND;

    ffdec_ctx->extra_hw_frames = get_extra_hw_frames(opts);

    AVBufferRef *hw_device_ctx_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VAAPI);
    if (!hw_device_ctx_ref)
        return -1;

    AVHWDeviceContext *hw_device_ctx = (AVHWDeviceContext *)hw_device_ctx_ref->data;
    AVVAAPIDeviceContext *hw_ctx = hw_device_ctx->hwctx;
    hw_ctx->display = ctx->opaque;

    ret = av_hwdevice_ctx_init(hw_device_ctx_ref);
    if (ret < 0) {
        av_buffer_unref(&hw_device_ctx_ref);
        return ret;
    }

    avctx->hw_device_ctx = hw_device_ctx_ref;
    avctx->thread_count = 1;
    avctx->get_format = get_format_vaapi;
    avctx->opaque = ctx;

    const AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
    ret = avcodec_open2(avctx, codec, NULL);
    if (ret < 0) {
        av_buffer_unref(&avctx->hw_device_ctx);
    }
    return ret;
}
#endif

static int ffdec_init_sw(struct decoder_ctx *ctx, const struct nmdi_opts *opts)
{
    AVCodecContext *avctx = ctx->avctx;
    avctx->thread_count = 0;

    const AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
    return avcodec_open2(avctx, codec, NULL);
}

static int ffdec_init_hw(struct decoder_ctx *ctx, const struct nmdi_opts *opts)
{
#if HAVE_MEDIACODEC_HWACCEL
    return init_mediacodec(ctx);
#elif HAVE_VAAPI_HWACCEL
    return init_vaapi(ctx, opts);
#endif
    return AVERROR_DECODER_NOT_FOUND;
}

static int ffdec_push_packet(struct decoder_ctx *ctx, const AVPacket *pkt)
{
    int ret;
    int pkt_consumed = 0;
    const int pkt_size = pkt ? pkt->size : 0;
    const int flush = !pkt_size;
    AVCodecContext *avctx = ctx->avctx;

    av_assert0(avctx->codec_type == AVMEDIA_TYPE_VIDEO ||
               avctx->codec_type == AVMEDIA_TYPE_AUDIO);

    TRACE(ctx, "Received packet of size %d", pkt_size);

    while (!pkt_consumed) {
        ret = avcodec_send_packet(avctx, pkt);
        if (ret == AVERROR(EAGAIN)) {
            ret = 0;
        } else if (ret < 0) {
            LOG(ctx, ERROR, "Error sending packet to %s decoder: %s",
                av_get_media_type_string(avctx->codec_type),
                av_err2str(ret));
            return ret;
        } else {
            pkt_consumed = 1;
        }

        const int draining = flush && pkt_consumed;
        int64_t next_pts = AV_NOPTS_VALUE;
        while (ret >= 0 || (draining && ret == AVERROR(EAGAIN))) {
            AVFrame *dec_frame = av_frame_alloc();

            if (!dec_frame)
                return AVERROR(ENOMEM);

            ret = avcodec_receive_frame(avctx, dec_frame);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOG(ctx, ERROR, "Error receiving frame from %s decoder: %s",
                    av_get_media_type_string(avctx->codec_type),
                    av_err2str(ret));
                    av_frame_free(&dec_frame);
                return ret;
            }

            if (ret >= 0) {
                /*
                 * If there are multiple frames in the packet, some frames may
                 * not have any PTS but we don't want to drop them.
                 */
                if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                    if (dec_frame->pts == AV_NOPTS_VALUE && next_pts != AV_NOPTS_VALUE)
                        dec_frame->pts = next_pts;
                    else if (next_pts == AV_NOPTS_VALUE)
                        next_pts = dec_frame->pts;
                    if (next_pts != AV_NOPTS_VALUE)
                        next_pts += dec_frame->nb_samples;
                }

                ret = nmdi_decoding_queue_frame(ctx->decoding_ctx, dec_frame);
                if (ret < 0) {
                    TRACE(ctx, "Could not queue frame: %s", av_err2str(ret));
                    av_frame_free(&dec_frame);
                    return ret;
                }
            } else {
                av_frame_free(&dec_frame);
            }
        }
    }

    if (ret == AVERROR(EAGAIN))
        ret = 0;

    if (flush)
        ret = nmdi_decoding_queue_frame(ctx->decoding_ctx, NULL);

    return ret;
}

static void ffdec_flush(struct decoder_ctx *ctx)
{
    AVCodecContext *avctx = ctx->avctx;
    avcodec_flush_buffers(avctx);
}

const struct decoder nmdi_decoder_ffmpeg_sw = {
    .name        = "ffmpeg_sw",
    .init        = ffdec_init_sw,
    .push_packet = ffdec_push_packet,
    .flush       = ffdec_flush,
};

const struct decoder nmdi_decoder_ffmpeg_hw = {
    .name        = "ffmpeg_hw",
    .init        = ffdec_init_hw,
    .push_packet = ffdec_push_packet,
    .flush       = ffdec_flush,
    .priv_data_size = sizeof(struct ffdec_context),
};
