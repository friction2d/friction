/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "canvas.h"

#include "Private/document.h"
#include "eevent.h"
#include "MovablePoints/smartnodepoint.h"

QPointF Canvas::snapPosToGrid(const QPointF& pos,
                              Qt::KeyboardModifiers modifiers,
                              bool forceSnap) const
{
    if (!mHasWorldToScreen) { return pos; }

    const auto& settings = mDocument.getGrid()->getSettings();

    if (!settings.snapEnabled) { return pos; }

    const bool bypassSnap = modifiers & Qt::ShiftModifier;
    if (bypassSnap) { return pos; }

    const bool gridEnabled = settings.snapToGrid && settings.show;
    const bool canvasSnapEnabled = settings.snapToCanvas;
    const bool pivotsSnapEnabled = settings.snapToPivots;
    const bool boxesSnapEnabled = settings.snapToBoxes;
    const bool nodesSnapEnabled = settings.snapToNodes;

    std::vector<QPointF> pivotTargets;
    std::vector<QPointF> boxTargets;
    std::vector<QPointF> nodeTargets;
    if (pivotsSnapEnabled || boxesSnapEnabled || nodesSnapEnabled) {
        collectSnapTargets(pivotsSnapEnabled,
                           boxesSnapEnabled,
                           nodesSnapEnabled,
                           pivotTargets,
                           boxTargets,
                           nodeTargets);
    }
    const bool hasPivotTargets = pivotsSnapEnabled && !pivotTargets.empty();
    const bool hasBoxTargets = boxesSnapEnabled && !boxTargets.empty();
    const bool hasNodeTargets = nodesSnapEnabled && !nodeTargets.empty();

    const bool hasSnapSource = gridEnabled || canvasSnapEnabled ||
                               hasPivotTargets || hasBoxTargets || hasNodeTargets;
    const bool shouldForce = (forceSnap && hasSnapSource) ||
                             (modifiers & Qt::ControlModifier);

    if (!hasSnapSource && !shouldForce) { return pos; }

    QRectF canvasRect;
    const QRectF* canvasRectPtr = nullptr;
    if (canvasSnapEnabled) {
        canvasRect = QRectF(QPointF(0.0, 0.0),
                            QSizeF(mWidth, mHeight));
        canvasRectPtr = &canvasRect;
    }

    return mDocument.getGrid()->maybeSnapPivot(pos,
                                               mWorldToScreen,
                                               shouldForce,
                                               false,
                                               canvasRectPtr,
                                               nullptr,
                                               hasPivotTargets ? &pivotTargets : nullptr,
                                               hasBoxTargets ? &boxTargets : nullptr,
                                               hasNodeTargets ? &nodeTargets : nullptr);
}

QPointF Canvas::snapEventPos(const eMouseEvent& e,
                             bool forceSnap) const
{
    return snapPosToGrid(e.fPos,
                         e.fModifiers,
                         forceSnap);
}

void Canvas::collectAnchorOffsets(const Friction::Core::Grid::Settings &settings)
{
    mGridMoveStartPivot = getSelectedBoxesAbsPivotPos();

    mGridSnapAnchorOffsets.clear();
    if (settings.snapAnchorPivot) {
        mGridSnapAnchorOffsets.emplace_back(QPointF(0.0, 0.0));
    }

    QRectF combinedRect;
    bool hasRect = false;
    for (const auto& box : qAsConst(mSelectedBoxes)) {
        const QRectF rect = box->getAbsBoundingRect();
        if (rect.width() < 0.0 || rect.height() < 0.0) { continue; }
        if (!hasRect) {
            combinedRect = rect;
            hasRect = true;
        } else {
            combinedRect = combinedRect.united(rect);
        }
    }

    if (hasRect && settings.snapAnchorBounds) {
        const QPointF topLeft = combinedRect.topLeft();
        const QPointF topRight = combinedRect.topRight();
        const QPointF bottomLeft = combinedRect.bottomLeft();
        const QPointF bottomRight = combinedRect.bottomRight();
        const QPointF topCenter((topLeft.x() + topRight.x()) * 0.5, topLeft.y());
        const QPointF bottomCenter((bottomLeft.x() + bottomRight.x()) * 0.5, bottomLeft.y());
        const QPointF leftCenter(topLeft.x(), (topLeft.y() + bottomLeft.y()) * 0.5);
        const QPointF rightCenter(topRight.x(), (topRight.y() + bottomRight.y()) * 0.5);
        const QPointF center = combinedRect.center();

        mGridSnapAnchorOffsets.emplace_back(topLeft - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(topRight - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(bottomLeft - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(bottomRight - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(topCenter - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(bottomCenter - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(leftCenter - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(rightCenter - mGridMoveStartPivot);
        mGridSnapAnchorOffsets.emplace_back(center - mGridMoveStartPivot);
    }

    if (settings.snapAnchorNodes) {
        const MovablePoint::PtOp gatherOffsets = [&](MovablePoint* point) {
            if (!point || !point->isSmartNodePoint()) { return; }
            const auto* node = static_cast<SmartNodePoint*>(point);
            if (node) {
                mGridSnapAnchorOffsets.emplace_back(node->getAbsolutePos() - mGridMoveStartPivot);
            }
        };
        for (const auto& box : qAsConst(mSelectedBoxes)) {
            if (!box) { continue; }
            box->selectAllCanvasPts(gatherOffsets, CanvasMode::pointTransform);
        }
    }
}

const QPair<bool, QPointF> Canvas::moveBySnapTargets(const Qt::KeyboardModifiers &modifiers,
                                                     const QPointF &moveBy,
                                                     const Friction::Core::Grid::Settings &settings,
                                                     const bool &includeSelectedBounds,
                                                     const bool &useAnchorOffsets,
                                                     const bool &mustHaveSelected)
{
    QPair<bool, QPointF> result;
    result.first = false;

    const bool snappingActive = settings.snapEnabled;
    if (!snappingActive) { return result; }

    const bool bypassSnap = modifiers & Qt::ShiftModifier;
    const bool forceSnap = modifiers & Qt::ControlModifier;
    const bool hasAnchorOffsets = useAnchorOffsets && !mGridSnapAnchorOffsets.empty();
    const bool pivotsSnapEnabled = snappingActive && settings.snapToPivots;
    const bool boxesSnapEnabled = snappingActive && settings.snapToBoxes;
    const bool nodesSnapEnabled = snappingActive && settings.snapToNodes;

    std::vector<QPointF> pivotTargets;
    std::vector<QPointF> boxTargets;
    std::vector<QPointF> nodeTargets;

    if (pivotsSnapEnabled || boxesSnapEnabled || nodesSnapEnabled) {
        collectSnapTargets(pivotsSnapEnabled,
                           boxesSnapEnabled,
                           nodesSnapEnabled,
                           pivotTargets,
                           boxTargets,
                           nodeTargets,
                           includeSelectedBounds);
    }
    const bool hasPivotTargets = pivotsSnapEnabled && !pivotTargets.empty();
    const bool hasBoxTargets = boxesSnapEnabled && !boxTargets.empty();
    const bool hasNodeTargets = nodesSnapEnabled && !nodeTargets.empty();
    const bool snapSourcesAvailable = snappingActive && (settings.snapToGrid ||
                                                         settings.snapToCanvas ||
                                                         hasPivotTargets ||
                                                         hasBoxTargets ||
                                                         hasNodeTargets ||
                                                         hasAnchorOffsets);

    if (mustHaveSelected && mSelectedBoxes.isEmpty()) { return result; }

    if (mHasWorldToScreen && (snapSourcesAvailable || forceSnap)) {
        const QPointF targetPivot = mGridMoveStartPivot + moveBy;
        QRectF canvasRect;
        const QRectF* canvasPtr = nullptr;
        if (settings.snapToCanvas) {
            canvasRect = QRectF(QPointF(0.0, 0.0), QSizeF(mWidth, mHeight));
            canvasPtr = &canvasRect;
        }
        const auto snapped = mDocument.getGrid()->maybeSnapPivot(targetPivot,
                                                                 mWorldToScreen,
                                                                 forceSnap,
                                                                 bypassSnap,
                                                                 canvasPtr,
                                                                 useAnchorOffsets ? &mGridSnapAnchorOffsets : nullptr,
                                                                 hasPivotTargets ? &pivotTargets : nullptr,
                                                                 hasBoxTargets ? &boxTargets : nullptr,
                                                                 hasNodeTargets ? &nodeTargets : nullptr);
        if (snapped != targetPivot) { result = {true, snapped - mGridMoveStartPivot}; }
    }

    return result;
}

void Canvas::collectSnapTargets(bool includePivots,
                                bool includeBounds,
                                bool includeNodes,
                                std::vector<QPointF>& pivotTargets,
                                std::vector<QPointF>& boxTargets,
                                std::vector<QPointF>& nodeTargets,
                                bool includeSelectedBounds) const
{
    pivotTargets.clear();
    boxTargets.clear();
    nodeTargets.clear();

    if ((!includePivots && !includeBounds && !includeNodes) || !mCurrentContainer) {
        return;
    }

    auto addIfValid = [](std::vector<QPointF>& target, const QPointF& pt) {
        if (isPointFinite(pt)) { target.push_back(pt); }
    };

    auto appendBoundsTargets = [&](const QRectF& rect) {
        const QRectF normalized = rect.normalized();
        if (normalized.isNull() || !normalized.isValid()) { return; }

        const QPointF topLeft = normalized.topLeft();
        const QPointF topRight = normalized.topRight();
        const QPointF bottomLeft = normalized.bottomLeft();
        const QPointF bottomRight = normalized.bottomRight();
        const QPointF topCenter((normalized.left() + normalized.right()) * 0.5, normalized.top());
        const QPointF bottomCenter((normalized.left() + normalized.right()) * 0.5, normalized.bottom());
        const QPointF leftCenter(normalized.left(), (normalized.top() + normalized.bottom()) * 0.5);
        const QPointF rightCenter(normalized.right(), (normalized.top() + normalized.bottom()) * 0.5);

        addIfValid(boxTargets, normalized.center());
        addIfValid(boxTargets, topLeft);
        addIfValid(boxTargets, topRight);
        addIfValid(boxTargets, bottomLeft);
        addIfValid(boxTargets, bottomRight);
        addIfValid(boxTargets, topCenter);
        addIfValid(boxTargets, bottomCenter);
        addIfValid(boxTargets, leftCenter);
        addIfValid(boxTargets, rightCenter);
    };

    const std::function<void(const ContainerBox*, bool)> recurse = [&](const ContainerBox* container,
                                                                       bool ancestorSelected) {
        if (!container) { return; }
        const auto& boxes = container->getContainedBoxes();
        for (auto* box : boxes) {
            if (!box) { continue; }
            const bool selectedBranch = ancestorSelected || box->isSelected();
            const bool visible = box->isVisible();

            if (!selectedBranch && visible) {
                if (includePivots) { addIfValid(pivotTargets, box->getPivotAbsPos()); }
                if (includeBounds) { appendBoundsTargets(box->getAbsBoundingRect()); }
                if (includeNodes) {
                    const MovablePoint::PtOp gather = [&](MovablePoint* point) {
                        if (!point || !point->isSmartNodePoint()) { return; }
                        const auto* node = static_cast<SmartNodePoint*>(point);
                        if (node) { addIfValid(nodeTargets, node->getAbsolutePos()); }
                    };
                    box->selectAllCanvasPts(gather, CanvasMode::pointTransform);
                }
            }

            if (const auto* childContainer = enve_cast<const ContainerBox*>(box)) {
                recurse(childContainer, selectedBranch);
            }
        }
    };

    recurse(mCurrentContainer, false);

    if (includeBounds && includeSelectedBounds) {
        for (const auto& selectedBox : mSelectedBoxes) {
            if (!selectedBox || !selectedBox->isVisible()) { continue; }
            appendBoundsTargets(selectedBox->getAbsBoundingRect());
        }
    }
}
