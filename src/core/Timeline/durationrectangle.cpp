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

#include "durationrectangle.h"
#include "CacheHandlers/hddcachablecachehandler.h"
#include "GUI/global.h"
#include "Boxes/boundingbox.h"
#include "Private/esettings.h"
#include "appsupport.h"

TimelineMovable::TimelineMovable(const Type type, Property &parentProp) :
    mType(type), mParentProperty(parentProp) {}


void TimelineMovable::setValueUnClamped(const int value) {
    const int oldValue = mValue;
    mValue = value;
    emit valueChanged(oldValue, mValue);
}

void TimelineMovable::setValue(const int value) {
    setValueUnClamped(qMin(mClampMax, qMax(mClampMin, value)));
}

int TimelineMovable::getValue() const {
    return mValue;
}

TimelineMovable *TimelineMovable::getMovableAt(
        const int pressX, const qreal pixelsPerFrame,
        const int minViewedFrame) {
    const int pressedDFrame = qRound(minViewedFrame + pressX/pixelsPerFrame);
    const int thisDFrame = mValue - minViewedFrame;
    if(thisDFrame == pressedDFrame) return this;
    return nullptr;
}

void TimelineMovable::changeFramePosBy(const int change) {
    setValue(mValue + change);
}

void TimelineMovable::setHovered(const bool hovered) {
    mHovered = hovered;
}

bool TimelineMovable::isHovered() {
    return mHovered;
}

void TimelineMovable::startPosTransform() {
    if(mTransformed) return;
    mTransformed = true;
    mSavedValue = mValue;
}

void TimelineMovable::cancelPosTransform() {
    if(!mTransformed) return;
    mTransformed = false;
    setValue(mSavedValue);
}

void TimelineMovable::finishPosTransform() {
    if(!mTransformed) return;
    mTransformed = false;
    UndoRedo ur;
    const qptr<TimelineMovable> thisQPtr = this;
    const int oldValue = mSavedValue;
    const int newValue = mValue;
    ur.fUndo = [thisQPtr, oldValue]() {
        if(thisQPtr) thisQPtr->setValueUnClamped(oldValue);
    };
    ur.fRedo = [thisQPtr, newValue]() {
        if(thisQPtr) thisQPtr->setValueUnClamped(newValue);
    };
    mParentProperty.prp_addUndoRedo(ur);
}

void TimelineMovable::selectionChangeTriggered(const bool shiftPressed) {
    if(const auto cont = enve_cast<eBoxOrSound*>(&mParentProperty)) {
        cont->selectionChangeTriggered(shiftPressed);
    }
}

bool TimelineMovable::isSelected() {
    if(const auto cont = enve_cast<eBoxOrSound*>(&mParentProperty)) {
        return cont->isSelected();
    }
    return false;
}

void TimelineMovable::setClamp(const int min, const int max) {
    setClampMin(min);
    setClampMax(max);
}

void TimelineMovable::setClampMax(const int max) {
    mClampMax = max;
}

void TimelineMovable::setClampMin(const int min) {
    mClampMin = min;
}

DurationRectangle::DurationRectangle(Property &parentProp) :
    TimelineMovable(DURATION_RECT, parentProp),
    mMinFrame(MIN_FRAME, parentProp),
    mMaxFrame(MAX_FRAME, parentProp) {
    connect(this, &TimelineMovable::valueChanged,
            this, &DurationRectangle::shiftChanged);

    setClamp(-1000000, 1000000);
    mMinFrame.setClamp(-1000000, 1000000);
    mMaxFrame.setClamp(-1000000, 1000000);

    connect(&mMinFrame, &TimelineMovable::valueChanged,
            this, [this](const int, const int to) {
        mMaxFrame.setClampMin(to);
    });
    connect(&mMaxFrame, &TimelineMovable::valueChanged,
            this, [this](const int, const int to) {
        mMinFrame.setClampMax(to);
    });

    connect(&mMinFrame, &TimelineMovable::valueChanged,
            this, &DurationRectangle::minRelFrameChanged);
    connect(&mMaxFrame, &TimelineMovable::valueChanged,
            this, &DurationRectangle::maxRelFrameChanged);
}

void DurationRectangle::setFramesDuration(const int duration) {
    mMaxFrame.setRelFrame(getMinRelFrame() + duration - 1);
}

void DurationRectangle::setMinRelFrame(const int minFrame) {
    mMinFrame.setRelFrame(minFrame);
}

void DurationRectangle::setMaxRelFrame(const int maxFrame) {
    mMaxFrame.setRelFrame(maxFrame);
}

void DurationRectangle::setMinAbsFrame(const int minFrame) {
    mMinFrame.setAbsFrame(minFrame);
}

void DurationRectangle::setMaxAbsFrame(const int maxFrame) {
    mMaxFrame.setAbsFrame(maxFrame);
}

int DurationRectangle::getFrameDuration() const {
    return mMaxFrame.getRelFrame() - mMinFrame.getRelFrame() + 1;
}

int DurationRectangle::getMinRelFrame() const {
    return mMinFrame.getRelFrame();
}

int DurationRectangle::getMaxRelFrame() const {
    return mMaxFrame.getRelFrame();
}

int DurationRectangle::getMinAbsFrame() const {
    return mMinFrame.getAbsFrame();
}

int DurationRectangle::getMaxAbsFrame() const {
    return mMaxFrame.getAbsFrame();
}

void DurationRectangle::draw(QPainter * const p,
                             const QRect& drawRect,
                             const qreal fps,
                             const qreal pixelsPerFrame,
                             const FrameRange &absFrameRange)
{
    const int clampedMin = qMax(absFrameRange.fMin, getMinAbsFrame());
    const int firstRelDrawFrame = clampedMin - absFrameRange.fMin;
    const int clampedMax = qMin(absFrameRange.fMax, getMaxAbsFrame());
    const int lastRelDrawFrame = clampedMax - absFrameRange.fMin;
    const int drawFrameSpan = lastRelDrawFrame - firstRelDrawFrame + 1;

    if (drawFrameSpan < 1) { return; }

    const QRect durRect(qFloor((firstRelDrawFrame + 0.5) * pixelsPerFrame),
                        drawRect.y(),
                        qCeil((drawFrameSpan - 1.0) * pixelsPerFrame),
                        drawRect.height());

    // why are we drawing cache handlers here?
    Q_UNUSED(fps)
    /*const int rectStartFrame = absFrameRange.fMin - mValue;
    const int rectEndFrame = absFrameRange.fMax - mValue;
    if(mRasterCacheHandler && mSoundCacheHandler) {
        const int soundHeight = drawRect.height()/3;
        const int rasterHeight = drawRect.height() - soundHeight;
        const QRect rasterRect(drawRect.x(), drawRect.y(),
                               drawRect.width(), rasterHeight);
        mRasterCacheHandler->drawCacheOnTimeline(p, rasterRect,
                                                 rectStartFrame,
                                                 rectEndFrame,
                                                 1,
                                                 durRect.right());
        const QRect soundRect(drawRect.x(), drawRect.y() + rasterHeight,
                              drawRect.width(), soundHeight);
        mSoundCacheHandler->drawCacheOnTimeline(p, soundRect,
                                                rectStartFrame,
                                                rectEndFrame, fps,
                                                durRect.right());
    } else if(mRasterCacheHandler) {
        mRasterCacheHandler->drawCacheOnTimeline(p, drawRect,
                                                 rectStartFrame,
                                                 rectEndFrame,
                                                 1,
                                                 durRect.right());
    } else if(mSoundCacheHandler) {
        mSoundCacheHandler->drawCacheOnTimeline(p, drawRect,
                                                rectStartFrame,
                                                rectEndFrame, fps,
                                                durRect.right());
    }*/

    QColor fillColor;
    const auto& sett = eSettings::instance();
    if (isSelected()) { fillColor = sett.fSelectedVisibilityRangeColor; }
    else { fillColor = sett.fVisibilityRangeColor; }

    p->fillRect(durRect.adjusted(0, 1, 0, -1), fillColor);
    if (mHovered) {
        p->setPen(QPen(Qt::white, .5));
        p->drawRect(durRect);
    }

    if (mMinFrame.isHovered()) { p->setPen(QPen(Qt::white)); }
    else { p->setPen(QPen(Qt::black)); }
    p->drawLine(durRect.topLeft(), durRect.bottomLeft());

    if (mMaxFrame.isHovered()) { p->setPen(QPen(Qt::white)); }
    else { p->setPen(QPen(Qt::black)); }
    p->drawLine(durRect.topRight(), durRect.bottomRight());

//    p->setPen(Qt::black);
//    p->setBrush(Qt::NoBrush);
    //p->drawRect(drawRect);
}

TimelineMovable *DurationRectangle::getMovableAt(const int pressX,
                                                 const qreal pixelsPerFrame,
                                                 const int minViewedFrame)
{
    const qreal startX = ((getMinAbsFrame() - minViewedFrame) + 0.5) * pixelsPerFrame;
    const qreal endX = ((getMaxAbsFrame() - minViewedFrame + 1) - 0.5) * pixelsPerFrame;
    if (qAbs(pressX - startX) < 5) { return &mMinFrame; }
    else if (qAbs(pressX - endX) < 5) { return &mMaxFrame; }
    else if (pressX > startX && pressX < endX) { return this; }
    return nullptr;
}

void DurationRectangle::startMinFramePosTransform() {
    mMinFrame.startPosTransform();
}

void DurationRectangle::finishMinFramePosTransform() {
    mMinFrame.finishPosTransform();
}

void DurationRectangle::cancelMinFramePosTransform() {
    mMinFrame.cancelPosTransform();
}

void DurationRectangle::moveMinFrame(const int change) {
    mMinFrame.changeFramePosBy(change);
}

void DurationRectangle::startMaxFramePosTransform() {
    mMaxFrame.startPosTransform();
}

void DurationRectangle::cancelMaxFramePosTransform() {
    mMaxFrame.cancelPosTransform();
}

void DurationRectangle::setValues(const RangeRectValues& values) {
    if(getRelShift() == values.fShift &&
       getMinRelFrame() == values.fMin &&
       getMaxRelFrame() == values.fMax) {
        return;
    }

    const auto oldRelRange = getRelFrameRange();
    mMinFrame.setValueUnClamped(values.fMin);
    mMaxFrame.setValueUnClamped(values.fMax);
    setRelShift(values.fShift);
    const auto newRelRange = getRelFrameRange();

    mParentProperty.prp_afterChangedRelRange(oldRelRange + newRelRange);
}

RangeRectValues DurationRectangle::getValues() const {
    return {getRelShift(), getMinRelFrame(), getMaxRelFrame()};
}

void DurationRectangle::finishMaxFramePosTransform() {
    mMaxFrame.finishPosTransform();
}

void DurationRectangle::moveMaxFrame(const int change) {
    mMaxFrame.changeFramePosBy(change);
}

void DurationRectangle::writeDurationRectangle(eWriteStream &dst) {
    dst << getMinRelFrame();
    dst << getMaxRelFrame();
    dst << getRelShift();
}

void DurationRectangle::readDurationRectangle(eReadStream& src) {
    int minFrame;
    int maxFrame;
    int shift;
    src >> minFrame;
    src >> maxFrame;
    src >> shift;
    mMinFrame.setValueUnClamped(minFrame);
    mMaxFrame.setValueUnClamped(maxFrame);
    setRelShift(shift);
}

void DurationRectangle::writeDurationRectangleXML(QDomElement& ele) const {
    const auto visRange = QString("%1 %2").arg(getMinRelFrame()).
                                           arg(getMaxRelFrame());
    ele.setAttribute("visFrameRange", visRange);
    ele.setAttribute("frameShift", getRelShift());
}

void DurationRectangle::readDurationRectangleXML(const QDomElement& ele) {
    const auto visRangeStr = ele.attribute("visFrameRange");
    const auto visRangeStrs = visRangeStr.split(' ', Qt::SkipEmptyParts);
    if(visRangeStrs.count() != 2) RuntimeThrow("Invalid frame range " + visRangeStr);
    mMinFrame.setValueUnClamped(Friction::Core::XmlExportHelpers::stringToInt(visRangeStrs[0]));
    mMaxFrame.setValueUnClamped(Friction::Core::XmlExportHelpers::stringToInt(visRangeStrs[1]));
    const auto shiftStr = ele.attribute("frameShift");
    setRelShift(Friction::Core::XmlExportHelpers::stringToInt(shiftStr));
}
