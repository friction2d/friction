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

#include "camerabox.h"
#include "canvas.h"
#include "skia/skiahelpers.h"
#include "skia/skqtconversions.h"
#include "Animators/transformanimator.h"

CameraBox::CameraBox() : BoundingBox("Camera", eBoxType::camera) {
    connect(this, &BoundingBox::prp_sceneChanged,
            this, [this](Canvas* const oldS, Canvas* const newS) {
        if (oldS) oldS->removeCameraBox(this);
        if (newS) newS->addCameraBox(this);
    });
}

bool CameraBox::relPointInsidePath(const QPointF &relPos) const {
    const auto canvas = getParentScene();
    if (!canvas) return false;
    const auto size = canvas->getCanvasSize();
    return QRectF(0, 0, size.width(), size.height()).contains(relPos);
}

void CameraBox::queTasks() {
    const auto canvas = getParentScene();
    if (canvas) {
        const auto size = canvas->getCanvasSize();
        setRelBoundingRect(QRectF(0, 0, size.width(), size.height()));
    }
    BoundingBox::queTasks();
}

QRectF CameraBox::getWorldBoundsAtFrame(const qreal relFrame) const {
    const auto canvas = getParentScene();
    if (!canvas) return {};
    const auto size = canvas->getCanvasSize();
    const QRectF localRect(0, 0, size.width(), size.height());
    return getRelativeTransformAtFrame(relFrame).mapRect(localRect);
}

void CameraBox::drawCameraBox(SkCanvas* const canvas, const float invScale) {
    if (!isVisible()) return;
    const auto parentScene = getParentScene();
    if (!parentScene) return;
    const auto size = parentScene->getCanvasSize();
    const QRectF localRect(0, 0, size.width(), size.height());
    SkPath rectPath;
    rectPath.addRect(toSkRect(localRect));
    SkiaHelpers::drawOutlineOverlay(canvas, rectPath, invScale,
                                    toSkMatrix(getTotalTransform()),
                                    true, 20.f, SK_ColorWHITE);
}
