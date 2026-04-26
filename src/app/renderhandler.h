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

#ifndef RENDERHANDLER_H
#define RENDERHANDLER_H
#include "framerange.h"
#include "Sound/audiohandler.h"
#include "smartPointers/ememory.h"
#include "CacheHandlers/usepointer.h"
#include "CacheHandlers/cachecontainer.h"

class Canvas;
class RenderInstanceSettings;
class SoundComposition;
class Document;
class VideoEncoder;
class MemoryHandler;

enum class PreviewState {
    stopped, rendering, playing, paused
};

class RenderHandler : public QObject {
    Q_OBJECT
public:
    RenderHandler(Document &document,
                  AudioHandler &audioHandler,
                  VideoEncoder &videoEncoder,
                  MemoryHandler &memoryHandler);

    void interruptPreview();
    void outOfMemory();
    void interruptPreviewRendering();
    void interruptOutputRendering();

    void playPreview();
    void stopPreview();
    void pausePreview();
    void resumePreview();
    void renderPreview();
    void renderFromSettings(RenderInstanceSettings * const settings);

    void setPreviewFrame(const int &frame);

    void setLoop(const bool loop);

    PreviewState currentPreviewState() const
    { return mPreviewState; }

    static RenderHandler* sInstance;
signals:
    void previewBeingRendered();
    void previewPaused();
    void previewBeingPlayed();
    void previewFinished();
private:
    void setFrameAction(const int frame);
    void setCurrentScene(Canvas * const scene);

    void finishEncoding();
    void playPreviewAfterAllTasksCompleted();

    void nextSaveOutputFrame();
    void nextPreviewRenderFrame();
    void nextPreviewFrame();
    void nextCurrentRenderFrame();

    void setPreviewState(const PreviewState state);
    void setRenderingPreview(const bool rendering);
    void setPreviewing(const bool previewing);

    void startAudio();
    void audioPushTimerExpired();
    void stopAudio();

    Document& mDocument;

    bool mLoop = false;

    // AUDIO
    AudioHandler& mAudioHandler;
    VideoEncoder& mVideoEncoder;
    qptr<SoundComposition> mCurrentSoundComposition;
    // AUDIO

    Canvas* mCurrentScene = nullptr;
    QTimer *mPreviewFPSTimer = nullptr;
    RenderInstanceSettings *mCurrentRenderSettings = nullptr;

    int mCurrentPreviewFrame;
    int mMaxPreviewFrame;
    int mMinPreviewFrame;

    PreviewState mPreviewState = PreviewState::stopped;
    //! @brief true if preview is currently playing
    bool mPreviewing = false;
    //! @brief true if currently preview is being rendered
    bool mRenderingPreview = false;

    int mCurrentEncodeFrame;
    int mCurrentEncodeSoundSecond;
    int mMaxSoundSec;
    int mFirstEncodeSoundSecond;

    FrameRange mCurrRenderRange;
    int mCurrentRenderFrame;
    int mMinRenderFrame = 0;
    int mMaxRenderFrame = 0;

    int mSavedCurrentFrame = 0;
    qreal mSavedResolutionFraction = 100;
};

#endif // RENDERHANDLER_H
