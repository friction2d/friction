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

#include "internallinkcanvas.h"
#include "linkcanvasrenderdata.h"
#include "Animators/transformanimator.h"
#include "canvas.h"
#include "svgexporter.h"

InternalLinkCanvas::InternalLinkCanvas(ContainerBox * const linkTarget,
                                       const bool innerLink) :
    InternalLinkGroupBox(linkTarget, innerLink) {
    mType = eBoxType::internalLinkCanvas;
    mFrameRemapping->disableAction();
    ca_prependChild(mTransformAnimator.data(), mClipToCanvas);
    ca_prependChild(mTransformAnimator.data(), mFrameRemapping);
}

void InternalLinkCanvas::enableFrameRemappingAction() {
    const auto finalTarget = static_cast<Canvas*>(getFinalTarget());
    const int minFrame = finalTarget->getMinFrame();
    const int maxFrame = finalTarget->getMaxFrame();
    mFrameRemapping->enableAction(minFrame, maxFrame, minFrame);
}

void InternalLinkCanvas::disableFrameRemappingAction() {
    mFrameRemapping->disableAction();
}

void InternalLinkCanvas::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    const PropertyMenu::CheckSelectedOp<InternalLinkCanvas> remapOp =
    [](InternalLinkCanvas* const box, const bool checked) {
        if(checked) box->enableFrameRemappingAction();
        else box->disableFrameRemappingAction();
    };
    menu->addCheckableAction("Frame Remapping",
                             mFrameRemapping->enabled(),
                             remapOp);

    menu->addSeparator();

    InternalLinkGroupBox::prp_setupTreeViewMenu(menu);
}

void InternalLinkCanvas::setupRenderData(const qreal relFrame,
                                         const QMatrix& parentM,
                                         BoxRenderData * const data,
                                         Canvas* const scene) {
    {
        BoundingBox::setupRenderData(relFrame, parentM, data, scene);
        const qreal remapped = mFrameRemapping->frame(relFrame);
        const auto thisM = getTotalTransformAtFrame(relFrame);
        processChildrenData(remapped, thisM, data, scene);
    }

    ContainerBox* finalTarget = getFinalTarget();
    auto canvasData = static_cast<LinkCanvasRenderData*>(data);
    const auto canvasTarget = static_cast<Canvas*>(finalTarget);
    canvasData->fBgColor = toSkColor(canvasTarget->getBgColorAnimator()->
            getColor(relFrame));
    //qreal res = mParentScene->getResolution();
    canvasData->fCanvasHeight = canvasTarget->getCanvasHeight();//*res;
    canvasData->fCanvasWidth = canvasTarget->getCanvasWidth();//*res;
    if(getParentGroup()->isLink()) {
        const auto ilc = static_cast<InternalLinkCanvas*>(getLinkTarget());
        canvasData->fClipToCanvas = ilc->clipToCanvas();
    } else {
        canvasData->fClipToCanvas = mClipToCanvas->getValue();
    }
}

void InternalLinkCanvas::saveSVG(SvgExporter& exp, DomEleTask* const task) const {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return;
    if(!mFrameRemapping->enabled()) {
        linkTarget->saveSVG(exp, task);
        return;
    }

    SvgExporter::FrameMapping mapping;
    const QPointer<const QrealFrameRemapping> remap = mFrameRemapping.get();
    mapping.active = true;
    mapping.discrete = true;
    mapping.mapper = [remap](const qreal frame) -> qreal {
        if(!remap) return frame;
        return remap->frame(frame);
    };

    const SvgExporter::FrameMappingScope scope(exp, mapping);
    linkTarget->saveSVG(exp, task);
}

bool InternalLinkCanvas::clipToCanvas() {
    return mClipToCanvas->getValue();
}

qsptr<BoundingBox> InternalLinkCanvas::createLink(const bool inner) {
    auto linkBox = enve::make_shared<InternalLinkCanvas>(this, inner);
    copyTransformationTo(linkBox.get());
    sWriteReadMember(this, linkBox.get(), &InternalLinkCanvas::mClipToCanvas);
    sWriteReadMember(this, linkBox.get(), &InternalLinkCanvas::mFrameRemapping);
    return std::move(linkBox);
}

stdsptr<BoxRenderData> InternalLinkCanvas::createRenderData() {
    return enve::make_shared<LinkCanvasRenderData>(this);
}

bool InternalLinkCanvas::relPointInsidePath(const QPointF &relPos) const {
    if(mClipToCanvas->getValue()) return getRelBoundingRect().contains(relPos);
    return InternalLinkGroupBox::relPointInsidePath(relPos);
}

void InternalLinkCanvas::anim_setAbsFrame(const int frame) {
    InternalLinkGroupBox::anim_setAbsFrame(frame);
    const auto canvasTarget = static_cast<Canvas*>(getFinalTarget());
    if(!canvasTarget) return;
    canvasTarget->anim_setAbsFrame(anim_getCurrentRelFrame());
}
