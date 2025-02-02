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

#include "fixedlenanimationrect.h"
#include "Properties/property.h"
#include "appsupport.h"

int FixedLenAnimationRect::getMinAnimRelFrame() const {
    return mMinAnimationFrame;
}

int FixedLenAnimationRect::getMaxAnimRelFrame() const {
    return mMaxAnimationFrame;
}

void FixedLenAnimationRect::bindToAnimationFrameRange() {
    if(getMinRelFrame() < getMinAnimRelFrame() ||
       getMinRelFrame() >= getMaxAnimRelFrame()) {
        setMinRelFrame(getMinAnimRelFrame());
    }
    mMinFrame.setClampMin(getMinAnimRelFrame());
    if(getMaxRelFrame() > getMaxAnimRelFrame() ||
       getMaxRelFrame() <= getMinAnimRelFrame()) {
        setMaxRelFrame(getMaxAnimRelFrame());
    }
    mMaxFrame.setClampMax(getMaxAnimRelFrame());
}

void FixedLenAnimationRect::setMinAnimRelFrame(const int min) {
    mMinAnimationFrame = min;
    if(mBoundToRange) bindToAnimationFrameRange();
}

void FixedLenAnimationRect::setMaxAnimRelFrame(const int max) {
    if(mSetMaxFrameAtLeastOnce) {
        if(getMaxRelFrame() == mMaxAnimationFrame) {
            setMaxRelFrame(max);
        }
    } else {
        mSetMaxFrameAtLeastOnce = true;
        setMaxRelFrame(max);
        mMinFrame.setClampMax(max);
    }
    mMaxAnimationFrame = max;
    if(mBoundToRange) bindToAnimationFrameRange();
}

void FixedLenAnimationRect::writeDurationRectangle(eWriteStream &dst) {
    dst << mBoundToRange;
    dst << mSetMaxFrameAtLeastOnce;
    dst << mMinAnimationFrame;
    dst << mMaxAnimationFrame;
    DurationRectangle::writeDurationRectangle(dst);
}

void FixedLenAnimationRect::readDurationRectangle(eReadStream& src) {
    bool boundToRange; src >> boundToRange;
    src >> mSetMaxFrameAtLeastOnce;
    src >> mMinAnimationFrame;
    src >> mMaxAnimationFrame;
    DurationRectangle::readDurationRectangle(src);
}

void FixedLenAnimationRect::writeDurationRectangleXEV(QDomElement& ele) const {
    DurationRectangle::writeDurationRectangleXEV(ele);
    const auto visRange = QString("%1 %2").arg(mMinAnimationFrame).
                                           arg(mMaxAnimationFrame);
    ele.setAttribute("animFrameRange", visRange);

    ele.setAttribute("maxFrameSet", mSetMaxFrameAtLeastOnce ? "true" : "false");
}

void FixedLenAnimationRect::readDurationRectangleXEV(const QDomElement& ele) {
    DurationRectangle::readDurationRectangleXEV(ele);
    const auto animRangeStr = ele.attribute("animFrameRange");
    const auto animRangeStrs = animRangeStr.split(' ', Qt::SkipEmptyParts);
    if(animRangeStrs.count() != 2) RuntimeThrow("Invalid frame range " + animRangeStr);
    mMinAnimationFrame = Friction::Core::XmlExportHelpers::stringToInt(animRangeStrs[0]);
    mMaxAnimationFrame = Friction::Core::XmlExportHelpers::stringToInt(animRangeStrs[1]);

    mSetMaxFrameAtLeastOnce = ele.attribute("maxFrameSet") == "true";
}
