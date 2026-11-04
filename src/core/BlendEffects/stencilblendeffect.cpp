/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "stencilblendeffect.h"

#include "Boxes/layerboxrenderdata.h"
#include "Boxes/boundingbox.h"

StencilBlendEffect::StencilBlendEffect()
    : BlendEffect("stencil mask", BlendEffectType::stencil)
{
    mInvert = enve::make_shared<BoolAnimator>("invert");
    mInvert->setCurrentBoolValue(false);
    ca_addChild(mInvert);
    ca_setGUIProperty(mInvert.get());
}

void StencilBlendEffect::blendSetup(ChildRenderData &data,
                                    const int index,
                                    const qreal relFrame,
                                    QList<ChildRenderData> &delayed) const
{
    Q_UNUSED(index)
    Q_UNUSED(delayed)

    if (!isPathValid()) { return; }

    const bool invert = mInvert->getEffectiveValue(relFrame);
    const auto maskPath = this->clipPath(relFrame);

    SkClipOp op = invert ? SkClipOp::kDifference : SkClipOp::kIntersect;

    data.fClip.fClipOps.append({maskPath, op, true});
}

void StencilBlendEffect::detachedBlendUISetup(const qreal relFrame,
                                              const int drawId,
                                              QList<UIDelayed> &delayed)
{
    Q_UNUSED(relFrame)
    Q_UNUSED(drawId)
    Q_UNUSED(delayed)
    /*delayed << [this, drawId](const int currentDrawId,
                                BoundingBox*,
                                BoundingBox*) {
        if (currentDrawId < drawId) { return static_cast<BlendEffect*>(nullptr); }
        return static_cast<BlendEffect*>(this);
    };*/
}

void StencilBlendEffect::detachedBlendSetup(const BoundingBox* const boxToDraw,
                                            const qreal relFrame,
                                            SkCanvas * const canvas,
                                            const SkFilterQuality filter,
                                            const int drawId,
                                            QList<Delayed> &delayed) const
{

    if (!isPathValid()) { return; }

    const bool invert = mInvert->getEffectiveValue(relFrame);
    const auto maskPath = this->clipPath(relFrame);

    delayed << [boxToDraw, drawId, maskPath, invert, canvas, filter]
        (const int currentDrawId, BoundingBox*, BoundingBox*) {
            if (currentDrawId < drawId) { return false; }

            SkClipOp op = invert ? SkClipOp::kDifference : SkClipOp::kIntersect;
            canvas->save();
            canvas->clipPath(maskPath, op, true);
            boxToDraw->drawPixmapSk(canvas, filter);
            canvas->restore();

            return true;
        };
}

void StencilBlendEffect::drawBlendSetup(const qreal relFrame,
                                        SkCanvas * const canvas) const
{
    if (!isPathValid()) { return; }

    const bool invert = mInvert->getEffectiveValue(relFrame);
    const auto maskPath = this->clipPath(relFrame);
    SkClipOp op = invert ? SkClipOp::kDifference : SkClipOp::kIntersect;

    canvas->clipPath(maskPath, op, true);
}
