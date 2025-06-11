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

#include "renderhandler.h"
#include "videoencoder.h"
#include "memoryhandler.h"
#include "Private/Tasks/taskscheduler.h"
#include "ViewLayers/viewlayer_render.h"
#include "Sound/soundcomposition.h"
#include "CacheHandlers/soundcachecontainer.h"
#include "CacheHandlers/sceneframecontainer.h"
#include "Private/document.h"

RenderHandler* RenderHandler::sInstance = nullptr;

RenderHandler::RenderHandler(Document &document,
                             AudioHandler& audioHandler,
                             VideoEncoder& videoEncoder,
                             MemoryHandler& memoryHandler) :
    mDocument(document),
    mAudioHandler(audioHandler),
    mVideoEncoder(videoEncoder) {
    Q_ASSERT(!sInstance);
    sInstance = this;

    connect(&memoryHandler, &MemoryHandler::allMemoryUsed,
            this, &RenderHandler::outOfMemory);

    mPreviewFPSTimer = new QTimer(this);
    mPreviewFPSTimer->setTimerType(Qt::PreciseTimer);
    connect(mPreviewFPSTimer, &QTimer::timeout,
            this, &RenderHandler::nextPreviewFrame);
    connect(mPreviewFPSTimer, &QTimer::timeout,
            this, &RenderHandler::audioPushTimerExpired);
    connect(audioHandler.audioOutput(), &QAudioOutput::notify,
            this, &RenderHandler::audioPushTimerExpired);

    const auto vidEmitter = videoEncoder.getEmitter();
//    connect(vidEmitter, &VideoEncoderEmitter::encodingStarted,
//            this, &SceneWindow::leaveOnlyInterruptionButtonsEnabled);
    connect(vidEmitter, &VideoEncoderEmitter::encodingFinished,
            this, &RenderHandler::interruptOutputRendering);
    connect(vidEmitter, &VideoEncoderEmitter::encodingInterrupted,
            this, &RenderHandler::interruptOutputRendering);
    connect(vidEmitter, &VideoEncoderEmitter::encodingFailed,
            this, &RenderHandler::interruptOutputRendering);
    connect(vidEmitter, &VideoEncoderEmitter::encodingStartFailed,
            this, &RenderHandler::interruptOutputRendering);
}

void RenderHandler::renderFromSettings(RenderInstanceSettings * const settings) {
    //setCurrentScene(settings->getTargetScene());
    if(VideoEncoder::sStartEncoding(settings)) {
        mSavedCurrentFrame = mCurrentViewLayer->getCurrentFrame();
        mSavedResolutionFraction = mCurrentViewLayer->getResolution();

        mCurrentRenderSettings = settings;
        const auto &renderSettings = settings->getRenderSettings();
        setFrameAction(renderSettings.fMinFrame);

        const qreal resolutionFraction = renderSettings.fResolution;
        mMinRenderFrame = renderSettings.fMinFrame;
        mMaxRenderFrame = renderSettings.fMaxFrame;
        const qreal fps = mCurrentViewLayer->getFps();
        mMaxSoundSec = qFloor(mMaxRenderFrame/fps);

        const auto nextFrameFunc = [this]() {
            nextSaveOutputFrame();
        };
        TaskScheduler::sSetTaskUnderflowFunc(nextFrameFunc);
        TaskScheduler::sSetAllTasksFinishedFunc(nextFrameFunc);

        mCurrentRenderFrame = renderSettings.fMinFrame;
        mCurrRenderRange = {mCurrentRenderFrame, mCurrentRenderFrame};

        mCurrentEncodeFrame = mCurrentRenderFrame;
        mFirstEncodeSoundSecond = qFloor(mCurrentRenderFrame/fps);
        mCurrentEncodeSoundSecond = mFirstEncodeSoundSecond;
        if(!VideoEncoder::sEncodeAudio())
            mMaxSoundSec = mCurrentEncodeSoundSecond - 1;
        //mCurrentViewLayer->setMinFrameUseRange(mCurrentRenderFrame);
        mCurrentSoundComposition->setMinFrameUseRange(mCurrentRenderFrame);
        mCurrentSoundComposition->scheduleFrameRange({mCurrentRenderFrame,
                                                      mCurrentRenderFrame});
        //mCurrentViewLayer->anim_setAbsFrame(mCurrentRenderFrame);
        mCurrentViewLayer->setIsRenderingToOutput(true);
        TaskScheduler::instance()->setAlwaysQue(true);
        //fitSceneToSize();
        if(!isZero6Dec(mSavedResolutionFraction - resolutionFraction)) {
            mCurrentViewLayer->setResolution(resolutionFraction);
            mDocument.actionFinished();
        } else {
            nextCurrentRenderFrame();
            if(TaskScheduler::sAllQuedCpuTasksFinished()) {
                nextSaveOutputFrame();
            }
        }
    }
}

void RenderHandler::setLoop(const bool loop) {
    mLoop = loop;
}

void RenderHandler::setFrameAction(const int frame) {
    //if(mCurrentViewLayer) mCurrentViewLayer->anim_setAbsFrame(frame);
    mDocument.actionFinished();
}

void RenderHandler::setCurrentViewLayer(ViewLayerRender * const viewLayer) {
    mCurrentViewLayer = viewLayer;
    mCurrentSoundComposition = viewLayer ? viewLayer->getSoundComposition() : nullptr;
}

void RenderHandler::nextCurrentRenderFrame() {
    auto& cacheHandler = mCurrentViewLayer->getSceneFramesHandler();
    int newCurrentRenderFrame = cacheHandler.
            firstEmptyFrameAtOrAfter(mCurrentRenderFrame + 1);
    const bool allDone = newCurrentRenderFrame > mMaxRenderFrame;
    newCurrentRenderFrame = qMin(mMaxRenderFrame, newCurrentRenderFrame);
    const FrameRange newSoundRange = {mCurrentRenderFrame, newCurrentRenderFrame};
    mCurrentSoundComposition->scheduleFrameRange(newSoundRange);
    mCurrentSoundComposition->setMaxFrameUseRange(newCurrentRenderFrame);
    //mCurrentViewLayer->setMaxFrameUseRange(newCurrentRenderFrame);

    mCurrentRenderFrame = newCurrentRenderFrame;
    mCurrRenderRange.fMax = mCurrentRenderFrame;
    if(allDone) Document::sInstance->actionFinished();
    else setFrameAction(mCurrentRenderFrame);
}

void RenderHandler::setPreviewState(const PreviewState state)
{
    if (mPreviewState == state) { return; }
    if (mPreviewState == PreviewState::stopped) {
        setRenderingPreview(true);
    } else if (mPreviewState == PreviewState::rendering) {
        setRenderingPreview(false);
        if (state == PreviewState::playing) { setPreviewing(true); }
    } else if (state == PreviewState::stopped) {
        setPreviewing(false);
    }
    mPreviewState = state;
}

void RenderHandler::renderPreview() {
    //setCurrentViewLayer(mDocument.fActiveScene);
    if(!mCurrentViewLayer) return;
    const auto nextFrameFunc = [this]() {
        nextPreviewRenderFrame();
    };
    TaskScheduler::sSetTaskUnderflowFunc(nextFrameFunc);
    TaskScheduler::sSetAllTasksFinishedFunc(nextFrameFunc);

    mSavedCurrentFrame = mCurrentViewLayer->getCurrentFrame();

    const auto fIn = mCurrentViewLayer->getFrameIn();
    const auto fOut = mCurrentViewLayer->getFrameOut();

    mMinRenderFrame = mLoop ? (fIn.enabled? fIn.frame : mCurrentViewLayer->getMinFrame()) - 1:
                          (fIn.enabled ? fIn.frame : mSavedCurrentFrame);

    mMaxRenderFrame = fOut.enabled ? fOut.frame : mCurrentViewLayer->getMaxFrame();

    mCurrentRenderFrame = mMinRenderFrame;
    mCurrRenderRange = {mCurrentRenderFrame, mCurrentRenderFrame};
    //mCurrentViewLayer->setMinFrameUseRange(mCurrentRenderFrame);
    mCurrentSoundComposition->setMinFrameUseRange(mCurrentRenderFrame);

    setPreviewState(PreviewState::rendering);

    emit previewBeingRendered();

    if(TaskScheduler::sAllQuedCpuTasksFinished()) {
        nextPreviewRenderFrame();
    }
}

void RenderHandler::interruptPreview() {
    if(mRenderingPreview) interruptPreviewRendering();
    else if(mPreviewing) stopPreview();
}

void RenderHandler::outOfMemory() {
    if(mRenderingPreview) {
        playPreview();
    }
}

void RenderHandler::setRenderingPreview(const bool rendering) {
    mRenderingPreview = rendering;
    //if(mCurrentViewLayer) mCurrentViewLayer->setRenderingPreview(rendering);
    TaskScheduler::instance()->setAlwaysQue(rendering);
}

void RenderHandler::setPreviewing(const bool previewing) {
    mPreviewing = previewing;
    //if(mCurrentViewLayer) mCurrentViewLayer->setPreviewing(previewing);
}

void RenderHandler::interruptPreviewRendering() {
    TaskScheduler::sClearAllFinishedFuncs();
    stopPreview();
}

void RenderHandler::interruptOutputRendering() {
    //if(mCurrentViewLayer) mCurrentViewLayer->setIsRenderingToOutput(false);
    TaskScheduler::instance()->setAlwaysQue(false);
    TaskScheduler::sClearAllFinishedFuncs();
    stopPreview();
}

void RenderHandler::stopPreview() {
    if(mCurrentViewLayer) {
        //mCurrentViewLayer->clearUseRange();
        setFrameAction(mSavedCurrentFrame);
        mCurrentViewLayer->setSceneFrame(mSavedCurrentFrame);
        //emit mCurrentViewLayer->currentFrameChanged(mSavedCurrentFrame);
        //emit mCurrentViewLayer->requestUpdate();
    }

    mPreviewFPSTimer->stop();
    stopAudio();
    emit previewFinished();
    setPreviewState(PreviewState::stopped);
}

void RenderHandler::pausePreview() {
    if(mPreviewing) {
        mAudioHandler.pauseAudio();
        mPreviewFPSTimer->stop();
        emit previewPaused();
        setPreviewState(PreviewState::paused);
    }
}

void RenderHandler::resumePreview() {
    if(mPreviewing) {
        mAudioHandler.resumeAudio();
        mPreviewFPSTimer->start();
        emit previewBeingPlayed();
        setPreviewState(PreviewState::playing);
    }
}

void RenderHandler::playPreviewAfterAllTasksCompleted() {
    if(mRenderingPreview) {
        TaskScheduler::sSetTaskUnderflowFunc(nullptr);
        Document::sInstance->actionFinished();
        if(TaskScheduler::sAllTasksFinished()) {
            playPreview();
        } else {
            TaskScheduler::sSetAllTasksFinishedFunc([this]() {
                playPreview();
            });
        }
    }
}

void RenderHandler::playPreview() {
    if(!mCurrentViewLayer) return;
    //setFrameAction(mSavedCurrentFrame);
    TaskScheduler::sClearAllFinishedFuncs();
    const auto fIn = mCurrentViewLayer->getFrameIn();
    const auto fOut = mCurrentViewLayer->getFrameOut();
    const int minPreviewFrame = fIn.enabled? (fIn.frame < mSavedCurrentFrame && mSavedCurrentFrame < mCurrentRenderFrame ? mSavedCurrentFrame : fIn.frame) : mSavedCurrentFrame;
    const int maxPreviewFrame = qMin(mMaxRenderFrame, mCurrentRenderFrame);
    if(minPreviewFrame >= maxPreviewFrame) return;
    mMinPreviewFrame = mLoop ? (fIn.enabled? fIn.frame : mCurrentViewLayer->getMinFrame()) : (fIn.enabled? (fIn.frame < minPreviewFrame && minPreviewFrame < mCurrentRenderFrame ? minPreviewFrame : fIn.frame) : minPreviewFrame);
    mMaxPreviewFrame = fOut.enabled ? fOut.frame : maxPreviewFrame;
    mCurrentPreviewFrame = minPreviewFrame;
    mCurrentViewLayer->setSceneFrame(mCurrentPreviewFrame);

    setPreviewState(PreviewState::playing);

    startAudio();

    const int mSecInterval = qRound(1000/mCurrentViewLayer->getFps());
    mPreviewFPSTimer->setInterval(mSecInterval);
    mPreviewFPSTimer->start();
    emit previewBeingPlayed();
    //emit mCurrentViewLayer->requestUpdate();
}

void RenderHandler::nextPreviewRenderFrame() {
    if(!mRenderingPreview) return;
    if(mCurrentRenderFrame >= mMaxRenderFrame) {
        playPreviewAfterAllTasksCompleted();
    } else {
        nextCurrentRenderFrame();
        if(TaskScheduler::sAllTasksFinished()) {
            nextPreviewRenderFrame();
        }
    }
}

void RenderHandler::nextPreviewFrame() {
    if(!mCurrentViewLayer) return;
    mCurrentPreviewFrame++;
    if(mCurrentPreviewFrame > mMaxPreviewFrame) {
        if(mLoop) {
            mCurrentPreviewFrame = mMinPreviewFrame - 1;
            nextPreviewFrame();
            stopAudio();
            startAudio();
        } else stopPreview();
    } else {
        mCurrentViewLayer->setSceneFrame(mCurrentPreviewFrame);
        //if(!mLoop) mCurrentViewLayer->setMinFrameUseRange(mCurrentPreviewFrame);
        //emit mCurrentViewLayer->currentFrameChanged(mCurrentPreviewFrame);
    }
    //emit mCurrentViewLayer->requestUpdate();
}

void RenderHandler::finishEncoding() {
    TaskScheduler::sClearAllFinishedFuncs();
    mCurrentRenderSettings = nullptr;
    //mCurrentViewLayer->setIsRenderingToOutput(false);
    TaskScheduler::instance()->setAlwaysQue(false);
    setFrameAction(mSavedCurrentFrame);
    /*if(!isZero4Dec(mSavedResolutionFraction - mCurrentViewLayer->getResolution())) {
        mCurrentViewLayer->setResolution(mSavedResolutionFraction);
        }*/
    mCurrentSoundComposition->clearUseRange();
    VideoEncoder::sFinishEncoding();
}

void RenderHandler::nextSaveOutputFrame() {
    const auto& sCacheHandler = mCurrentSoundComposition->getCacheHandler();
    const qreal fps = /*mCurrentViewLayer->getFps()*/60;
    const int sampleRate = eSoundSettings::sSampleRate();
    while(mCurrentEncodeSoundSecond <= mMaxSoundSec) {
        const auto cont = sCacheHandler.atFrame(mCurrentEncodeSoundSecond);
        if(!cont) break;
        const auto sCont = cont->ref<SoundCacheContainer>();
        const auto samples = sCont->getSamples();
        if(mCurrentEncodeSoundSecond == mFirstEncodeSoundSecond) {
            const int minSample = qRound(mMinRenderFrame*sampleRate/fps);
            const int max = samples->fSampleRange.fMax;
            VideoEncoder::sAddCacheContainerToEncoder(
                        samples->mid({minSample, max}));
        } else {
            VideoEncoder::sAddCacheContainerToEncoder(
                        enve::make_shared<Samples>(samples));
        }
        mCurrentEncodeSoundSecond++;
    }
    if(mCurrentEncodeSoundSecond > mMaxSoundSec) VideoEncoder::sAllAudioProvided();

    const auto& cacheHandler = mCurrentViewLayer->getSceneFramesHandler();
    while(mCurrentEncodeFrame <= mMaxRenderFrame) {
        const auto cont = cacheHandler.atFrame(mCurrentEncodeFrame);
        if(!cont) break;
        VideoEncoder::sAddCacheContainerToEncoder(cont->ref<SceneFrameContainer>());
        mCurrentEncodeFrame = cont->getRangeMax() + 1;
    }

    //mCurrentViewLayer->renderCurrentFrameToOutput(*mCurrentRenderSettings);
    if(mCurrentRenderFrame >= mMaxRenderFrame) {
        if(mCurrentEncodeSoundSecond <= mMaxSoundSec) return;
        if(mCurrentEncodeFrame <= mMaxRenderFrame) return;
        TaskScheduler::sSetTaskUnderflowFunc(nullptr);
        Document::sInstance->actionFinished();
        if(TaskScheduler::sAllTasksFinished()) {
            finishEncoding();
        } else {
            TaskScheduler::sSetAllTasksFinishedFunc([this]() {
                finishEncoding();
            });
        }
    } else {
        mCurrentRenderSettings->setCurrentRenderFrame(mCurrentRenderFrame);
        nextCurrentRenderFrame();
        if(TaskScheduler::sAllTasksFinished()) {
            nextSaveOutputFrame();
        }
    }
}

void RenderHandler::startAudio() {
    mAudioHandler.startAudio();
    if(mCurrentSoundComposition)
        mCurrentSoundComposition->start(mCurrentPreviewFrame);
    audioPushTimerExpired();
}

void RenderHandler::stopAudio() {
    mAudioHandler.stopAudio();
    if(mCurrentSoundComposition) mCurrentSoundComposition->stop();
}

void RenderHandler::audioPushTimerExpired() {
    if(!mCurrentSoundComposition) return;
    while(auto request = mAudioHandler.dataRequest()) {
        const qint64 len = mCurrentSoundComposition->read(
                    request.fData, request.fSize);
        if(len <= 0) break;
        request.fSize = int(len);
        mAudioHandler.provideData(request);
    }
}
