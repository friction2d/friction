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

#include "boxpathpoint.h"
#include "pointhelpers.h"
#include "Animators/transformanimator.h"
#include "Animators/qpointfanimator.h"
#include "themesupport.h"

BoxPathPoint::BoxPathPoint(QPointFAnimator * const associatedAnimator,
                           BoxTransformAnimator * const boxTrans) :
    AnimatedPoint(associatedAnimator, TYPE_PIVOT_POINT) {
    setRadius(7);
    setTransform(boxTrans);
    setSelectionEnabled(false);
}

void BoxPathPoint::setRelativePos(const QPointF &relPos) {
    const auto bTrans = static_cast<BoxTransformAnimator*>(getTransform());
    bTrans->setPivotFixedTransform(relPos);
}

void BoxPathPoint::startTransform() {
    MovablePoint::startTransform();
    const auto bTrans = static_cast<BoxTransformAnimator*>(getTransform());
    bTrans->startPivotTransform();
}

void BoxPathPoint::finishTransform() {
    const auto bTrans = static_cast<BoxTransformAnimator*>(getTransform());
    bTrans->finishPivotTransform();
}

void BoxPathPoint::drawSk(SkCanvas * const canvas,
                          const CanvasMode mode,
                          const float invScale,
                          const bool keyOnCurrent,
                          const bool ctrlPressed)
{
    Q_UNUSED(mode)
    Q_UNUSED(keyOnCurrent)
    Q_UNUSED(ctrlPressed)
    const SkPoint absPos = toSkPoint(getAbsolutePos());
    drawOnAbsPosSk(canvas,
                   absPos,
                   invScale,
                   toSkColor(ThemeSupport::getThemeColorOrange(155)));
    canvas->save();
    canvas->translate(absPos.x(), absPos.y());
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(toSkColor(ThemeSupport::getThemeButtonBaseColor()));
    const float scaledHalfRadius = toSkScalar(getRadius()*0.5)*invScale;
    canvas->drawLine(-scaledHalfRadius, 0, scaledHalfRadius, 0, paint);
    canvas->drawLine(0, -scaledHalfRadius, 0, scaledHalfRadius, paint);
    canvas->restore();
}
