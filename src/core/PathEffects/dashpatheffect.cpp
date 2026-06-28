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

#include "dashpatheffect.h"
#include "Animators/qrealanimator.h"

DashPathEffect::DashPathEffect() :
    PathEffect("dash effect", PathEffectType::DASH) {
    mSize = enve::make_shared<QrealAnimator>("scale");
    mSize->setValueRange(0.1, 9999.999);
    mSize->setCurrentBaseValue(1);

    mDashLength = enve::make_shared<QrealAnimator>("length");
    mDashLength->setValueRange(0.1, 9999.999);
    mDashLength->setCurrentBaseValue(10);

    mSpaceLength = enve::make_shared<QrealAnimator>("spacing");
    mSpaceLength->setValueRange(0.1, 9999.999);
    mSpaceLength->setCurrentBaseValue(5);

    mOffset = enve::make_shared<QrealAnimator>("offset");
    mOffset->setValueRange(0, 9999.999);
    mOffset->setCurrentBaseValue(0);

    ca_addChild(mSize);
    ca_addChild(mDashLength);
    ca_addChild(mSpaceLength);
    ca_addChild(mOffset);

    ca_setGUIProperty(mDashLength.get());
}

class DashEffectCaller : public PathEffectCaller {
public:
    DashEffectCaller(const qreal dashLength, const qreal spaceLength,
                     const qreal offset, const qreal size) :
        mDashLength(toSkScalar(dashLength * size)),
        mSpaceLength(toSkScalar(spaceLength * size)),
        mOffset(toSkScalar(offset)) {}

    void apply(SkPath& path);
private:
    const float mDashLength;
    const float mSpaceLength;
    const float mOffset;
};

void DashEffectCaller::apply(SkPath &path) {
    SkPath src;
    path.swap(src);
    path.setFillType(src.getFillType());
    const float intervals[] = { mDashLength, mSpaceLength };
    SkStrokeRec rec(SkStrokeRec::kHairline_InitStyle);
    SkRect cullRec = src.getBounds();
    SkDashPathEffect::Make(intervals, 2, mOffset)->filterPath(&path, src, &rec, &cullRec);
}

stdsptr<PathEffectCaller> DashPathEffect::getEffectCaller(
        const qreal relFrame, const qreal influence) const {
    const qreal size = mSize->getEffectiveValue(relFrame);
    const qreal dashLength = mDashLength->getEffectiveValue(relFrame);
    const qreal spaceLength = mSpaceLength->getEffectiveValue(relFrame);
    const qreal offset = mOffset->getEffectiveValue(relFrame);
    return enve::make_shared<DashEffectCaller>(
        dashLength * influence, spaceLength * influence,
        offset * influence, size * influence * 5);
}
