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

#include "pointshandler.h"

PointsHandler::PointsHandler() {}

void PointsHandler::insertPt(const int id, const stdsptr<MovablePoint> &pt) {
    pt->setTransform(mTrans);
    mPts.insert(id, pt);
}

void PointsHandler::appendPt(const stdsptr<MovablePoint> &pt) {
    return insertPt(mPts.count(), pt);
}

void PointsHandler::remove(const stdsptr<MovablePoint> &pt) {
    if(pt->isSelected()) removeFromSelection(pt.get());
    mPts.removeOne(pt);
}

void PointsHandler::removeLast() {
    remove(mPts.last());
}

void PointsHandler::clear() {
    mPts.clear();
    mSelectedPts.clear();
}

void PointsHandler::addToSelection(MovablePoint * const pt) {
    mSelectedPts << pt;
}

void PointsHandler::removeFromSelection(MovablePoint * const pt) {
    mSelectedPts.removeOne(pt);
}

void PointsHandler::clearSelection() { mSelectedPts.clear(); }

MovablePoint *PointsHandler::getPointAtAbsPos(const QPointF &absPos,
                                              const CanvasMode mode,
                                              const qreal invScale) {
    for(int i = mPts.count() - 1; i >= 0; i--) {
        const auto& pt = mPts.at(i);
        const auto at = pt->getPointAtAbsPos(absPos, mode, invScale);
        if(at) return at;
    }
    return nullptr;
}

void PointsHandler::addAllPointsToSelection(const MovablePoint::PtOp& adder,
                                            const CanvasMode mode) const {
    for(const auto& pt : mPts) {
        if(pt->isSelected() || pt->isHidden(mode)) continue;
        if(pt->selectionEnabled()) adder(pt.get());
    }
}

void PointsHandler::addInRectForSelection(const QRectF &absRect,
                                          const MovablePoint::PtOp& adder,
                                          const CanvasMode mode) const {
    for(const auto& pt : mPts) {
        if(!pt->selectionEnabled()) continue;
        if(pt->isSelected() || pt->isHidden(mode)) continue;
        pt->rectPointsSelection(absRect, mode, adder);
    }
}

void PointsHandler::drawPoints(SkCanvas * const canvas,
                               const float invScale,
                               const bool keyOnCurrentFrame,
                               const CanvasMode mode,
                               const bool ctrlPressed) const {
    for(int i = mPts.count() - 1; i >= 0; i--) {
        const auto& pt = mPts.at(i);
        if(pt->isVisible(mode))
            pt->drawSk(canvas, mode, invScale,
                       keyOnCurrentFrame, ctrlPressed);
    }
}

bool PointsHandler::isEmpty() const { return mPts.isEmpty(); }

int PointsHandler::count() const { return mPts.count(); }

void PointsHandler::move(const int from, const int to) {
    mPts.move(from, to);
}

BasicTransformAnimator *PointsHandler::transform() const {
    return mTrans;
}

void PointsHandler::setTransform(BasicTransformAnimator * const trans) {
    if(trans == mTrans) return;
    for(const auto& pt : qAsConst(mPts)) pt->setTransform(trans);
    mTrans = trans;
}
