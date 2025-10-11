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

#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include "core_global.h"

#include <QString>
#include <QList>
#include "skia/skiaincludes.h"
#include "Tasks/updatable.h"
#include "renderinstancesettings.h"
#include "framerange.h"
#include "CacheHandlers/samples.h"
#include "Sound/esoundsettings.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libswresample/swresample.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/opt.h>
}

class SceneFrameContainer;

class CORE_EXPORT SoundIterator {
public:
    SoundIterator() {}

    bool hasValue() const {
        return !mSamples.isEmpty();
    }

    bool hasSamples(const int samples) const {
        if(mSamples.isEmpty()) return false;
        int rem = samples - (mEndSample - mCurrentSample);
        if(rem <= 0) return true;
        for(int i = 1 ; i < mSamples.count(); i++) {
            rem -= mSamples.at(i)->fSampleRange.span();
            if(rem <= 0) return true;
        }
        return false;
    }

    void fillFrame(AVFrame* const frame) {
        Q_ASSERT(frame->channel_layout == mCurrentSamples->fChannelLayout);
        Q_ASSERT(frame->format == mCurrentSamples->fFormat);
        Q_ASSERT(frame->sample_rate == mCurrentSamples->fSampleRate);
        const int nChannels = static_cast<int>(mCurrentSamples->fNChannels);
        const int sampleSize = int(mCurrentSamples->fSampleSize);
        int remaining = frame->nb_samples;
        int frameSample = 0;
        if(mCurrentSamples->fPlanar) {
            while(remaining > 0) {
                const int cpySamples = qMin(remaining,
                                            mEndSample - mCurrentSample + 1);
                const uint cpyBytes = static_cast<uint>(cpySamples*sampleSize);
                for(int j = 0; j < nChannels; j++) {
                    memcpy(frame->data[j] + frameSample*sampleSize,
                           mCurrentData[j] + mCurrentSample*sampleSize, cpyBytes);
                }

                remaining -= cpySamples;
                mCurrentSample += cpySamples;
                frameSample += cpySamples;
                if(mCurrentSample > mEndSample) {
                    if(!next()) {
                        frame->nb_samples = frameSample;
                        return;
                    }
                }
            }
        } else {
            while(remaining > 0) {
                const int cpySamples = qMin(remaining,
                                            mEndSample - mCurrentSample + 1)*nChannels;
                const uint cpyBytes = static_cast<uint>(cpySamples*sampleSize);
                memcpy(frame->data[0] + frameSample*sampleSize*nChannels,
                        mCurrentData[0] + mCurrentSample*sampleSize*nChannels, cpyBytes);

                remaining -= cpySamples;
                mCurrentSample += cpySamples;
                frameSample += cpySamples;
                if(mCurrentSample > mEndSample) {
                    if(!next()) {
                        frame->nb_samples = frameSample;
                        return;
                    }
                }
            }
        }
    }

    bool next() {
        if(mSamples.isEmpty()) return false;
        mSamples.removeFirst();
        if(!updateCurrent()) return false;
        return true;
    }

    void add(const stdsptr<Samples>& sound) {
        mSamples << sound;
        if(mSamples.count() == 1) updateCurrent();
    }

    void clear() {
        mSamples.clear();
        updateCurrent();
    }
private:
    bool updateCurrent() {
        if(mSamples.isEmpty()) {
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

    int mCurrentSample = 0;
    int mEndSample = 0;
    uchar** mCurrentData = nullptr;
    Samples* mCurrentSamples = nullptr;
    QList<stdsptr<Samples>> mSamples;
};

typedef struct CORE_EXPORT OutputStream {
    // pts of the next frame that will be generated
    int64_t fNextPts;

    AVStream *fStream = nullptr;
    AVCodecContext *fCodec = nullptr;
    AVFrame *fDstFrame = nullptr;
    AVFrame *fSrcFrame = nullptr;
    struct SwsContext *fSwsCtx = nullptr;
    struct SwrContext *fSwrCtx = nullptr;
    // Cached per-frame duration in stream time_base ticks
    int64_t fFrameDuration = 0;
} OutputStream;

class CORE_EXPORT VideoEncoderEmitter : public QObject {
    Q_OBJECT
public:
    VideoEncoderEmitter() {}
signals:
    void encodingStarted();
    void encodingFinished();
    void encodingInterrupted();

    void encodingStartFailed();
    void encodingFailed();
};

class CORE_EXPORT VideoEncoder : public eHddTask {
    e_OBJECT
protected:
    VideoEncoder();
public:
    void process();
    void beforeProcessing(const Hardware);
    void afterProcessing();

    bool startNewEncoding(RenderInstanceSettings * const settings) {
        return startEncoding(settings);
    }

    void interruptCurrentEncoding() {
        if(isActive()) mInterruptEncoding = true;
        else interrupEncoding();
    }

    void finishCurrentEncoding() {
        if(!mCurrentlyEncoding) return;
        if(isActive()) mEncodingFinished = true;
        else finishEncodingSuccess();
    }

    void addContainer(const stdsptr<SceneFrameContainer> &cont);
    void addContainer(const stdsptr<Samples> &cont);
    void allAudioProvided();

    static VideoEncoder *sInstance;

    static bool isValidProfile(const AVCodec *codec, int profile);

    static void sInterruptEncoding();
    static bool sStartEncoding(RenderInstanceSettings *settings);
    static void sAddCacheContainerToEncoder(const stdsptr<SceneFrameContainer> &cont);
    static void sAddCacheContainerToEncoder(const stdsptr<Samples> &cont);
    static void sAllAudioProvided();
    static void sFinishEncoding();
    static bool sEncodingSuccessfulyStarted();
    static bool sEncodeAudio();

    VideoEncoderEmitter *getEmitter() {
        return &mEmitter;
    }

    bool getCurrentlyEncoding() const {
        return mCurrentlyEncoding;
    }
protected:
    void clearContainers();
    VideoEncoderEmitter mEmitter;
    void interrupEncoding();
    void finishEncodingSuccess();
    void finishEncodingNow();
    bool startEncoding(RenderInstanceSettings * const settings);
    void startEncodingNow();

    bool mEncodingSuccesfull = false;
    bool mEncodingFinished = false;
    bool mInterruptEncoding = false;

    eSoundSettingsData mInSoundSettings;
    OutputStream mVideoStream;
    OutputStream mAudioStream;
    AVFormatContext *mFormatContext = nullptr;
    const AVOutputFormat *mOutputFormat = nullptr;
    bool mCurrentlyEncoding = false;
    QList<stdsptr<SceneFrameContainer>> mNextContainers;
    QList<stdsptr<Samples>> mNextSoundConts;

    RenderSettings mRenderSettings;
    OutputSettings mOutputSettings;
    RenderInstanceSettings *mRenderInstanceSettings = nullptr;
    QByteArray mPathByteArray;
    bool mEncodeVideo = false;
    bool mEncodeAudio = false;
    bool mAllAudioProvided = false;

    bool _mAllAudioProvided = false;
    int _mCurrentContainerId = 0;
    int _mCurrentContainerFrame = 0; // some containers will add multiple frames
    FrameRange _mRenderRange;

    QList<stdsptr<SceneFrameContainer>> _mContainers;
    SoundIterator mSoundIterator;
};

#endif // VIDEOENCODER_H
