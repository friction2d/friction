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

#include "pathpivot.h"
#include "canvas.h"
#include "pointhelpers.h"
#include "Animators/transformanimator.h"
#include "themesupport.h"

PathPivot::PathPivot(const Canvas * const parent) :
    NonAnimatedMovablePoint(parent->getTransformAnimator(),
                            TYPE_PIVOT_POINT),
    mCanvas(parent) {
    setRadius(7);
}

void PathPivot::drawSk(SkCanvas * const canvas,
                       const CanvasMode mode,
                       const float invScale,
                       const bool keyOnCurrent,
                       const bool ctrlPressed)
{
    Q_UNUSED(keyOnCurrent)
    Q_UNUSED(ctrlPressed)
    if (!isVisible(mode)) { return; }
    const SkPoint absPos = toSkPoint(getAbsolutePos());
    const auto colors = eSettings::instance().fColors;
    drawOnAbsPosSk(canvas,
                   absPos,
                   invScale,
                   toSkColor(Friction::Core::Theme::getThemeColorGreen(155))); // TODO
    canvas->save();
    canvas->translate(absPos.x(), absPos.y());
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(toSkColor(colors.baseButton));
    const float scaledHalfRadius = toSkScalar(getRadius()*0.5)*invScale;
    canvas->drawLine(-scaledHalfRadius, 0, scaledHalfRadius, 0, paint);
    canvas->drawLine(0, -scaledHalfRadius, 0, scaledHalfRadius, paint);
    canvas->restore();
}

void PathPivot::drawTransforming(SkCanvas * const canvas,
                                 const CanvasMode mode,
                                 const float invScale,
                                 const float interval)
{
    drawSk(canvas, mode, invScale, false, false);

    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorYELLOW);
    paint.setAntiAlias(true);
    canvas->drawLine(toSkPoint(getAbsolutePos()),
                     toSkPoint(mMousePos), paint);

    SkPaint paintDashed;
    paintDashed.setStyle(SkPaint::kStroke_Style);
    paintDashed.setColor(SK_ColorRED);
    const float intervals[2] = {interval, interval};
    paintDashed.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    paintDashed.setAntiAlias(true);
    canvas->drawLine(toSkPoint(getAbsolutePos()),
                     toSkPoint(mMousePos), paintDashed);
    paintDashed.setPathEffect(nullptr);

}

bool PathPivot::isVisible(const CanvasMode mode) const {
    if(mCanvas->getPivotLocal()) return false;
    if(mode == CanvasMode::pointTransform) return !mCanvas->isPointSelectionEmpty();
    else if(mode == CanvasMode::boxTransform) return !mCanvas->isBoxSelectionEmpty();
    return false;
}
