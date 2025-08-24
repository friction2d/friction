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

#include "smartpathanimator.h"

#include "node.h"
#include "wrappedint.h"

void SmartPathAnimator::removeNode(const int nodeId, const bool approx) {
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->getValue().actionRemoveNode(nodeId, approx);
    }
    baseValue().actionRemoveNode(nodeId, approx);
}

void SmartPathAnimator::actionRemoveNode(const int nodeId,
                                         const bool approx)
{
    prp_pushUndoRedoName("Remove Node");

    prp_startTransform();

    const auto& keys = anim_getKeys();
    for (const auto &key : keys) { key->startValueTransform(); }
    removeNode(nodeId, approx);
    for (const auto &key : keys) { key->finishValueTransform(); }
    prp_finishTransform();

    if (baseValue().isEmpty()) { emit emptied(); }
    else { prp_afterWholeInfluenceRangeChanged(); }
}

int SmartPathAnimator::actionAddNewAtStart(const QPointF &relPos) {
    return actionAddNewAtStart({false, false, CtrlsMode::corner,
                                relPos, relPos, relPos});
}

int SmartPathAnimator::actionAddNewAtStart(const NormalNodeData &data)
{
    if (baseValue().getNodeCount() == 0) { return actionAddFirstNode(data); }

    prp_pushUndoRedoName(tr("Add New Node"));

    prp_startTransform();

    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->startValueTransform();
        spKey->getValue().actionPrependNode();
        spKey->finishValueTransform();
    }

    const int id = baseValue().actionPrependNode();
    getCurrentlyEdited()->actionSetNormalNodeValues(id, data);
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
    return id;
}

int SmartPathAnimator::actionAddNewAtEnd(const QPointF &relPos) {
    return actionAddNewAtEnd({false, false, CtrlsMode::corner,
                              relPos, relPos, relPos});
}

int SmartPathAnimator::actionAddNewAtEnd(const NormalNodeData &data)
{
    if (baseValue().getNodeCount() == 0) { return actionAddFirstNode(data); }

    prp_pushUndoRedoName(tr("Add New Node"));

    prp_startTransform();

    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->startValueTransform();
        spKey->getValue().actionAppendNodeAtEndNode();
        spKey->finishValueTransform();
    }

    const int id = baseValue().actionAppendNodeAtEndNode();
    getCurrentlyEdited()->actionSetNormalNodeValues(id, data);
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
    return id;
}

int SmartPathAnimator::actionInsertNodeBetween(const int node1Id,
                                               const int node2Id,
                                               const qreal t,
                                               const NodePointValues& vals)
{
    prp_pushUndoRedoName(tr("Add New Node"));

    prp_startTransform();
    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->startValueTransform();
        spKey->getValue().actionInsertNodeBetween(node1Id, node2Id, t);
        spKey->finishValueTransform();
    }

    const auto curr = getCurrentlyEdited();
    if (&baseValue() != curr) {
        baseValue().actionInsertNodeBetween(node1Id, node2Id, t);
    }
    const int id = curr->actionInsertNodeBetween(node1Id, node2Id, vals);

    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
    return id;
}

int SmartPathAnimator::insertNodeBetween(
        const int node1Id, const int node2Id, const qreal t) {
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->getValue().actionInsertNodeBetween(node1Id, node2Id, t);
    }

    const auto curr = getCurrentlyEdited();
    if(curr->getNodePtr(node1Id)->getCtrlsMode() == CtrlsMode::symmetric) {
        curr->actionSetNormalNodeCtrlsMode(node1Id, CtrlsMode::smooth);
    }
    if(curr->getNodePtr(node2Id)->getCtrlsMode() == CtrlsMode::symmetric) {
        curr->actionSetNormalNodeCtrlsMode(node2Id, CtrlsMode::smooth);
    }
    const int id = baseValue().actionInsertNodeBetween(node1Id, node2Id, t);
    curr->actionPromoteDissolvedNodeToNormal(id);
    return id;
}

int SmartPathAnimator::actionInsertNodeBetween(const int node1Id,
                                               const int node2Id,
                                               const qreal t)
{
    prp_pushUndoRedoName(tr("Add New Node"));

    prp_startTransform();
    const auto& keys = anim_getKeys();
    for (const auto &key : keys) { key->startValueTransform(); }
    const int id = insertNodeBetween(node1Id, node2Id, t);
    for (const auto &key : keys) { key->finishValueTransform(); }
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
    return id;
}

void SmartPathAnimator::connectNodes(
        const int node1Id, const int node2Id) {
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->getValue().actionConnectNodes(node1Id, node2Id);
    }

    baseValue().actionConnectNodes(node1Id, node2Id);
}

void SmartPathAnimator::actionConnectNodes(const int node1Id,
                                           const int node2Id)
{
    prp_pushUndoRedoName(tr("Connect Nodes"));

    prp_startTransform();
    const auto& keys = anim_getKeys();
    for (const auto &key : keys) { key->startValueTransform(); }

    connectNodes(node1Id, node2Id);

    for (const auto &key : keys) { key->finishValueTransform(); }

    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
}

void SmartPathAnimator::actionMergeNodes(const int node1Id,
                                         const int node2Id)
{
    prp_pushUndoRedoName(tr("Merge Nodes"));

    prp_startTransform();
    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->startValueTransform();
        spKey->getValue().actionMergeNodes(node1Id, node2Id);
        spKey->finishValueTransform();
    }

    baseValue().actionMergeNodes(node1Id, node2Id);
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
}

void SmartPathAnimator::actionMoveNodeBetween(const int nodeId,
                                              const int prevNodeId,
                                              const int nextNodeId)
{
    prp_pushUndoRedoName(tr("Move Node"));
    prp_startTransform();
    getCurrentlyEdited()->actionMoveNodeBetween(
                nodeId, prevNodeId, nextNodeId);
    prp_finishTransform();
    changed();
}

void SmartPathAnimator::close() {
    connectNodes(0, baseValue().getNodeCount() - 1);
}

void SmartPathAnimator::actionClose() {
    actionConnectNodes(0, baseValue().getNodeCount() - 1);
}

void SmartPathAnimator::actionDisconnectNodes(const int node1Id,
                                              const int node2Id)
{
    prp_pushUndoRedoName(tr("Disconnect Nodes"));

    prp_startTransform();
    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        auto& keyPath = spKey->getValue();
        key->startValueTransform();
        keyPath.actionDisconnectNodes(node1Id, node2Id);
        key->finishValueTransform();
    }

    baseValue().actionDisconnectNodes(node1Id, node2Id);
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
}

void SmartPathAnimator::actionReverseCurrent()
{
    prp_pushUndoRedoName(tr("Reverse Nodes"));

    prp_startTransform();
    getCurrentlyEdited()->actionReversePath();
    prp_finishTransform();
    changed();
}

void SmartPathAnimator::actionReverseAll()
{
    prp_pushUndoRedoName(tr("Reverse Nodes"));

    prp_startTransform();

    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        key->startValueTransform();
        spKey->getValue().actionReversePath();
        key->finishValueTransform();
    }

    baseValue().actionReversePath();
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
}

void SmartPathAnimator::actionAppendMoveAllFrom(SmartPathAnimator * const other)
{
    prp_pushUndoRedoName(tr("Connect Paths"));

    prp_startTransform();

    anim_coordinateKeysWith(other);
    const auto& keys = anim_getKeys();
    for (int i = 0; i < keys.count(); i++) {
        const auto thisKey = anim_getKeyAtIndex<SmartPathKey>(i);
        const auto otherKey = other->anim_getKeyAtIndex<SmartPathKey>(i);

        thisKey->startValueTransform();
        thisKey->getValue().actionAppendMoveAllFrom(std::move(otherKey->getValue()));
        thisKey->finishValueTransform();
    }

    baseValue().actionAppendMoveAllFrom(std::move(other->baseValue()));
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
    other->prp_afterWholeInfluenceRangeChanged();
}

void SmartPathAnimator::actionPrependMoveAllFrom(SmartPathAnimator * const other)
{
    prp_pushUndoRedoName(tr("Connect Paths"));

    prp_startTransform();

    anim_coordinateKeysWith(other);
    const auto& keys = anim_getKeys();
    for (int i = 0; i < keys.count(); i++) {
        const auto thisKey = anim_getKeyAtIndex<SmartPathKey>(i);
        const auto otherKey = other->anim_getKeyAtIndex<SmartPathKey>(i);
        thisKey->startValueTransform();
        thisKey->getValue().actionPrependMoveAllFrom(std::move(otherKey->getValue()));
        thisKey->finishValueTransform();
    }

    baseValue().actionPrependMoveAllFrom(std::move(other->baseValue()));
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
    other->prp_afterWholeInfluenceRangeChanged();
}

QVector<int> replaceNodeIds(const int count1, const int count2) {
    const int min = qMin(count1, count2);
    const int max = qMax(count1, count2);
    QVector<int> result;
    result.reserve(min);
    for(int i = 0; i < min; i++) {
        const qreal t = (1. + i)/(min + 1.);
        qreal targetF = (max - 1)*t;
        int target;
        if(isInteger4Dec(targetF) || isZero4Dec(t - 0.5)) {
            target = qRound(targetF);
        } else if(t < 0.5) {
            target = qFloor(targetF);
        } else {
            target = qCeil(targetF);
        }
        result << target;
    }
    return result;
}

void SmartPathAnimator::actionReplaceSegments(
        int beginNodeId, int endNodeId,
        const QList<qCubicSegment2D>& with) {
    const auto edit = getCurrentlyEdited();
    const auto forward = edit->mid(beginNodeId, endNodeId);
    const auto backward = edit->mid(endNodeId, beginNodeId);

    CubicList withSegs(with);

    const auto calcAvgDist = [&withSegs](const NodeList& to) {
        QList<qCubicSegment2D> oldSegList;
        for(int i = 0; i < to.count() - 1; i++) {
            const auto iNode = to.at(i);
            const auto nextNode = to.at(i + 1);
            oldSegList << qCubicSegment2D{iNode->p1(), iNode->c2(),
                                          nextNode->c0(), nextNode->p1()};
        }
        CubicList oldSegs(oldSegList);
        qreal sum = 0;
        int count = 0;
        const qreal len1 = withSegs.getTotalLength();
        const qreal len2 = oldSegs.getTotalLength();
        for(qreal t1 = 0.1; t1 < 0.95; t1 += 0.1) {
            const QPointF pos = withSegs.posAtLength(t1*len1);
            qreal minDist = 100000;
            for(qreal t2 = 0; t2 < 1.01; t2 += 0.05) {
                const QPointF oldPos = oldSegs.posAtLength(t2*len2);
                const qreal dist = pointToLen(oldPos - pos);
                if(dist < minDist) minDist = dist;
            }
            sum += minDist;
            count++;
        }
        for(qreal t2 = 0.1; t2 < 0.95; t2 += 0.1) {
            const QPointF oldPos = oldSegs.posAtLength(t2*len2);
            qreal minDist = 100000;
            for(qreal t1 = 0; t1 < 1.01; t1 += 0.05) {
                const QPointF pos = withSegs.posAtLength(t1*len1);
                const qreal dist = pointToLen(oldPos - pos);
                if(dist < minDist) minDist = dist;
            }
            sum += minDist;
            count++;
        }

        if(count == 0) return 0.;
        return sum/count;
    };
    const qreal forwardDist = calcAvgDist(forward);
    const qreal backwardDist = calcAvgDist(backward);
    bool reverse;
    const bool sameDist = isZero4Dec(forwardDist - backwardDist);
    if(sameDist) {
        const bool bConnect = beginNodeId == 0 &&
                              endNodeId == edit->getNodeCount() - 1;
        const bool eConnect = beginNodeId == edit->getNodeCount() - 1 &&
                              endNodeId == 0;
        reverse = bConnect || (!eConnect && beginNodeId > endNodeId);
    } else {
        reverse = forwardDist > backwardDist;
    }
    return actionReplaceSegments(beginNodeId, endNodeId, with, reverse);
}

void SmartPathAnimator::actionReplaceSegments(int beginNodeId,
                                              int endNodeId,
                                              const QList<qCubicSegment2D>& with,
                                              const bool reverse)
{
    if (with.isEmpty() || beginNodeId == endNodeId) { return; }

    prp_pushUndoRedoName(tr("Replace Nodes"));

    prp_startTransform();

    const auto edit = getCurrentlyEdited();
    const bool close = !isClosed() && ((endNodeId < beginNodeId && !reverse) ||
                                       (beginNodeId < endNodeId && reverse));
    int totalCount = edit->getNodeCount();
    const int oldCount = WrappedInt::sCount(beginNodeId, endNodeId,
                                            totalCount, reverse) - 2;
    const int newCount = with.count() - 1;
    const bool changeAll = newCount != oldCount;

    const auto replaceIds = replaceNodeIds(oldCount, newCount);

    const auto& keys = anim_getKeys();
    if (changeAll) {
        for (const auto &key : keys) { key->startValueTransform(); }
    }

    if (close) { this->close(); }

    const auto& firstSeg = with.first();
    const auto& lastSeg = with.last();
    edit->actionSetNormalNodeCtrlsMode(beginNodeId, CtrlsMode::corner);
    edit->actionSetNormalNodeCtrlsMode(endNodeId, CtrlsMode::corner);
    if (reverse) {
        edit->actionSetNormalNodeC0(beginNodeId, firstSeg.c1());
        edit->actionSetNormalNodeC2(endNodeId, lastSeg.c2());
    } else {
        edit->actionSetNormalNodeC2(beginNodeId, firstSeg.c1());
        edit->actionSetNormalNodeC0(endNodeId, lastSeg.c2());
    }

    const auto edited = getCurrentlyEdited();

    const int iInc = reverse ? -1 : 1;
    for (WrappedInt i(beginNodeId + iInc, totalCount, reverse); i != endNodeId; i++) {
        const auto iNode = edited->getNodePtr(i.toInt());
        if (iNode->isDissolved()) {
            edited->actionPromoteDissolvedNodeToNormal(i.toInt());
        }
    }

    if (newCount < oldCount) {
        const bool remove = !anim_hasKeys();
        if (remove) {
            const int removeCount = oldCount - newCount;
            for (int i = 0; i < removeCount; i++) {
                const int idValue = beginNodeId + iInc;
                const int nodeId = WrappedInt(idValue, totalCount, reverse).toInt();
                removeNode(nodeId, false);
                totalCount--;
                if (beginNodeId > nodeId) { beginNodeId--; }
                if (endNodeId > nodeId)  { endNodeId--; }
            }
        } else {
            QVector<int> removeIds;
            for (int i = 0; i < oldCount; i++) {
                if(replaceIds.contains(i)) continue;
                removeIds << i;
            }
            for (int relId : removeIds) {
                const int idValue = beginNodeId + iInc + iInc*relId;
                const int nodeId = WrappedInt(idValue, totalCount, reverse).toInt();
                edited->actionDemoteToDissolved(nodeId, false);
            }
        }
    } else if (newCount > oldCount) {
        QVector<int> insertIds;
        for (int i = 0; i < newCount; i++) {
            if (replaceIds.contains(i)) { continue; }
            insertIds << i;
        }
        QVector<qreal> insertTs;
        insertTs.reserve(insertIds.count());
        for (int i = 0; i < insertIds.count(); i++) {
            int count = 1;
            int prevJ = i;
            for (int j = i + 1; j < insertIds.count(); j++) {
                if (j == prevJ + 1) { count++; }
                else { break; }
                prevJ = j;
            }
            qreal lastAbs = 0;
            for (int k = 0; k < count; k++) {
                const qreal abs = qreal(k + 1)/(count + 1);
                qreal t = gMapTToFragment(lastAbs, 1, abs);
                if (reverse) { t = 1 - t; }
                insertTs << t;
                lastAbs = abs;
            }
            i += count - 1;
        }

        std::sort(insertIds.begin(), insertIds.end());
        const bool twoEndNodes = (beginNodeId == 0 && reverse) ||
                                 (endNodeId == 0 && beginNodeId == totalCount - 1 && !reverse);
        for (int i = 0; i < insertIds.count(); i++) {
            const int insertId = insertIds.at(i);
            const qreal insertT = insertTs.at(i);
            const int prevId = WrappedInt(beginNodeId + iInc*insertId,
                                          totalCount, reverse).toInt();
            const int nextId = WrappedInt(prevId + iInc,
                                          totalCount, reverse).toInt();
            int orderedPrevId = qMin(prevId, nextId);
            int orderedNextId = qMax(prevId, nextId);
            if (twoEndNodes) { std::swap(orderedPrevId, orderedNextId); }

            insertNodeBetween(orderedPrevId, orderedNextId, insertT);
            edited->actionPromoteDissolvedNodeToNormal(nextId);
            totalCount++;
            if (beginNodeId > orderedPrevId) { beginNodeId++; }
            if (endNodeId > orderedPrevId) { endNodeId++; }
        }
    }

    int skipped = 0;
    for (int i = 0; i < newCount + skipped; i++) {
        const int nodeId = WrappedInt(beginNodeId + iInc + iInc*i,
                                      totalCount, reverse).toInt();
        const auto node = edited->getNodePtr(nodeId);
        if (node->isDissolved()) {
            skipped++;
            continue;
        }
        const int segId = i - skipped;
        const auto& seg = with.at(segId);
        const auto& nextSeg = with.at(segId + 1);
        const QPointF c0 = reverse ? nextSeg.c1() : seg.c2();
        const QPointF p1 = seg.p3();
        const QPointF c2 = reverse ? seg.c2() : nextSeg.c1();

        const CtrlsMode ctrlsMode = gGuessCtrlsMode(c0, p1, c2, true, true);
        const NormalNodeData values(true, true, ctrlsMode, c0, p1, c2);

        edited->actionSetNormalNodeValues(nodeId, values);
    }

    if (changeAll) {
        for (const auto &key : keys) { key->finishValueTransform(); }
    }
    prp_finishTransform();

    if (changeAll) { prp_afterWholeInfluenceRangeChanged(); }
    else { changed(); }
}

int SmartPathAnimator::actionAddFirstNode(const QPointF &relPos) {
    return actionAddFirstNode({false, false, CtrlsMode::symmetric,
                               relPos, relPos, relPos});
}

int SmartPathAnimator::actionAddFirstNode(const NormalNodeData &data)
{
    prp_pushUndoRedoName(tr("Add New Node"));

    prp_startTransform();

    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        key->startValueTransform();
        spKey->getValue().actionAddFirstNode(data);
        key->finishValueTransform();
    }

    const int id = baseValue().actionAddFirstNode(data);
    prp_finishTransform();

    prp_afterWholeInfluenceRangeChanged();
    return id;
}
