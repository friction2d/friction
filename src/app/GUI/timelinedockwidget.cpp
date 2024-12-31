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

#include "timelinedockwidget.h"

#include <QKeyEvent>
#include <QScrollBar>

#include "Private/document.h"
#include "GUI/global.h"
#include "GUI/BoxesList/boxscrollwidget.h"
#include "GUI/BoxesList/boxsinglewidget.h"

#include "mainwindow.h"
#include "canvaswindow.h"
#include "canvas.h"
#include "animationdockwidget.h"
#include "widgets/widgetstack.h"
#include "widgets/actionbutton.h"
#include "timelinewidget.h"
#include "widgets/framescrollbar.h"
#include "renderinstancesettings.h"
#include "layouthandler.h"
#include "memoryhandler.h"
#include "appsupport.h"

TimelineDockWidget::TimelineDockWidget(Document& document,
                                       LayoutHandler * const layoutH,
                                       MainWindow * const parent)
    : QWidget(parent)
    , mDocument(document)
    , mMainWindow(parent)
    , mTimelineLayout(layoutH->timelineLayout())
    , mToolBar(nullptr)
    , mFrameStartSpin(nullptr)
    , mFrameEndSpin(nullptr)
    , mFrameRewindAct(nullptr)
    , mFrameFastForwardAct(nullptr)
    , mCurrentFrameSpinAct(nullptr)
    , mCurrentFrameSpin(nullptr)
    , mRenderProgressAct(nullptr)
    , mRenderProgress(nullptr)
{
    connect(RenderHandler::sInstance, &RenderHandler::previewFinished,
            this, &TimelineDockWidget::previewFinished);
    connect(RenderHandler::sInstance, &RenderHandler::previewBeingPlayed,
            this, &TimelineDockWidget::previewBeingPlayed);
    connect(RenderHandler::sInstance, &RenderHandler::previewBeingRendered,
            this, &TimelineDockWidget::previewBeingRendered);
    connect(RenderHandler::sInstance, &RenderHandler::previewPaused,
            this, &TimelineDockWidget::previewPaused);

    connect(Document::sInstance, &Document::canvasModeSet,
            this, &TimelineDockWidget::updateButtonsVisibility);

    setFocusPolicy(Qt::NoFocus);

    mMainLayout = new QVBoxLayout(this);
    setLayout(mMainLayout);
    mMainLayout->setSpacing(0);
    mMainLayout->setMargin(0);

    mFrameRewindAct = new QAction(QIcon::fromTheme("rewind"),
                                  tr("Rewind"),
                                  this);
    mFrameRewindAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                      "rewind",
                                                                      "Shift+Left").toString()));
    mFrameRewindAct->setData(tr("Go to First Frame"));
    connect(mFrameRewindAct, &QAction::triggered,
            this, [this]() {
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        scene->anim_setAbsFrame(scene->getFrameRange().fMin);
        mDocument.actionFinished();
    });

    mFrameFastForwardAct = new QAction(QIcon::fromTheme("fastforward"),
                                       tr("Fast Forward"),
                                       this);
    mFrameFastForwardAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                           "fastForward",
                                                                           "Shift+Right").toString()));
    mFrameFastForwardAct->setData(tr("Go to Last Frame"));
    connect(mFrameFastForwardAct, &QAction::triggered,
            this, [this]() {
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        scene->anim_setAbsFrame(scene->getFrameRange().fMax);
        mDocument.actionFinished();
    });

    mPlayFromBeginningButton = new QAction(QIcon::fromTheme("preview"),
                                           tr("Play Preview From Start"),
                                           this);
    connect(mPlayFromBeginningButton, &QAction::triggered,
            this, [this]() {
        /*const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        scene->anim_setAbsFrame(scene->getFrameRange().fMin);
        renderPreview();*/
        const auto state = RenderHandler::sInstance->currentPreviewState();
        setPreviewFromStart(state);
    });

    mPlayButton = new QAction(QIcon::fromTheme("play"),
                              tr("Play Preview"),
                              this);

    mStopButton = new QAction(QIcon::fromTheme("stop"),
                              tr("Stop Preview"),
                              this);

    connect(mStopButton, &QAction::triggered,
            this, &TimelineDockWidget::interruptPreview);

    mLoopButton = new QAction(QIcon::fromTheme("preview_loop"),
                              tr("Loop Preview"),
                              this);
    mLoopButton->setCheckable(true);
    connect(mLoopButton, &QAction::triggered,
            this, &TimelineDockWidget::setLoop);

    mFrameStartSpin = new FrameSpinBox(this);
    mFrameStartSpin->setKeyboardTracking(false);
    mFrameStartSpin->setObjectName("LeftSpinBox");
    mFrameStartSpin->setAlignment(Qt::AlignHCenter);
    mFrameStartSpin->setFocusPolicy(Qt::ClickFocus);
    mFrameStartSpin->setToolTip(tr("Scene frame start"));
    mFrameStartSpin->setRange(0, INT_MAX);
    connect(mFrameStartSpin,
            &QSpinBox::editingFinished,
            this, [this]() {
            const auto scene = *mDocument.fActiveScene;
            if (!scene) { return; }
            auto range = scene->getFrameRange();
            int frame = mFrameStartSpin->value();
            if (range.fMin == frame) { return; }
            if (frame >= range.fMax) {
                mFrameStartSpin->setValue(range.fMin);
                return;
            }
            range.fMin = frame;
            scene->setFrameRange(range);
    });

    mFrameEndSpin = new FrameSpinBox(this);
    mFrameEndSpin->setKeyboardTracking(false);
    mFrameEndSpin->setAlignment(Qt::AlignHCenter);
    mFrameEndSpin->setFocusPolicy(Qt::ClickFocus);
    mFrameEndSpin->setToolTip(tr("Scene frame end"));
    mFrameEndSpin->setRange(1, INT_MAX);
    connect(mFrameEndSpin,
            &QSpinBox::editingFinished,
            this, [this]() {
            const auto scene = *mDocument.fActiveScene;
            if (!scene) { return; }
            auto range = scene->getFrameRange();
            int frame = mFrameEndSpin->value();
            if (range.fMax == frame) { return; }
            if (frame <= range.fMin) {
                mFrameEndSpin->setValue(range.fMax);
                return;
            }
            range.fMax = frame;
            scene->setFrameRange(range);
    });

    mCurrentFrameSpin = new FrameSpinBox(this);
    mCurrentFrameSpin->setKeyboardTracking(false);
    mCurrentFrameSpin->setAlignment(Qt::AlignHCenter);
    mCurrentFrameSpin->setObjectName(QString::fromUtf8("SpinBoxNoButtons"));
    mCurrentFrameSpin->setFocusPolicy(Qt::NoFocus);
    mCurrentFrameSpin->setToolTip(tr("Current frame"));
    mCurrentFrameSpin->setRange(-INT_MAX, INT_MAX);
    mCurrentFrameSpin->setReadOnly(true);
    mCurrentFrameSpin->setEnabled(false);

    const auto mPrevKeyframeAct = new QAction(QIcon::fromTheme("prev_keyframe"),
                                              QString(),
                                              this);
    mPrevKeyframeAct->setToolTip(tr("Previous Keyframe"));
    mPrevKeyframeAct->setData(mPrevKeyframeAct->toolTip());
    connect(mPrevKeyframeAct, &QAction::triggered,
            this, [this]() {
        if (setPrevKeyframe()) {
            mDocument.actionFinished();
        }
    });

    const auto mNextKeyframeAct = new QAction(QIcon::fromTheme("next_keyframe"),
                                              QString(),
                                              this);
    mNextKeyframeAct->setToolTip(tr("Next Keyframe"));
    mNextKeyframeAct->setData(mNextKeyframeAct->toolTip());
    connect(mNextKeyframeAct, &QAction::triggered,
            this, [this]() {
        if (setNextKeyframe()) {
            mDocument.actionFinished();
        }
    });

    mToolBar = new QToolBar(this);
    mToolBar->setMovable(false);

    mRenderProgress = new QProgressBar(this);
    mRenderProgress->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);
    mRenderProgress->setFixedWidth(mCurrentFrameSpin->width());
    mRenderProgress->setFormat(tr("Cache %p%"));

    mToolBar->addWidget(mFrameStartSpin);

    eSizesUI::widget.add(mToolBar, [this](const int size) {
        //mRenderProgress->setFixedHeight(eSizesUI::button);
        mToolBar->setIconSize(QSize(size, size));
    });

    QWidget *spacerWidget1 = new QWidget(this);
    spacerWidget1->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Minimum);
    mToolBar->addWidget(spacerWidget1);

    mToolBar->addAction(mFrameRewindAct);
    mToolBar->addAction(mPrevKeyframeAct);
    mToolBar->addAction(mNextKeyframeAct);
    mToolBar->addAction(mFrameFastForwardAct);
    mRenderProgressAct = mToolBar->addWidget(mRenderProgress);
    mCurrentFrameSpinAct = mToolBar->addWidget(mCurrentFrameSpin);
    mToolBar->addAction(mPlayFromBeginningButton);
    mToolBar->addAction(mPlayButton);
    mToolBar->addAction(mStopButton);
    mToolBar->addAction(mLoopButton);

    mMainWindow->cmdAddAction(mFrameRewindAct);
    mMainWindow->cmdAddAction(mPrevKeyframeAct);
    mMainWindow->cmdAddAction(mNextKeyframeAct);
    mMainWindow->cmdAddAction(mFrameFastForwardAct);
    mMainWindow->cmdAddAction(mPlayFromBeginningButton);
    mMainWindow->cmdAddAction(mPlayButton);
    mMainWindow->cmdAddAction(mStopButton);
    mMainWindow->cmdAddAction(mLoopButton);

    mRenderProgressAct->setVisible(false);

    QWidget *spacerWidget2 = new QWidget(this);
    spacerWidget2->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Minimum);
    mToolBar->addWidget(spacerWidget2);

    mToolBar->addWidget(mFrameEndSpin);

    mMainLayout->addWidget(mToolBar);
    mMainLayout->addSpacing(2);

    mPlayFromBeginningButton->setEnabled(false);
    mPlayButton->setEnabled(false);
    mStopButton->setEnabled(false);

    connect(&mDocument, &Document::activeSceneSet,
            this, [this](Canvas* const scene) {
        mPlayFromBeginningButton->setEnabled(scene);
        mPlayButton->setEnabled(scene);
        mStopButton->setEnabled(scene);
    });

    mMainLayout->addWidget(mTimelineLayout);

    previewFinished();

    connect(&mDocument, &Document::activeSceneSet,
            this, &TimelineDockWidget::updateSettingsForCurrentCanvas);

}

void TimelineDockWidget::updateFrameRange(const FrameRange &range)
{
    mRenderProgress->setRange(range.fMin, range.fMax);
    if (range.fMin != mFrameStartSpin->value()) {
        mFrameStartSpin->blockSignals(true);
        mFrameStartSpin->setValue(range.fMin);
        mFrameStartSpin->blockSignals(false);
    }
    if (range.fMax != mFrameEndSpin->value()) {
        mFrameEndSpin->blockSignals(true);
        mFrameEndSpin->setValue(range.fMax);
        mFrameEndSpin->blockSignals(false);
    }
}

void TimelineDockWidget::handleCurrentFrameChanged(int frame)
{
    mCurrentFrameSpin->setValue(frame);
    if (mRenderProgress->isVisible()) { mRenderProgress->setValue(frame); }
}

void TimelineDockWidget::showRenderStatus(bool show)
{
    if (!show) { mRenderProgress->setValue(0); }
    mCurrentFrameSpinAct->setVisible(!show);
    mRenderProgressAct->setVisible(show);
}

void TimelineDockWidget::setLoop(const bool loop)
{
    RenderHandler::sInstance->setLoop(loop);
}

bool TimelineDockWidget::processKeyPress(QKeyEvent *event)
{
    const int key = event->key();
    const auto mods = event->modifiers();
    const auto state = RenderHandler::sInstance->currentPreviewState();
    if (key == Qt::Key_Escape) { // stop playback
        if (state == PreviewState::stopped) { return false; }
        interruptPreview();
    } else if (key == Qt::Key_Space && (mods & Qt::ShiftModifier)) { // play from first frame
        /*const auto scene = *mDocument.fActiveScene;
        if (!scene) { return false; }
        if (state != PreviewState::stopped) { interruptPreview(); }
        scene->anim_setAbsFrame(scene->getFrameRange().fMin);
        renderPreview();*/
        if (!setPreviewFromStart(state)) { return false; }
    } else if (key == Qt::Key_Space) { // start/resume playback
        switch (state) {
            case PreviewState::stopped: renderPreview(); break;
            case PreviewState::rendering: playPreview(); break;
            case PreviewState::playing: pausePreview(); break;
            case PreviewState::paused: resumePreview(); break;
        }
    } else if (key == Qt::Key_K) { // split clip
        splitClip();
    } else if (key == Qt::Key_M) { // set marker
        setMarker();
    } else if (key == Qt::Key_I || key == Qt::Key_O) { // set frame in/out
        switch(key) {
            case Qt::Key_I: setIn(); break;
            case Qt::Key_O: setOut(); break;
            default:;
        }
    } else if (key == Qt::Key_Right && !(mods & Qt::ControlModifier)) { // next frame
        mDocument.incActiveSceneFrame();
    } else if (key == Qt::Key_Left && !(mods & Qt::ControlModifier)) { // previous frame
        mDocument.decActiveSceneFrame();
    } else if (key == Qt::Key_Down && !(mods & Qt::ControlModifier)) { // previous keyframe
        /*const auto scene = *mDocument.fActiveScene;
        if (!scene) { return false; }
        int targetFrame;
        const int frame = mDocument.getActiveSceneFrame();
        if (scene->anim_prevRelFrameWithKey(frame, targetFrame)) {
            mDocument.setActiveSceneFrame(targetFrame);
        }*/
        if (!setPrevKeyframe()) { return false; }
    } else if (key == Qt::Key_Up && !(mods & Qt::ControlModifier)) { // next keyframe
        /*const auto scene = *mDocument.fActiveScene;
        if (!scene) { return false; }
        int targetFrame;
        const int frame = mDocument.getActiveSceneFrame();
        if (scene->anim_nextRelFrameWithKey(frame, targetFrame)) {
            mDocument.setActiveSceneFrame(targetFrame);
        }*/
        if (!setNextKeyframe()) { return false; }
    } else {
        return false;
    }
    return true;
}

void TimelineDockWidget::previewFinished()
{
    //setPlaying(false);
    mFrameStartSpin->setEnabled(true);
    mFrameEndSpin->setEnabled(true);
    mCurrentFrameSpinAct->setEnabled(true);
    showRenderStatus(false);
    mPlayFromBeginningButton->setDisabled(false);
    mStopButton->setDisabled(true);
    mPlayButton->setIcon(QIcon::fromTheme("play"));
    mPlayButton->setText(tr("Play Preview"));
    disconnect(mPlayButton, nullptr, this, nullptr);
    connect(mPlayButton, &QAction::triggered,
            this, &TimelineDockWidget::renderPreview);
}

void TimelineDockWidget::previewBeingPlayed()
{
    mFrameStartSpin->setEnabled(false);
    mFrameEndSpin->setEnabled(false);
    mCurrentFrameSpinAct->setEnabled(false);
    showRenderStatus(false);
    mPlayFromBeginningButton->setDisabled(true);
    mStopButton->setDisabled(false);
    mPlayButton->setIcon(QIcon::fromTheme("pause"));
    mPlayButton->setText(tr("Pause Preview"));
    disconnect(mPlayButton, nullptr, this, nullptr);
    connect(mPlayButton, &QAction::triggered,
            this, &TimelineDockWidget::pausePreview);
}

void TimelineDockWidget::previewBeingRendered()
{
    mFrameStartSpin->setEnabled(false);
    mFrameEndSpin->setEnabled(false);
    mCurrentFrameSpinAct->setEnabled(false);
    showRenderStatus(true);
    mPlayFromBeginningButton->setDisabled(true);
    mStopButton->setDisabled(false);
    mPlayButton->setIcon(QIcon::fromTheme("play"));
    mPlayButton->setText(tr("Play Preview"));
    disconnect(mPlayButton, nullptr, this, nullptr);
    connect(mPlayButton, &QAction::triggered,
            this, &TimelineDockWidget::playPreview);
}

void TimelineDockWidget::previewPaused()
{
    mFrameStartSpin->setEnabled(true);
    mFrameEndSpin->setEnabled(true);
    mCurrentFrameSpinAct->setEnabled(true);
    showRenderStatus(false);
    mPlayFromBeginningButton->setDisabled(true);
    mStopButton->setDisabled(false);
    mPlayButton->setIcon(QIcon::fromTheme("play"));
    mPlayButton->setText(tr("Resume Preview"));
    disconnect(mPlayButton, nullptr, this, nullptr);
    connect(mPlayButton, &QAction::triggered,
            this, &TimelineDockWidget::resumePreview);
}

bool TimelineDockWidget::setPreviewFromStart(PreviewState state)
{
    const auto scene = *mDocument.fActiveScene;
    if (!scene) { return false; }
    if (state != PreviewState::stopped) { interruptPreview(); }
    scene->anim_setAbsFrame(scene->getFrameRange().fMin);
    renderPreview();
    return true;
}

bool TimelineDockWidget::setNextKeyframe()
{
    const auto scene = *mDocument.fActiveScene;
    if (!scene) { return false; }
    int targetFrame;
    const int frame = mDocument.getActiveSceneFrame();
    if (scene->anim_nextRelFrameWithKey(frame, targetFrame)) {
        mDocument.setActiveSceneFrame(targetFrame);
    }
    return true;
}

bool TimelineDockWidget::setPrevKeyframe()
{
    const auto scene = *mDocument.fActiveScene;
    if (!scene) { return false; }
    int targetFrame;
    const int frame = mDocument.getActiveSceneFrame();
    if (scene->anim_prevRelFrameWithKey(frame, targetFrame)) {
        mDocument.setActiveSceneFrame(targetFrame);
    }
    return true;
}

void TimelineDockWidget::resumePreview()
{
    RenderHandler::sInstance->resumePreview();
}

void TimelineDockWidget::updateButtonsVisibility(const CanvasMode mode)
{
    Q_UNUSED(mode)
}

void TimelineDockWidget::pausePreview()
{
    RenderHandler::sInstance->pausePreview();
}

void TimelineDockWidget::playPreview()
{
    RenderHandler::sInstance->playPreview();
}

void TimelineDockWidget::renderPreview()
{
    RenderHandler::sInstance->renderPreview();
}

void TimelineDockWidget::interruptPreview()
{
    RenderHandler::sInstance->interruptPreview();
}

void TimelineDockWidget::updateSettingsForCurrentCanvas(Canvas* const canvas)
{
    if (!canvas) { return; }

    const auto range = canvas->getFrameRange();
    updateFrameRange(range);
    handleCurrentFrameChanged(canvas->anim_getCurrentAbsFrame());

    mCurrentFrameSpin->setDisplayTimeCode(canvas->getDisplayTimecode());
    mFrameStartSpin->setDisplayTimeCode(canvas->getDisplayTimecode());
    mFrameEndSpin->setDisplayTimeCode(canvas->getDisplayTimecode());

    mCurrentFrameSpin->updateFps(canvas->getFps());
    mFrameStartSpin->updateFps(canvas->getFps());
    mFrameEndSpin->updateFps(canvas->getFps());

    connect(canvas, &Canvas::fpsChanged,
            this, [this](const qreal fps) {
        mCurrentFrameSpin->updateFps(fps);
        mFrameStartSpin->updateFps(fps);
        mFrameEndSpin->updateFps(fps);
    });
    connect(canvas, &Canvas::displayTimeCodeChanged,
            this, [this](const bool enabled) {
        mCurrentFrameSpin->setDisplayTimeCode(enabled);
        mFrameStartSpin->setDisplayTimeCode(enabled);
        mFrameEndSpin->setDisplayTimeCode(enabled);
    });

    connect(canvas,
            &Canvas::newFrameRange,
            this, [this](const FrameRange range) {
            updateFrameRange(range);
    });
    connect(canvas, &Canvas::currentFrameChanged,
            this, &TimelineDockWidget::handleCurrentFrameChanged);

    update(); // needed for loaded markers
}

void TimelineDockWidget::stopPreview()
{
    const auto state = RenderHandler::sInstance->currentPreviewState();
    switch (state) {
    case PreviewState::paused:
        interruptPreview();
        break;
    case PreviewState::playing:
    case PreviewState::rendering:
        interruptPreview();
        renderPreview();
        break;
    default:;
    }
}

void TimelineDockWidget::setIn()
{
    const auto scene = *mDocument.fActiveScene;
    if (!scene) { return; }
    const auto frame = scene->getCurrentFrame();
    if (scene->getFrameOut().enabled) {
        if (frame >= scene->getFrameOut().frame) { return; }
    }
    bool apply = frame == 0 ? true : (scene->getFrameIn().frame != frame);
    scene->setFrameIn(apply, frame);
}

void TimelineDockWidget::setOut()
{
    const auto scene = *mDocument.fActiveScene;
    if (!scene) { return; }
    const auto frame = scene->getCurrentFrame();
    if (scene->getFrameIn().enabled) {
        if (frame <= scene->getFrameIn().frame) { return; }
    }
    bool apply = (scene->getFrameOut().frame != frame);
    scene->setFrameOut(apply, frame);
}

void TimelineDockWidget::setMarker()
{
    const auto scene = *mDocument.fActiveScene;
    if (!scene) { return; }
    const auto frame = scene->getCurrentFrame();
    scene->setMarker(frame);
}

void TimelineDockWidget::splitClip()
{
    const auto scene = *mDocument.fActiveScene;
    if (!scene) { return; }
    scene->splitAction();
}
