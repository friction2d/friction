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

#ifndef DURATIONRECTANGLE_H
#define DURATIONRECTANGLE_H
#include <QtCore>
#include <QPainter>
#include <QObject>
#include "smartPointers/ememory.h"
#include "framerange.h"
#include "Properties/property.h"
class Property;
class HddCachableCacheHandler;

class CORE_EXPORT TimelineMovable : public SelfRef {
    Q_OBJECT
public:
    enum Type {
        MIN_FRAME,
        MAX_FRAME,
        DURATION_RECT
    };
    TimelineMovable(const Type type, Property& parentProp);

    virtual void pressed(const bool shiftPressed) {
        Q_UNUSED(shiftPressed)
    }

    virtual Qt::CursorShape getHoverCursorShape() {
        return Qt::ArrowCursor;
    }

    virtual TimelineMovable *getMovableAt(
            const int pressX, const qreal pixelsPerFrame,
            const int minViewedFrame);

    void setValueUnClamped(const int value);
    void setValue(const int value);
    int getValue() const;

    void changeFramePosBy(const int change);

    void setHovered(const bool hovered);
    bool isHovered();

    void startPosTransform();
    void cancelPosTransform();
    void finishPosTransform();

    bool isDurationRect() { return mType == DURATION_RECT; }
    bool isMinFrame() { return mType == MIN_FRAME; }
    bool isMaxFrame() { return mType == MAX_FRAME; }

    Property* getParentProperty() const { return &mParentProperty; }

    void selectionChangeTriggered(const bool shiftPressed);

    bool isSelected();

    void setClamp(const int min, const int max);

    void setClampMin(const int min);
    void setClampMax(const int max);
signals:
    void valueChanged(const int from, const int to);
    void finishedTransform();
protected:
    const Type mType;
    Property& mParentProperty;

    bool mHovered = false;
    int mClampMin = 0;
    int mClampMax = 0;
    int mValue = 0;
private:
    bool mTransformed = false;
    int mSavedValue = 0;
};

class CORE_EXPORT DurationMinMax : public TimelineMovable {
public:
    DurationMinMax(const Type type, Property& parentProp) :
        TimelineMovable(type, parentProp) {}

    Qt::CursorShape getHoverCursorShape() {
        return Qt::SplitHCursor;
    }

    void setRelFrame(const int relFrame) {
        setValue(relFrame);
    }

    int getRelFrame() const {
        return getValue();
    }

    void setAbsFrame(const int absFrame) {
        const int relFrame = mParentProperty.prp_absFrameToRelFrame(absFrame);
        setRelFrame(relFrame);
    }

    int getAbsFrame() const {
        const int absFrame = mParentProperty.prp_relFrameToAbsFrame(getRelFrame());
        return absFrame;
    }
private:
    using TimelineMovable::setValue;
    using TimelineMovable::getValue;
};

struct CORE_EXPORT RangeRectValues {
    int fShift;
    int fMin;
    int fMax;
};

class CORE_EXPORT DurationRectangle : public TimelineMovable {
    Q_OBJECT
public:
    DurationRectangle(Property &parentProp);

    virtual void draw(QPainter * const p,
                      const QRect &drawRect,
                      const qreal fps,
                      const qreal pixelsPerFrame,
                      const FrameRange &absFrameRange);

    virtual TimelineMovable *getMovableAt(
                      const int pressX,
                      const qreal pixelsPerFrame,
                      const int minViewedFrame);
    virtual bool hasAnimationFrameRange() { return false; }

    virtual void writeDurationRectangle(eWriteStream& dst);
    virtual void readDurationRectangle(eReadStream &src);

    virtual void writeDurationRectangleXML(QDomElement& ele) const;
    virtual void readDurationRectangleXML(const QDomElement& ele);

    Qt::CursorShape getHoverCursorShape() {
        return Qt::OpenHandCursor;
    }

    void setFramesDuration(const int duration);
    int getFrameDuration() const;

    void setMinRelFrame(const int minFrame);
    void setMaxRelFrame(const int maxFrame);

    void setMinAbsFrame(const int minFrame);
    void setMaxAbsFrame(const int maxFrame);

    int getMinRelFrame() const;
    int getMaxRelFrame() const;

    int getMinAbsFrame() const;
    int getMaxAbsFrame() const;

    FrameRange getRelFrameRange() const{
        return {getMinRelFrame(), getMaxRelFrame()};
    }

    FrameRange getRelFrameRangeToTheRight() {
        return {getMaxRelFrame() + 1, FrameRange::EMAX};
    }

    FrameRange getRelFrameRangeToTheLeft() {
        return {FrameRange::EMIN, getMinRelFrame() - 1};
    }

    FrameRange getAbsFrameRange() const{
        return {getMinAbsFrame(), getMaxAbsFrame()};
    }

    FrameRange getAbsFrameRangeToTheRight() {
        return {getMaxAbsFrame() + 1, FrameRange::EMAX};
    }

    FrameRange getAbsFrameRangeToTheLeft() {
        return {FrameRange::EMIN, getMinAbsFrame() - 1};
    }

    void moveMinFrame(const int change);
    void finishMinFramePosTransform();
    void cancelMinFramePosTransform();
    void startMinFramePosTransform();

    void moveMaxFrame(const int change);
    void finishMaxFramePosTransform();
    void cancelMaxFramePosTransform();
    void startMaxFramePosTransform();

    void setRasterCacheHandler(const HddCachableCacheHandler * const handler) {
        mRasterCacheHandler = handler;
    }

    void setSoundCacheHandler(const HddCachableCacheHandler * const handler) {
        mSoundCacheHandler = handler;
    }

    void setRelShift(const int shift) { setValue(shift); }

    int getRelShift() const { return getValue(); }
    void setValues(const RangeRectValues& values);
    RangeRectValues getValues() const;
private:
    using TimelineMovable::setValue;
    using TimelineMovable::getValue;
signals:
    void minRelFrameChanged(const int from, const int to);
    void maxRelFrameChanged(const int from, const int to);
    void shiftChanged(const int from, const int to);
protected:
    const HddCachableCacheHandler * mRasterCacheHandler = nullptr;
    const HddCachableCacheHandler * mSoundCacheHandler = nullptr;
    DurationMinMax mMinFrame;
    DurationMinMax mMaxFrame;
};

#endif // DURATIONRECTANGLE_H
