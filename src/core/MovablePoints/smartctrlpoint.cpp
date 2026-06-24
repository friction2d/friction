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

#include "smartctrlpoint.h"
#include "smartnodepoint.h"
#include "pointhelpers.h"
#include "Animators/SmartPath/smartpathanimator.h"
#include "Animators/transformanimator.h"
#include "Private/esettings.h"

SmartCtrlPoint::SmartCtrlPoint(SmartNodePoint * const parentPoint,
                               const Type& type) :
    NonAnimatedMovablePoint(parentPoint->getTransform(),
                            MovablePointType::TYPE_CTRL_POINT),
    mCtrlType(type), mParentPoint_k(parentPoint) {
    disableSelection();
    updateRadius();
}

void SmartCtrlPoint::drawSk(SkCanvas * const canvas, const CanvasMode mode,
                            const float invScale, const bool keyOnCurrent,
                            const bool ctrlPressed) {
    Q_UNUSED(mode)
    Q_UNUSED(keyOnCurrent)
    Q_UNUSED(ctrlPressed)
    const auto& sett = eSettings::instance();
    const QColor settColor = isSelected() ? sett.fPathControlSelectedColor :
                                            sett.fPathControlColor;
    const SkColor fillCol = toSkColor(settColor);
    const SkPoint absPos = toSkPoint(getAbsolutePos());
    drawOnAbsPosSk(canvas, absPos, invScale, fillCol);
}

void SmartCtrlPoint::setRelativePos(const QPointF &relPos) {
    NonAnimatedMovablePoint::setRelativePos(relPos);
    if(mCtrlType == C0) mParentPoint_k->c0Moved(getRelativePos());
    else mParentPoint_k->c2Moved(getRelativePos());
}

void SmartCtrlPoint::rotateRelativeToSavedPivot(const qreal rotate) {
    const QPointF savedValue = getSavedRelPos() - mParentPoint_k->getSavedRelPos();
    QTransform mat;
    mat.rotate(rotate);
    setRelativePos(mat.map(savedValue) + mParentPoint_k->getRelativePos());
}

void SmartCtrlPoint::scale(const qreal sx, const qreal sy) {
    const QPointF savedValue = getSavedRelPos() - mParentPoint_k->getSavedRelPos();
    QTransform mat;
    mat.scale(sx, sy);
    setRelativePos(mat.map(savedValue) + mParentPoint_k->getRelativePos());
}

void SmartCtrlPoint::updateRadius() {
    setRadius(5.*eSettings::instance().fPathControlScaling);
}

bool SmartCtrlPoint::enabled() const {
    if(mCtrlType == C0) return mParentPoint_k->getC0Enabled();
    // if(mCtrlType == C2)
    else return mParentPoint_k->getC2Enabled();
}

void SmartCtrlPoint::startTransform() {
    NonAnimatedMovablePoint::startTransform();
    if(mParentPoint_k->getCtrlsMode() != CtrlsMode::corner) {
        mOtherCtrlPt_cv->NonAnimatedMovablePoint::startTransform();
    }
    mParentPoint_k->getTargetAnimator()->prp_startTransform();
}

void SmartCtrlPoint::finishTransform() {
    if(mParentPoint_k->getCtrlsMode() != CtrlsMode::corner) {
        mOtherCtrlPt_cv->NonAnimatedMovablePoint::finishTransform();
    }
    mParentPoint_k->getTargetAnimator()->prp_finishTransform();
}

void SmartCtrlPoint::cancelTransform() {
    NonAnimatedMovablePoint::cancelTransform();
    if(mParentPoint_k->getCtrlsMode() != CtrlsMode::corner) {
        mOtherCtrlPt_cv->NonAnimatedMovablePoint::cancelTransform();
    }
    mParentPoint_k->getTargetAnimator()->prp_cancelTransform();
}

void SmartCtrlPoint::setOtherCtrlPt(SmartCtrlPoint * const ctrlPt) {
    mOtherCtrlPt_cv = ctrlPt;
}

void SmartCtrlPoint::remove() {
    if(mCtrlType == C0) mParentPoint_k->setC0Enabled(false);
    else mParentPoint_k->setC2Enabled(false);
}

bool SmartCtrlPoint::isVisible(const CanvasMode mode) const {
    if(mParentPoint_k->isDissolved() ||
       mParentPoint_k->isHidden(mode)) return false;
    if(mCtrlType == C0) return mParentPoint_k->getC0Enabled() &&
                               mParentPoint_k->isPrevNormalSelected();
    if(mCtrlType == C2) return mParentPoint_k->getC2Enabled() &&
                               mParentPoint_k->isNextNormalSelected();
    return false;
}
