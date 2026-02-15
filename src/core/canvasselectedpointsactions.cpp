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
#include "Animators/SmartPath/smartpathanimator.h"
#include "MovablePoints/pathpointshandler.h"
#include "Private/document.h"
#include <QVector>
#include <algorithm>

QList<SmartNodePoint*> Canvas::getSortedSelectedNodes() {
    QList<SmartNodePoint*> nodes;
    for(const auto& point : qAsConst(mSelectedPoints_d)) {
        if(point->isSmartNodePoint()) {
            nodes << static_cast<SmartNodePoint*>(point);
        }
    }
    std::sort(nodes.begin(), nodes.end(),
              [](SmartNodePoint* pt1, SmartNodePoint* pt2) {
        return pt1->getNodeId() > pt2->getNodeId();
    });
    return nodes;
}

bool Canvas::connectPoints()
{
    prp_pushUndoRedoName(tr("Connect Nodes"));
    const auto nodes = getSortedSelectedNodes();
    QList<SmartNodePoint*> endNodes;
    for (const auto& node : nodes) {
        if (node->isEndPoint()) { endNodes.append(node); }
    }
    if (endNodes.count() == 2) {
        const auto point1 = endNodes.first();
        const auto point2 = endNodes.last();
        const auto node1 = point1->getTargetNode();
        const auto node2 = point2->getTargetNode();

        const auto handler = point1->getHandler();
        const bool success = point2->actionConnectToNormalPoint(point1);
        if (success) {
            clearPointsSelection();
            const int targetId1 = node1->getNodeId();
            const auto sel1 = handler->getPointWithId<SmartNodePoint>(targetId1);
            addPointToSelection(sel1);
            const int targetId2 = node2->getNodeId();
            const auto sel2 = handler->getPointWithId<SmartNodePoint>(targetId2);
            addPointToSelection(sel2);
        } else { return false; }
    }
    return true;
}

void Canvas::disconnectPoints()
{
    prp_pushUndoRedoName(tr("Disconnect Nodes"));
    const auto nodes = getSortedSelectedNodes();
    for (const auto& node : nodes) {
        const auto nextPoint = node->getNextPoint();
        if (!nextPoint || !nextPoint->isSelected()) { continue; }
        node->actionDisconnectFromNormalPoint(nextPoint);
        break;
    }
    clearPointsSelection();
}

void Canvas::mergePoints()
{
    prp_pushUndoRedoName(tr("Merge Nodes"));
    const auto nodes = getSortedSelectedNodes();

    if (nodes.count() == 2) {
        const auto firstPoint = nodes.last();
        const auto secondPoint = nodes.first();

        const bool ends = firstPoint->isEndPoint() && secondPoint->isEndPoint();
        const bool neigh = firstPoint->getPreviousPoint() == secondPoint ||
                           firstPoint->getNextPoint() == secondPoint;
        if (!ends && !neigh) { return; }
        if (ends) {
            const bool success = connectPoints();
            if (success) { mergePoints(); }
            return;
        }
        removePointFromSelection(secondPoint);
        firstPoint->actionMergeWithNormalPoint(secondPoint);
    } else {
        for (const auto& node : nodes) {
            const auto nextPoint = node->getNextPoint();
            if (!nextPoint || !nextPoint->isSelected()) { continue; }
            node->actionMergeWithNormalPoint(nextPoint);
        }
        clearPointsSelection();
    }
}

void Canvas::splitPoints()
{
    prp_pushUndoRedoName(tr("Split Nodes"));
    const auto nodes = getSortedSelectedNodes();
    if (nodes.isEmpty()) { return; }

    QVector<stdptr<SmartNodePoint>> selection;
    selection.reserve(nodes.count() * 2);
    const auto rememberPoint = [&selection](SmartNodePoint* const candidate) {
        if (!candidate) { return; }
        stdptr<SmartNodePoint> ref(candidate);
        SmartNodePoint* const raw = ref.get();
        if (!raw) { return; }
        const bool alreadyStored = std::any_of(selection.cbegin(),
                                               selection.cend(),
                                               [raw](const stdptr<SmartNodePoint>& stored) {
            return stored.get() == raw;
        });
        if (!alreadyStored) {
            selection.append(ref);
        }
    };
    bool changed = false;

    for (const auto& node : nodes) {
        if (!node) { continue; }
        rememberPoint(node);
        if (!node->getTargetAnimator()) { continue; }
        if (!node->getTargetPath()) { continue; }
        if (!node->isNormal()) { continue; }

        auto handler = node->getHandler();
        if (!handler) { continue; }

        SmartNodePoint* neighbor = nullptr;
        bool usePrev = false;
        if (node->hasNextNormalPoint()) {
            neighbor = handler->getNextNormalNode(node->getNodeId());
        }
        if (!neighbor || neighbor == node) {
            neighbor = handler->getPrevNormalNode(node->getNodeId());
            usePrev = true;
        }
        if (!neighbor || neighbor == node) { continue; }

        const int nodeId = node->getNodeId();
        const int neighborId = neighbor->getNodeId();
        const NodePointValues values = node->getPointValues();
        const qreal t = usePrev ? 1.0 : 0.0;

        int newId = -1;
        if (usePrev) {
            newId = node->getTargetAnimator()->actionInsertNodeBetween(neighborId,
                                                                       nodeId,
                                                                       t,
                                                                       values);
        } else {
            newId = node->getTargetAnimator()->actionInsertNodeBetween(nodeId,
                                                                       neighborId,
                                                                       t,
                                                                       values);
        }
        if (newId < 0) { continue; }

        auto newNode = handler->getPointWithId<SmartNodePoint>(newId);
        if (!newNode) { continue; }

        rememberPoint(newNode);

        node->actionDisconnectFromNormalPoint(newNode);
        changed = true;
    }

    if (!changed) { return; }

    clearPointsSelection();
    for (const auto& stored : selection) {
        if (auto* const point = stored.get()) {
            addPointToSelection(point);
        }
    }
}

void Canvas::subdivideSegments()
{
    prp_pushUndoRedoName(tr("Subdivide Segments"));
    const auto nodes = getSortedSelectedNodes();
    for (const auto& node : nodes) {
        const auto nextPoint = node->getNextPoint();
        if (!nextPoint || !nextPoint->isSelected()) { continue; }
        NormalSegment(node, nextPoint).divideAtT(0.5);
    }
    clearPointsSelection();
}

void Canvas::makeSelectedNodeFirst()
{
    const auto nodes = getSortedSelectedNodes();
    if (nodes.count() != 1) { return; }

    auto node = nodes.first();
    if (!node) { return; }
    const int nodeId = node->getNodeId();

    auto animator = node->getTargetAnimator();
    if (!animator) { return; }
    auto handler = node->getHandler();
    if (!handler) { return; }

    if (!animator->isClosed()) {
        // Open paths cannot change the first node; flipping direction is only allowed from the last node.
        const int lastNodeId = handler->count() - 1;
        if (lastNodeId >= 0 && nodeId == lastNodeId) {
            reverseSelectedNodesOrder();
        }
        return;
    }

    if (nodeId <= 0) { return; }

    clearPointsSelection();

    animator->actionSetFirstNode(nodeId);
    handler->updateAllPoints();
    auto firstNode = handler->getPointWithId<SmartNodePoint>(0);
    if (firstNode) {
        addPointToSelection(firstNode);
    }
}

void Canvas::reverseSelectedNodesOrder()
{
    bool autoSelected = false;
    auto nodes = getSortedSelectedNodes();
    if (nodes.isEmpty()) {
        selectAllPointsAction();
        nodes = getSortedSelectedNodes();
        autoSelected = true;
    }
    if (nodes.isEmpty()) { return; }

    auto node = nodes.first();
    if (!node) { return; }

    auto animator = node->getTargetAnimator();
    if (!animator) { return; }
    auto handler = node->getHandler();
    if (!handler) { return; }

    auto firstPoint = handler->getPointWithId<SmartNodePoint>(0);
    if (!firstPoint) { return; }
    const auto path = firstPoint->getTargetPath();
    if (!path) { return; }
    const bool closed = path->isClosed();
    const int nodeCount = path->getNodeCount();
    if (nodeCount <= 1) { return; }

    clearPointsSelection();

    animator->actionReverseCurrent();
    handler->updateAllPoints();

    if (closed) {
        SmartNodePoint* newPoint = handler->getPointWithId<SmartNodePoint>(nodeCount - 1);
        if (newPoint) {
            const int rotatedId = newPoint->getNodeId();
            if (rotatedId > 0) {
                animator->actionSetFirstNode(rotatedId);
                handler->updateAllPoints();
            }
        }
    }

    auto firstNode = handler->getPointWithId<SmartNodePoint>(0);
    if (!autoSelected && firstNode) {
        addPointToSelection(firstNode);
    }
}

void Canvas::setPointCtrlsMode(const CtrlsMode mode) {
    for(const auto& point : qAsConst(mSelectedPoints_d)) {
        if(point->isSmartNodePoint()) {
            auto asNodePt = static_cast<SmartNodePoint*>(point);
            asNodePt->setCtrlsMode(mode);
        }
    }
}

void Canvas::makeSegmentCurve()
{
    prp_pushUndoRedoName(tr("Make Segments Curves"));
    QList<SmartNodePoint*> selectedSNodePoints;
    for (const auto& point : qAsConst(mSelectedPoints_d)) {
        if (point->isSmartNodePoint()) {
            const auto asNodePt = static_cast<SmartNodePoint*>(point);
            selectedSNodePoints.append(asNodePt);
        }
    }
    for (const auto& selectedPoint : selectedSNodePoints) {
        SmartNodePoint * const nextPoint = selectedPoint->getNextPoint();
        SmartNodePoint * const prevPoint = selectedPoint->getPreviousPoint();
        if (selectedSNodePoints.contains(nextPoint)) {
            selectedPoint->setC2Enabled(true);
        }
        if (selectedSNodePoints.contains(prevPoint)) {
            selectedPoint->setC0Enabled(true);
        }
    }
}

void Canvas::makeSegmentLine()
{
    prp_pushUndoRedoName(tr("Make Segments Lines"));
    QList<SmartNodePoint*> selectedSNodePoints;
    for (const auto& point : qAsConst(mSelectedPoints_d)) {
        if (point->isSmartNodePoint()) {
            auto asNodePt = static_cast<SmartNodePoint*>(point);
            selectedSNodePoints.append(asNodePt);
        }
    }
    for (const auto& selectedPoint : selectedSNodePoints) {
        SmartNodePoint * const nextPoint = selectedPoint->getNextPoint();
        SmartNodePoint * const prevPoint = selectedPoint->getPreviousPoint();
        if (selectedSNodePoints.contains(nextPoint)) {
            selectedPoint->setC2Enabled(false);
        }
        if (selectedSNodePoints.contains(prevPoint)) {
            selectedPoint->setC0Enabled(false);
        }
    }
}

void Canvas::finishSelectedPointsTransform() {
    for(const auto& point : qAsConst(mSelectedPoints_d)) {
        point->finishTransform();
    }
}

void Canvas::startSelectedPointsTransform() {
    for(const auto& point : qAsConst(mSelectedPoints_d)) {
        point->startTransform();
    }
}

void Canvas::cancelSelectedPointsTransform() {
    for(const auto& point : qAsConst(mSelectedPoints_d)) {
        point->cancelTransform();
    }
}

void Canvas::moveSelectedPointsByAbs(const QPointF &by,
                                     const bool startTransform) {
    if(startTransform) {
        startSelectedPointsTransform();
        for(const auto& point : qAsConst(mSelectedPoints_d)) {
            point->moveByAbs(by);
        }
    } else {
        for(const auto& point : qAsConst(mSelectedPoints_d)) {
            point->moveByAbs(by);
        }
    }
}

void Canvas::selectAndAddContainedPointsToSelection(const QRectF& absRect) {
    const auto adder = [this](MovablePoint* const pt) {
        addPointToSelection(pt);
    };
    for(const auto& box : qAsConst(mSelectedBoxes)) {
        box->selectAndAddContainedPointsToList(absRect, adder, mCurrentMode);
    }
}

void Canvas::addPointToSelection(MovablePoint* const point) {
    if(point->isSelected()) return;
    const auto ptDeselector = [this, point]() {
        removePointFromSelection(point);
    };
    point->setSelected(true, ptDeselector);
    mSelectedPoints_d.append(point);
    emit pointSelectionChanged();
    schedulePivotUpdate();
}

void Canvas::removePointFromSelection(MovablePoint * const point) {
    point->setSelected(false);
    mSelectedPoints_d.removeOne(point);
    emit pointSelectionChanged();
    schedulePivotUpdate();
}

void Canvas::removeSelectedPointsAndClearList()
{
    if (mPressedPoint && mPressedPoint->isCtrlPoint()) {
        mPressedPoint->finishTransform();
        removePointFromSelection(mPressedPoint);
        mPressedPoint->remove();
        schedulePivotUpdate();
        return;
    }

    const auto selected = mSelectedPoints_d;
    if (selected.count() < 1) {
        removeSelectedBoxesAndClearList();
        return;
    }

    for (const auto& point : selected) {
        point->setSelected(false);
        point->remove();
    }
    mSelectedPoints_d.clear();
    emit pointSelectionChanged();
    schedulePivotUpdate();
}

void Canvas::removeSelectedPointsApprox()
{
    if (mPressedPoint && mPressedPoint->isCtrlPoint()) {
        mPressedPoint->finishTransform();
        removePointFromSelection(mPressedPoint);
        if (auto *smartPoint = dynamic_cast<SmartNodePoint*>(mPressedPoint.data())) {
            smartPoint->actionRemove(true);
        } else {
            mPressedPoint->remove();
        }
        schedulePivotUpdate();
        return;
    }

    const auto selected = mSelectedPoints_d;
    if (selected.count() < 1) {
        removeSelectedBoxesAndClearList();
        return;
    }

    for (const auto& point : selected) {
        point->setSelected(false);
        if (auto *smartPoint = dynamic_cast<SmartNodePoint*>(point)) {
            smartPoint->actionRemove(true);
        } else {
            point->remove();
        }
    }
    mSelectedPoints_d.clear();
    emit pointSelectionChanged();
    schedulePivotUpdate();
}


void Canvas::clearPointsSelection() {
    for(const auto& point : qAsConst(mSelectedPoints_d)) {
        if(point) point->setSelected(false);
    }

    mSelectedPoints_d.clear();
    emit pointSelectionChanged();
    if(mCurrentMode == CanvasMode::pointTransform) schedulePivotUpdate();
//    if(mLastPressedPoint) {
//        mLastPressedPoint->setSelected(false);
//        mLastPressedPoint = nullptr;
//    }
//    setCurrentEndPoint(nullptr);
}

void Canvas::clearLastPressedPoint() {
    if(mPressedPoint) {
        mPressedPoint->setSelected(false);
        mPressedPoint = nullptr;
    }
}

QPointF Canvas::getSelectedPointsAbsPivotPos() {
    if(mSelectedPoints_d.isEmpty()) return QPointF(0, 0);
    QPointF posSum(0, 0);
    for(const auto& point : qAsConst(mSelectedPoints_d)) {
        posSum += point->getAbsolutePos();
    }
    const qreal invCount = 1./mSelectedPoints_d.count();
    return posSum*invCount;
}

bool Canvas::isPointSelectionEmpty() const {
    return mSelectedPoints_d.isEmpty();
}

int Canvas::getPointsSelectionCount() const {
    return mSelectedPoints_d.count();
}

void Canvas::rotateSelectedPointsBy(const qreal rotBy,
                                    const QPointF &absOrigin,
                                    const bool startTrans) {
    if(mSelectedPoints_d.isEmpty()) return;
    if(startTrans) {
        if(mDocument.fLocalPivot) {
            for(const auto& point : qAsConst(mSelectedPoints_d)) {
                point->startTransform();
                point->saveTransformPivotAbsPos(point->getAbsolutePos());
                point->rotateRelativeToSavedPivot(rotBy);
            }
        } else {
            for(const auto& point : qAsConst(mSelectedPoints_d)) {
                point->startTransform();
                point->saveTransformPivotAbsPos(absOrigin);
                point->rotateRelativeToSavedPivot(rotBy);
            }
        }
    } else {
        for(const auto& point : qAsConst(mSelectedPoints_d)) {
            point->rotateRelativeToSavedPivot(rotBy);
        }
    }
}

void Canvas::scaleSelectedPointsBy(const qreal scaleXBy,
                                   const qreal scaleYBy,
                                   const QPointF &absOrigin,
                                   const bool startTrans) {
    if(mSelectedPoints_d.isEmpty()) return;
    if(startTrans) {
        if(mDocument.fLocalPivot) {
            for(const auto& point : qAsConst(mSelectedPoints_d)) {
                point->startTransform();
                point->saveTransformPivotAbsPos(point->getAbsolutePos());
                point->scaleRelativeToSavedPivot(scaleXBy, scaleYBy);
            }
        } else {
            for(const auto& point : qAsConst(mSelectedPoints_d)) {
                point->startTransform();
                point->saveTransformPivotAbsPos(absOrigin);
                point->scaleRelativeToSavedPivot(scaleXBy, scaleYBy);
            }
        }
    } else {
        for(const auto& point : qAsConst(mSelectedPoints_d)) {
            point->scaleRelativeToSavedPivot(scaleXBy, scaleYBy);
        }
    }
}

void Canvas::shearSelectedPointsBy(const qreal shearXBy,
                                   const qreal shearYBy,
                                   const QPointF &absOrigin,
                                   const bool startTrans)
{
    if (mSelectedPoints_d.isEmpty()) { return; }
    if (startTrans) {
        if (mDocument.fLocalPivot) {
            for (const auto& point : qAsConst(mSelectedPoints_d)) {
                point->startTransform();
                point->saveTransformPivotAbsPos(point->getAbsolutePos());
                point->shearRelativeToSavedPivot(shearXBy, shearYBy);
            }
        } else {
            for (const auto& point : qAsConst(mSelectedPoints_d)) {
                point->startTransform();
                point->saveTransformPivotAbsPos(absOrigin);
                point->shearRelativeToSavedPivot(shearXBy, shearYBy);
            }
        }
    } else {
        for (const auto& point : qAsConst(mSelectedPoints_d)) {
            point->shearRelativeToSavedPivot(shearXBy, shearYBy);
        }
    }
}

void Canvas::clearPointsSelectionOrDeselect() {
    if(mSelectedPoints_d.isEmpty() ) {
        deselectAllBoxes();
    } else {
        clearPointsSelection();
        clearCurrentSmartEndPoint();
        clearLastPressedPoint();
    }
}
