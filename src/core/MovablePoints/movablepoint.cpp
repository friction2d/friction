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

#include "movablepoint.h"
#include "skia/skqtconversions.h"
#include "pointhelpers.h"
#include "Animators/transformanimator.h"
#include "themesupport.h"
#include "Private/esettings.h"
#include "Private/document.h"

MovablePoint::MovablePoint(const MovablePointType type) : mType(type) {}

MovablePoint::MovablePoint(BasicTransformAnimator * const trans,
                           const MovablePointType type) :
    MovablePoint(type) {
    setTransform(trans);
}

void MovablePoint::startTransform() {
    mSavedRelPos = getRelativePos();
}

const QPointF &MovablePoint::getSavedRelPos() const {
    return mSavedRelPos;
}

void MovablePoint::drawHovered(SkCanvas * const canvas,
                               const float invScale)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.5f*invScale);
    paint.setColor(toSkColor(eSettings::instance().fColors.pointHoverOutline));
    canvas->drawCircle(toSkPoint(getAbsolutePos()),
                       static_cast<float>(mRadius)*invScale, paint);
    //pen.setCosmetic(true);
    //p->setPen(pen);
//    drawCosmeticEllipse(p, getAbsolutePos(),
//                        mRadius, mRadius);
}

void MovablePoint::setAbsolutePos(const QPointF &pos) {
    if(mTrans_cv) setRelativePos(mTrans_cv->mapAbsPosToRel(pos));
    else setRelativePos(pos);
}

QPointF MovablePoint::mapRelativeToAbsolute(const QPointF &relPos) const {
    if(mTrans_cv) return mTrans_cv->mapRelPosToAbs(relPos);
    else return relPos;
}

QPointF MovablePoint::mapAbsoluteToRelative(const QPointF &absPos) const {
    if(mTrans_cv) return mTrans_cv->mapAbsPosToRel(absPos);
    else return absPos;
}

QPointF MovablePoint::getAbsolutePos() const {
    return mapRelativeToAbsolute(getRelativePos());
}

void MovablePoint::drawOnAbsPosSk(SkCanvas * const canvas,
                                  const SkPoint &absPos,
                                  const float invScale,
                                  const SkColor &fillColor,
                                  const bool keyOnCurrent)
{
    // Update global pivot used for gizmos with this point's absolute position
    const auto doc = Document::sInstance;
    doc->fPivotPosForGizmos = getAbsolutePos();
    doc->fPivotPosForGizmosValid = true;

    const float scaledRadius = static_cast<float>(mRadius)*invScale;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(fillColor);

    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(absPos, scaledRadius, paint);

    const auto colors = eSettings::instance().fColors;

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(toSkColor(colors.baseButton));
    paint.setStrokeWidth(invScale);
    canvas->drawCircle(absPos, scaledRadius, paint);

    if (keyOnCurrent) {
        const float halfRadius = scaledRadius*0.5f;

        paint.setColor(toSkColor(colors.pointKeyInner));
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(absPos, halfRadius, paint);

        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(0.5f*invScale);
        paint.setColor(toSkColor(colors.pointKeyOuter));
        canvas->drawCircle(absPos, halfRadius, paint);
    }
}

void MovablePoint::drawSk(SkCanvas * const canvas,
                          const CanvasMode mode,
                          const float invScale,
                          const bool keyOnCurrent,
                          const bool ctrlPressed)
{
    Q_UNUSED(mode)
    Q_UNUSED(keyOnCurrent)
    Q_UNUSED(ctrlPressed)
    const auto colors = eSettings::instance().fColors;
    const SkColor fillCol = mSelected ? toSkColor(colors.pointSelected) : toSkColor(colors.point);
    const SkPoint absPos = toSkPoint(getAbsolutePos());
    drawOnAbsPosSk(canvas, absPos, invScale, fillCol);
}

bool MovablePoint::isVisible(const CanvasMode mode) const {
    return mode == CanvasMode::pointTransform;
}

BasicTransformAnimator *MovablePoint::getTransform() {
    return mTrans_cv;
}

void MovablePoint::setTransform(BasicTransformAnimator * const trans) {
    mTrans_cv = trans;
}

bool MovablePoint::isPointAtAbsPos(const QPointF &absPoint,
                                   const CanvasMode mode,
                                   const qreal invScale) {
    if(isHidden(mode)) return false;
    const QPointF dist = getAbsolutePos() - absPoint;
    return pointToLen(dist) < mRadius*invScale;
}

void MovablePoint::rectPointsSelection(const QRectF &absRect,
                                       const CanvasMode mode,
                                       const PtOp &adder) {
    if(!selectionEnabled()) return;
    if(isHidden(mode)) return;
    if(isContainedInRect(absRect)) adder(this);
}

bool MovablePoint::isContainedInRect(const QRectF &absRect) {
    return absRect.contains(getAbsolutePos());
}

void MovablePoint::rotateRelativeToSavedPivot(const qreal rot) {
    QMatrix mat;
    mat.translate(mPivot.x(), mPivot.y());
    mat.rotate(rot);
    mat.translate(-mPivot.x(), -mPivot.y());
    moveToRel(mat.map(mSavedRelPos));
}

void MovablePoint::scaleRelativeToSavedPivot(const qreal sx,
                                             const qreal sy)
{
    QMatrix mat;
    mat.translate(mPivot.x(), mPivot.y());
    mat.scale(sx, sy);
    mat.translate(-mPivot.x(), -mPivot.y());
    moveToRel(mat.map(mSavedRelPos));
}

void MovablePoint::shearRelativeToSavedPivot(const qreal shearX,
                                             const qreal shearY)
{
    QMatrix mat;
    mat.translate(mPivot.x(), mPivot.y());
    mat.shear(shearX, shearY);
    mat.translate(-mPivot.x(), -mPivot.y());
    moveToRel(mat.map(mSavedRelPos));
}

void MovablePoint::saveTransformPivotAbsPos(const QPointF &absPivot) {
    if(mTrans_cv) mPivot = mTrans_cv->mapAbsPosToRel(absPivot);
    else mPivot = absPivot;
}

void MovablePoint::rotateBy(const qreal rot) {
    QMatrix rotMatrix;
    rotMatrix.translate(-mPivot.x(), -mPivot.y());
    rotMatrix.rotate(rot);
    rotMatrix.translate(mPivot.x(), mPivot.y());
    setRelativePos(rotMatrix.map(mSavedRelPos));
}

void MovablePoint::scale(const qreal scaleXBy,
                         const qreal scaleYBy) {
    QMatrix scaleMatrix;
    scaleMatrix.translate(-mPivot.x(), -mPivot.y());
    scaleMatrix.scale(scaleXBy, scaleYBy);
    scaleMatrix.translate(mPivot.x(), mPivot.y());
    setRelativePos(scaleMatrix.map(mSavedRelPos));
}

void MovablePoint::moveToAbs(const QPointF& absPos) {
    setAbsolutePos(absPos);
}

void MovablePoint::moveByAbs(const QPointF &absTrans) {
    moveToAbs(mapRelativeToAbsolute(mSavedRelPos) + absTrans);
}

void MovablePoint::moveToRel(const QPointF &relPos) {
    moveByRel(relPos - mSavedRelPos);
}

void MovablePoint::moveByRel(const QPointF &relTranslation) {
    setRelativePos(mSavedRelPos + relTranslation);
}

void MovablePoint::scale(const qreal scaleBy) {
    scale(scaleBy, scaleBy);
}

void MovablePoint::setRadius(const qreal radius) {
    mRadius = radius;
}

qreal MovablePoint::getRadius() {
    return mRadius;
}

void MovablePoint::setSelected(const bool selected, const Op &deselect) {
    mSelected = selected;
    mRemoveFromSelection = deselect;
}

void MovablePoint::deselect() {
    if(mRemoveFromSelection) mRemoveFromSelection();
}

bool MovablePoint::isNodePoint() {
    return mType == MovablePointType::TYPE_PATH_POINT;
}

bool MovablePoint::isSmartNodePoint() {
    return mType == MovablePointType::TYPE_SMART_PATH_POINT;
}

bool MovablePoint::isPivotPoint() {
    return mType == MovablePointType::TYPE_PIVOT_POINT;
}

bool MovablePoint::isCtrlPoint() {
    return mType == MovablePointType::TYPE_CTRL_POINT;
}
