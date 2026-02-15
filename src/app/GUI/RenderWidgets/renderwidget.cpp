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

#include "renderwidget.h"
#include "canvas.h"
#include "GUI/global.h"
#include "renderinstancewidget.h"
#include "optimalscrollarena/scrollarea.h"
#include "videoencoder.h"
#include "renderhandler.h"
#include "videoencoder.h"
#include "themesupport.h"
#include "../mainwindow.h"
#include "../timelinedockwidget.h"

RenderWidget::RenderWidget(QWidget *parent)
    : QWidget(parent)
    , mMainLayout(nullptr)
    , mRenderProgressBar(nullptr)
    , mStartRenderButton(nullptr)
    , mStopRenderButton(nullptr)
    , mAddRenderButton(nullptr)
    , mClearQueueButton(nullptr)
    , mContWidget(nullptr)
    , mContLayout(nullptr)
    , mScrollArea(nullptr)
    , mCurrentRenderedSettings(nullptr)
    , mState(RenderState::none)
{
    mMainLayout = new QVBoxLayout(this);
    mMainLayout->setMargin(0);
    mMainLayout->setSpacing(0);
    setLayout(mMainLayout);

    const auto topWidget = new QWidget(this);
    topWidget->setContentsMargins(0, 0, 0, 0);
    const auto topLayout = new QHBoxLayout(topWidget);

    const auto bottomWidget = new QWidget(this);
    bottomWidget->setContentsMargins(0, 0, 0, 0);
    const auto bottomLayout = new QHBoxLayout(bottomWidget);

    setPalette(ThemeSupport::getDarkPalette());
    setAutoFillBackground(true);

    bottomWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    topWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    mRenderProgressBar = new QProgressBar(this);
    mRenderProgressBar->setMinimumWidth(10);
    setSizePolicy(QSizePolicy::Expanding,
                  QSizePolicy::Expanding);
    mRenderProgressBar->setFormat("");
    mRenderProgressBar->setValue(0);

    mStartRenderButton = new QPushButton(QIcon::fromTheme("render_animation"),
                                         tr("Render"),
                                         this);
    mStartRenderButton->setFocusPolicy(Qt::NoFocus);
    mStartRenderButton->setSizePolicy(QSizePolicy::Preferred,
                                      QSizePolicy::Preferred);
    connect(mStartRenderButton, &QPushButton::pressed,
            this, qOverload<>(&RenderWidget::render));

    mStopRenderButton = new QPushButton(QIcon::fromTheme("cancel"),
                                        QString(),
                                        this);
    mStopRenderButton->setToolTip(tr("Stop Rendering"));
    mStopRenderButton->setFocusPolicy(Qt::NoFocus);
    mStopRenderButton->setSizePolicy(QSizePolicy::Preferred,
                                     QSizePolicy::Preferred);
    connect(mStopRenderButton, &QPushButton::pressed,
            this, &RenderWidget::stopRendering);
    mStopRenderButton->setEnabled(false);

    mAddRenderButton = new QPushButton(QIcon::fromTheme("plus"),
                                       QString(),
                                       this);
    mAddRenderButton->setToolTip(tr("Add current scene to queue"));
    mAddRenderButton->setFocusPolicy(Qt::NoFocus);
    mAddRenderButton->setSizePolicy(QSizePolicy::Preferred,
                                    QSizePolicy::Preferred);
    connect(mAddRenderButton, &QPushButton::pressed,
            this, []() {
        MainWindow::sGetInstance()->addCanvasToRenderQue();
    });

    mClearQueueButton = new QPushButton(QIcon::fromTheme("trash"),
                                        QString(),
                                        this);
    mClearQueueButton->setToolTip(tr("Clear Queue"));
    mClearQueueButton->setFocusPolicy(Qt::NoFocus);
    mClearQueueButton->setSizePolicy(QSizePolicy::Preferred,
                                     QSizePolicy::Preferred);
    connect(mClearQueueButton, &QPushButton::pressed,
            this, &RenderWidget::clearRenderQueue);

    eSizesUI::widget.add(mStartRenderButton, [this](const int size) {
        Q_UNUSED(size)
        mStartRenderButton->setFixedHeight(eSizesUI::button);
        mStopRenderButton->setFixedHeight(eSizesUI::button);
        mAddRenderButton->setFixedHeight(eSizesUI::button);
        mClearQueueButton->setFixedHeight(eSizesUI::button);
    });

    mContWidget = new QWidget(this);
    mContWidget->setPalette(ThemeSupport::getDarkPalette());
    mContWidget->setAutoFillBackground(true);
    mContWidget->setContentsMargins(0, 0, 0, 0);
    mContLayout = new QVBoxLayout(mContWidget);
    mContLayout->setAlignment(Qt::AlignTop);
    mContLayout->setMargin(0);
    mContLayout->setSpacing(0);
    mContWidget->setLayout(mContLayout);
    mScrollArea = new ScrollArea(this);
    mScrollArea->setWidget(mContWidget);
    mScrollArea->setWidgetResizable(true);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    topLayout->addWidget(mAddRenderButton);
    topLayout->addStretch();
    topLayout->addWidget(mClearQueueButton);

    bottomLayout->addWidget(mStartRenderButton);
    bottomLayout->addWidget(mRenderProgressBar);
    bottomLayout->addWidget(mStopRenderButton);

    mMainLayout->addWidget(topWidget);
    mMainLayout->addWidget(mScrollArea);
    mMainLayout->addWidget(bottomWidget);

    const auto vidEmitter = VideoEncoder::sInstance->getEmitter();
    connect(vidEmitter, &VideoEncoderEmitter::encodingStarted,
            this, &RenderWidget::handleRenderStarted);

    connect(vidEmitter, &VideoEncoderEmitter::encodingFinished,
            this, &RenderWidget::handleRenderFinished);
    connect(vidEmitter, &VideoEncoderEmitter::encodingFinished,
            this, &RenderWidget::sendNextForRender);

    connect(vidEmitter, &VideoEncoderEmitter::encodingInterrupted,
            this, &RenderWidget::clearAwaitingRender);
    connect(vidEmitter, &VideoEncoderEmitter::encodingInterrupted,
            this, &RenderWidget::handleRenderInterrupted);

    connect(vidEmitter, &VideoEncoderEmitter::encodingFailed,
            this, &RenderWidget::handleRenderFailed);
    connect(vidEmitter, &VideoEncoderEmitter::encodingFailed,
            this, &RenderWidget::sendNextForRender);

    connect(vidEmitter, &VideoEncoderEmitter::encodingStartFailed,
            this, &RenderWidget::handleRenderFailed);
    connect(vidEmitter, &VideoEncoderEmitter::encodingStartFailed,
            this, &RenderWidget::sendNextForRender);
}

void RenderWidget::createNewRenderInstanceWidgetForCanvas(Canvas *canvas)
{
    const auto wid = new RenderInstanceWidget(canvas, this);
    addRenderInstanceWidget(wid);
}

void RenderWidget::addRenderInstanceWidget(RenderInstanceWidget *wid)
{
    mContLayout->addWidget(wid);
    connect(wid, &RenderInstanceWidget::destroyed,
            this, [this, wid]() {
        mRenderInstanceWidgets.removeOne(wid);
        mAwaitingSettings.removeOne(wid);
    });
    connect(wid, &RenderInstanceWidget::duplicate,
            this, [this](RenderInstanceSettings& sett) {
        addRenderInstanceWidget(new RenderInstanceWidget(sett, this));
    });
    mRenderInstanceWidgets << wid;
}

void RenderWidget::handleRenderState(const RenderState &state)
{
    mState = state;

    QString renderStateFormat;
    switch (mState) {
    case RenderState::rendering:
        renderStateFormat = tr("%p%");
        break;
    case RenderState::error:
        renderStateFormat = tr("Error");
        break;
    case RenderState::paused:
        renderStateFormat = tr("Paused %p%");
        break;
    case RenderState::waiting:
        renderStateFormat = tr("Waiting %p%");
        break;
    default:
        renderStateFormat = "";
        break;
    }
    bool isIdle = (mState == RenderState::error ||
                   mState == RenderState::finished ||
                   mState == RenderState::none);
    mStartRenderButton->setEnabled(isIdle);
    mStopRenderButton->setEnabled(!isIdle);
    mAddRenderButton->setEnabled(isIdle);
    mRenderProgressBar->setFormat(renderStateFormat);

    const auto timeline = MainWindow::sGetInstance()->getTimeLineWidget();
    if (timeline) { timeline->setEnabled(isIdle); }

    emit renderStateChanged(renderStateFormat, mState);

    if (isIdle) {
        emit progress(mRenderProgressBar->maximum(), mRenderProgressBar->maximum());
        mRenderProgressBar->setValue(0);
        mRenderProgressBar->setRange(0, 100);
    }
}

void RenderWidget::handleRenderStarted()
{
    handleRenderState(RenderState::rendering);
}

void RenderWidget::handleRenderFinished()
{
    handleRenderState(RenderState::finished);
}

void RenderWidget::handleRenderInterrupted()
{
    handleRenderState(RenderState::finished);
}

void RenderWidget::handleRenderFailed()
{
    handleRenderState(RenderState::error);
}

void RenderWidget::setRenderedFrame(const int frame)
{
    if (!mCurrentRenderedSettings) { return; }
    mRenderProgressBar->setValue(frame);
    emit progress(frame, mRenderProgressBar->maximum());
}

void RenderWidget::clearRenderQueue()
{
    if (mState == RenderState::rendering ||
        mState == RenderState::paused ||
        mState == RenderState::waiting) {
        stopRendering();
    }
    for (int i = mRenderInstanceWidgets.count() - 1; i >= 0; i--) {
        delete mRenderInstanceWidgets.at(i);
    }
}

void RenderWidget::write(eWriteStream &dst) const
{
    dst << mRenderInstanceWidgets.count();
    for (const auto widget : mRenderInstanceWidgets) {
        widget->write(dst);
    }
}

void RenderWidget::read(eReadStream &src)
{
    int nWidgets; src >> nWidgets;
    for (int i = 0; i < nWidgets; i++) {
        const auto wid = new RenderInstanceWidget(nullptr, this);
        wid->read(src);
        addRenderInstanceWidget(wid);
    }
}

void RenderWidget::updateRenderSettings()
{
    for (const auto &wid: qAsConst(mRenderInstanceWidgets)) {
        wid->updateRenderSettings();
    }
}

void RenderWidget::render(RenderInstanceSettings &settings)
{
    const RenderSettings &renderSettings = settings.getRenderSettings();
    mRenderProgressBar->setRange(renderSettings.fMinFrame,
                                 renderSettings.fMaxFrame);
    mRenderProgressBar->setValue(renderSettings.fMinFrame);
    handleRenderState(RenderState::waiting);
    mCurrentRenderedSettings = &settings;

    // the scene we want to render MUST be current if multiple scenes are in queue
    const auto scene = mCurrentRenderedSettings->getTargetCanvas();
    if (scene) {
        const auto handler = MainWindow::sGetInstance()->getLayoutHandler();
        const int index = handler->getSceneId(scene);
        if (index > -1 && !handler->isCurrentScene(index)) {
            handler->setCurrentScene(index);
        }
    }

    RenderHandler::sInstance->renderFromSettings(&settings);
    connect(&settings, &RenderInstanceSettings::renderFrameChanged,
            this, &RenderWidget::setRenderedFrame);
    connect(&settings, &RenderInstanceSettings::stateChanged,
            this, &RenderWidget::handleRenderState);
}

void RenderWidget::render()
{
    int c = 0;
    for (RenderInstanceWidget *wid : qAsConst(mRenderInstanceWidgets)) {
        if (!wid->isChecked()) { continue; }
        mAwaitingSettings << wid;
        wid->getSettings().setCurrentState(RenderState::waiting);
        c++;
    }
    if (c > 0) { handleRenderState(RenderState::waiting); }
    else { handleRenderState(RenderState::none); }
    sendNextForRender();
}

void RenderWidget::stopRendering()
{
    clearAwaitingRender();
    VideoEncoder::sInterruptEncoding();
    if (mCurrentRenderedSettings) {
        disconnect(mCurrentRenderedSettings, nullptr, this, nullptr);
        mCurrentRenderedSettings = nullptr;
    }
}

void RenderWidget::clearAwaitingRender()
{
    for (RenderInstanceWidget *wid : qAsConst(mAwaitingSettings)) {
        wid->getSettings().setCurrentState(RenderState::none);
    }
    handleRenderState(RenderState::none);
    mAwaitingSettings.clear();
}

void RenderWidget::sendNextForRender()
{
    if (mAwaitingSettings.isEmpty()) { return; }
    const auto wid = mAwaitingSettings.takeFirst();
    if (wid->isChecked() && wid->getSettings().getTargetCanvas()) {
        //disableButtons();
        wid->setDisabled(true);
        render(wid->getSettings());
    } else { sendNextForRender(); }
}

int RenderWidget::count()
{
    return mRenderInstanceWidgets.count();
}
