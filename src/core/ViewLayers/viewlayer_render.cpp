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


#include "viewlayer_render.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTransform>

#include "basecanvas.h"
#include "utils.h"
#include "efiltersettings.h"
#include "GUI/global.h"

ViewLayerRender::ViewLayerRender(Document &document, BaseCanvas *canvas)
    : ViewLayer("render_layer")
    , _baseCanvas(canvas)
    , _document(document) {
    Q_ASSERT(!sInstance);
    sInstance = this;
};

void ViewLayerRender::repaint(SkCanvas * const canvas) {
    auto scene = *document.fActiveScene;

    /* ======== Setup ======== */

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);

    const qreal zoom = _baseCanvas->zoom();
    const QTransform translation = _baseCanvas->translation();

    const qreal pixelRatio = QGuiApplication::primaryScreen()->devicePixelRatio();
    const qreal qInverseZoom = 1/zoom * pixelRatio;
    const float inverseZoom = toSkScalar(qInverseZoom);

    const SkRect canvasRect = SkRect::MakeWH(_baseCanvas->width(), _baseCanvas->height());
    const auto filter = eFilterSettings::sDisplay(zoom, _baseCanvas->resolution());
    const QColor qBackgroundColor = _baseCanvas->backgroundColor();
    const float intervals[2] = {eSizesUI::widget*0.25f*inverseZoom,
        eSizesUI::widget*0.25f*inverseZoom};

    // Move canvas to a certain position
    const SkMatrix skCanvasTranslation  = toSkMatrix(translation);
    canvas->concat(skCanvasTranslation);

    /* ======== Rendering ======== */

    auto sceneFrame = scene->getSceneFrame();

    if (sceneFrame) {
        canvas->clear(SK_ColorBLACK);
        canvas->save();

        if (qBackgroundColor.alpha() != 255)
            drawTransparencyMesh(canvas, canvasRect);

        const float reversedResolution = toSkScalar(1/sceneFrame.fResolution);
        canvas->scale(reversedResolution, reversedResolution);

        sceneFrame.drawImage(canvas, filter);

        canvas->restore();
    }
};

void ViewLayerRender::setSceneFrame(const int relFrame) {};

void ViewLayerRender::setSceneFrame(const stdsptr<SceneFrameContainer> &cont) {
    _sceneFrame = cont;
    emit requestUpdate();
};

void ViewLayerRender::setResolution(const qreal percent) {
    _baseCanvas->setResolution(percent);
};

qreal ViewLayerRender::getResolution() const {
    return _baseCanvas->resolution();
};
