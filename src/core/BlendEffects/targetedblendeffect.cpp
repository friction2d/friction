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

#include "targetedblendeffect.h"

#include "Boxes/layerboxrenderdata.h"
#include "Boxes/boundingbox.h"

TargetedBlendEffect::TargetedBlendEffect() :
    BlendEffect("targeted", BlendEffectType::targeted) {
    const auto poses = QStringList() << "above" << "below";
    mAboveBelow = enve::make_shared<ComboBoxProperty>("position", poses);
    mTarget = enve::make_shared<BoxTargetProperty>("target");

    ca_addChild(mAboveBelow);
    ca_addChild(mTarget);
}

void TargetedBlendEffect::blendSetup(
        ChildRenderData &data,
        const int index,
        const qreal relFrame,
        QList<ChildRenderData> &delayed) const {
    Q_UNUSED(index);
    const auto target = this->target();
    if(!target) return;
    const bool isAbove = above();
    ChildRenderData iData(data.fData);
    auto& iClip = iData.fClip;
    iClip.fTargetBox = target;
    iClip.fAbove = isAbove;
    if(!isPathValid()) {
        data.fClip.fClipOps.append({SkPath(), SkClipOp::kIntersect, false});
    } else {
        const auto clipPath = this->clipPath(relFrame);
        data.fClip.fClipOps.append({clipPath, SkClipOp::kDifference, false});
        iClip.fClipOps.append({clipPath, SkClipOp::kIntersect, false});
    }

    delayed << iData;
}

void TargetedBlendEffect::detachedBlendUISetup(
        const qreal relFrame, const int drawId,
        QList<UIDelayed> &delayed) {
    Q_UNUSED(relFrame)
    Q_UNUSED(drawId)
    const auto target = this->target();
    if(!target) return;
    const bool isAbove = above();
    delayed << [this, target, isAbove]
               (int, BoundingBox* prev, BoundingBox* next) {
        const bool above = isAbove && prev == target;
        const bool below = !isAbove && next == target;
        if(!above && !below) return static_cast<BlendEffect*>(nullptr);
        return static_cast<BlendEffect*>(this);
    };
}

void TargetedBlendEffect::detachedBlendSetup(
        const BoundingBox* const boxToDraw,
        const qreal relFrame,
        SkCanvas * const canvas,
        const SkFilterQuality filter,
        const int drawId,
        QList<Delayed> &delayed) const {
    Q_UNUSED(drawId)
    const auto target = this->target();
    if(!target) return;
    const bool isAbove = above();
    if(isPathValid()) {
        const auto clipPath = this->clipPath(relFrame);
        delayed << [boxToDraw, target, isAbove, clipPath, canvas, filter]
                   (int, BoundingBox* prev, BoundingBox* next) {
            const bool above = isAbove && prev == target;
            const bool below = !isAbove && next == target;
            if(!above && !below) return false;
            canvas->save();
            canvas->clipPath(clipPath, SkClipOp::kIntersect, false);
            boxToDraw->drawPixmapSk(canvas, filter);
            canvas->restore();
            return true;
        };
    } else {
        delayed << [boxToDraw, target, isAbove, canvas, filter]
                   (int, BoundingBox* prev, BoundingBox* next) {
            const bool above = isAbove && prev == target;
            const bool below = !isAbove && next == target;
            if(!above && !below) return false;
            canvas->save();
            boxToDraw->drawPixmapSk(canvas, filter);
            canvas->restore();
            return true;
        };
    }
}

void TargetedBlendEffect::drawBlendSetup(
        const qreal relFrame,
        SkCanvas * const canvas) const {
    const auto target = this->target();
    if(!target) return;
    if(isPathValid()) {
        const auto clipPath = this->clipPath(relFrame);
        canvas->clipPath(clipPath, SkClipOp::kDifference, false);
    } else {
        canvas->clipPath(SkPath(), SkClipOp::kIntersect, false);
    }
}

void TargetedBlendEffect::saveSVG(SvgExporter &exp,
                                  QDomElement &ele,
                                  const FrameRange &visRange,
                                  DomEleTask *task,
                                  const BoundingBox *targetBox) const
{
    Q_UNUSED(exp)
    Q_UNUSED(ele)
    Q_UNUSED(visRange)
    Q_UNUSED(task)
    Q_UNUSED(targetBox)
}

BoundingBox *TargetedBlendEffect::target() const {
    return mTarget->getTarget();
}

bool TargetedBlendEffect::above() const {
    return mAboveBelow->getCurrentValue() == 0;
}
