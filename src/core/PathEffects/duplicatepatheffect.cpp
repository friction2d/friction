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

#include "duplicatepatheffect.h"
#include "Animators/qpointfanimator.h"
#include "Animators/intanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/boundingbox.h"
#include "Properties/boolproperty.h"

DuplicatePathEffect::DuplicatePathEffect() :
    PathEffect("duplicate effect", PathEffectType::Duplicate) {
    mTranslation = enve::make_shared<QPointFAnimator>("translation");
    mTranslation->setBaseValue(QPointF(10, 10));
    ca_addChild(mTranslation);

    mRotation = enve::make_shared<QrealAnimator>(0, -360, 360, 1, "rotation");
    ca_addChild(mRotation);

    mCount = enve::make_shared<IntAnimator>(1, 0, 25, 1, "count");
    ca_addChild(mCount);

    mUseCustomPivot = enve::make_shared<BoolProperty>("use custom pivot");
    ca_addChild(mUseCustomPivot);

    mCustomPivot = enve::make_shared<QPointFAnimator>("custom pivot");
    mCustomPivot->setBaseValue(QPointF(0, 0));
    ca_addChild(mCustomPivot);
}

class DuplicateEffectCaller : public PathEffectCaller {
public:
    DuplicateEffectCaller(const int count,
                          const qreal dX,
                          const qreal dY,
                          const qreal rot,
                          const bool useCustomPivot,
                          const SkPoint customPivot,
                          const SkPoint fallbackPivot,
                          BoxTransformAnimator * const transform,
                          const qreal relFrame) :
        mCount(count),
        mDX(toSkScalar(dX)),
        mDY(toSkScalar(dY)),
        mRot(toSkScalar(rot)),
        mUseCustomPivot(useCustomPivot),
        mCustomPivot(customPivot),
        mFallbackPivot(fallbackPivot),
        mTransform(transform),
        mRelFrame(relFrame),
        mPivotAnimator(transform ? transform->getPivotAnimator() : nullptr) {}

    void apply(SkPath& path);
private:
    const int mCount;
    const float mDX;
    const float mDY;
    const float mRot;
    const bool mUseCustomPivot;
    const SkPoint mCustomPivot;
    const SkPoint mFallbackPivot;
    BoxTransformAnimator * const mTransform;
    const qreal mRelFrame;
    QPointFAnimator * const mPivotAnimator;
};

void DuplicateEffectCaller::apply(SkPath &path) {
    SkPoint pivot;
    if(mUseCustomPivot) {
        pivot = mCustomPivot;
    } else if(mTransform) {
        pivot = toSkPoint(mTransform->getPivot(mRelFrame));
    } else if(mPivotAnimator) {
        pivot = toSkPoint(mPivotAnimator->getEffectiveValue(mRelFrame));
    } else {
        pivot = mFallbackPivot;
    }

    const SkPath src = path;
    for(int i = 1; i <= mCount; i++) {
        SkMatrix m;
        m.setTranslate(i*mDX, i*mDY);
        m.preRotate(i*mRot, pivot.x(), pivot.y());
        path.addPath(src, m);
    }
}


stdsptr<PathEffectCaller> DuplicatePathEffect::getEffectCaller(
        const qreal relFrame, const qreal influence) const {
    const int count = mCount->getEffectiveIntValue(relFrame);
    const qreal dX = mTranslation->getEffectiveXValue(relFrame)*influence;
    const qreal dY = mTranslation->getEffectiveYValue(relFrame)*influence;
    const qreal rot = mRotation->getEffectiveValue(relFrame)*influence;
    const bool useCustomPivot = mUseCustomPivot->getValue();
    const SkPoint customPivot = toSkPoint(mCustomPivot->getEffectiveValue(relFrame));
    SkPoint fallbackPivot = SkPoint::Make(0.f, 0.f);
    BoxTransformAnimator *transform = nullptr;
    if(const auto owner = getFirstAncestor<BoundingBox>()) {
        fallbackPivot = toSkPoint(owner->getRelBoundingRect().center());
        transform = owner->getBoxTransformAnimator();
    }
    return enve::make_shared<DuplicateEffectCaller>(count,
                                                    dX,
                                                    dY,
                                                    rot,
                                                    useCustomPivot,
                                                    customPivot,
                                                    fallbackPivot,
                                                    transform,
                                                    relFrame);
}

bool DuplicatePathEffect::skipZeroInfluence(const qreal relFrame) const {
    const int count = mCount->getEffectiveIntValue(relFrame);
    return count > 0;
}
