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


#include "viewlayer_export.h"

#include <SkPaint>
#include <SkRect>

#include "../efiltersettings.h"


ViewLayerExport::ViewLayerExport(Document &document) : ViewLayer("export_layer", document) {

};

ViewLayerExport::repaint(SkCanvas *canvas) {

    /* ======== Setup ======== */

    SkPaint paint;
    paint.setStyle(skPaint::kFill_Style);    

    const qreal pixelRatio = qApp->devicePixelRatio();
    const qreal zoom = canvas.zoom();
    const qreal translation = canvas.translation();
    const qreal qInverseZoom = 1/zoom * pixelRatio;
    const float inverseZoom = toSkScalar(qInverseZoom);
    
    const SkRect canvasRect = SkRect::MakeWH(width, height);
    const auto filter = eFilterSettings::sDisplay(zoom, canvas.resolution);
    const SkMatrix skCanvasTranslation  = toSkMatrix(translation);
    const QColor backgroundColor = backgroundColor->getColor();
    const float intervals[2] = {eSizesUI::widget*0.25f*inverseZoom,
        eSizesUI::widget*0.25f*inverseZoom};

    // Move canvas to a certain position
    canvas->concat(translation);

    /* ======== Rendering ======== */
    
    // Render preview
    if (isPreviewingOrRendering()) {
        if (sceneFrame) {
            canvas->clear(SK_COlorBLACK);
            canvas->save();
            if (backgroundColor.alpha() != 255)
                drawTransparencyMesh(canvas, canvasRect);

            const float reversedResolution = toSkScalar(1/sceneFrame->fResolution);
            canvas->scale(reversedResolution, reversedResolution);

            sceneFrame->drawImage(canvas, filter);
            canvas->restore();
        }

        return;
    }

    // Else, render normal canvas mode

    canvas->save();

    if (clipToCanvasSize) {
        canvas->clear(SK_ColorBLACK);
        canvas->clipRect(canvasRect);
    } else {
        canvas->clear(ThemeSupport::getThemeBaseSkColor());

        paint.setColor(SK_ColorGRAY);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setPathEffect(dashPathEffect);
        
        auto currentBounds = toSkRect(bounds());
        canvas->drawRect(currentBounds, paint);
    }

    const bool drawCanvas = sceneFrame && sceneFrame->fBoxState == stateId;

    if (backgroundColor.alpha() != 255)
        drawTransparencyMesh(canvas, canvasRect);

    if (!clipToCanvasSize || !drawCanvas) {
        canvas->saveLayer(nullptr, nullptr);

        if (backgroundColor.alpha() == 255 &&
            skCanvasTranslation.mapRect(translation).contains(toSkRect(drawRect))) {
            canvas->clear(toSkColor(backgroundColor));
        } else {
            paint.setStyle(SkPaint::kFill_Style);
            paint.setColor(toSkColor(backgroundColor));

            canvas->drawRect(canvasRect, paint);
        }

        drawContained(canvas, filter);
        canvas->restore();
    } else if (drawCanvas) {
        canvas->save();
        const float reversedResolution = toSkScalar(1/sceneFrame->fResolution);
        canvas->scale(reversedResolution, reversedResolution);
        sceneFrame->drawImage(canvas, filter);
        canvas->restore();
    }

    canvas->restore();

    if (!enve_cast<Canvas*>(currentContainer)) {
        currentContainer->drawBoundingRect(canvas, inverseZoom);
    }

    /*
    if(CurrentMode == CanvasMode::boxTransform ||
       mCurrentMode == CanvasMode::pointTransform) {
        if(mTransMode == TransformMode::rotate ||
           mTransMode == TransformMode::scale) {
            mRotPivot->drawTransforming(canvas, mCurrentMode, invZoom,
                                        eSizesUI::widget*0.25f*invZoom);
        } else if(!mouseGrabbing || mRotPivot->isSelected()) {
            mRotPivot->drawSk(canvas, mCurrentMode, invZoom, false, false);
        }
    } else if(mCurrentMode == CanvasMode::drawPath) {
        const SkScalar nodeSize = 0.15f*eSizesUI::widget*invZoom;
        SkPaint paint;
        paint.setStyle(SkPaint::kFill_Style);
        paint.setAntiAlias(true);

        const auto& pts = mDrawPath.smoothPts();
        const auto drawColor = eSettings::instance().fLastUsedStrokeColor;
        paint.setARGB(255,
                      drawColor.red(),
                      drawColor.green(),
                      drawColor.blue());
        const SkScalar ptSize = 0.25*nodeSize;
        for(const auto& pt : pts) {
            canvas->drawCircle(pt.x(), pt.y(), ptSize, paint);
        }

        const bool drawFitted = mDocument.fDrawPathManual &&
                                mManualDrawPathState == ManualDrawPathState::drawn;
        if(drawFitted) {
            paint.setARGB(255, 255, 0, 0);
            const auto& highlightPts = mDrawPath.forceSplits();
            for(const int ptId : highlightPts) {
                const auto& pt = pts.at(ptId);
                canvas->drawCircle(pt.x(), pt.y(), nodeSize, paint);
            }
            const auto& fitted = mDrawPath.getFitted();
            paint.setARGB(255, 255, 0, 0);
            for(const auto& seg : fitted) {
                const auto path = seg.toSkPath();
                SkiaHelpers::drawOutlineOverlay(canvas, path, invZoom, SK_ColorWHITE);
                const auto& p0 = seg.p0();
                canvas->drawCircle(p0.x(), p0.y(), nodeSize, paint);
            }
            if(!mDrawPathTmp.isEmpty()) {
                SkiaHelpers::drawOutlineOverlay(canvas, mDrawPathTmp,
                                                invZoom, SK_ColorWHITE);
            }
        }

        paint.setARGB(255, 0, 75, 155);
        if(mHoveredPoint_d && mHoveredPoint_d->isSmartNodePoint()) {
            const QPointF pos = mHoveredPoint_d->getAbsolutePos();
            const qreal r = 0.5*qInvZoom*mHoveredPoint_d->getRadius();
            canvas->drawCircle(pos.x(), pos.y(), r, paint);
        }
        if(mDrawPathFirst) {
            const QPointF pos = mDrawPathFirst->getAbsolutePos();
            const qreal r = 0.5*qInvZoom*mDrawPathFirst->getRadius();
            canvas->drawCircle(pos.x(), pos.y(), r, paint);
        }
    }

        if(mHoveredPoint_d) {
            mHoveredPoint_d->drawHovered(canvas, invZoom);
        } else if(mHoveredNormalSegment.isValid()) {
            mHoveredNormalSegment.drawHoveredSk(canvas, invZoom);
        } else if(mHoveredBox) {
            if(!mCurrentNormalSegment.isValid()) {
                mHoveredBox->drawHoveredSk(canvas, invZoom);
            }
        }
    //}

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(invZoom);
    paint.setColor(SK_ColorGRAY);
    paint.setPathEffect(nullptr);
    canvas->drawRect(canvasRect, paint);

    canvas->resetMatrix();

    if(mTransMode != TransformMode::none || mValueInput.inputEnabled())
        mValueInput.draw(canvas, drawRect.height() - eSizesUI::widget);
    */
}