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
#include "Boxes/pathbox.h"
#include "svgexporter.h"

StencilBlendEffect::StencilBlendEffect()
    : BlendEffect("stencil mask", BlendEffectType::stencil)
{
    mInvert = enve::make_shared<BoolAnimator>("invert");
    mInvert->setCurrentBoolValue(false);
    ca_addChild(mInvert);
    ca_setGUIProperty(mInvert.get());

    connect(this, &eEffect::effectVisibilityChanged,
            this, [this](bool) {
        if (auto box = clipPathSource()) {
            box->planUpdate(UpdateReason::userChange);
        }
    });
}

void StencilBlendEffect::blendSetup(ChildRenderData &data,
                                    const int index,
                                    const qreal relFrame,
                                    QList<ChildRenderData> &delayed) const
{
    Q_UNUSED(index)
    Q_UNUSED(delayed)

    if (!isVisible() || !isPathValid()) { return; }

    const bool invert = mInvert->getEffectiveValue(relFrame);
    const auto maskPath = this->clipPath(relFrame);
    SkClipOp op = invert ? SkClipOp::kDifference : SkClipOp::kIntersect;

    if (!data.fClip.fClipOps.isEmpty()) {
        auto& lastClip = data.fClip.fClipOps.last();
        if (lastClip.fClipPathOp == op) {
            Op(lastClip.fClipPath,
               maskPath,
               kUnion_SkPathOp,
               &lastClip.fClipPath);
            return;
        }
    }

    data.fClip.fClipOps.append({maskPath, op, true});
}

void StencilBlendEffect::detachedBlendUISetup(const qreal relFrame,
                                              const int drawId,
                                              QList<UIDelayed> &delayed)
{
    Q_UNUSED(relFrame)

    delayed << [this, drawId](const int currentDrawId,
                              BoundingBox*,
                              BoundingBox*) {
        if (currentDrawId < drawId) { return static_cast<BlendEffect*>(nullptr); }
        return static_cast<BlendEffect*>(this);
    };
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

void StencilBlendEffect::saveSVG(SvgExporter& exp,
                                 QDomElement& ele,
                                 const FrameRange& visRange,
                                 DomEleTask* task,
                                 const BoundingBox* targetBox) const
{
    Q_UNUSED(task)
    Q_UNUSED(targetBox)

    auto maskBox = clipPathSource();
    if (!maskBox || !isVisible()) { return; }

    const qreal relFrame = prp_absFrameToRelFrameF(visRange.fMin);
    const bool invert = mInvert->getEffectiveValue(relFrame);

    if (ele.childNodes().isEmpty()) {
        if (invert) {
            // Silhouette (Invert)
            QDomElement bgRect = exp.createElement("rect");
            bgRect.setAttribute("x", "-500%");
            bgRect.setAttribute("y", "-500%");
            bgRect.setAttribute("width", "1000%");
            bgRect.setAttribute("height", "1000%");
            bgRect.setAttribute("fill", "#ffffff");
            ele.appendChild(bgRect);
        }
    }

    auto groupTask = enve::make_shared<DomEleTask>(exp, visRange);
    exp.addNextTask(groupTask);

    QDomElement& stableGroup = groupTask->initialize("g");

    stableGroup.setAttribute("color",
                             invert ? "#000000" /* Silhouette */ : "#ffffff" /* Stencil */);

    ele.appendChild(stableGroup);

    maskBox->saveSVGWithTransform(exp,
                                  stableGroup,
                                  visRange,
                                  "FORCE_EXPORT");

    groupTask->queTask();
}
