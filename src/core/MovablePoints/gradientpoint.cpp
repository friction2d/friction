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

#include "gradientpoint.h"
#include "pointhelpers.h"
#include "Boxes/pathbox.h"
#include "Animators/qpointfanimator.h"
#include "Animators/transformanimator.h"

GradientPoint::GradientPoint(QPointFAnimator * const associatedAnimator,
                             BoundingBox* const parent) :
    AnimatedPoint(associatedAnimator, parent->getTransformAnimator(),
                  TYPE_GRADIENT_POINT) {}

void GradientPoint::setColor(const QColor &fillColor) {
    mFillColor = fillColor;
}

void GradientPoint::drawSk(SkCanvas * const canvas, const CanvasMode mode,
                           const float invScale, const bool keyOnCurrent,
                           const bool ctrlPressed) {
    Q_UNUSED(mode)
    Q_UNUSED(ctrlPressed)

    const SkPoint absPos = toSkPoint(getAbsolutePos());
    canvas->save();

    const float scaledRadius = toSkScalar(getRadius())*invScale;

    SkPaint paint;
    paint.setAntiAlias(true);
    const SkColor paintColor = toSkColor(mFillColor);
    paint.setColor(paintColor);

    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(absPos, scaledRadius, paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorBLACK);
    paint.setStrokeWidth(1.5f*invScale);
    canvas->drawCircle(absPos, scaledRadius, paint);
    paint.setColor(SK_ColorWHITE);
    paint.setStrokeWidth(0.75f*invScale);
    canvas->drawCircle(absPos, scaledRadius, paint);

    if(keyOnCurrent) {
        paint.setColor(SK_ColorRED);
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(absPos, scaledRadius*0.5f, paint);

        paint.setStyle(SkPaint::kStroke_Style);
        paint.setColor(SK_ColorBLACK);
        canvas->drawCircle(absPos, scaledRadius*0.5f, paint);
    }
    canvas->restore();
}
