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

#include "canvas.h"
#include "MovablePoints/smartnodepoint.h"
#include "Boxes/smartvectorpath.h"
#include "eevent.h"

void Canvas::clearCurrentSmartEndPoint() {
    setCurrentSmartEndPoint(nullptr);
}

void Canvas::setCurrentSmartEndPoint(SmartNodePoint * const point) {
    if(mLastEndPoint) mLastEndPoint->setSelected(false);
    if(point) point->setSelected(true);
    mLastEndPoint = point;
}
#include "MovablePoints/pathpointshandler.h"
#include "Animators/SmartPath/smartpathcollection.h"
#include "Animators/transformanimator.h"

void Canvas::handleAddSmartPointMousePress(const eMouseEvent &e) {
    if(mLastEndPoint ? mLastEndPoint->isHidden(mCurrentMode) : false) {
        clearCurrentSmartEndPoint();
    }
    qptr<BoundingBox> test;

    auto nodePointUnderMouse = static_cast<SmartNodePoint*>(mPressedPoint.data());
    if(nodePointUnderMouse ? !nodePointUnderMouse->isEndPoint() : false) {
        nodePointUnderMouse = nullptr;
    }
    if(nodePointUnderMouse == mLastEndPoint &&
            nodePointUnderMouse) return;
    if(!mLastEndPoint && !nodePointUnderMouse) {
        const auto newPath = enve::make_shared<SmartVectorPath>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());
        const auto relPos = newPath->mapAbsPosToRel(e.fPos);
        newPath->getBoxTransformAnimator()->setPosition(relPos.x(), relPos.y());
        const auto newHandler = newPath->getPathAnimator();
        const auto node = newHandler->createNewSubPathAtRelPos({0, 0});
        setCurrentSmartEndPoint(node);
    } else {
        if(!nodePointUnderMouse) {
            const auto newPoint = mLastEndPoint->actionAddPointAbsPos(e.fPos);
            //newPoint->startTransform();
            setCurrentSmartEndPoint(newPoint);
        } else if(!mLastEndPoint) {
            setCurrentSmartEndPoint(nodePointUnderMouse);
        } else { // mCurrentSmartEndPoint
            const auto targetNode = nodePointUnderMouse->getTargetNode();
            const auto handler = nodePointUnderMouse->getHandler();
            const bool success = nodePointUnderMouse->isEndPoint() &&
                    mLastEndPoint->actionConnectToNormalPoint(
                        nodePointUnderMouse);
            if(success) {
                const int newTargetId = targetNode->getNodeId();
                const auto sel = handler->getPointWithId<SmartNodePoint>(newTargetId);
                setCurrentSmartEndPoint(sel);
            }
        }
    } // pats is not null
}


void Canvas::handleAddSmartPointMouseMove(const eMouseEvent &e) {
    if(!mLastEndPoint) return;
    if(mStartTransform) mLastEndPoint->startTransform();
    if(mLastEndPoint->hasNextNormalPoint() &&
       mLastEndPoint->hasPrevNormalPoint()) {
        mLastEndPoint->setCtrlsMode(CtrlsMode::corner);
        mLastEndPoint->setC0Enabled(true);
        mLastEndPoint->moveC0ToAbsPos(e.fPos);
    } else {
        if(!mLastEndPoint->hasNextNormalPoint() &&
           !mLastEndPoint->hasPrevNormalPoint()) {
            mLastEndPoint->setCtrlsMode(CtrlsMode::corner);
            mLastEndPoint->setC2Enabled(true);
        } else {
            mLastEndPoint->setCtrlsMode(CtrlsMode::symmetric);
        }
        if(mLastEndPoint->hasNextNormalPoint()) {
            mLastEndPoint->moveC0ToAbsPos(e.fPos);
        } else {
            mLastEndPoint->moveC2ToAbsPos(e.fPos);
        }
    }
}

void Canvas::handleAddSmartPointMouseRelease(const eMouseEvent &e) {
    Q_UNUSED(e)
    if(mLastEndPoint) {
        if(!mStartTransform) mLastEndPoint->finishTransform();
        //mCurrentSmartEndPoint->prp_afterWholeInfluenceRangeChanged();
        if(!mLastEndPoint->isEndPoint())
            clearCurrentSmartEndPoint();
    }
}
