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

#include "layerboxrenderdata.h"
#include "skia/skqtconversions.h"

ContainerBoxRenderData::ContainerBoxRenderData(BoundingBox * const parentBox) :
    BoxRenderData(parentBox) {
    mDelayDataSet = true;
}

void ContainerBoxRenderData::transformRenderCanvas(SkCanvas &canvas) const {
    canvas.translate(toSkScalar(-fGlobalRect.x()),
                     toSkScalar(-fGlobalRect.y()));
}
#include "pointhelpers.h"
void ContainerBoxRenderData::updateRelBoundingRect() {
    fRelBoundingRect = QRectF();
    const auto invTrans = fTotalTransform.inverted();
    for(const auto &child : qAsConst(fChildrenRenderData)) {
        if(child->fRelBoundingRect.isEmpty()) continue;
        QPointF tl = child->fRelBoundingRect.topLeft();
        QPointF tr = child->fRelBoundingRect.topRight();
        QPointF br = child->fRelBoundingRect.bottomRight();
        QPointF bl = child->fRelBoundingRect.bottomLeft();

        const auto trans = child->fTotalTransform*invTrans;

        tl = trans.map(tl);
        tr = trans.map(tr);
        br = trans.map(br);
        bl = trans.map(bl);

        if(fRelBoundingRect.isNull()) {
            fRelBoundingRect.setTop(qMin4(tl.y(), tr.y(), br.y(), bl.y()));
            fRelBoundingRect.setLeft(qMin4(tl.x(), tr.x(), br.x(), bl.x()));
            fRelBoundingRect.setBottom(qMax4(tl.y(), tr.y(), br.y(), bl.y()));
            fRelBoundingRect.setRight(qMax4(tl.x(), tr.x(), br.x(), bl.x()));
        } else {
            fRelBoundingRect.setTop(qMin(fRelBoundingRect.top(),
                                         qMin4(tl.y(), tr.y(), br.y(), bl.y())));
            fRelBoundingRect.setLeft(qMin(fRelBoundingRect.left(),
                                          qMin4(tl.x(), tr.x(), br.x(), bl.x())));
            fRelBoundingRect.setBottom(qMax(fRelBoundingRect.bottom(),
                                            qMax4(tl.y(), tr.y(), br.y(), bl.y())));
            fRelBoundingRect.setRight(qMax(fRelBoundingRect.right(),
                                           qMax4(tl.x(), tr.x(), br.x(), bl.x())));
        }

        fOtherGlobalRects << child->fGlobalRect;
    }
}

void ContainerBoxRenderData::drawSk(SkCanvas * const canvas) {
    for(const auto &child : qAsConst(fChildrenRenderData)) {
        canvas->save();
        if(!child.fClip.fClipOps.isEmpty()) {
            const SkMatrix transform = canvas->getTotalMatrix();
            canvas->concat(toSkMatrix(fResolutionScale));
            child.fClip.clip(canvas);
            canvas->setMatrix(transform);
        }
        child->drawOnParentLayer(canvas);
        canvas->restore();
    }
}
