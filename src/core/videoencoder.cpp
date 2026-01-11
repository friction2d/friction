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
#include "CacheHandlers/sceneframecontainer.h"
#include "canvas.h"
#include "Sound/soundcomposition.h"

using namespace Friction::Core;
using namespace Friction::Utils;

VideoEncoder *VideoEncoder::sInstance = nullptr;

bool SoundIterator::hasValue() const
{
    return !mSamples.isEmpty();
}

bool SoundIterator::hasSamples(const int samples) const
{
    if (mSamples.isEmpty()) { return false; }
    int rem = samples - (mEndSample - mCurrentSample);
    if (rem <= 0) { return true; }
    for (int i = 1 ; i < mSamples.count(); i++) {
        rem -= mSamples.at(i)->fSampleRange.span();
        if (rem <= 0) { return true; }
    }
    return false;
}

void SoundIterator::fillFrame(AVFrame * const frame)
{
    Q_ASSERT(Friction::Utils::FFmpegHelper::compareLayout(frame->ch_layout,
                                                          mCurrentSamples->fChannelLayout));
    Q_ASSERT(frame->format == mCurrentSamples->fFormat);
    Q_ASSERT(frame->sample_rate == mCurrentSamples->fSampleRate);

    const int nChannels = static_cast<int>(mCurrentSamples->fNChannels);
    const int sampleSize = int(mCurrentSamples->fSampleSize);
    int remaining = frame->nb_samples;
    int frameSample = 0;

    if (mCurrentSamples->fPlanar) {
        while (remaining > 0) {
            const int cpySamples = qMin(remaining,
                                        mEndSample - mCurrentSample + 1);
            const uint cpyBytes = static_cast<uint>(cpySamples*sampleSize);
            for (int j = 0; j < nChannels; j++) {
                memcpy(frame->data[j] + frameSample*sampleSize,
                       mCurrentData[j] + mCurrentSample*sampleSize, cpyBytes);
            }

            remaining -= cpySamples;
            mCurrentSample += cpySamples;
            frameSample += cpySamples;
            if (mCurrentSample > mEndSample) {
                if (!next()) {
                    frame->nb_samples = frameSample;
                    return;
                }
            }
        }
    } else {
        while (remaining > 0) {
            const int cpySamples = qMin(remaining,
                                        mEndSample - mCurrentSample + 1)*nChannels;
            const uint cpyBytes = static_cast<uint>(cpySamples*sampleSize);
            memcpy(frame->data[0] + frameSample*sampleSize*nChannels,
                   mCurrentData[0] + mCurrentSample*sampleSize*nChannels, cpyBytes);

            remaining -= cpySamples;
            mCurrentSample += cpySamples;
            frameSample += cpySamples;
            if (mCurrentSample > mEndSample) {
                if (!next()) {
                    frame->nb_samples = frameSample;
                    return;
                }
            }
        }
    }
}

bool SoundIterator::next()
{
    if (mSamples.isEmpty()) { return false; }
    mSamples.removeFirst();
    if (!updateCurrent()) { return false; }
    return true;
}

void SoundIterator::add(const stdsptr<Samples> &sound)
{
    mSamples << sound;
    if (mSamples.count() == 1) { updateCurrent(); }
}

void SoundIterator::clear()
{
    mSamples.clear();
    updateCurrent();
}

bool SoundIterator::updateCurrent()
{
    if (mSamples.isEmpty()) {
        mCurrentSamples = nullptr;
        mCurrentData = nullptr;
        mCurrentSample = 0;
        mEndSample = 0;
        return false;
    }
    mCurrentSamples = mSamples.first().get();
    mCurrentData = mCurrentSamples->fData;
    const auto samplesRange = mCurrentSamples->fSampleRange;
    mCurrentSample = 0;
    mEndSample = samplesRange.fMax - samplesRange.fMin;
    return true;
}

VideoEncoder::VideoEncoder()
{
    Q_ASSERT(!sInstance);
    sInstance = this;
}

void VideoEncoder::addContainer(const stdsptr<SceneFrameContainer>& cont)
{
    if (!cont) { return; }
    mNextContainers.append(cont);
    if (getState() < eTaskState::qued ||
        getState() > eTaskState::processing) { queTask(); }
}

void VideoEncoder::addContainer(const stdsptr<Samples>& cont)
{
    if (!cont) { return; }
    mNextSoundConts.append(cont);
    if (getState() < eTaskState::qued ||
        getState() > eTaskState::processing) { queTask(); }
}

void VideoEncoder::allAudioProvided()
{
    mAllAudioProvided = true;
    if (getState() < eTaskState::qued ||
        getState() > eTaskState::processing) { queTask(); }
}

static void addVideoStream(FFmpegHelper::OutputStream * const ost,
                           AVFormatContext * const oc,
                           const OutputSettings &outSettings,
                           const RenderSettings &renSettings)
{
    const AVCodec * const codec = outSettings.fVideoCodec;

    ost->fStream = avformat_new_stream(oc, nullptr);
    if (!ost->fStream) { RuntimeThrow("Could not alloc stream"); }

    AVCodecContext * const c = avcodec_alloc_context3(codec);
    if (!c) { RuntimeThrow("Could not alloc an encoding context"); }

    ost->fCodec = c;

    /* Put sample parameters. */
    c->bit_rate = outSettings.fVideoBitrate;
    /* Resolution must be a multiple of two. */
    c->width    = renSettings.fVideoWidth;
    c->height   = renSettings.fVideoHeight;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    ost->fStream->time_base = renSettings.fTimeBase;

    const AVRational targetFps = av_inv_q(renSettings.fTimeBase);
    if (targetFps.num > 0 && targetFps.den > 0) {
        ost->fStream->avg_frame_rate = targetFps;
        ost->fStream->r_frame_rate = targetFps;
        c->framerate = targetFps;
    }

    c->time_base       = ost->fStream->time_base;

    if (FFmpegHelper::isValidProfile(codec,
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
    c->pix_fmt       = outSettings.fVideoPixelFormat; // RGBA;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        c->max_b_frames = 2;
    } else if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
    }
    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

static void addAudioStream(FFmpegHelper::OutputStream * const ost,
                           AVFormatContext * const oc,
                           const OutputSettings &settings,
                           const eSoundSettingsData& inSound)
{
    const AVCodec * const codec = settings.fAudioCodec;

    ost->fStream = avformat_new_stream(oc, nullptr);
    if (!ost->fStream) { RuntimeThrow("Could not alloc stream"); }

    AVCodecContext * const c = avcodec_alloc_context3(codec);
    if (!c) { RuntimeThrow("Could not alloc an encoding context"); }

    ost->fCodec = c;

    c->sample_fmt     = settings.fAudioSampleFormat;
    c->sample_rate    = settings.fAudioSampleRate;
    FFmpegHelper::setCodecLayout(c, settings.fAudioChannelsLayout);
    c->bit_rate       = settings.fAudioBitrate;
    c->time_base      = { 1, c->sample_rate };

    ost->fStream->time_base = { 1, c->sample_rate };

    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (ost->fSwrCtx) { swr_free(&ost->fSwrCtx); }

    AVChannelLayout in_layout;
    av_channel_layout_from_mask(&in_layout,
                                inSound.fChannelLayout);

    int ret = swr_alloc_set_opts2(&ost->fSwrCtx,
                                  &c->ch_layout,
                                  c->sample_fmt,
                                  c->sample_rate,
                                  &in_layout,
                                  inSound.fSampleFormat,
                                  inSound.fSampleRate,
                                  0, nullptr);

    av_channel_layout_uninit(&in_layout);

    if (ret < 0) {
        AV_RuntimeThrow(ret, "Failed to allocate audio resampler context");
    }

    ret = swr_init(ost->fSwrCtx);
    if (ret < 0) {
        AV_RuntimeThrow(ret, "Resampler initialization failed");
    }

#ifdef QT_DEBUG
    int codecChannels = c->ch_layout.nb_channels;
    char layoutBuf[64];
    av_channel_layout_describe(&c->ch_layout, layoutBuf,
                               sizeof(layoutBuf));

    qDebug() << "--- Audio Debug ---";
    qDebug() << "Channels:" << "src:" << inSound.channelCount()
             << "codec:" << codecChannels;

    qDebug() << "Layout:" << "src mask:" << inSound.fChannelLayout
             << "codec desc:" << layoutBuf;

    qDebug() << "Sample rate:" << "src:" << inSound.fSampleRate
             << "codec:" << c->sample_rate;

    qDebug() << "Sample format:" << "src:" << av_get_sample_fmt_name(inSound.fSampleFormat)
             << "codec:" << av_get_sample_fmt_name(c->sample_fmt);
    qDebug() << "Bitrate:" << c->bit_rate;
#endif
}

static void processAudioStream(AVFormatContext * const oc,
                               FFmpegHelper::OutputStream * const ost,
                               SoundIterator &iterator,
                               bool * const audioEnabled)
{
    iterator.fillFrame(ost->fSrcFrame);
    bool gotOutput = ost->fSrcFrame;

    ost->fSrcFrame->pts = ost->fNextPts;
    ost->fNextPts += ost->fSrcFrame->nb_samples;

    try {
        FFmpegHelper::encodeAudioFrame(oc,
                                       ost,
                                       ost->fSrcFrame,
                                       &gotOutput);
    } catch(...) {
        RuntimeThrow("Error while encoding audio frame");
    }

    *audioEnabled = gotOutput;
}

void VideoEncoder::startEncodingNow()
{
    Q_ASSERT(mRenderInstanceSettings);

    if (!mOutputFormat) {
        mOutputFormat = av_guess_format(nullptr,
                                        mPathByteArray.data(),
                                        nullptr);
        if (!mOutputFormat) {
            RuntimeThrow("No AVOutputFormat provided. "
                         "Could not guess AVOutputFormat from file extension");
        }
    }

    const auto scene = mRenderInstanceSettings->getTargetCanvas();
    mFormatContext = avformat_alloc_context();
    if (!mFormatContext) { RuntimeThrow("Error allocating AVFormatContext"); }

    mFormatContext->oformat = const_cast<AVOutputFormat*>(mOutputFormat);
    mFormatContext->url = av_strdup(mPathByteArray.constData());

    _mCurrentContainerFrame = 0;

    // add streams
    mAllAudioProvided = false;
    mEncodeVideo = false;
    mEncodeAudio = false;

    if (mOutputSettings.fVideoCodec &&
        mOutputSettings.fVideoEnabled) {
        try {
            addVideoStream(&mVideoStream,
                           mFormatContext,
                           mOutputSettings,
                           mRenderSettings);
        } catch (...) {
            RuntimeThrow("Error adding video stream");
        }
        mEncodeVideo = true;
    }

    const auto soundComp = scene->getSoundComposition();
    if (mOutputFormat->audio_codec != AV_CODEC_ID_NONE &&
       mOutputSettings.fAudioEnabled && soundComp->hasAnySounds()) {
        eSoundSettings::sSave();
        eSoundSettings::sSetSampleRate(mOutputSettings.fAudioSampleRate);
        eSoundSettings::sSetSampleFormat(mOutputSettings.fAudioSampleFormat);
        eSoundSettings::sSetChannelLayout(mOutputSettings.fAudioChannelsLayout);
        mInSoundSettings = eSoundSettings::sData();
        try {
            addAudioStream(&mAudioStream,
                           mFormatContext,
                           mOutputSettings,
                           mInSoundSettings);
        } catch (...) {
            RuntimeThrow("Error adding audio stream");
        }
        mEncodeAudio = true;
    }

    if (!mEncodeAudio && !mEncodeVideo) { RuntimeThrow("No streams to render"); }

    // open streams
    if (mEncodeVideo) {
        try {
            FFmpegHelper::openVideo(mFormatContext,
                                    mOutputSettings.fVideoCodec,
                                    &mVideoStream);
        } catch (...) {
            RuntimeThrow("Error opening video stream");
        }
    }
    if (mEncodeAudio) {
        try {
            FFmpegHelper::openAudio(mOutputSettings.fAudioCodec,
                                    &mAudioStream,
                                    mInSoundSettings.fSampleFormat,
                                    mInSoundSettings.fChannelLayout,
                                    mInSoundSettings.fSampleRate);
        } catch (...) {
            RuntimeThrow("Error opening audio stream");
        }
    }

    if (!(mOutputFormat->flags & AVFMT_NOFILE)) {
        const int avioRet = avio_open(&mFormatContext->pb,
                                      mPathByteArray.data(),
                                      AVIO_FLAG_WRITE);
        if (avioRet < 0) {
            AV_RuntimeThrow(avioRet,
                            "Could not open " + mPathByteArray.data());
        }
    }

    const int whRet = avformat_write_header(mFormatContext, nullptr);
    if (whRet < 0) {
        AV_RuntimeThrow(whRet,
                        "Could not write header to " + mPathByteArray.data());
    }
}

bool VideoEncoder::startEncoding(RenderInstanceSettings * const settings)
{
    if (mCurrentlyEncoding) { return false; }

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
    } catch (const std::exception& e) {
        gPrintExceptionCritical(e);
        mRenderInstanceSettings->setCurrentState(RenderState::error, e.what());
        mEmitter.encodingStartFailed();
        return false;
    }
}

void VideoEncoder::interrupEncoding()
{
    if (!mCurrentlyEncoding) { return; }
    mRenderInstanceSettings->setCurrentState(RenderState::none, "Interrupted");
    finishEncodingNow();
    mEmitter.encodingInterrupted();
}

void VideoEncoder::finishEncodingSuccess()
{
    mRenderInstanceSettings->setCurrentState(RenderState::finished);
    mEncodingSuccesfull = true;
    finishEncodingNow();
    mEmitter.encodingFinished();
}

void VideoEncoder::finishEncodingNow()
{
    if (!mCurrentlyEncoding) { return; }

    if (mEncodeVideo) {
        FFmpegHelper::flushStream(&mVideoStream,
                                  mFormatContext);
    }
    if (mEncodeAudio) {
        FFmpegHelper::flushStream(&mAudioStream,
                                  mFormatContext);
    }

    // set the number of frames in the video stream
    if (mEncodeVideo &&
        mVideoStream.fStream &&
        mVideoStream.fCodec) {
        mVideoStream.fStream->nb_frames = mVideoStream.fNextPts;
        const AVRational fps = av_inv_q(mRenderSettings.fTimeBase);
        if (fps.num > 0 && fps.den > 0) {
            mVideoStream.fStream->avg_frame_rate = fps;
            mVideoStream.fStream->r_frame_rate = fps;
        }
    }

    if (mEncodingSuccesfull) { av_write_trailer(mFormatContext); }

    // close each codec
    if (mEncodeVideo) {
        FFmpegHelper::closeStream(&mVideoStream);
    }
    if (mEncodeAudio) {
        FFmpegHelper::closeStream(&mAudioStream);
    }

    if (mOutputFormat) {
        if (!(mOutputFormat->flags & AVFMT_NOFILE)) {
            avio_close(mFormatContext->pb);
        }
    } else if (mFormatContext) {
        avio_close(mFormatContext->pb);
    }
    if (mFormatContext) {
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

void VideoEncoder::clearContainers()
{
    _mContainers.clear();
    mSoundIterator.clear();
}

void VideoEncoder::process()
{
    bool hasVideo = !_mContainers.isEmpty(); // local encode
    bool hasAudio;
    if (mEncodeAudio) {
        if (_mAllAudioProvided) {
            hasAudio = mSoundIterator.hasValue();
        } else {
            hasAudio = mSoundIterator.hasSamples(mAudioStream.fSrcFrame->nb_samples);
        }
    } else { hasAudio = false; }

    while ((mEncodeVideo && hasVideo) ||
           (mEncodeAudio && hasAudio)) {
        bool videoAligned = true;
        if (mEncodeVideo && mEncodeAudio) {
            videoAligned = av_compare_ts(mVideoStream.fNextPts,
                                         mVideoStream.fCodec->time_base,
                                         mAudioStream.fNextPts,
                                         mAudioStream.fCodec->time_base) <= 0;
        }
        const bool encodeVideo = mEncodeVideo && hasVideo && videoAligned;
        if (encodeVideo) {
            const auto cacheCont = _mContainers.at(_mCurrentContainerId);
            const auto contRange = cacheCont->getRange()*_mRenderRange;
            const int nFrames = contRange.span();
            try {
                FFmpegHelper::writeVideoFrame(mFormatContext,
                                              &mVideoStream,
                                              cacheCont->getImage(),
                                              &hasVideo);
            } catch (...) {
                RuntimeThrow("Failed to write video frame");
            }
            if (++_mCurrentContainerFrame >= nFrames) {
                _mCurrentContainerId++;
                _mCurrentContainerFrame = 0;
                hasVideo = _mCurrentContainerId < _mContainers.count();
            }
        }
        bool audioAligned = true;
        if (mEncodeVideo && mEncodeAudio) {
            audioAligned = av_compare_ts(mVideoStream.fNextPts,
                                         mVideoStream.fCodec->time_base,
                                         mAudioStream.fNextPts,
                                         mAudioStream.fCodec->time_base) >= 0;
        }
        const bool encodeAudio = mEncodeAudio && hasAudio && audioAligned;
        if (encodeAudio) {
            try {
                processAudioStream(mFormatContext, &mAudioStream,
                                   mSoundIterator, &hasAudio);
            } catch (...) {
                RuntimeThrow("Failed to process audio stream");
            }
            hasAudio = _mAllAudioProvided ? mSoundIterator.hasValue() :
                                            mSoundIterator.hasSamples(mAudioStream.fSrcFrame->nb_samples);
        }
        if (!encodeVideo && !encodeAudio) { break; }
    }
}


void VideoEncoder::beforeProcessing(const Hardware)
{
    _mCurrentContainerId = 0;
    _mAllAudioProvided = mAllAudioProvided;
    if (_mContainers.isEmpty()) {
        _mContainers.swap(mNextContainers);
    } else {
        for (const auto& cont : mNextContainers) {
            _mContainers.append(cont);
        }
        mNextContainers.clear();
    }
    for (const auto& sound : mNextSoundConts) {
        mSoundIterator.add(sound);
    }
    mNextSoundConts.clear();
    _mRenderRange = {mRenderSettings.fMinFrame,
                     mRenderSettings.fMaxFrame};
    if (!mCurrentlyEncoding) { clearContainers(); }
}

void VideoEncoder::afterProcessing()
{
    const auto currCanvas = mRenderInstanceSettings->getTargetCanvas();
    if (_mCurrentContainerId != 0) {
        const auto lastEncoded = _mContainers.at(_mCurrentContainerId - 1);
        currCanvas->setSceneFrame(lastEncoded);
        currCanvas->setMinFrameUseRange(lastEncoded->getRange().fMax + 1);
    }

    for (int i = _mContainers.count() - 1; i >= _mCurrentContainerId; i--) {
        const auto &cont = _mContainers.at(i);
        mNextContainers.prepend(cont);
    }
    _mContainers.clear();

    if (mInterruptEncoding) {
        interrupEncoding();
        mInterruptEncoding = false;
    } else if (unhandledException()) {
        gPrintExceptionCritical(takeException());
        mRenderInstanceSettings->setCurrentState(RenderState::error, "Error");
        finishEncodingNow();
        mEmitter.encodingFailed();
    } else if (mEncodingFinished) { finishEncodingSuccess(); }
    else if (!mNextContainers.isEmpty()) { queTask(); }
}

bool VideoEncoder::startNewEncoding(RenderInstanceSettings * const settings)
{
    return startEncoding(settings);
}

void VideoEncoder::interruptCurrentEncoding()
{
    if (isActive()) { mInterruptEncoding = true; }
    else { interrupEncoding(); }
}

void VideoEncoder::finishCurrentEncoding()
{
    if (!mCurrentlyEncoding) { return; }
    if (isActive()) { mEncodingFinished = true; }
    else { finishEncodingSuccess(); }
}

void VideoEncoder::sFinishEncoding()
{
    sInstance->finishCurrentEncoding();
}

bool VideoEncoder::sEncodingSuccessfulyStarted()
{
    return sInstance->getCurrentlyEncoding();
}

bool VideoEncoder::sEncodeAudio()
{
    return sInstance->mEncodeAudio;
}

VideoEncoderEmitter *VideoEncoder::getEmitter()
{
    return &mEmitter;
}

bool VideoEncoder::getCurrentlyEncoding() const
{
    return mCurrentlyEncoding;
}

void VideoEncoder::sInterruptEncoding()
{
    sInstance->interruptCurrentEncoding();
}

bool VideoEncoder::sStartEncoding(RenderInstanceSettings *settings)
{
    return sInstance->startNewEncoding(settings);
}

void VideoEncoder::sAddCacheContainerToEncoder(const stdsptr<SceneFrameContainer> &cont)
{
    sInstance->addContainer(cont);
}

void VideoEncoder::sAddCacheContainerToEncoder(const stdsptr<Samples> &cont)
{
    sInstance->addContainer(cont);
}

void VideoEncoder::sAllAudioProvided()
{
    sInstance->allAudioProvided();
}
