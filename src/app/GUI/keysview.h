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

#ifndef KEYSVIEW_H
#define KEYSVIEW_H

#include <QWidget>
#include <QPointer>
#include <QApplication>

#include "misc/keyfocustarget.h"
#include "smartPointers/ememory.h"
#include "framerange.h"
#include "conncontextobjlist.h"
#include "themesupport.h"


const QList<QColor> ANIMATOR_COLORS = {ThemeSupport::getThemeColorRed(),
                                       ThemeSupport::getThemeColorBlue(),
                                       ThemeSupport::getThemeColorYellow(),
                                       ThemeSupport::getThemeColorPink(),
                                       ThemeSupport::getThemeColorGreen()};

class Key;
class QrealKey;
class QrealAnimator;
class GraphAnimator;
class QrealPoint;
class Scene;
class MainWindow;
class BoxScroller;
class AnimationDockWidget;
class TimelineMovable;
class KeysClipboard;
class Animator;
class BoxScrollWidget;
class TimelineHighlightWidget;

#include "GUI/valueinput.h"

enum class CtrlsMode : short;

class KeysView : public QWidget, public KeyFocusTarget {
    Q_OBJECT
public:
    explicit KeysView(BoxScrollWidget *boxesListVisible,
                      QWidget *parent = nullptr);

    void setCurrentScene(Scene* const scene);

    void setGraphViewed(const bool bT);

    int getMinViewedFrame();
    int getMaxViewedFrame();
    qreal getPixelsPerFrame();

    void updatePixelsPerFrame();

    void addKeyToSelection(Key * const key);
    void removeKeyFromSelection(Key * const key);
    void clearKeySelection();
    void selectKeysInSelectionRect();

    bool hasFrameIn(const int frame);
    bool hasFrameOut(const int frame);
    bool hasFrameMarker(const int frame);

    // graph

    void graphPaint(QPainter *p);
    void graphWheelEvent(QWheelEvent *event);
    bool graphProcessFilteredKeyEvent(QKeyEvent *event);
    void graphResizeEvent(QResizeEvent *);
    void graphAddViewedAnimator(GraphAnimator * const animator);
    void graphIncScale(const qreal inc);
    void graphSetScale(const qreal scale);
    void graphUpdateDimensions();
    void graphIncMinShownVal(const qreal inc);
    void graphSetMinShownVal(const qreal newMinShownVal);
    void graphGetValueAndFrameFromPos(const QPointF &pos,
                                      qreal &value, qreal &frame) const;
    void graphMiddleMove(const QPointF &movePos);
    void gMiddlePress(const QPointF &pressPos);
    void gMouseRelease();
    void graphMousePress(const QPointF &pressPos);
    void graphMiddleRelease();
    void graphSetCtrlsModeForSelected(const CtrlsMode mode);
    void graphDeletePressed();
    void graphResetValueScaleAndMinShown();
    void scheduleGraphUpdateAfterKeysChanged();
    void graphUpdateAfterKeysChangedIfNeeded();
    void deleteSelectedKeys();
    void middleMove(const QPointF &movePos);
    void middlePress(const QPointF &pressPos);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);
    void graphRemoveViewedAnimator(GraphAnimator * const animator);
    void clearHoveredMovable();
    bool KFT_keyPressEvent(QKeyEvent *event);
protected:
    ValueInput mValueInput;

    QPoint mLastMovePos;

#ifdef Q_OS_MAC
    bool event(QEvent *e);
#endif

    void resizeEvent(QResizeEvent *e);
    void wheelEvent(QWheelEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *);

    void leaveEvent(QEvent *) {
        clearHovered();
        update();
    }
protected:
    void KFT_setFocusToWidget();
    void KFT_clearFocus();

    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
signals:
    void changedViewedFrames(FrameRange);
    void wheelEventSignal(QWheelEvent*);
    void statusMessage(const QString &message);

#ifdef Q_OS_MAC
    void nativeEventSignal(QNativeGestureEvent *e);
    void panEventSignal(QWheelEvent *e);
#endif

public:
    static QColor sGetAnimatorColor(const int i);

    void graphResetValueScaleAndMinShownAction();
    void graphSetValueLinesDisabled(const bool disabled);
    void graphUpdateAfterKeysChanged();

    void setFramesRange(const FrameRange &range);

    void graphMakeSegmentsLinearAction();
    void graphMakeSegmentsSmoothAction();
    void graphSetSmoothCtrlAction();
    void graphSetSymmetricCtrlAction();
    void graphSetCornerCtrlAction();

    void setViewedVerticalRange(const int top, const int bottom);
    void clearHovered();

    int graphGetAnimatorId(GraphAnimator * const anim);
    QrealPoint *graphGetPointAtPos(const QPointF &pressPos) const;
    bool graphValidateVisible(GraphAnimator * const animator);
    void graphUpdateVisible();
    bool graphIsSelected(GraphAnimator * const anim);
    void graphEasingAction(const QString &easing);
    void graphEasingApply(QrealAnimator *anim,
                          const FrameRange &range,
                          const QString &easing);
    bool graphEasingApplyExpression(QrealAnimator *anim,
                                    const FrameRange &range,
                                    const QString &easing);

    TimelineHighlightWidget *requestHighlighter();
private:
    void getKeysInRect(QRectF selectionRect,
                       const qreal pixelsPerFrame,
                       QList<Key *> &listKeys);
    void drawKeys(QPainter * const p, const qreal pixelsPerFrame,
                  const FrameRange &viewedFrameRange);
    Key *getKeyAtPos(const int pressX, const int pressY,
                     const qreal pixelsPerFrame,
                     const int minViewedFrame);
    TimelineMovable *getRectangleMovableAtPos(
            const int pressX, const int pressY,
            const qreal pixelsPerFrame,
            const int minViewedFrame);

    void graphAddToViewedAnimatorList(GraphAnimator * const animator);
    qreal xToFrame(const qreal x) const;

    void cancelTransform();
    void finishTransform();

    void scrollRight();
    void scrollLeft();
    void handleMouseMoveScroll(const QPoint &pos);
    void handleMouseMove(const QPoint &pos, const Qt::MouseButtons &buttons);
    void graphConstrainAnimatorCtrlsFrameValues();
    void graphGetAnimatorsMinMaxValue(qreal &minVal, qreal &maxVal);
    void graphMakeSegmentsSmoothAction(const bool smooth);
    void clearHoveredKey();

    stdsptr<KeysClipboard> getSelectedKeysClipboardContainer();

    QTimer *mScrollTimer;

    void updateHovered(const QPoint &posU);

    QPointer<TimelineMovable> mHoveredMovable;
    stdptr<Key> mHoveredKey;

    void grabMouseAndTrack() {
        mIsMouseGrabbing = true;
#ifndef QT_DEBUG
        grabMouse();
#endif
    }

    void releaseMouseAndDontTrack() {
        mMoveDFrame = 0;
        mMovingKeys = false;
        mScalingKeys = false;
        mMovingRect = false;
        mSelecting = false;
        mIsMouseGrabbing = false;
#ifndef QT_DEBUG
        releaseMouse();
#endif
    }

    bool mIsMouseGrabbing = false;
    int mViewedTop = 0;
    int mViewedBottom = 0;

    TimelineHighlightWidget* mHighlighter = nullptr;

    BoxScrollWidget *mBoxesListWidget;
    QRectF mSelectionRect;
    bool mSelecting = false;
    bool mGraphViewed = false;
    qreal mPixelsPerFrame;
    QPointF mMiddlePressPos;

    bool mGraphUpdateAfterKeysChangedNeeded = false;

    QPointer<TimelineMovable> mLastPressedMovable;
    Key *mLastPressedKey = nullptr;
    bool mFirstMove = false;
    qreal mMoveDFrame = 0;
    QPoint mLastPressPos;
    bool mMovingKeys = false;
    bool mScalingKeys = false;
    bool mMovingRect = false;
    bool mMoveAllSelected = false;
    bool mPressedCtrlPoint = false;

    qptr<Scene> mCurrentScene;
    QList<Animator*> mSelectedKeysAnimators;
    ConnContextObjList<GraphAnimator*> mGraphAnimators;

    int mMinViewedFrame = 0;
    int mMaxViewedFrame = 50;

    int mSavedMinViewedFrame = 0;
    int mSavedMaxViewedFrame = 0;

    bool mPanEvent = false;

    // graph

    bool graph_mValueLinesVisible = true;
    qreal mPixelsPerValUnit = 0;
    qreal mMinShownVal = 0;
    stdptr<QrealPoint> mGHoveredPoint;
    stdptr<QrealPoint> mGPressedPoint;
    qreal mMinMoveVal = 0;
    qreal mMaxMoveVal = 0;
    qreal mMinMoveFrame = 0;
    qreal mMaxMoveFrame = 0;
    qreal mSavedMinShownValue = 0;
    qreal mValueInc = 0;
    int mValuePrec = 2;
};

#endif // KEYSVIEW_H
