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

#include "gradientpoints.h"
#include "MovablePoints/gradientpoint.h"
#include "skia/skqtconversions.h"
#include "Boxes/pathbox.h"
#include "Animators/qvector3danimator.h"
#include "Animators/qpointfanimator.h"

GradientPoints::GradientPoints(BoundingBox * const parent) :
    StaticComplexAnimator("gradient points") {

    setPointsHandler(enve::make_shared<PointsHandler>());

    mStartAnimator = enve::make_shared<QPointFAnimator>("point1");
    ca_addChild(mStartAnimator);
    mStartPoint = enve::make_shared<GradientPoint>(mStartAnimator.get(), parent);
    getPointsHandler()->appendPt(mStartPoint);

    mEndAnimator = enve::make_shared<QPointFAnimator>("point2");
    ca_addChild(mEndAnimator);

    mEndPoint = enve::make_shared<GradientPoint>(mEndAnimator.get(), parent);
    getPointsHandler()->appendPt(mEndPoint);

    mEnabled = false;
}

void GradientPoints::enable() {
    mEnabled = true;
}

void GradientPoints::applyTransform(const QMatrix &transform) {
    mStartAnimator->applyTransform(transform);
    mEndAnimator->applyTransform(transform);
}

void GradientPoints::setPositions(const QPointF &startPos,
                                  const QPointF &endPos) {
    mStartPoint->setRelativePos(startPos);
    mEndPoint->setRelativePos(endPos);
}

void GradientPoints::disable() {
    mEnabled = false;
}

void GradientPoints::prp_drawCanvasControls(
        SkCanvas * const canvas, const CanvasMode mode,
        const float invScale, const bool ctrlPressed) {
    if(mode != CanvasMode::pointTransform) return;
    if(mEnabled) {
        const SkPoint startPos = toSkPoint(mStartPoint->getAbsolutePos());
        const SkPoint endPos = toSkPoint(mEndPoint->getAbsolutePos());
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);
        paint.setStrokeWidth(1.5f*invScale);
        paint.setStyle(SkPaint::kStroke_Style);

        canvas->drawLine(startPos, endPos, paint);
        paint.setColor(SK_ColorWHITE);
        paint.setStrokeWidth(0.75f*invScale);
        canvas->drawLine(startPos, endPos, paint);
        Property::prp_drawCanvasControls(canvas, mode, invScale, ctrlPressed);
    }
}

QPointF GradientPoints::getStartPointAtRelFrame(const int relFrame) {
    return mStartAnimator->getEffectiveValue(relFrame);
}

QPointF GradientPoints::getEndPointAtRelFrame(const int relFrame) {
    return mEndAnimator->getEffectiveValue(relFrame);
}

QPointF GradientPoints::getStartPoint(const qreal relFrame) {
    return mStartAnimator->getEffectiveValue(relFrame);
}

QPointF GradientPoints::getEndPoint(const qreal relFrame) {
    return mEndAnimator->getEffectiveValue(relFrame);
}

void GradientPoints::setColors(const QColor& startColor,
                               const QColor& endColor) {
    mStartPoint->setColor(startColor);
    mEndPoint->setColor(endColor);
}
