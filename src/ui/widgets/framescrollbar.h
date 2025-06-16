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

#ifndef ANIMATiONWIDGETSCROLLBAR_H
#define ANIMATiONWIDGETSCROLLBAR_H

#include "ui_global.h"

#include <QWidget>
#include <QFontMetrics>

#include "smartPointers/ememory.h"
#include "Private/scene.h"
#include "themesupport.h"

class HddCachableCacheHandler;

class UI_EXPORT FrameScrollBar : public QWidget
{
    Q_OBJECT
public:
    struct InteractiveMarker
    {
        int frame;
        bool enabled;
        bool in;
        bool out;
    };
    explicit FrameScrollBar(const int minSpan,
                            const int maxSpan,
                            const bool range,
                            const bool clamp,
                            const bool bottom = false,
                            QWidget *parent = nullptr);
    bool setFirstViewedFrame(const int firstFrame);
    void setFramesSpan(int newSpan);

    int getMaxFrame();
    int getMinFrame();

    bool hasFrameIn(const int frame);
    bool hasFrameOut(const int frame);
    bool hasFrameMarker(const int frame);
    const QString getFrameMarkerText(const int frame);
    const QPair<bool,int> getFrameIn();
    const QPair<bool,int> getFrameOut();

    FrameRange getViewedRange() const;
    int getFirstViewedFrame() const;
    int getLastViewedFrame() const;

    void setHandleColor(const QColor &col) {
        mHandleColor = col;
    }

    void setCurrentCanvas(Scene * const canvas);

    void setDisplayedFrameRange(const FrameRange& range);
    void setViewedFrameRange(const FrameRange& range);
    void setCanvasFrameRange(const FrameRange& range);
    void callWheelEvent(QWheelEvent *event);

#ifdef Q_OS_MAC
    void callNativeGestures(QNativeGestureEvent *e);
    void callPanEvent(QWheelEvent *e);
#endif

protected:
    qreal posToFrame(int xPos);
    void paintEvent(QPaintEvent *);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
signals:
    void triggeredFrameRangeChange(FrameRange);
    void frameRangeChange(FrameRange);
private:
    void emitChange();
    void emitTriggeredChange();

    bool mClamp;
    bool mDisplayTime = false;
    bool mRange;
    bool mPressed = false;
    bool mBottom;

    int mSavedFirstFrame;

    int mFirstViewedFrame = 0;
    int mViewedFramesSpan = 50;

    int mDrawFrameInc = 20;

    FrameRange mFrameRange{0, 200};

    int mMinSpan;
    int mMaxSpan;
    int mSpanInc;

    FrameRange mCanvasRange{0, 200};

    qreal mFps;
    qreal mLastMousePressFrame;

    QFontMetrics mFm;

    InteractiveMarker mGrabbedMarker;

    QColor mHandleColor = ThemeSupport::getThemeButtonBorderColor();
    qptr<Scene> mCurrentScene;
};

#endif // ANIMATiONWIDGETSCROLLBAR_H
