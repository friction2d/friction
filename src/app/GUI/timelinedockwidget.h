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

#ifndef BOXESLISTANIMATIONDOCKWIDGET_H
#define BOXESLISTANIMATIONDOCKWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QApplication>
#include <QScrollBar>
#include <QComboBox>
#include <QMenuBar>
#include <QLineEdit>
#include <QWidgetAction>
#include <QToolBar>
#include <QStackedWidget>
#include <QToolButton>
#include <QProgressBar>
#include <QTimer>

#include "smartPointers/ememory.h"
#include "framerange.h"
#include "timelinebasewrappernode.h"
#include "widgets/qdoubleslider.h"
#include "renderhandler.h"
#include "widgets/framespinbox.h"

class FrameScrollBar;
class TimelineWidget;
class MainWindow;
class AnimationDockWidget;
class RenderWidget;
class ActionButton;
class Canvas;
class Document;
class LayoutHandler;
class BrushContexedWrapper;

enum class CanvasMode : short;

class TimelineDockWidget : public QWidget
{
public:
    explicit TimelineDockWidget(Document &document,
                                LayoutHandler* const layoutH,
                                MainWindow * const parent);
    bool processKeyPress(QKeyEvent *event);
    void previewFinished();
    void previewBeingPlayed();
    void previewBeingRendered();
    void previewPaused();
    void stepPreview();

    bool setPreviewFromStart(PreviewState state);
    bool setNextKeyframe();
    bool setPrevKeyframe();

    void updateSettingsForCurrentCanvas(Canvas * const canvas);

    void stopPreview();

    void setIn();
    void setOut();
    void setMarker();
    void splitClip();

private:
    void setLoop(const bool loop);
    void interruptPreview();
    void jumpToIntermediateFrame(bool forward);

    void playPreview();
    void renderPreview();
    void pausePreview();
    void resumePreview();
    void setStepPreviewStop(const bool pause = false);
    void setStepPreviewStart();
    void gotoFrame(int frame);

    void updateButtonsVisibility(const CanvasMode mode);

    void updateFrameRange(const FrameRange &range);
    void handleCurrentFrameChanged(int frame);

    void showRenderStatus(bool show);

    void addSpacer();
    void addBlankAction();

    Document& mDocument;
    MainWindow* const mMainWindow;
    QStackedWidget* const mTimelineLayout;

    QToolBar *mToolBar;

    QVBoxLayout *mMainLayout;

    QAction *mPlayFromBeginningButton;
    QAction *mPlayButton;
    QAction *mStopButton;
    QAction *mLoopButton;

    FrameSpinBox *mFrameStartSpin;
    FrameSpinBox *mFrameEndSpin;

    QAction *mFrameRewindAct;
    QAction *mFrameFastForwardAct;
    QAction *mCurrentFrameSpinAct;
    FrameSpinBox *mCurrentFrameSpin;

    QAction *mRenderProgressAct;
    QProgressBar *mRenderProgress;

    QTimer *mStepPreviewTimer;

    QList<TimelineWidget*> mTimelineWidgets;
    //AnimationDockWidget *mAnimationDockWidget;
};

#endif // BOXESLISTANIMATIONDOCKWIDGET_H
