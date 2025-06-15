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


#include "viewlayer_preview.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTransform>

#include "basecanvas.h"
#include "utils.h"
#include "Drawables/rectangle_drawable.h"
#include "Boxes/containerbox.h"
#include "efiltersettings.h"
#include "GUI/global.h"


ViewLayerPreview::ViewLayerPreview(Document &document, BaseCanvas *canvas)
    : ViewLayer("preview_layer")
    , _baseCanvas(canvas)
    , _document(document) {
    Q_ASSERT(!sInstance);
    sInstance = this;
};

void ViewLayerPreview::repaint(SkCanvas * const canvas) {

    /* ======== Setup ======== */

    const qreal zoom = _baseCanvas->zoom();
    const QTransform translation = _baseCanvas->translation();

    const qreal pixelRatio = QGuiApplication::primaryScreen()->devicePixelRatio();
    const qreal qInverseZoom = 1/zoom * pixelRatio;
    const float inverseZoom = toSkScalar(qInverseZoom);

    const SkRect canvasRect = SkRect::MakeWH(_baseCanvas->width(), _baseCanvas->height());
    const auto filter = eFilterSettings::sDisplay(zoom, _baseCanvas->resolution());
    const QColor backgroundColor = _baseCanvas->backgroundColor();
    const float intervals[2] = {eSizesUI::widget*0.25f*inverseZoom,
        eSizesUI::widget*0.25f*inverseZoom};

    // Move canvas to a certain position
    const SkMatrix skCanvasTranslation  = toSkMatrix(translation);
    canvas->concat(skCanvasTranslation);

    /* ======== Rendering ======== */

    canvas->save();
/*
    if (clipToCanvasSize) {
        canvas->clear(SK_ColorBLACK);
        canvas->clipRect(canvasRect);
    } else {
        canvas->clear(ThemeSupport::getThemeBaseSkColor());

        //paint.setColor(SK_ColorGRAY);
        //paint.setStyle(SkPaint::kStroke_Style);
        //paint.setPathEffect(dashPathEffect);

        auto currentBounds = toSkRect(bounds());
        canvas->drawRect(currentBounds, //paint);
    }
*/

    if (backgroundColor.alpha() != 255) drawTransparencyMesh(canvas, canvasRect);

    canvas->saveLayer(nullptr, nullptr);

    if (backgroundColor.alpha() == 255 /*&&
        //skCanvasTranslation.mapRect(translation).contains(toSkRect(drawRect))*/) {
        canvas->clear(toSkColor(backgroundColor));
    } else {
        auto canvasRectangle = RectangleDrawable();

        canvasRectangle.setSize(canvasRect.width(), canvasRect.height());
        canvasRectangle.setFillColor(backgroundColor);

        canvasRectangle.drawToCanvas(canvas);
    }

    drawContainedBoxesToCanvas(canvas, filter);
    canvas->restore();
};

// I put an underscore in the name to avoid naming conflicts lol
void _handleDelayed(QList<BlendEffect::Delayed> &delayed,
                   const int drawId,
                   BoundingBox* const prevBox,
                   BoundingBox* const nextBox) {
    for(int i = 0; i < delayed.count(); i++) {
        const auto& del = delayed.at(i);
        if(del(drawId, prevBox, nextBox)) delayed.removeAt(i--);
    }
}

void ViewLayerPreview::containedDetachedBlendSetup(
        SkCanvas * const canvas,
        const SkFilterQuality filter, int& drawId,
        QList<BlendEffect::Delayed> &delayed) const {
    for(int i = _containedBoxesRef->count() - 1; i >= 0; i--) {
        const auto& box = _containedBoxesRef->at(i);
        if(box->isVisibleAndInVisibleDurationRect()) {
            if(box->isGroup()) {
                const auto cBox = static_cast<ContainerBox*>(box);
                cBox->containedDetachedBlendSetup(canvas, filter, drawId, delayed);
            } else {
                box->detachedBlendSetup(canvas, filter, drawId, delayed);
                drawId++;
            }
        }
    }
}

void ViewLayerPreview::drawContainedBoxesToCanvas(SkCanvas * const canvas,
                                     const SkFilterQuality filter) const {
    int drawId = 0;
    QList<BlendEffect::Delayed> delayed;
    containedDetachedBlendSetup(canvas, filter, drawId, delayed);
    drawId = 0;
    drawContainedBoxesToCanvas(canvas, filter, drawId, delayed);
    _handleDelayed(delayed, INT_MAX, nullptr, nullptr);
};

void ViewLayerPreview::drawContainedBoxesToCanvas(SkCanvas * const canvas,
                                     const SkFilterQuality filter, int& drawId,
                                     QList<BlendEffect::Delayed> &delayed) const {
    auto activeScene = _document.fActiveScene;
    auto containedBoxes = activeScene->getCurrentGroup();

    if(containedBoxes->isEmpty()) return;
    _handleDelayed(delayed, drawId, nullptr, containedBoxes->last());

    const auto count = containedBoxes->count() - 1;
    for(int i = count; i >= 0; i--) {
        const auto& box = containedBoxes->at(i);
        const auto& nextBox = i == 0 ? nullptr : containedBoxes->at(i - 1);
        if(box->isVisibleAndInVisibleDurationRect()) {
            box->drawPixmapSk(canvas, filter, drawId, delayed);
            if(!box->isGroup()) drawId++;
        }
        _handleDelayed(delayed, drawId, box, nextBox);
    }
}
