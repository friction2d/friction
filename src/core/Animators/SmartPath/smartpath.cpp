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

#include "smartpath.h"

#include "XML/xmlexporthelpers.h"
#include "appsupport.h"

SmartPath::SmartPath(const SkPath &path) {
    setPath(path);
}

SmartPath::SmartPath(const NodeList &path) : mNodesList(path) {}

void SmartPath::actionOpen() {
    if(!isClosed()) return;
    mNodesList.splitNodeAndDisconnect(getNodeCount() - 1);
}

void SmartPath::actionRemoveNode(const int nodeId, const bool approx) {
    mNodesList.removeNode(nodeId, approx);
}

int SmartPath::actionAddFirstNode(const QPointF &c0,
                                  const QPointF &p1,
                                  const QPointF &c2) {
    const int insertId = mNodesList.appendNode(Node(c0, p1, c2));
    return insertId;
}

int SmartPath::actionAddFirstNode(const NormalNodeData& data) {
    const int insertId = mNodesList.appendNode(Node(data));
    return insertId;
}

int SmartPath::actionPrependNode() {
    if(mNodesList.count() == 0) RuntimeThrow("Cannot prepend new node ");
    Node * const endNode = mNodesList.at(0);
    if(!endNode->isNormal())
        RuntimeThrow("Invalid node type. End nodes should always be NORMAL.");
    const NodePointValues vals = {endNode->p1(), endNode->p1(), endNode->p1()};
    return actionPrependNode(vals);
}

int SmartPath::actionPrependNode(const NodePointValues &values) {
    return mNodesList.prependNode(Node(values.fC0, values.fP1, values.fC2));
}

int SmartPath::actionAppendNodeAtEndNode(const int endNodeId) {
    Node * const endNode = mNodesList.at(endNodeId);
    if(!endNode->isNormal())
        RuntimeThrow("Invalid node type. End nodes should always be NORMAL.");
    const NodePointValues vals = {endNode->p1(), endNode->p1(), endNode->p1()};
    return actionAppendNodeAtEndNode(vals);
}

int SmartPath::actionAppendNodeAtEndNode(const NodePointValues &values) {
    return mNodesList.appendNode(Node(values.fC0, values.fP1, values.fC2));
}

int SmartPath::actionAppendNodeAtEndNode() {
    if(mNodesList.count() == 0) RuntimeThrow("Cannot append new node ");
    return actionAppendNodeAtEndNode(mNodesList.count() - 1);
}

int SmartPath::insertNodeBetween(const int prevId,
                                 const int nextId,
                                 const Node& nodeBlueprint) {
    if(!mNodesList.nodesConnected(prevId, nextId))
        RuntimeThrow("Cannot insert between not connected nodes");
    return mNodesList.insertNodeAfter(prevId, nodeBlueprint);
}

int SmartPath::actionInsertNodeBetween(const int prevId,
                                       const int nextId,
                                       const qreal t) {
    if(prevId == nextId)
        RuntimeThrow("Cannot insert a node between a single node");

    const auto prev = getNodePtr(prevId);
    const auto next = getNodePtr(nextId);

    if(!mNodesList.nodesConnected(prevId, nextId)) {
        const int prevNormalId = prev->getNodeId();
        const int nextNormalId = next->getNodeId();
        const int nDiss = prevNormalId < nextNormalId ?
                    nextNormalId - prevNormalId - 1 :
                    mNodesList.count() - 1 - prevNormalId + nextNormalId;
        const Node * iPrevNode = prev;

        for(int i = 0; i < nDiss; i++) {
            const auto iCurrNode = mNodesList.nextNode(iPrevNode);
            if(iCurrNode->t() > t) {
                return insertNodeBetween(iPrevNode->getNodeId(),
                                         iCurrNode->getNodeId(), Node(t));
            }
            iPrevNode = iCurrNode;
        }
        return insertNodeBetween(prevNodeId(nextId), nextId, Node(t));
    }

    if(prev->isDissolved() || next->isDissolved()) {
        const qreal prevT = prev->isNormal() ? 0 : prev->t();
        const qreal nextT = next->isNormal() ? 1 : next->t();

        const qreal mappedT = gMapTFromFragment(prevT, nextT, t);
        return insertNodeBetween(prevId, nextId, Node(mappedT));
    }
    return insertNodeBetween(prevId, nextId, Node(t));
}

int SmartPath::actionInsertNodeBetween(
        const int prevId, const int nextId,
        const QPointF &c0, const QPointF &p1, const QPointF &c2) {
    return insertNodeBetween(prevId, nextId, Node(c0, p1, c2));
}

int SmartPath::actionInsertNodeBetween(const int prevId, const int nextId,
                                       const NodePointValues& vals) {
    return actionInsertNodeBetween(prevId, nextId, vals.fC0, vals.fP1, vals.fC2);
}

void SmartPath::actionPromoteDissolvedNodeToNormal(const int nodeId) {
    mNodesList.promoteDissolvedNodeToNormal(nodeId);
}

void SmartPath::actionDemoteToDissolved(const int nodeId, const bool approx) {
    mNodesList.demoteNormalNodeToDissolved(nodeId, approx);
}

void SmartPath::actionMoveNodeBetween(const int movedNodeId,
                                      const int prevNodeId,
                                      const int nextNodeId) {
    if(!mNodesList.nodesConnected(prevNodeId, nextNodeId))
        RuntimeThrow("Trying to move between not connected nodes");
    const int targetId = (movedNodeId < prevNodeId ? prevNodeId : nextNodeId);
    mNodesList.moveNode(movedNodeId, targetId);
}

void SmartPath::actionDisconnectNodes(const int node1Id, const int node2Id) {
    int prevId;
    int nextId;
    if(nextNodeId(node1Id) == node2Id) {
        prevId = node1Id;
        nextId = node2Id;
    } else if(nextNodeId(node2Id) == node1Id) {
        prevId = node2Id;
        nextId = node1Id;
    } else {
        RuntimeThrow("Trying to disconnect not connected nodes");
    }

    Node * const prevNode = mNodesList.at(prevId);
    Node * const nextNode = mNodesList.at(nextId);

    if(prevNode->isDissolved())
        actionPromoteDissolvedNodeToNormal(prevId);
    if(nextNode->isDissolved())
        actionPromoteDissolvedNodeToNormal(nextId);

    if(isClosed()) {
        mNodesList.moveNodesToFrontStartingWith(nextId);
        mNodesList.setClosed(false);
    } else {
        mLastDetached = mNodesList.detachNodesStartingWith(nextId);
    }
}

void SmartPath::actionConnectNodes(const int node1Id, const int node2Id) {
    if((node1Id == 0 && node2Id == mNodesList.count() - 1) ||
       (node2Id == 0 && node1Id == mNodesList.count() - 1)) {
        mNodesList.setClosed(true);
    } else return;
}

void SmartPath::actionSetDissolvedNodeT(const int nodeId, const qreal t) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isDissolved()) return;
    node->setT(t);
    updateDissolvedNodePosition(nodeId, node);
}

void SmartPath::actionSetNormalNodeValues(
        const int nodeId, const QPointF &c0,
        const QPointF &p1, const QPointF &c2) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    node->setC0(c0);
    node->setP1(p1);
    node->setC2(c2);
}

void SmartPath::actionSetNormalNodeValues(const int nodeId,
                                          const NormalNodeData &data) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    node->setNormalData(data);
}

void SmartPath::actionSetNormalNodeP1(const int nodeId, const QPointF &p1) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    node->setP1(p1);
}

void SmartPath::actionSetNormalNodeC0(const int nodeId, const QPointF &c0) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    node->setC0(c0);
}

void SmartPath::actionSetNormalNodeC2(const int nodeId, const QPointF &c2) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    node->setC2(c2);
}

void SmartPath::actionSetNormalNodeCtrlsMode(const int nodeId, const CtrlsMode mode) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    mNodesList.setNodeCtrlsMode(node, mode);
}

void SmartPath::actionSetNormalNodeC0Enabled(const int nodeId, const bool enabled) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    mNodesList.setNodeC0Enabled(node, enabled);
}

void SmartPath::actionSetNormalNodeC2Enabled(const int nodeId, const bool enabled) {
    Node * const node = mNodesList.at(nodeId);
    if(!node->isNormal()) return;
    mNodesList.setNodeC2Enabled(node, enabled);
}

void SmartPath::actionSetFirstNode(const int nodeId)
{
    if (nodeId <= 0) { return; }
    if (nodeId >= mNodesList.count()) { return; }
    mNodesList.moveNodesToFrontStartingWith(nodeId);
}

void SmartPath::actionReversePath() {
    mNodesList.reverse();
}

void SmartPath::actionAppendMoveAllFrom(SmartPath &&other) {
    mNodesList.append(std::move(other.mNodesList));
    other.clear();
}

void SmartPath::actionPrependMoveAllFrom(SmartPath &&other) {
    mNodesList.prepend(std::move(other.mNodesList));
    other.clear();
}

void SmartPath::actionMergeNodes(const int node1Id, const int node2Id) {
    mNodesList.mergeNodes(node1Id, node2Id);
}

void SmartPath::reset() {
    mNodesList.clear();
    mLastDetached.clear();
}

void SmartPath::clear() {
    reset();
}

SkPath SmartPath::getPathAt() const {
    return mNodesList.toSkPath();
}

void SmartPath::setPath(const SkPath &path) {
    mNodesList.setPath(path);
}

const Node *SmartPath::getNodePtr(const int id) const {
    if(id < 0) return nullptr;
    if(id >= mNodesList.count()) return nullptr;
    return mNodesList[id];
}

int SmartPath::prevNodeId(const int nodeId) const {
    const auto node = mNodesList.prevNode(nodeId);
    if(node) return node->getNodeId();
    return -1;
}

int SmartPath::nextNodeId(const int nodeId) const {
    const auto node = mNodesList.nextNode(nodeId);
    if(node) return node->getNodeId();
    return -1;
}

int SmartPath::prevNormalId(const int nodeId) const {
    const auto node = mNodesList.prevNormal(nodeId);
    if(node) return node->getNodeId();
    return -1;
}

int SmartPath::nextNormalId(const int nodeId) const {
    const auto node = mNodesList.nextNormal(nodeId);
    if(node) return node->getNodeId();
    return -1;
}

qValueRange SmartPath::dissolvedTRange(const int nodeId) {
    return {mNodesList.prevT(nodeId), mNodesList.nextT(nodeId)};
}

void SmartPath::updateDissolvedNodePosition(const int nodeId) {
    mNodesList.updateDissolvedNodePosition(nodeId);
}

void SmartPath::updateDissolvedNodePosition(const int nodeId, Node * const node) {
    mNodesList.updateDissolvedNodePosition(nodeId, node);
}

void SmartPath::addDissolvedNodes(const int add) {
    Q_ASSERT(getNodeCount() >= 2);
    const int n0 = getNodeCount() - 2;
    const int n1 = n0 + 1;
    for(int i = 0; i < add; i++)
        actionInsertNodeBetween(n0, n1, 0.5);
}

void SmartPath::sInterpolate(const SmartPath &path1, const SmartPath &path2,
                             const qreal path2Weight, SmartPath &target) {
    target = NodeList::sInterpolate(path1.getNodesRef(),
                                    path2.getNodesRef(),
                                    path2Weight);
}

NodeList SmartPath::getAndClearLastDetached() {
    NodeList detached;
    mLastDetached.swap(detached);
    return detached;
}

QString SmartPath::toXEV() const {
    QString result;
    const QString blueprint = QStringLiteral("%1 %2 %3 %4 %5 %6 %7");
    for(const auto& node : mNodesList) {
        if(!result.isEmpty()) result += ',';
        if(node->isDissolved()) {
            result += QString::number(node->t());
        } else {
            const auto c0 = node->c0();
            const auto p1 = node->p1();
            const auto c2 = node->c2();

            const QString c0x = node->getC0Enabled() ?
                                QString::number(c0.x()) : "*";
            const QString c0y = node->getC0Enabled() ?
                                QString::number(c0.y()) : "*";

            const QString c2x = node->getC0Enabled() ?
                                QString::number(c2.x()) : "*";
            const QString c2y = node->getC0Enabled() ?
                                QString::number(c2.y()) : "*";

            const int ctrlMode = static_cast<int>(node->getCtrlsMode());

            result += blueprint.arg(c0x, c0y).
                                arg(p1.x()).arg(p1.y()).
                                arg(c2x, c2y).arg(ctrlMode);
        }
    }
    return result;
}

void SmartPath::loadXEV(const QStringRef& xev) {
    ListOfNodes listOfNodes;

    const auto nodes = xev.split(',', Qt::SkipEmptyParts);
    for(const auto& node : nodes) {
        const auto values = node.split(' ', Qt::SkipEmptyParts);
        if(values.count() == 1) {
            const qreal t = XmlExportHelpers::stringToDouble(values[0]);
            listOfNodes.append(Node(t));
        } else if(values.count() == 7) {
            const qreal p1x = XmlExportHelpers::stringToDouble(values[2]);
            const qreal p1y = XmlExportHelpers::stringToDouble(values[3]);

            const bool c0xEnabled = values[0] != '*';
            const bool c0yEnabled = values[1] != '*';
            const qreal c0x = c0xEnabled ? XmlExportHelpers::stringToDouble(values[0]) : p1x;
            const qreal c0y = c0yEnabled ? XmlExportHelpers::stringToDouble(values[1]) : p1y;

            const bool c2xEnabled = values[4] != '*';
            const bool c2yEnabled = values[5] != '*';
            const qreal c2x = c2xEnabled ? XmlExportHelpers::stringToDouble(values[4]) : p1x;
            const qreal c2y = c2yEnabled ? XmlExportHelpers::stringToDouble(values[5]) : p1y;

            bool ok;
            const int ctrlMode = values[6].toInt(&ok);
            ok = ok && qBound(0, ctrlMode, 2) == ctrlMode;
            if(!ok) RuntimeThrow("Invalid node control mode " + values[6].toString());

            Node node({c0x, c0y}, {p1x, p1y}, {c2x, c2y});
            node.setC0Enabled(c0xEnabled || c0yEnabled);
            node.setC2Enabled(c2xEnabled || c2yEnabled);
            node.assignCtrlsMode(static_cast<CtrlsMode>(ctrlMode));
            listOfNodes.append(node);
        } else RuntimeThrow("Invalid node values " + node.toString());
    }

    mNodesList = NodeList(std::move(listOfNodes));
}

bool SmartPath::isClockwise() const {
    if(mNodesList.isEmpty()) return false;
    QPointF prevPos = mNodesList.at(0)->p1();
    qreal sum = 0;
    const auto lineTo = [&prevPos, &sum](const QPointF& pos) {
        sum += (pos.x() - prevPos.x()) * (pos.y() + prevPos.y());
        prevPos = pos;
    };
    const int nNodes = mNodesList.count();
    for(int i = 0; i < nNodes; i++) {
        const auto node = mNodesList.at(i);
        if(node->isDissolved()) continue;
        if(i != 0 || isClosed()) lineTo(node->c0());
        lineTo(node->p1());
        if(i != nNodes - 1 || isClosed()) lineTo(node->c2());
    }
    return sum > 0;
}

NodeList SmartPath::mid(const int first, const int last) const {
    return mNodesList.mid(first, last);
}

eWriteStream &operator<<(eWriteStream &dst, const SmartPath &path) {
    path.write(dst);
    return dst;
}

eReadStream &operator>>(eReadStream &src, SmartPath &path) {
    path.read(src);
    return src;
}
