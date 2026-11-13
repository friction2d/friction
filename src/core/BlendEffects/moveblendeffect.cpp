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

#include "moveblendeffect.h"

#include "Boxes/layerboxrenderdata.h"
#include "Boxes/boundingbox.h"

MoveBlendEffect::MoveBlendEffect() :
    BlendEffect("move", BlendEffectType::move) {
    mZIndex = enve::make_shared<IntAnimator>("z-index");
    mZIndex->setPrefferedValueStep(0.2);
    ca_addChild(mZIndex);
    ca_setGUIProperty(mZIndex.get());
}

int MoveBlendEffect::calcDIndex(const qreal relFrame) const {
    const int dIndex = zIndex(relFrame);
    return dIndex + (dIndex > 0);
}

void MoveBlendEffect::blendSetup(
        ChildRenderData &data,
        const int index,
        const qreal relFrame,
        QList<ChildRenderData> &delayed) const {
    const int dIndex = calcDIndex(relFrame);
    if(dIndex == 0) return;
    const int zIndex = index + dIndex;
    ChildRenderData iData(data.fData);
    auto& iClip = iData.fClip;
    iClip.fTargetIndex = zIndex;
    if(!isPathValid()) {
        data.fClip.fClipOps.append({SkPath(), SkClipOp::kIntersect, false});
    } else {
        const auto clipPath = this->clipPath(relFrame);
        data.fClip.fClipOps.append({clipPath, SkClipOp::kDifference, false});
        iClip.fClipOps.append({clipPath, SkClipOp::kIntersect, false});
    }

    delayed << iData;
}

void MoveBlendEffect::detachedBlendUISetup(
        const qreal relFrame, const int drawId,
        QList<UIDelayed> &delayed) {
    const int dIndex = calcDIndex(relFrame);
    if(dIndex == 0) return;
    const int zIndex = drawId + dIndex;
    delayed << [this, zIndex](const int drawId,
                              BoundingBox*, BoundingBox*) {
        if(drawId < zIndex) return static_cast<BlendEffect*>(nullptr);
        return static_cast<BlendEffect*>(this);
    };
}

void MoveBlendEffect::detachedBlendSetup(const BoundingBox* const boxToDraw,
                                         const qreal relFrame,
                                         SkCanvas * const canvas,
                                         const SkFilterQuality filter,
                                         const int drawId,
                                         QList<Delayed> &delayed) const {
    const int dIndex = calcDIndex(relFrame);
    if(dIndex == 0) return;
    const int zIndex = drawId + dIndex;
    if(isPathValid()) {
        const auto clipPath = this->clipPath(relFrame);
        delayed << [boxToDraw, zIndex, clipPath, canvas, filter]
                   (const int drawId, BoundingBox*, BoundingBox*) {
            if(drawId < zIndex) return false;
            canvas->save();
            canvas->clipPath(clipPath, SkClipOp::kIntersect, false);
            boxToDraw->drawPixmapSk(canvas, filter);
            canvas->restore();
            return true;
        };
    } else {
        delayed << [boxToDraw, zIndex, canvas, filter]
                   (const int drawId, BoundingBox*, BoundingBox*) {
            if(drawId < zIndex) return false;
            canvas->save();
            boxToDraw->drawPixmapSk(canvas, filter);
            canvas->restore();
            return true;
        };
    }
}

void MoveBlendEffect::drawBlendSetup(const qreal relFrame,
                                     SkCanvas * const canvas) const {
    const int dIndex = calcDIndex(relFrame);
    if(dIndex == 0) return;
    if(isPathValid()) {
        const auto clipPath = this->clipPath(relFrame);
        canvas->clipPath(clipPath, SkClipOp::kDifference, false);
    } else {
        canvas->clipPath(SkPath(), SkClipOp::kIntersect, false);
    }
}

void MoveBlendEffect::saveSVG(SvgExporter &exp,
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

int MoveBlendEffect::zIndex(const qreal relFrame) const {
    return mZIndex->getEffectiveIntValue(relFrame);
}
