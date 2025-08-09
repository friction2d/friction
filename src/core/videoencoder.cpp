/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
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

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "videoencoder.h"
#include <QByteArray>
#include "Boxes/boxrendercontainer.h"
#include "CacheHandlers/sceneframecontainer.h"
#include "canvas.h"

#define AV_RuntimeThrow(errId, message) \
{ \
    char * const errMsg = new char[AV_ERROR_MAX_STRING_SIZE]; \
    av_make_error_string(errMsg, AV_ERROR_MAX_STRING_SIZE, errId); \
    try { \
        RuntimeThrow(errMsg); \
    } catch(...) { \
        delete[] errMsg; \
        RuntimeThrow(message); \
    } \
}

using namespace Friction::Core;

VideoEncoder *VideoEncoder::sInstance = nullptr;

VideoEncoder::VideoEncoder() {
    Q_ASSERT(!sInstance);
    sInstance = this;
}

void VideoEncoder::addContainer(const stdsptr<SceneFrameContainer>& cont) {
    if(!cont) return;
    mNextContainers.append(cont);
    if(getState() < eTaskState::qued || getState() > eTaskState::processing) queTask();
}

void VideoEncoder::addContainer(const stdsptr<Samples>& cont) {
    if(!cont) return;
    mNextSoundConts.append(cont);
    if(getState() < eTaskState::qued || getState() > eTaskState::processing) queTask();
}

void VideoEncoder::allAudioProvided() {
    mAllAudioProvided = true;
    if(getState() < eTaskState::qued || getState() > eTaskState::processing) queTask();
}

bool VideoEncoder::isValidProfile(const AVCodec *codec,
                                  int profile)
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

static AVFrame *allocPicture(enum AVPixelFormat pix_fmt,
                             const int width, const int height) {
    AVFrame * const picture = av_frame_alloc();
    if(!picture) RuntimeThrow("Could not allocate frame");

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    const int ret = av_frame_get_buffer(picture, 32);
    if(ret < 0) AV_RuntimeThrow(ret, "Could not allocate frame data")

    return picture;
}

static void openVideo(const AVCodec * const codec, OutputStream * const ost) {
    AVCodecContext * const c = ost->fCodec;
    ost->fNextPts = 0;
    /* open the codec */
    int ret = avcodec_open2(c, codec, nullptr);
    if(ret < 0) AV_RuntimeThrow(ret, "Could not open codec")

    /* Allocate the encoded raw picture. */
    ost->fDstFrame = allocPicture(c->pix_fmt, c->width, c->height);
    if(!ost->fDstFrame) RuntimeThrow("Could not allocate picture");

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->fStream->codecpar, c);
    if(ret < 0) AV_RuntimeThrow(ret, "Could not copy the stream parameters")
}

static void addVideoStream(OutputStream * const ost,
                           AVFormatContext * const oc,
                           const OutputSettings &outSettings,
                           const RenderSettings &renSettings) {
    const AVCodec * const codec = outSettings.fVideoCodec;

//    if(!codec) {
//        /* find the video encoder */
//        codec = avcodec_find_encoder(codec_id);
//        if(!codec) {
//            fprintf(stderr, "codec not found\n");
//            return false;
//        }
//    }

    ost->fStream = avformat_new_stream(oc, nullptr);
    if(!ost->fStream) RuntimeThrow("Could not alloc stream");

    AVCodecContext * const c = avcodec_alloc_context3(codec);
    if(!c) RuntimeThrow("Could not alloc an encoding context");

    ost->fCodec = c;

    /* Put sample parameters. */
    c->bit_rate = outSettings.fVideoBitrate;//settings->getVideoBitrate();
    /* Resolution must be a multiple of two. */
    c->width    = renSettings.fVideoWidth;
    c->height   = renSettings.fVideoHeight;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    ost->fStream->time_base = renSettings.fTimeBase;
    c->time_base       = ost->fStream->time_base;

    if (VideoEncoder::isValidProfile(codec,
                                     outSettings.fVideoProfile)) {
        c->profile = outSettings.fVideoProfile;
    }

    for (const auto &opt : outSettings.fVideoOptions.fValues) {
        switch (opt.fType) {
        case FormatType::fTypeCodec:
            av_opt_set(c->priv_data,
                       opt.fKey.toStdString().c_str(),
                       opt.fValue.toStdString().c_str(), 0);
            break;
        case FormatType::fTypeFormat:
            av_opt_set(oc->priv_data,
                       opt.fKey.toStdString().c_str(),
                       opt.fValue.toStdString().c_str(), 0);
            break;
        case FormatType::fTypeMeta:
            av_dict_set(&oc->metadata,
                        opt.fKey.toStdString().c_str(),
                        opt.fValue.toStdString().c_str(), 0);
            break;
        default:;
        }
    }

    c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt       = outSettings.fVideoPixelFormat;//RGBA;
    if(c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        c->max_b_frames = 2;
    } else if(c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
    }
    /* Some formats want stream headers to be separate. */
    if(oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

static AVFrame *getVideoFrame(OutputStream * const ost,
                              const sk_sp<SkImage> &image) {
    AVCodecContext *c = ost->fCodec;

    /* check if we want to generate more frames */
//    if(av_compare_ts(ost->next_pts, c->time_base,
//                      STREAM_DURATION, (AVRational) { 1, 1 }) >= 0)
//        return nullptr;

    /* as we only generate a rgba picture, we must convert it
     * to the codec pixel format if needed */
    if(!ost->fSwsCtx) {
        ost->fSwsCtx = sws_getContext(c->width, c->height,
                                      AV_PIX_FMT_RGBA,
                                      c->width, c->height,
                                      c->pix_fmt, SWS_BICUBIC,
                                      nullptr, nullptr, nullptr);
        if(!ost->fSwsCtx)
            RuntimeThrow("Cannot initialize the conversion context");
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

    const uint8_t * const dstSk[] = {static_cast<uint8_t*>(pixmap.writable_addr())};
    int linesizesSk[4];

    av_image_fill_linesizes(linesizesSk, AV_PIX_FMT_RGBA, image->width());
    const int ret = av_frame_make_writable(ost->fDstFrame) ;
    if(ret < 0) AV_RuntimeThrow(ret, "Could not make AVFrame writable")

    sws_scale(ost->fSwsCtx, dstSk,
              linesizesSk, 0, c->height, ost->fDstFrame->data,
              ost->fDstFrame->linesize);

    ost->fDstFrame->pts = ost->fNextPts++;

    return ost->fDstFrame;
}

static void writeVideoFrame(AVFormatContext * const oc,
                            OutputStream * const ost,
                            const sk_sp<SkImage> &image,
                            bool * const encodeVideo) {
    AVCodecContext * const c = ost->fCodec;

    AVFrame * frame;
    try {
        frame = getVideoFrame(ost, image);
    } catch(...) {
        RuntimeThrow("Failed to retrieve video frame");
    }


    // encode the image
    const int ret = avcodec_send_frame(c, frame);
    if(ret < 0) AV_RuntimeThrow(ret, "Error submitting a frame for encoding")

    while(ret >= 0) {
        AVPacket pkt;
        av_init_packet(&pkt);

        const int recRet = avcodec_receive_packet(c, &pkt);
        if(recRet >= 0) {
            av_packet_rescale_ts(&pkt, c->time_base, ost->fStream->time_base);
            pkt.stream_index = ost->fStream->index;

            // Write the compressed frame to the media file.
            const int interRet = av_interleaved_write_frame(oc, &pkt);
            if(interRet < 0) AV_RuntimeThrow(interRet, "Error while writing video frame")
        } else if(recRet == AVERROR(EAGAIN) || recRet == AVERROR_EOF) {
            *encodeVideo = ret != AVERROR_EOF;
            break;
        } else {
            AV_RuntimeThrow(recRet, "Error encoding a video frame")
        }

        av_packet_unref(&pkt);
    }
}

static void addAudioStream(OutputStream * const ost,
                           AVFormatContext * const oc,
                           const OutputSettings &settings,
                           const eSoundSettingsData& inSound) {
    const AVCodec * const codec = settings.fAudioCodec;

//    /* find the audio encoder */
//    codec = avcodec_find_encoder(codec_id);
//    if(!codec) {
//        fprintf(stderr, "codec not found\n");
//        return false;
//    }

    ost->fStream = avformat_new_stream(oc, nullptr);
    if(!ost->fStream) RuntimeThrow("Could not alloc stream");

    AVCodecContext * const c = avcodec_alloc_context3(codec);
    if(!c) RuntimeThrow("Could not alloc an encoding context");
    ost->fCodec = c;

    /* put sample parameters */
    c->sample_fmt     = settings.fAudioSampleFormat;
    c->sample_rate    = settings.fAudioSampleRate;
    c->channel_layout = settings.fAudioChannelsLayout;
    c->channels       = av_get_channel_layout_nb_channels(c->channel_layout);
    c->bit_rate       = settings.fAudioBitrate;
    c->time_base      = { 1, c->sample_rate };

    ost->fStream->time_base = { 1, c->sample_rate };

    // some formats want stream headers to be separate
    if(oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* initialize sample format conversion;
     * to simplify the code, we always pass the data through lavr, even
     * if the encoder supports the generated format directly -- the price is
     * some extra data copying;
     */

    if(ost->fSwrCtx) swr_free(&ost->fSwrCtx);
    ost->fSwrCtx = swr_alloc();
    if(!ost->fSwrCtx) RuntimeThrow("Error allocating the resampling context");
    av_opt_set_int(ost->fSwrCtx, "in_channel_count",  inSound.channelCount(), 0);
    av_opt_set_int(ost->fSwrCtx, "out_channel_count", c->channels, 0);
    av_opt_set_int(ost->fSwrCtx, "in_channel_layout",  inSound.fChannelLayout, 0);
    av_opt_set_int(ost->fSwrCtx, "out_channel_layout", c->channel_layout, 0);
    av_opt_set_int(ost->fSwrCtx, "in_sample_rate", inSound.fSampleRate, 0);
    av_opt_set_int(ost->fSwrCtx, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->fSwrCtx, "in_sample_fmt", inSound.fSampleFormat, 0);
    av_opt_set_sample_fmt(ost->fSwrCtx, "out_sample_fmt", c->sample_fmt,  0);
    swr_init(ost->fSwrCtx);
    if(!swr_is_initialized(ost->fSwrCtx)) {
        RuntimeThrow("Resampler has not been properly initialized");
    }

#ifdef QT_DEBUG
    qDebug() << "name" << "src" << "output";
    qDebug() << "channels" << inSound.channelCount() <<
                              c->channels;
    qDebug() << "channel layout" << inSound.fChannelLayout <<
                                    c->channel_layout;
    qDebug() << "sample rate" << inSound.fSampleRate <<
                                 c->sample_rate;
    qDebug() << "sample format" << av_get_sample_fmt_name(inSound.fSampleFormat) <<
                                   av_get_sample_fmt_name(c->sample_fmt);
    qDebug() << "bitrate" << settings.fAudioBitrate;
#endif
}

static AVFrame *allocAudioFrame(enum AVSampleFormat sample_fmt,
                                const uint64_t& channel_layout,
                                const int sample_rate,
                                const int nb_samples) {
    AVFrame * const frame = av_frame_alloc();

    if(!frame) RuntimeThrow("Error allocating an audio frame");

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->channels = av_get_channel_layout_nb_channels(channel_layout);
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if(nb_samples) {
        const int ret = av_frame_get_buffer(frame, 0);
        if(ret < 0) AV_RuntimeThrow(ret, "Error allocating an audio buffer")
    }

    return frame;
}

static void openAudio(const AVCodec * const codec, OutputStream * const ost,
                      const eSoundSettingsData& inSound) {
    AVCodecContext * const c = ost->fCodec;

    /* open it */

    const int ret = avcodec_open2(c, codec, nullptr);
    if(ret < 0) AV_RuntimeThrow(ret, "Could not open codec")

    ost->fNextPts = 0;

    const bool varFS = c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE;
    const int nb_samples = varFS ? 10000 : c->frame_size;

    ost->fDstFrame = allocAudioFrame(c->sample_fmt, c->channel_layout,
                                     c->sample_rate, nb_samples);
    if(!ost->fDstFrame) RuntimeThrow("Could not alloc audio frame");

    ost->fSrcFrame = allocAudioFrame(inSound.fSampleFormat,
                                     inSound.fChannelLayout,
                                     inSound.fSampleRate,
                                     nb_samples);
    if(!ost->fSrcFrame) RuntimeThrow("Could not alloc temporary audio frame");

    /* copy the stream parameters to the muxer */
    const int parRet = avcodec_parameters_from_context(ost->fStream->codecpar, c);
    if(parRet < 0) AV_RuntimeThrow(parRet, "Could not copy the stream parameters")
}

/* if a frame is provided, send it to the encoder, otherwise flush the encoder;
 * return 1 when encoding is finished, 0 otherwise
 */
static void encodeAudioFrame(AVFormatContext * const oc,
                             OutputStream * const ost,
                             AVFrame * const frame,
                             bool * const encodeAudio) {
    const int ret = avcodec_send_frame(ost->fCodec, frame);
    if(ret < 0) AV_RuntimeThrow(ret, "Error submitting a frame for encoding")

    while(true) {
        AVPacket pkt;
        av_init_packet(&pkt);

        const int recRet = avcodec_receive_packet(ost->fCodec, &pkt);
        if(recRet >= 0) {
            av_packet_rescale_ts(&pkt, ost->fCodec->time_base, ost->fStream->time_base);
            pkt.stream_index = ost->fStream->index;

            /* Write the compressed frame to the media file. */
            const int interRet = av_interleaved_write_frame(oc, &pkt);
            if(interRet < 0) AV_RuntimeThrow(interRet, "Error while writing audio frame")
        } else if(recRet == AVERROR(EAGAIN) || recRet == AVERROR_EOF) {
            *encodeAudio = recRet == AVERROR(EAGAIN);
            break;
        } else {
            AV_RuntimeThrow(recRet, "Error encoding an audio frame")
        }
    }
}

static void processAudioStream(AVFormatContext * const oc,
                               OutputStream * const ost,
                               SoundIterator &iterator,
                               bool * const audioEnabled) {
    iterator.fillFrame(ost->fSrcFrame);
    bool gotOutput = ost->fSrcFrame;

//    const int nb_samples =
//            swr_convert(ost->fSwrCtx,
//                        ost->fDstFrame->data, ost->fDstFrame->nb_samples,
//                        const_cast<const uint8_t**>(ost->fSrcFrame->data),
//                        ost->fSrcFrame->nb_samples);
//    ost->fDstFrame->nb_samples = nb_samples;
//    ost->fDstFrame->pts = ost->fNextPts;

//    ost->fNextPts += ost->fDstFrame->nb_samples;

    ost->fSrcFrame->pts = ost->fNextPts;
    ost->fNextPts += ost->fSrcFrame->nb_samples;

    try {
        encodeAudioFrame(oc, ost, ost->fSrcFrame, &gotOutput);
    } catch(...) {
        RuntimeThrow("Error while encoding audio frame");
    }

    *audioEnabled = gotOutput;
}

#include "Sound/soundcomposition.h"
void VideoEncoder::startEncodingNow() {
    Q_ASSERT(mRenderInstanceSettings);
    if(!mOutputFormat) {
        mOutputFormat = av_guess_format(nullptr, mPathByteArray.data(), nullptr);
        if(!mOutputFormat) {
            RuntimeThrow("No AVOutputFormat provided. "
                         "Could not guess AVOutputFormat from file extension");
        }
    }
    const auto scene = mRenderInstanceSettings->getTargetCanvas();
    mFormatContext = avformat_alloc_context();
    if(!mFormatContext) RuntimeThrow("Error allocating AVFormatContext");

    mFormatContext->oformat = const_cast<AVOutputFormat*>(mOutputFormat);
    mFormatContext->url = av_strdup(mPathByteArray.constData());

    _mCurrentContainerFrame = 0;
    // add streams
    mAllAudioProvided = false;
    mEncodeVideo = false;
    mEncodeAudio = false;
    if(mOutputSettings.fVideoCodec && mOutputSettings.fVideoEnabled) {
        try {
            addVideoStream(&mVideoStream, mFormatContext,
                           mOutputSettings, mRenderSettings);
        } catch (...) {
            RuntimeThrow("Error adding video stream");
        }
        mEncodeVideo = true;
    }
    const auto soundComp = scene->getSoundComposition();
    if(mOutputFormat->audio_codec != AV_CODEC_ID_NONE &&
       mOutputSettings.fAudioEnabled && soundComp->hasAnySounds()) {
        eSoundSettings::sSave();
        eSoundSettings::sSetSampleRate(mOutputSettings.fAudioSampleRate);
        eSoundSettings::sSetSampleFormat(mOutputSettings.fAudioSampleFormat);
        eSoundSettings::sSetChannelLayout(mOutputSettings.fAudioChannelsLayout);
        mInSoundSettings = eSoundSettings::sData();
        try {
            addAudioStream(&mAudioStream, mFormatContext, mOutputSettings,
                           mInSoundSettings);
        } catch (...) {
            RuntimeThrow("Error adding audio stream");
        }
        mEncodeAudio = true;
    }
    if(!mEncodeAudio && !mEncodeVideo) RuntimeThrow("No streams to render");
    // open streams
    if(mEncodeVideo) {
        try {
            openVideo(mOutputSettings.fVideoCodec, &mVideoStream);
        } catch (...) {
            RuntimeThrow("Error opening video stream");
        }
    }
    if(mEncodeAudio) {
        try {
            openAudio(mOutputSettings.fAudioCodec, &mAudioStream,
                      mInSoundSettings);
        } catch (...) {
            RuntimeThrow("Error opening audio stream");
        }
    }

    //av_dump_format(mFormatContext, 0, mPathByteArray.data(), 1);
    if(!(mOutputFormat->flags & AVFMT_NOFILE)) {
        const int avioRet = avio_open(&mFormatContext->pb,
                                      mPathByteArray.data(),
                                      AVIO_FLAG_WRITE);
        if(avioRet < 0) AV_RuntimeThrow(avioRet,
                                        "Could not open " + mPathByteArray.data())
    }

    const int whRet = avformat_write_header(mFormatContext, nullptr);
    if(whRet < 0) AV_RuntimeThrow(whRet,
                                  "Could not write header to " + mPathByteArray.data())
}

bool VideoEncoder::startEncoding(RenderInstanceSettings * const settings) {
    if(mCurrentlyEncoding) return false;
    mRenderInstanceSettings = settings;
    mRenderInstanceSettings->renderingAboutToStart();
    mOutputSettings = mRenderInstanceSettings->getOutputRenderSettings();
    mRenderSettings = mRenderInstanceSettings->getRenderSettings();
    mPathByteArray = mRenderInstanceSettings->getOutputDestination().toUtf8();

    mOutputFormat = mOutputSettings.fOutputFormat;
    mSoundIterator = SoundIterator();
    try {
        startEncodingNow();
        mCurrentlyEncoding = true;
        mEncodingFinished = false;
        mRenderInstanceSettings->setCurrentState(RenderState::rendering);
        mEmitter.encodingStarted();
        return true;
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
        mRenderInstanceSettings->setCurrentState(RenderState::error, e.what());
        mEmitter.encodingStartFailed();
        return false;
    }
}

void VideoEncoder::interrupEncoding() {
    if(!mCurrentlyEncoding) return;
    mRenderInstanceSettings->setCurrentState(RenderState::none, "Interrupted");
    finishEncodingNow();
    mEmitter.encodingInterrupted();
}

void VideoEncoder::finishEncodingSuccess() {
    mRenderInstanceSettings->setCurrentState(RenderState::finished);
    mEncodingSuccesfull = true;
    finishEncodingNow();
    mEmitter.encodingFinished();
}

static void flushStream(OutputStream * const ost,
                        AVFormatContext * const formatCtx) {
    if(!ost) return;
    if(!ost->fCodec) return;
    AVPacket pkt;
    av_init_packet(&pkt);
    int ret = avcodec_send_frame(ost->fCodec, nullptr);
    while(ret >= 0) {
        ret = avcodec_receive_packet(ost->fCodec, &pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // Write packet
            avcodec_flush_buffers(ost->fCodec);
            break;
        }
        if(pkt.pts != AV_NOPTS_VALUE)
            pkt.pts = av_rescale_q(pkt.pts, ost->fCodec->time_base,
                                   ost->fStream->time_base);
        if(pkt.dts != AV_NOPTS_VALUE)
            pkt.dts = av_rescale_q(pkt.dts, ost->fCodec->time_base,
                                   ost->fStream->time_base);
        if(pkt.duration > 0)
            pkt.duration = av_rescale_q(pkt.duration, ost->fCodec->time_base,
                                        ost->fStream->time_base);
        pkt.stream_index = ost->fStream->index;
        ret = av_interleaved_write_frame(formatCtx, &pkt);
    }
}

static void closeStream(OutputStream * const ost) {
    if(!ost) return;
    if(ost->fCodec) {
        avcodec_close(ost->fCodec);
        avcodec_free_context(&ost->fCodec);
    }
    if(ost->fDstFrame) av_frame_free(&ost->fDstFrame);
    if(ost->fSrcFrame) av_frame_free(&ost->fSrcFrame);
    if(ost->fSwsCtx) sws_freeContext(ost->fSwsCtx);
    if(ost->fSwrCtx) swr_free(&ost->fSwrCtx);
    *ost = OutputStream();
}

void VideoEncoder::finishEncodingNow() {
    if(!mCurrentlyEncoding) return;

    if(mEncodeVideo) flushStream(&mVideoStream, mFormatContext);
    if(mEncodeAudio) flushStream(&mAudioStream, mFormatContext);

    if(mEncodingSuccesfull) av_write_trailer(mFormatContext);

    /* Close each codec. */
    if(mEncodeVideo) closeStream(&mVideoStream);
    if(mEncodeAudio) closeStream(&mAudioStream);

    if(mOutputFormat) {
        if(!(mOutputFormat->flags & AVFMT_NOFILE)) {
            avio_close(mFormatContext->pb);
        }
    } else if(mFormatContext) {
        avio_close(mFormatContext->pb);
    }
    if(mFormatContext) {
        avformat_free_context(mFormatContext);
        mFormatContext = nullptr;
    }

    mEncodeAudio = false;
    mEncodeVideo = false;
    mCurrentlyEncoding = false;
    mEncodingSuccesfull = false;
    mNextContainers.clear();
    mNextSoundConts.clear();
    clearContainers();

    eSoundSettings::sRestore();
}

void VideoEncoder::clearContainers() {
    _mContainers.clear();
    mSoundIterator.clear();
}

void VideoEncoder::process() {
    bool hasVideo = !_mContainers.isEmpty(); // local encode
    bool hasAudio;
    if(mEncodeAudio) {
        if(_mAllAudioProvided) {
            hasAudio = mSoundIterator.hasValue();
        } else {
            hasAudio = mSoundIterator.hasSamples(mAudioStream.fSrcFrame->nb_samples);
        }
    } else hasAudio = false;
    while((mEncodeVideo && hasVideo) || (mEncodeAudio && hasAudio)) {
        bool videoAligned = true;
        if(mEncodeVideo && mEncodeAudio) {
            videoAligned = av_compare_ts(mVideoStream.fNextPts,
                                         mVideoStream.fCodec->time_base,
                                         mAudioStream.fNextPts,
                                         mAudioStream.fCodec->time_base) <= 0;
        }
        const bool encodeVideo = mEncodeVideo && hasVideo && videoAligned;
        if(encodeVideo) {
            const auto cacheCont = _mContainers.at(_mCurrentContainerId);
            const auto contRange = cacheCont->getRange()*_mRenderRange;
            const int nFrames = contRange.span();
            try {
                writeVideoFrame(mFormatContext, &mVideoStream,
                                cacheCont->getImage(), &hasVideo);
                avcodec_flush_buffers(mVideoStream.fCodec);
            } catch(...) {
                RuntimeThrow("Failed to write video frame");
            }
            if(++_mCurrentContainerFrame >= nFrames) {
                _mCurrentContainerId++;
                _mCurrentContainerFrame = 0;
                hasVideo = _mCurrentContainerId < _mContainers.count();
            }
        }
        bool audioAligned = true;
        if(mEncodeVideo && mEncodeAudio) {
            audioAligned = av_compare_ts(mVideoStream.fNextPts,
                                         mVideoStream.fCodec->time_base,
                                         mAudioStream.fNextPts,
                                         mAudioStream.fCodec->time_base) >= 0;
        }
        const bool encodeAudio = mEncodeAudio && hasAudio && audioAligned;
        if(encodeAudio) {
            try {
                processAudioStream(mFormatContext, &mAudioStream,
                                   mSoundIterator, &hasAudio);
                avcodec_flush_buffers(mAudioStream.fCodec);
            } catch(...) {
                RuntimeThrow("Failed to process audio stream");
            }
            hasAudio = _mAllAudioProvided ? mSoundIterator.hasValue() :
                                            mSoundIterator.hasSamples(mAudioStream.fSrcFrame->nb_samples);
        }
        if(!encodeVideo && !encodeAudio) break;
    }
}


void VideoEncoder::beforeProcessing(const Hardware) {
    _mCurrentContainerId = 0;
    _mAllAudioProvided = mAllAudioProvided;
    if(_mContainers.isEmpty()) {
        _mContainers.swap(mNextContainers);
    } else {
        for(const auto& cont : mNextContainers)
            _mContainers.append(cont);
        mNextContainers.clear();
    }
    for(const auto& sound : mNextSoundConts)
        mSoundIterator.add(sound);
    mNextSoundConts.clear();
    _mRenderRange = {mRenderSettings.fMinFrame, mRenderSettings.fMaxFrame};
    if(!mCurrentlyEncoding) clearContainers();
}

void VideoEncoder::afterProcessing() {
    const auto currCanvas = mRenderInstanceSettings->getTargetCanvas();
    if(_mCurrentContainerId != 0) {
        const auto lastEncoded = _mContainers.at(_mCurrentContainerId - 1);
        currCanvas->setSceneFrame(lastEncoded);
        currCanvas->setMinFrameUseRange(lastEncoded->getRange().fMax + 1);
    }

    for(int i = _mContainers.count() - 1; i >= _mCurrentContainerId; i--) {
        const auto &cont = _mContainers.at(i);
        mNextContainers.prepend(cont);
    }
    _mContainers.clear();

    if(mInterruptEncoding) {
        interrupEncoding();
        mInterruptEncoding = false;
    } else if(unhandledException()) {
        gPrintExceptionCritical(takeException());
        mRenderInstanceSettings->setCurrentState(RenderState::error, "Error");
        finishEncodingNow();
        mEmitter.encodingFailed();
    } else if(mEncodingFinished) finishEncodingSuccess();
    else if(!mNextContainers.isEmpty()) queTask();
}

void VideoEncoder::sFinishEncoding() {
    sInstance->finishCurrentEncoding();
}

bool VideoEncoder::sEncodingSuccessfulyStarted() {
    return sInstance->getCurrentlyEncoding();
}

bool VideoEncoder::sEncodeAudio() {
    return sInstance->mEncodeAudio;
}

void VideoEncoder::sInterruptEncoding() {
    sInstance->interruptCurrentEncoding();
}

bool VideoEncoder::sStartEncoding(RenderInstanceSettings *settings) {
    return sInstance->startNewEncoding(settings);
}

void VideoEncoder::sAddCacheContainerToEncoder(const stdsptr<SceneFrameContainer> &cont) {
    sInstance->addContainer(cont);
}

void VideoEncoder::sAddCacheContainerToEncoder(
        const stdsptr<Samples> &cont) {
    sInstance->addContainer(cont);
}

void VideoEncoder::sAllAudioProvided() {
    sInstance->allAudioProvided();
}
