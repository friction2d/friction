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

#include "framescrollbar.h"
#include <QMouseEvent>
#include <QPainter>
#include <QMenu>

#include "GUI/global.h"
#include "colorhelpers.h"
#include "appsupport.h"

FrameScrollBar::FrameScrollBar(const int minSpan,
                               const int maxSpan,
                               const bool range,
                               const bool clamp,
                               const bool bottom,
                               QWidget *parent)
    : QWidget(parent)
    , mFm(QFontMetrics(font()))
    , mGrabbedMarker({0, false, false, false})
{
    mDisplayTime = AppSupport::getSettings("ui",
                                           "DisplayTimecode",
                                           false).toBool();
    mMinSpan = minSpan;
    mMaxSpan = maxSpan;
    mRange = range;
    mClamp = clamp;
    mBottom = bottom;
    setFramesSpan(0);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
}

qreal FrameScrollBar::posToFrame(int xPos) {
    return (xPos - eSizesUI::widget/2)*
            (mFrameRange.fMax - mFrameRange.fMin + (mRange ? 0 : 1) ) /
            (qreal(width()) - 2*eSizesUI::widget) + mFrameRange.fMin;
}

void FrameScrollBar::setCurrentCanvas(Canvas * const canvas)
{
    mCurrentCanvas = canvas;
    //if (mCurrentCanvas) { mDisplayTime = mCurrentCanvas->getDisplayTimecode(); }
    if (mCurrentCanvas) { mCurrentCanvas->setDisplayTimecode(mDisplayTime); }
    update();
}

void FrameScrollBar::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), ThemeSupport::getThemeBaseDarkerColor());

    const int dFrame = mFrameRange.fMax - mFrameRange.fMin + (mRange ? 0 : 1);
    if (dFrame <= 0) { return; }
    const qreal pixPerFrame = (width() - 2.*eSizesUI::widget)/dFrame;
    if (pixPerFrame < 0) { return; }

    const int f0 = -qCeil(0.5*eSizesUI::widget/pixPerFrame);
    const int minFrame = mFrameRange.fMin + f0;
    const qreal x0 = f0*pixPerFrame + eSizesUI::widget*0.5;

    const int f1 = qCeil(1.5*eSizesUI::widget/pixPerFrame);
    const int maxFrame = mFrameRange.fMax + f1;
    const qreal w1 = width() - 1.5*eSizesUI::widget + f1*pixPerFrame - x0;

    // draw cache
    if (mCurrentCanvas) {
        const qreal fps = mCurrentCanvas->getFps();
        mFps = fps;
        if (!mRange) {
            const int soundHeight = eSizesUI::widget / 1.5;
            const int rasterHeight = eSizesUI::widget - soundHeight;
            const QRectF rasterRect(x0, 0, w1, rasterHeight);
            const auto& rasterCache = mCurrentCanvas->getSceneFramesHandler();
            rasterCache.drawCacheOnTimeline(&p, rasterRect, minFrame, maxFrame);

            const QRectF soundRect(x0, rasterHeight, w1, soundHeight);
            const auto& soundCache = mCurrentCanvas->getSoundCacheHandler();
            soundCache.drawCacheOnTimeline(&p, soundRect, minFrame, maxFrame, fps);
        }
    }

    QColor col = mHandleColor;

    const qreal inc = mDrawFrameInc*pixPerFrame;
    const int minMod = minFrame%mDrawFrameInc;
    qreal xL = (-minMod + (mRange ? 0. : 0.5))*pixPerFrame + x0;
    int currentFrame = minFrame - minMod;
    const qreal threeFourthsHeight = height()*0.75;
    const qreal maxX = width() + eSizesUI::widget;

    QRectF handleRect;
    const int hLeftFrames = mFirstViewedFrame - minFrame;
    const qreal handleFixedWidth = 16;
    const qreal handleWidth = (mViewedFramesSpan*pixPerFrame);
    const qreal handleLeft = mBottom ? (hLeftFrames*pixPerFrame + x0) : ((hLeftFrames*pixPerFrame + x0)+(handleWidth/2)-(handleFixedWidth/2));

    handleRect.setLeft(handleLeft);
    handleRect.setTop(mBottom ? 2 : 0);
    handleRect.setWidth(mBottom ? handleWidth : handleFixedWidth);
    handleRect.setBottom(mBottom ? 6 : height()/3);
    if (mRange) { p.fillRect(handleRect, col); }

    // draw the stuff ...
    if (!mRange) {
        const auto colOrange = ThemeSupport::getThemeFrameMarkerColor();
        const auto colGreen = ThemeSupport::getThemeColorGreen();
        const auto frameIn = getFrameIn();
        const auto frameOut = getFrameOut();

        p.translate(eSizesUI::widget/2, 0);
        qreal xT = pixPerFrame*0.5;
        int iInc = 1;
        bool mult5 = true;
        while (iInc*pixPerFrame < eSizesUI::widget/2) {
            if (mult5) { iInc *= 5; }
            else { iInc *= 2; }
        }
        int mMinFrame = mFrameRange.fMin;
        int mMaxFrame = mFrameRange.fMax;
        mMinFrame += qCeil((-xT)/pixPerFrame);
        mMinFrame = mMinFrame - mMinFrame%iInc - 1;
        mMaxFrame += qFloor((width() - 40 - xT)/pixPerFrame) - mMaxFrame%iInc;

        // draw in/out range
        if (frameIn.first && frameOut.first) {
            p.setPen(Qt::NoPen);
            const qreal xTT1 = xT + (frameIn.second - mFrameRange.fMin) * pixPerFrame;
            const qreal xTT2 = xT + (frameOut.second - mFrameRange.fMin) * pixPerFrame;
            const int h = mFm.height();
            const auto rect = QRectF(xTT1, h, xTT2 - xTT1, height() - h);
            p.fillRect(rect, QBrush(ThemeSupport::getThemeColorGreenDark(),
                                    Qt::SolidPattern));
            p.drawRect(rect);
        }

        // draw markers (and in/out)
        for (int i = minFrame; i <= maxFrame; i++) {
            bool hasIn = frameIn.first ? hasFrameIn(i) : false;
            bool hasOut = frameOut.first ? hasFrameOut(i) : false;
            bool hasMark = hasFrameMarker(i);
            if (!hasIn && !hasOut && !hasMark) { continue; }
            const QColor col = hasMark ? colOrange : colGreen;
            p.setPen(QPen(col, 2, Qt::SolidLine));
            const qreal xTT = xT + (i - mFrameRange.fMin + 1)*pixPerFrame;
            p.drawLine(QPointF(xTT, 0), QPointF(xTT, mFm.height() + 4));

            const QString drawValue = hasIn ? tr("In") : hasOut ? tr("Out") : getFrameMarkerText(i);
            p.setPen(Qt::NoPen);
            const auto rect = QRectF(xTT + 1, 0,
                                     mFm.horizontalAdvance(drawValue) + 2,
                                     mFm.height());
            p.fillRect(rect, QBrush(col, Qt::SolidPattern));
            p.drawRect(rect);
            p.setPen(Qt::black);
            p.drawText(rect, Qt::AlignCenter, drawValue);
        }

        // draw minor
        p.setPen(QPen(Qt::darkGray, 2));
        for (int i = mMinFrame; i <= mMaxFrame; i += iInc) {
            const qreal xTT = xT + (i - mFrameRange.fMin + 1)*pixPerFrame;
            p.drawLine(QPointF(xTT, threeFourthsHeight + 6), QPointF(xTT, height()));
        }

        // draw main
        p.setPen(QPen(Qt::white, 2));
        p.translate(-(eSizesUI::widget/2), 0);
        bool timecode = mDisplayTime && mFps > 0;
        if (qAbs(pixPerFrame) > 0.11) {
            while (xL < maxX) {
                p.drawLine(QPointF(xL, threeFourthsHeight + 4), QPointF(xL, height()));
                QString drawValue = QString::number(currentFrame);
                if (timecode) { drawValue = AppSupport::getTimeCodeFromFrame(currentFrame, mFps); }
                p.drawText(QRectF(xL - inc, 0, 2 * inc, threeFourthsHeight + 18), Qt::AlignCenter, drawValue);
                xL += inc;
                currentFrame += mDrawFrameInc;
            }
        }

        // draw handle
        if (!mGrabbedMarker.enabled) {
            QPainterPath path;
            path.moveTo(handleRect.left() + (handleRect.width() / 2), handleRect.bottom());
            path.lineTo(handleRect.topLeft());
            path.lineTo(handleRect.topRight());
            path.lineTo(handleRect.left() + (handleRect.width() / 2), handleRect.bottom());
            p.fillPath(path, ThemeSupport::getThemeHighlightColor());
        }
    }

    p.end();
}

void FrameScrollBar::setFramesSpan(int newSpan) {
    mViewedFramesSpan = clampInt(newSpan, mMinSpan, mMaxSpan);
    if(mClamp) setFirstViewedFrame(mFirstViewedFrame);
}

int FrameScrollBar::getMaxFrame() {
    return mFrameRange.fMax;
}

int FrameScrollBar::getMinFrame() {
    return mFrameRange.fMin;
}

bool FrameScrollBar::hasFrameIn(const int frame)
{
    if (!mCurrentCanvas) { return false; }
    const auto frameIn = mCurrentCanvas->getFrameIn();
    if ((frameIn.enabled && frame + 1 == frameIn.frame)) { return true; }
    return false;
}

bool FrameScrollBar::hasFrameOut(const int frame)
{
    if (!mCurrentCanvas) { return false; }
    const auto frameOut = mCurrentCanvas->getFrameOut();
    if ((frameOut.enabled && frame + 1 == frameOut.frame)) { return true; }
    return false;
}

bool FrameScrollBar::hasFrameMarker(const int frame)
{
    if (!mCurrentCanvas) { return false; }
    return mCurrentCanvas->hasMarkerEnabled(frame + 1);
}

const QString FrameScrollBar::getFrameMarkerText(const int frame)
{
    if (!mCurrentCanvas) { return QString(); }
    return mCurrentCanvas->getMarkerText(frame + 1);
}

const QPair<bool, int> FrameScrollBar::getFrameIn()
{
    if (!mCurrentCanvas) { return {false, 0}; }
    const auto in = mCurrentCanvas->getFrameIn();
    return {in.enabled, in.frame};
}

const QPair<bool, int> FrameScrollBar::getFrameOut()
{
    if (!mCurrentCanvas) { return {false, 0}; }
    const auto out = mCurrentCanvas->getFrameOut();
    return {out.enabled, out.frame};
}

void FrameScrollBar::wheelEvent(QWheelEvent *event)
{
#ifdef Q_OS_MAC
    if (event->angleDelta().y() == 0) { return; }
#endif
    if(mRange) {
        if(event->modifiers() & Qt::CTRL) {
            if(event->angleDelta().y() > 0) {
                setFirstViewedFrame(mFirstViewedFrame - mViewedFramesSpan/20);
            } else {
                setFirstViewedFrame(mFirstViewedFrame + mViewedFramesSpan/20);
            }
        } else {
            int newFramesSpan = mViewedFramesSpan;
            if(event->angleDelta().y() > 0) newFramesSpan *= 0.85;
            else newFramesSpan *= 1.15;
            setFramesSpan(newFramesSpan);
        }
    } else {
        if(event->angleDelta().y() > 0) {
            setFirstViewedFrame(mFirstViewedFrame - 1);
        } else {
            setFirstViewedFrame(mFirstViewedFrame + 1);
        }
    }

    emitTriggeredChange();
    update();
}

bool FrameScrollBar::setFirstViewedFrame(const int firstFrame) {
    if(mClamp) {
        if(mRange) {
            mFirstViewedFrame = clampInt(firstFrame, mFrameRange.fMin, mFrameRange.fMax -
                                          mViewedFramesSpan);
        } else {
            mFirstViewedFrame = clampInt(firstFrame, mFrameRange.fMin, mFrameRange.fMax);
        }
        return true;
    } else {
        mFirstViewedFrame = firstFrame;
        return true;
    }

}

void FrameScrollBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton && !mRange) {
        QMenu menu(this);
        /*QAction *clampAction = new QAction("Clamp", this);
        clampAction->setCheckable(true);
        clampAction->setChecked(mClamp);
        menu.addAction(clampAction);*/

        //menu.addSeparator();

        QAction *timeAction = new QAction(QIcon::fromTheme("visible"),
                                          tr("Display Timecodes"), this);
        timeAction->setCheckable(true);
        timeAction->setChecked(mDisplayTime);
        menu.addAction(timeAction);

        QAction *framesAction = new QAction(QIcon::fromTheme("visible"),
                                            tr("Display Frames"), this);
        framesAction->setCheckable(true);
        framesAction->setChecked(!mDisplayTime);
        menu.addAction(framesAction);

        bool hasMarker = mCurrentCanvas ? mCurrentCanvas->hasMarker(mCurrentCanvas->getCurrentFrame()) : false;

        const auto setFrameInAct = new QAction(QIcon::fromTheme("range-in"),
                                               tr("Set In"), this);
        const auto setFrameOutAct = new QAction(QIcon::fromTheme("range-out"),
                                                tr("Set Out"), this);
        const auto clearFrameOutAct = new QAction(QIcon::fromTheme("range-clear"),
                                                  tr("Clear In/Out"), this);
        const auto setMarkerAct = new QAction(QIcon::fromTheme("markers-add"),
                                              tr(hasMarker ? "Remove Marker" : "Add Marker"), this);
        const auto clearMarkersAct = new QAction(QIcon::fromTheme("trash"),
                                                 tr("Clear Markers"), this);
        const auto openMarkerEditorAct = new QAction(QIcon::fromTheme("markers-edit"),
                                                     tr("Edit Markers"), this);
        const auto splitDurationAct = new QAction(QIcon::fromTheme("cut"),
                                                  tr("Split Clip"), this);

        menu.addSeparator();
        menu.addAction(setFrameInAct);
        menu.addAction(setFrameOutAct);
        menu.addAction(clearFrameOutAct);
        menu.addSeparator();
        menu.addAction(setMarkerAct);
        menu.addAction(openMarkerEditorAct);
        menu.addAction(clearMarkersAct);
        menu.addSeparator();
        menu.addAction(splitDurationAct);

        QAction* selectedAction = menu.exec(event->globalPos());
        if (selectedAction) {
            /*if (selectedAction == clampAction) {
                mClamp = !mClamp;
            } else*/ if (selectedAction == framesAction) {
                mDisplayTime = false;
                update();
                if (mCurrentCanvas) {
                    mCurrentCanvas->setDisplayTimecode(mDisplayTime);
                }
                AppSupport::setSettings("ui", "DisplayTimecode", mDisplayTime);
            } else if (selectedAction == timeAction) {
                mDisplayTime = true;
                update();
                if (mCurrentCanvas) {
                    mCurrentCanvas->setDisplayTimecode(mDisplayTime);
                }
                AppSupport::setSettings("ui", "DisplayTimecode", mDisplayTime);
            } else if (selectedAction == clearFrameOutAct) {
                if (mCurrentCanvas) {
                    mCurrentCanvas->clearFrameInOut();
                }
            } else if (selectedAction == setFrameInAct) {
                if (mCurrentCanvas) {
                    const auto frame = mCurrentCanvas->getCurrentFrame();
                    if (mCurrentCanvas->getFrameOut().enabled) {
                        if (frame >= mCurrentCanvas->getFrameOut().frame) { return; }
                    }
                    bool apply = frame == 0 ? true : (mCurrentCanvas->getFrameIn().frame != frame);
                    mCurrentCanvas->setFrameIn(apply, frame);
                }
            } else if (selectedAction == setFrameOutAct) {
                if (mCurrentCanvas) {
                    const auto frame = mCurrentCanvas->getCurrentFrame();
                    if (mCurrentCanvas->getFrameIn().enabled) {
                        if (frame <= mCurrentCanvas->getFrameIn().frame) { return; }
                    }
                    bool apply = (mCurrentCanvas->getFrameOut().frame != frame);
                    mCurrentCanvas->setFrameOut(apply, frame);
                }
            } else if (selectedAction == setMarkerAct) {
                if (mCurrentCanvas) {
                    mCurrentCanvas->setMarker(mCurrentCanvas->getCurrentFrame());
                }
            } else if (selectedAction == clearMarkersAct) {
                if (mCurrentCanvas) {
                    mCurrentCanvas->clearMarkers();
                }
            } else if (selectedAction == splitDurationAct) {
                if (mCurrentCanvas) {
                    mCurrentCanvas->splitAction();
                }
            } else if (selectedAction == openMarkerEditorAct) {
                if (mCurrentCanvas) {
                    mCurrentCanvas->openMarkerEditor();
                }
            }
        }
        return;
    }
    mPressed = true;
    mLastMousePressFrame = posToFrame(event->x() );
    bool hasMarker = mCurrentCanvas ? mCurrentCanvas->hasMarker(mLastMousePressFrame) : false;
    bool hasMarkerIn = mCurrentCanvas ? mCurrentCanvas->hasMarkerIn(mLastMousePressFrame) : false;
    bool hasMarkerOut = mCurrentCanvas ? mCurrentCanvas->hasMarkerOut(mLastMousePressFrame) : false;
    if (event->button() == Qt::LeftButton &&
        (hasMarker || hasMarkerIn || hasMarkerOut)) { // grab current marker
        mGrabbedMarker.enabled = true;
        mGrabbedMarker.in = hasMarkerIn;
        mGrabbedMarker.out = hasMarkerOut;
        mGrabbedMarker.frame = mLastMousePressFrame;
        return;
    }
    if (mLastMousePressFrame < mFirstViewedFrame ||
        mLastMousePressFrame > mFirstViewedFrame + mViewedFramesSpan) {
        setFirstViewedFrame(qRound(mLastMousePressFrame - mViewedFramesSpan/2.));
        emitTriggeredChange();
    }
    mSavedFirstFrame = mFirstViewedFrame;
    update();
}

void FrameScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    qreal newFrame = posToFrame(event->x() );
    if (mGrabbedMarker.enabled && mCurrentCanvas) { // move grabbed marker
        if (mGrabbedMarker.in) {
            if (mCurrentCanvas->hasMarkerIn(newFrame) ||
                mCurrentCanvas->hasMarkerOut(newFrame)) { return; }
            mCurrentCanvas->setFrameIn(true, newFrame);
        } else if (mGrabbedMarker.out) {
            if (mCurrentCanvas->hasMarkerOut(newFrame) ||
                mCurrentCanvas->hasMarkerIn(newFrame)) { return; }
            mCurrentCanvas->setFrameOut(true, newFrame);
        } else {
            if (mCurrentCanvas->hasMarker(newFrame)) { return; }
            mCurrentCanvas->moveMarkerFrame(mGrabbedMarker.frame, newFrame);
        }
        mGrabbedMarker.frame = newFrame;
        return;
    }
    int moveFrame = qRound(newFrame - mLastMousePressFrame);
    if (setFirstViewedFrame(mSavedFirstFrame + moveFrame)) {
        emitTriggeredChange();
        update();
    }
}

void FrameScrollBar::mouseReleaseEvent(QMouseEvent *)
{
    mPressed = false;
    if (mGrabbedMarker.enabled) { // release grabbed marker
        mGrabbedMarker.enabled = false;
        mGrabbedMarker.in = false;
        mGrabbedMarker.out = false;
        mGrabbedMarker.frame = 0;
    }
    update();
}

void FrameScrollBar::setDisplayedFrameRange(const FrameRange& range) {
    mFrameRange = range;

    const int dFrame = mFrameRange.fMax - mFrameRange.fMin + (mRange ? 0 : 1);
    int divInc = 3;
    mDrawFrameInc = 5000;
    while(mDrawFrameInc && dFrame/mDrawFrameInc < 3) {
        if(divInc == 3) {
            divInc = 0;
            mDrawFrameInc *= 4;
            mDrawFrameInc /= 10;
        } else {
            mDrawFrameInc /= 2;
        }
        divInc++;
    }
    mDrawFrameInc = qMax(1, mDrawFrameInc);
    mMaxSpan = range.span() - 1;
    setViewedFrameRange({mFirstViewedFrame, mFirstViewedFrame + mViewedFramesSpan});
}

void FrameScrollBar::setViewedFrameRange(const FrameRange& range) {
    setFirstViewedFrame(range.fMin);
    setFramesSpan(range.span() - 1);
    update();
    emitChange();
}

void FrameScrollBar::setCanvasFrameRange(const FrameRange &range) {
    mCanvasRange = range;
    update();
}

void FrameScrollBar::callWheelEvent(QWheelEvent *event)
{
    if (!mRange) { return; }
    bool triggered = false;
    if (event->modifiers() & Qt::CTRL) {
        int newFramesSpan = mViewedFramesSpan;
        if (event->angleDelta().y() > 0) { newFramesSpan *= 0.85; }
        else { newFramesSpan *= 1.15; }
        setFramesSpan(newFramesSpan);
        triggered = true;
   } else if (event->modifiers() & Qt::SHIFT) {
        if (event->angleDelta().y() > 0) {
            setFirstViewedFrame(mFirstViewedFrame - mViewedFramesSpan/20);
            triggered = true;
        } else {
            setFirstViewedFrame(mFirstViewedFrame + mViewedFramesSpan/20);
            triggered = true;
        }
    }
    if (!triggered) { return; }

    emitTriggeredChange();
    update();
}

#ifdef Q_OS_MAC
void FrameScrollBar::callNativeGestures(QNativeGestureEvent *e)
{
    if (!e || !mRange) { return; }
    bool triggered = false;

    if (e->gestureType() == Qt::ZoomNativeGesture) {
        int newFramesSpan = mViewedFramesSpan;
        if (e->value() < 0) { newFramesSpan *= 0.85; }
        else { newFramesSpan *= 1.15; }
        setFramesSpan(newFramesSpan);
        triggered = true;
    } else if (e->gestureType() == Qt::SmartZoomNativeGesture) {
        setFramesSpan(mFrameRange.EMAX);
        triggered = true;
    }

    if (!triggered) { return; }
    emitTriggeredChange();
    update();
}

void FrameScrollBar::callPanEvent(QWheelEvent *e)
{
    if (!e || !mRange) { return; }
    bool triggered = false;

    if (e->angleDelta().x() == 0) { return; }
    if (e->angleDelta().x() > 0) {
        setFirstViewedFrame(mFirstViewedFrame - mViewedFramesSpan / 20);
        triggered = true;
    } else {
        setFirstViewedFrame(mFirstViewedFrame + mViewedFramesSpan / 20);
        triggered = true;
    }

    if (!triggered) { return; }
    emitTriggeredChange();
    update();
}
#endif

void FrameScrollBar::emitChange() {
    emit frameRangeChange(getViewedRange());
}

void FrameScrollBar::emitTriggeredChange() {
    emit triggeredFrameRangeChange(getViewedRange());
}

FrameRange FrameScrollBar::getViewedRange() const {
    return {mFirstViewedFrame, getLastViewedFrame()};
}

int FrameScrollBar::getFirstViewedFrame() const {
    return mFirstViewedFrame;
}

int FrameScrollBar::getLastViewedFrame() const {
    return mFirstViewedFrame + mViewedFramesSpan;
}
