/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

#include "ffmpeghelper.h"

using namespace Friction::Utils;

int FFmpegHelper::setCodecLayout(AVCodecContext *c,
                                 uint64_t mask)
{
    av_channel_layout_uninit(&c->ch_layout);
    return av_channel_layout_from_mask(&c->ch_layout, mask);
}

int FFmpegHelper::setFrameLayout(AVFrame *frame,
                                 uint64_t mask)
{
    av_channel_layout_uninit(&frame->ch_layout);
    return av_channel_layout_from_mask(&frame->ch_layout, mask);
}

bool FFmpegHelper::compareLayout(const AVChannelLayout &frameLayout,
                                 uint64_t mask)
{
    AVChannelLayout maskLayout;
    if (av_channel_layout_from_mask(&maskLayout, mask) < 0) {
        return false;
    }
    int result = av_channel_layout_compare(&frameLayout, &maskLayout);
    av_channel_layout_uninit(&maskLayout);
    return result == 0;
}

int FFmpegHelper::getNbChannels(uint64_t layoutMask)
{
    if (layoutMask == 0) { return 0; }
    AVChannelLayout layout;
    if (av_channel_layout_from_mask(&layout, layoutMask) < 0) {
        return 0;
    }
    int n = layout.nb_channels;
    av_channel_layout_uninit(&layout);
    return n;
}

uint64_t FFmpegHelper::getLayoutMask(const AVChannelLayout *layout)
{
    if (!layout || layout->order == AV_CHANNEL_ORDER_UNSPEC) {
        return 0;
    }
    if (layout->order == AV_CHANNEL_ORDER_NATIVE) {
        return layout->u.mask;
    }
    return 0;
}

bool FFmpegHelper::isValidProfile(const AVCodec *codec,
                                  const int profile)
{
    if (profile < 0 || !codec) { return false; }
    switch (codec->id) {
    case AV_CODEC_ID_H264:
        switch (profile) {
        case FF_PROFILE_H264_BASELINE:
        case FF_PROFILE_H264_MAIN:
        case FF_PROFILE_H264_HIGH:
            return true;
        default:;
        }
        break;
    case AV_CODEC_ID_PRORES:
        switch (profile) {
        case FF_PROFILE_PRORES_PROXY:
        case FF_PROFILE_PRORES_LT:
        case FF_PROFILE_PRORES_STANDARD:
        case FF_PROFILE_PRORES_HQ:
        case FF_PROFILE_PRORES_4444:
        case FF_PROFILE_PRORES_XQ:
            return true;
        default:;
        }
        break;
    case AV_CODEC_ID_AV1:
        switch (profile) {
        case FF_PROFILE_AV1_MAIN:
        case FF_PROFILE_AV1_HIGH:
        case FF_PROFILE_AV1_PROFESSIONAL:
            return true;
        default:;
        }
        break;
    case AV_CODEC_ID_VP9:
        switch (profile) {
        case FF_PROFILE_VP9_0:
        case FF_PROFILE_VP9_1:
        case FF_PROFILE_VP9_2:
        case FF_PROFILE_VP9_3:
            return true;
        default:;
        }
        break;
    case AV_CODEC_ID_MPEG4:
        switch (profile) {
        case FF_PROFILE_MPEG4_SIMPLE:
        case FF_PROFILE_MPEG4_CORE:
        case FF_PROFILE_MPEG4_MAIN:
            return true;
        default:;
        }
        break;
    case AV_CODEC_ID_VC1:
        switch (profile) {
        case FF_PROFILE_VC1_SIMPLE:
        case FF_PROFILE_VC1_MAIN:
        case FF_PROFILE_VC1_COMPLEX:
        case FF_PROFILE_VC1_ADVANCED:
            return true;
        default:;
        }
        break;
    default:;
    }
    return false;
}

void FFmpegHelper::openVideo(AVFormatContext * const oc,
                             const AVCodec * const codec,
                             OutputStream * const ost)
{
    AVCodecContext * const c = ost->fCodec;
    ost->fNextPts = 0;

    c->time_base = ost->fStream->time_base;
    c->framerate = av_inv_q(c->time_base);

    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (c->pix_fmt == AV_PIX_FMT_NONE) {
        switch (codec->id) {
        case AV_CODEC_ID_PRORES:
            c->pix_fmt = (c->profile == FF_PROFILE_PRORES_4444 ||
                          c->profile == FF_PROFILE_PRORES_XQ)
                             ? AV_PIX_FMT_YUVA444P10 : AV_PIX_FMT_YUV422P10;
            break;
        case AV_CODEC_ID_VP8:
        case AV_CODEC_ID_VP9:
        case AV_CODEC_ID_WEBP:
            c->pix_fmt = AV_PIX_FMT_YUVA420P;
            break;
        default:
            c->pix_fmt = AV_PIX_FMT_YUV420P;
            break;
        }
    }

    int ret = avcodec_open2(c, codec, nullptr);
    if (ret < 0) { AV_RuntimeThrow(ret, "Could not open codec"); }

    ost->fDstFrame = allocPicture(c->pix_fmt,
                                  c->width,
                                  c->height);
    if (!ost->fDstFrame) { RuntimeThrow("Could not allocate picture"); }

    ret = avcodec_parameters_from_context(ost->fStream->codecpar, c);
    if (ret < 0) { AV_RuntimeThrow(ret, "Could not copy the stream parameters"); }
}

void FFmpegHelper::openAudio(const AVCodec * const codec,
                             OutputStream * const ost,
                             const AVSampleFormat sampleFormat,
                             const uint64_t channelLayout,
                             const int sampleRate)
{
    AVCodecContext * const c = ost->fCodec;

    const int ret = avcodec_open2(c, codec, nullptr);
    if (ret < 0) { AV_RuntimeThrow(ret, "Could not open codec"); }

    ost->fNextPts = 0;

    const bool varFS = c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE;
    const int nb_samples = varFS ? 10000 : c->frame_size;

    uint64_t codecMask = getLayoutMask(&c->ch_layout);
    if (codecMask == 0 && c->ch_layout.nb_channels > 0) {
        AVChannelLayout tempLayout;
        av_channel_layout_default(&tempLayout, c->ch_layout.nb_channels);
        codecMask = tempLayout.u.mask;
        av_channel_layout_uninit(&tempLayout);
    }

    ost->fDstFrame = allocAudioFrame(c->sample_fmt,
                                     codecMask,
                                     c->sample_rate,
                                     nb_samples);
    if (!ost->fDstFrame) { RuntimeThrow("Could not alloc audio frame"); }

    ost->fSrcFrame = allocAudioFrame(sampleFormat,
                                     channelLayout,
                                     sampleRate,
                                     nb_samples);
    if (!ost->fSrcFrame) { RuntimeThrow("Could not alloc temporary audio frame"); }

    /* copy the stream parameters to the muxer */
    const int parRet = avcodec_parameters_from_context(ost->fStream->codecpar, c);
    if (parRet < 0) { AV_RuntimeThrow(parRet, "Could not copy the stream parameters"); }
}

AVFrame *FFmpegHelper::allocAudioFrame(AVSampleFormat sample_fmt,
                                       const uint64_t &channel_layout,
                                       const int sample_rate,
                                       const int nb_samples)
{
    AVFrame* frame = av_frame_alloc();

    if (!frame) { RuntimeThrow("Error allocating an audio frame"); }

    frame->format = sample_fmt;
    setFrameLayout(frame, channel_layout);
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        const int ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            av_frame_free(&frame);
            AV_RuntimeThrow(ret, "Error allocating an audio buffer");
        }
    }

    return frame;
}

void FFmpegHelper::encodeAudioFrame(AVFormatContext * const oc,
                                    OutputStream * const ost,
                                    AVFrame * const frame,
                                    bool * const encodeAudio)
{
    const int ret = avcodec_send_frame(ost->fCodec, frame);
    if (ret < 0) { AV_RuntimeThrow(ret, "Error submitting a frame for encoding"); }

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) { RuntimeThrow("Could not allocate audio AVPacket"); }

    while (true) {
        av_packet_unref(pkt);
        const int recRet = avcodec_receive_packet(ost->fCodec, pkt);
        if (recRet >= 0) {
            av_packet_rescale_ts(pkt, ost->fCodec->time_base, ost->fStream->time_base);
            pkt->stream_index = ost->fStream->index;

            const int interRet = av_interleaved_write_frame(oc, pkt);
            if (interRet < 0) {
                av_packet_free(&pkt);
                AV_RuntimeThrow(interRet, "Error while writing audio frame");
            }
        } else if (recRet == AVERROR(EAGAIN) || recRet == AVERROR_EOF) {
            *encodeAudio = (recRet == AVERROR(EAGAIN));
            break;
        } else {
            av_packet_free(&pkt);
            AV_RuntimeThrow(recRet, "Error encoding an audio frame");
        }
    }

    av_packet_free(&pkt);
}

AVFrame* FFmpegHelper::allocPicture(AVPixelFormat pix_fmt,
                                    const int width,
                                    const int height)
{
    AVFrame* picture = av_frame_alloc();
    if (!picture) { RuntimeThrow("Could not allocate frame"); }

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    const int ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        av_frame_free(&picture);
        AV_RuntimeThrow(ret, "Could not allocate frame data");
    }

    return picture;
}

AVFrame *FFmpegHelper::getVideoFrame(OutputStream * const ost,
                                     const sk_sp<SkImage> &image)
{
    AVCodecContext *c = ost->fCodec;

    // as we only generate a rgba picture, we must convert it
    // to the codec pixel format if needed
    if (!ost->fSwsCtx) {
        ost->fSwsCtx = sws_getContext(c->width,
                                      c->height,
                                      AV_PIX_FMT_RGBA,
                                      c->width,
                                      c->height,
                                      c->pix_fmt,
                                      SWS_BICUBIC,
                                      nullptr,
                                      nullptr,
                                      nullptr);
        if (!ost->fSwsCtx) {
            RuntimeThrow("Cannot initialize the conversion context");
        }
    }

    SkPixmap pixmap;
    image->peekPixels(&pixmap);

    // check if we need to convert to "unpremultiplied"
    const bool unpremul = c->codec_id == AV_CODEC_ID_PNG; // for now only check for PNG
    if (unpremul) {
        SkImageInfo unpremulInfo = SkImageInfo::Make(pixmap.width(),
                                                     pixmap.height(),
                                                     kRGBA_8888_SkColorType,
                                                     kUnpremul_SkAlphaType,
                                                     pixmap.info().refColorSpace());
        SkBitmap unpremulBitmap;
        if (unpremulBitmap.tryAllocPixels(unpremulInfo)) {
            const bool converted = image->readPixels(unpremulInfo,
                                                     unpremulBitmap.getPixels(),
                                                     unpremulBitmap.rowBytes(),
                                                     0, 0);
            if (converted) { unpremulBitmap.peekPixels(&pixmap); }
        }
    }

    const uint8_t* srcData[] = { static_cast<const uint8_t*>(pixmap.addr()) };
    int srcLinesizes[] = { static_cast<int>(pixmap.rowBytes()), 0, 0, 0 };

    const int ret = av_frame_make_writable(ost->fDstFrame);
    if (ret < 0) { AV_RuntimeThrow(ret, "Could not make AVFrame writable"); }

    sws_scale(ost->fSwsCtx,
              srcData,
              srcLinesizes,
              0,
              c->height,
              ost->fDstFrame->data,
              ost->fDstFrame->linesize);

    ost->fDstFrame->pts = ost->fNextPts++;

    return ost->fDstFrame;
}

void FFmpegHelper::writeVideoFrame(AVFormatContext * const oc,
                                   OutputStream * const ost,
                                   const sk_sp<SkImage> &image,
                                   bool * const encodeVideo)
{
    AVCodecContext * const c = ost->fCodec;

    AVFrame * frame = nullptr;
    try {
        frame = getVideoFrame(ost, image);
    } catch (...) {
        RuntimeThrow("Failed to retrieve video frame");
    }

    const int sendRet = avcodec_send_frame(c, frame);
    if (sendRet < 0) { AV_RuntimeThrow(sendRet, "Error submitting a frame for encoding"); }

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) { RuntimeThrow("Could not allocate AVPacket"); }

    while (true) {
        av_packet_unref(pkt);
        const int recRet = avcodec_receive_packet(c, pkt);
        if (recRet == 0) {
            av_packet_rescale_ts(pkt, c->time_base, ost->fStream->time_base);

            if (ost->fFrameDuration <= 0) {
                AVRational frameBase;
                if (ost->fStream->avg_frame_rate.num > 0 &&
                    ost->fStream->avg_frame_rate.den > 0) {
                    frameBase = av_inv_q(ost->fStream->avg_frame_rate);
                } else {
                    frameBase = c->time_base;
                }

                ost->fFrameDuration = av_rescale_q(1, frameBase,
                                                   ost->fStream->time_base);
                if (ost->fFrameDuration <= 0) { ost->fFrameDuration = 1; }
            }

            pkt->duration = ost->fFrameDuration;
            pkt->stream_index = ost->fStream->index;

            const int interRet = av_interleaved_write_frame(oc, pkt);
            if (interRet < 0) {
                av_packet_free(&pkt);
                AV_RuntimeThrow(interRet, "Error while writing video frame");
            }
        } else if (recRet == AVERROR(EAGAIN) || recRet == AVERROR_EOF) {
            *encodeVideo = (recRet != AVERROR_EOF);
            break;
        } else {
            av_packet_free(&pkt);
            AV_RuntimeThrow(recRet, "Error encoding a video frame");
        }
    }

    av_packet_free(&pkt);
}

void FFmpegHelper::flushStream(OutputStream * const ost,
                               AVFormatContext * const formatCtx)
{
    if (!ost || !ost->fCodec) { return; }

    int sendRet = avcodec_send_frame(ost->fCodec, nullptr);
    if (sendRet < 0 && sendRet != AVERROR_EOF) { return; }

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) { RuntimeThrow("Could not allocate AVPacket for flushing"); }

    while (true) {
        av_packet_unref(pkt);

        int ret = avcodec_receive_packet(ost->fCodec, pkt);

        if (ret == AVERROR_EOF) {
            // encoder is empty
            break;
        } else if (ret == AVERROR(EAGAIN)) {
            // should not happen
            break;
        } else if (ret < 0) {
            av_packet_free(&pkt);
            AV_RuntimeThrow(ret, "Error encoding a frame during flush");
        }

        if (pkt->pts != AV_NOPTS_VALUE) {
            pkt->pts = av_rescale_q(pkt->pts,
                                    ost->fCodec->time_base,
                                    ost->fStream->time_base);
        }
        if (pkt->dts != AV_NOPTS_VALUE) {
            pkt->dts = av_rescale_q(pkt->dts,
                                    ost->fCodec->time_base,
                                    ost->fStream->time_base);
        }

        if (ost->fFrameDuration <= 0) {
            AVRational frameBase;
            if (ost->fStream->avg_frame_rate.num > 0 &&
                ost->fStream->avg_frame_rate.den > 0) {
                frameBase = av_inv_q(ost->fStream->avg_frame_rate);
            } else {
                frameBase = ost->fCodec->time_base;
            }

            ost->fFrameDuration = av_rescale_q(1, frameBase,
                                               ost->fStream->time_base);
            if (ost->fFrameDuration <= 0) { ost->fFrameDuration = 1; }
        }

        pkt->duration = ost->fFrameDuration;
        pkt->stream_index = ost->fStream->index;

        int interRet = av_interleaved_write_frame(formatCtx, pkt);
        if (interRet < 0) {
            av_packet_free(&pkt);
            AV_RuntimeThrow(interRet, "Error while writing flushed frame");
        }
    }

    av_packet_free(&pkt);
}

void FFmpegHelper::closeStream(OutputStream * const ost)
{
    if (!ost) { return; }
    if (ost->fCodec) {
        avcodec_free_context(&ost->fCodec);
    }
    if (ost->fDstFrame) { av_frame_free(&ost->fDstFrame); }
    if (ost->fSrcFrame) { av_frame_free(&ost->fSrcFrame); }
    if (ost->fSwsCtx) { sws_freeContext(ost->fSwsCtx); }
    if (ost->fSwrCtx) { swr_free(&ost->fSwrCtx); }

    *ost = OutputStream();
}

QList<FFmpegHelper::ChannelLayoutInfo>
FFmpegHelper::getSupportedLayouts(const AVCodec *codec)
{
    QList<ChannelLayoutInfo> list;
    if (!codec) { return  list; }
    const AVChannelLayout* layouts = codec->ch_layouts;
    if (!layouts) {
        list.append({AV_CH_LAYOUT_MONO, "Mono"});
        list.append({AV_CH_LAYOUT_STEREO, "Stereo"});
    } else {
        for (int i = 0; layouts[i].nb_channels > 0; i++) {
            uint64_t mask = 0;
            if (layouts[i].order == AV_CHANNEL_ORDER_NATIVE) {
                mask = layouts[i].u.mask;
            } else {
                AVChannelLayout temp;
                av_channel_layout_default(&temp, layouts[i].nb_channels);
                mask = temp.u.mask;
                av_channel_layout_uninit(&temp);
            }
            if (mask > 0) {
                char buf[64];
                av_channel_layout_describe(&layouts[i], buf, sizeof(buf));
                QString layoutName = QString::fromUtf8(buf);
                if (!layoutName.isEmpty()) {
                    layoutName[0] = layoutName[0].toUpper();
                }
                ChannelLayoutInfo info{mask, layoutName};
                list.append(info);
            }
        }
    }
    return list;
}
