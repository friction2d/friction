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

#include "Tasks/updatable.h"
#include "renderinstancesettings.h"
#include "framerange.h"
#include "CacheHandlers/samples.h"
#include "Sound/esoundsettings.h"
#include "utils/ffmpeghelper.h"

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

class CORE_EXPORT SoundIterator
{
public:
    SoundIterator() {}
    bool hasValue() const;
    bool hasSamples(const int samples) const;
    void fillFrame(AVFrame* const frame);
    bool next();
    void add(const stdsptr<Samples>& sound);
    void clear();

private:
    bool updateCurrent();
    int mCurrentSample = 0;
    int mEndSample = 0;
    uchar** mCurrentData = nullptr;
    Samples* mCurrentSamples = nullptr;
    QList<stdsptr<Samples>> mSamples;
};

class CORE_EXPORT VideoEncoderEmitter : public QObject
{
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

class CORE_EXPORT VideoEncoder : public eHddTask
{
    e_OBJECT
protected:
    VideoEncoder();

public:
    void process();
    void beforeProcessing(const Hardware);
    void afterProcessing();
    bool startNewEncoding(RenderInstanceSettings * const settings);
    void interruptCurrentEncoding();
    void finishCurrentEncoding();
    void addContainer(const stdsptr<SceneFrameContainer> &cont);
    void addContainer(const stdsptr<Samples> &cont);
    void allAudioProvided();

    static VideoEncoder *sInstance;

    static void sInterruptEncoding();
    static bool sStartEncoding(RenderInstanceSettings *settings);
    static void sAddCacheContainerToEncoder(const stdsptr<SceneFrameContainer> &cont);
    static void sAddCacheContainerToEncoder(const stdsptr<Samples> &cont);
    static void sAllAudioProvided();
    static void sFinishEncoding();
    static bool sEncodingSuccessfulyStarted();
    static bool sEncodeAudio();

    VideoEncoderEmitter *getEmitter();

    bool getCurrentlyEncoding() const;

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
    Friction::Utils::FFmpegHelper::OutputStream mVideoStream;
    Friction::Utils::FFmpegHelper::OutputStream mAudioStream;
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
    int _mCurrentContainerFrame = 0;
    FrameRange _mRenderRange;

    QList<stdsptr<SceneFrameContainer>> _mContainers;
    SoundIterator mSoundIterator;
};

#endif // VIDEOENCODER_H
