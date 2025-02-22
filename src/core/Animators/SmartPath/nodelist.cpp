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

#include "nodelist.h"
#include "skia/skiaincludes.h"
#include "pointhelpers.h"
#include "exceptions.h"
#include "smartPointers/ememory.h"
#include "wrappedint.h"

NodeList::NodeList(ListOfNodes &&other) : mNodes(std::move(other)) {}

NodeList::NodeList(const NodeList &other) :
    mNodes(other.mNodes), mClosed(other.mClosed) {}

qCubicSegment2D gSegmentFromNodes(const Node& prevNode,
                                  const Node& nextNode) {
    return qCubicSegment2D(prevNode.p1(), prevNode.c2(),
                           nextNode.c0(), nextNode.p1());
}

void NodeList::moveNode(const int fromId, const int toId) {
    mNodes.moveNode(fromId, toId);
    Node * const node = at(toId);
    if(node->isDissolved()) {
        const qreal prevTV = prevT(toId);
        const qreal tFrag = nextT(toId) - prevTV;
        node->setT(0.5*tFrag + prevTV);
    }
}

void NodeList::updateDissolvedNodePosition(const int nodeId) {
    updateDissolvedNodePosition(nodeId, at(nodeId));
}

void NodeList::updateDissolvedNodePosition(const int nodeId,
                                           Node * const node) {
    if(node->isNormal()) RuntimeThrow("Unsupported node type");
    const Node * const prevNode = prevNormal(nodeId);
    const Node * const nextNode = nextNormal(nodeId);
    const auto normalSeg = gSegmentFromNodes(*prevNode, *nextNode);
    node->mP1 = normalSeg.posAtT(node->mT);
}

bool NodeList::read(eReadStream& src) {
    int nNodes;
    src >> nNodes;
    mNodes.clear();
    for(int i = 0; i < nNodes; i++) {
        Node node;
        src.read(&node, sizeof(Node));
        mNodes.append(node);
    }
    src >> mClosed;
    return true;
}

bool NodeList::write(eWriteStream& dst) const {
    const int nNodes = mNodes.count();
    dst << nNodes;
    for(const auto& node : mNodes) {
        dst.write(node.get(), sizeof(Node));
    }
    dst << mClosed;
    return true;
}

void NodeList::normalize() {
    const int iMax = count();
    for(int i = 0; i < iMax; i++) {
        const auto node = at(i);
        if(node->isDissolved()) promoteDissolvedNodeToNormal(i);
    }
}

NodeList NodeList::normalized() const {
    NodeList result = *this;
    result.normalize();
    return result;
}

NodeList NodeList::mid(int first, int last) const {
    first = WrappedInt(first, count(), false).toInt();
    last = WrappedInt(last, count(), false).toInt();
    if(first == last) return *this;
    NodeList result;
    for(WrappedInt i(first, count(), false);; i++) {
        const int id = i.toInt();
        result.appendNode(*at(id));
        if(id == last) break;
    }

    return result;
}

void NodeList::append(NodeList &&other) {
    mNodes.appendNodes(std::move(other.mNodes));
}

void NodeList::prepend(NodeList &&other) {
    mNodes.prependNodes(std::move(other.mNodes));
}

void NodeList::moveNodesToFrontStartingWith(const int first) {
    mNodes.moveNodesToFrontStartingWith(first);
}

NodeList NodeList::detachNodesStartingWith(const int first) {
    return NodeList(mNodes.detachNodesStartingWith(first));
}

void NodeList::swap(NodeList &other) {
    mNodes.swap(other.getList());
    const bool wasClosed = mClosed;
    mClosed = other.isClosed();
    other.setClosed(wasClosed);
}

void NodeList::removeNodeFromList(const int nodeId) {
    mNodes.removeAt(nodeId);
}

Node *NodeList::insertNodeToList(const int nodeId, const Node &node) {
    if(nodeId < 0 || nodeId > mNodes.count())
        RuntimeThrow("Wrong insert id");
    mNodes.insert(nodeId, node);
    Node * const insertedNode = mNodes[nodeId];
    return insertedNode;
}

void NodeList::reset() {
    mClosed = false;
    mNodes.clear();
}

int NodeList::insertNodeBefore(const int nextId,
                               const Node& nodeBlueprint) {
    const int insertId = nextId;
    Node * const insertedNode = insertNodeToList(insertId, nodeBlueprint);
    if(nodeBlueprint.isDissolved())
        promoteDissolvedNodeToNormal(insertId, insertedNode);
    return insertId;
}

int NodeList::insertNodeAfter(const int prevId,
                              const Node& nodeBlueprint) {
    const int insertId = prevId + 1;
//    Node * const insertedNode =
   /* const auto node = */insertNodeToList(insertId, nodeBlueprint);
//    if(nodeBlueprint.isDissolved())
//        updateDissolvedNodePosition(insertId, node);
//    if(nodeBlueprint.isDissolved())
//        promoteDissolvedNodeToNormal(insertId, insertedNode);
    return insertId;
}

int NodeList::prependNode(const Node &nodeBlueprint) {
    const int insertId = 0;
    insertNodeToList(insertId, nodeBlueprint);
    return insertId;
}

int NodeList::appendNode(const Node &nodeBlueprint) {
    const int insertId = mNodes.count();
    insertNodeToList(insertId, nodeBlueprint);
    return insertId;
}

Node *NodeList::appendAndGetNode(const Node &nodeBlueprint) {
    return mNodes[appendNode(nodeBlueprint)];
}

void NodeList::approximateBeforeDemoteOrRemoval(
        const qreal nodeT,
        Node * const node,
        Node * const prevNormalV,
        Node * const nextNormalV) {
    if(!node || !prevNormalV || !nextNormalV) return;
    if(!prevNormalV->getC2Enabled()) prevNormalV->setC2Enabled(true);
    if(!nextNormalV->getC0Enabled()) nextNormalV->setC0Enabled(true);
    prevNormalV->assignCtrlsMode(CtrlsMode::corner);
    nextNormalV->assignCtrlsMode(CtrlsMode::corner);

    const auto prevSeg = gSegmentFromNodes(*prevNormalV, *node);
    const QPointF midPrevPt = prevSeg.posAtT(0.5);

    const auto nextSeg = gSegmentFromNodes(*node, *nextNormalV);
    const QPointF midNextPt = nextSeg.posAtT(0.5);

    auto seg = gSegmentFromNodes(*prevNormalV, *nextNormalV);
    seg.makePassThroughRel(node->p1(), nodeT);
    seg.makePassThroughRel(midPrevPt, nodeT*0.5);
    seg.makePassThroughRel(midNextPt, nodeT + 0.5*(1 - nodeT));
    seg.makePassThroughRel(node->p1(), nodeT);

    prevNormalV->setC2(seg.c1());
    nextNormalV->setC0(seg.c2());
}

void NodeList::removeNode(const int nodeId, const bool approx) {
    removeNode(nodeId, at(nodeId), approx);
}

void NodeList::removeNode(const int nodeId, Node * const node,
                          const bool approx) {
    if(node->isNormal()) {
        if(approx) {
            Node * const prevNormalV = prevNormal(nodeId);
            Node * const nextNormalV = nextNormal(nodeId);
            approximateBeforeDemoteOrRemoval(0.5, node,
                                             prevNormalV, nextNormalV);
        }

        Node * currNode = node;
        bool hasPrevNormal = false;
        bool hasPrevDissolved = false;
        while((currNode = prevNode(currNode))) {
            if(currNode->isNormal()) {
                hasPrevNormal = true;
                break;
            }
            if(currNode->isDissolved()) {
                hasPrevDissolved = true;
                currNode->setT(currNode->t()*0.5);
            }
        }
        currNode = node;
        bool hasNextNormal = false;
        bool hasNextDissolved = false;
        while((currNode = nextNode(currNode))) {
            if(currNode->isNormal()) {
                hasNextNormal = true;
                break;
            }
            if(currNode->isDissolved()) {
                hasNextDissolved = true;
                currNode->setT(currNode->t()*0.5 + 0.5);
            }
        }

        if(hasPrevDissolved && !hasNextNormal) {
            const auto pDiss = prevNode(node);
            promoteDissolvedNodeToNormal(pDiss);
        }

        if(hasNextDissolved && !hasPrevNormal) {
            const auto nDiss = nextNode(node);
            promoteDissolvedNodeToNormal(nDiss);
        }
    }
    removeNodeFromList(nodeId);
}

void NodeList::demoteNormalNodeToDissolved(const int nodeId,
                                           const bool approx) {
    demoteNormalNodeToDissolved(nodeId, at(nodeId), approx);
}

void NodeList::demoteNormalNodeToDissolved(const int nodeId,
                                           Node * const node,
                                           const bool approx) {
    if(node->isDissolved()) return;
    Node * const prevNormalV = prevNormal(nodeId);
    Node * const nextNormalV = nextNormal(nodeId);
    if(!prevNormalV || !nextNormalV) return;

    if(approx) {
        approximateBeforeDemoteOrRemoval(0.5, node,
                                         prevNormalV, nextNormalV);
    }

    setNodeType(node, NodeType::dissolved);

    const int prevNormalId = prevNormalV->getNodeId();
    const int nextNormalId = nextNormalV->getNodeId();
    const int nDiss = prevNormalId < nextNormalId ?
                nextNormalId - prevNormalId - 1 :
                count() - 1 - prevNormalId + nextNormalId;
    const qreal tInc = 1./(nDiss + 1);
    qreal iT = tInc;
    Node * iPrevNode = prevNormalV;
    for(int i = 0; i < nDiss; i++) {
        const auto iCurrNode = nextNode(iPrevNode);
        iCurrNode->setT(iT);
        iPrevNode = iCurrNode;
        iT += tInc;
    }

    updateDissolvedNodePosition(nodeId);
}

void NodeList::promoteDissolvedNodeToNormal(const int nodeId,
                                            Node * const node) {
    if(node->isNormal()) return;
    Node * const prevNormalV = prevNormal(nodeId);
    Node * const nextNormalV = nextNormal(nodeId);

    const auto seg = gSegmentFromNodes(*prevNormalV, *nextNormalV);
    const auto div = seg.dividedAtT(node->t());
    const auto& first = div.first;
    const auto& second = div.second;
    prevNormalV->setC2(first.c1());
    node->setC0(first.c2());
    node->setP1(first.p3());
    node->setC2(second.c1());
    nextNormalV->setC0(second.c2());
    setNodeType(node, NodeType::normal);
    const bool segmentNotLinear = prevNormalV->getC2Enabled() ||
                                  nextNormalV->getC0Enabled();
    if(segmentNotLinear) {
        const CtrlsMode prevMode = prevNormalV->getCtrlsMode();
        const bool makePrevSmooth = prevNormalV->getC0Enabled() &&
                                    prevNormal(prevNormalV) &&
                                    prevMode == CtrlsMode::symmetric;
        if(makePrevSmooth) setNodeCtrlsMode(prevNormalV, CtrlsMode::smooth);
        setNodeCtrlsMode(node, CtrlsMode::smooth);
        const CtrlsMode nextMode = nextNormalV->getCtrlsMode();
        const bool makeNextSmooth = nextNormalV->getC2Enabled() &&
                                    nextNormal(nextNormalV) &&
                                    nextMode == CtrlsMode::symmetric;
        if(makeNextSmooth) setNodeCtrlsMode(nextNormalV, CtrlsMode::smooth);
    } else {
        node->setC0Enabled(false);
        node->setC2Enabled(false);
    }
    for(int i = prevNormalV->getNodeId() + 1; i < nodeId; i++) {
        Node * const iNode = mNodes[i];
        if(iNode->isDissolved()) {
            const qreal iT = gMapTToFragment(0, node->t(), iNode->t());
            iNode->setT(iT);
        }
    }
    for(int i = nodeId + 1; i < nextNormalV->getNodeId(); i++) {
        Node * const iNode = mNodes[i];
        if(iNode->isDissolved()) {
            const qreal iT = gMapTToFragment(node->t(), 1, iNode->t());
            iNode->setT(iT);
        }
    }
}

void NodeList::promoteDissolvedNodeToNormal(const int nodeId) {
    promoteDissolvedNodeToNormal(nodeId, at(nodeId));
}

void NodeList::promoteDissolvedNodeToNormal(Node * const node) {
    promoteDissolvedNodeToNormal(node->mId, node);
}

void NodeList::splitNode(const int nodeId) {
    Node * const node = mNodes[nodeId];
    Node newNode = *node;
    if(node->isNormal()) {
        node->mC2 = node->mP1;
        newNode.mC0 = newNode.mP1;
    }
    const int nextNormalIdV = nextNormal(nodeId)->getNodeId();
    if(nextNormalIdV == 0) {
        insertNodeBefore(nextNormalIdV, newNode);
    } else insertNodeAfter(nodeId, newNode);
}

void NodeList::splitNodeAndDisconnect(const int nodeId) {
    splitNode(nodeId);
}

void NodeList::mergeNodes(const int node1Id, const int node2Id) {
    if(!nodesConnected(node1Id, node2Id))
        RuntimeThrow("Only neighbouring connected nodes can be merged");
    const int resId = qMin(node1Id, node2Id);
    const int otherId = qMax(node1Id, node2Id);
    Node * const resultingNode = at(resId);
    Node * const otherNode = at(otherId);
    *resultingNode = Node::sInterpolateNormal(*resultingNode, *otherNode, 0.5);
    resultingNode->setNodeId(node1Id);
    removeNodeFromList(node2Id);
}

bool NodeList::nodesConnected(const int node1Id, const int node2Id) const {
    if(qAbs(node2Id - node1Id) == 1) return true;
    const bool oneIsFirst = node1Id == 0 || node2Id == 0;
    const bool oneIsLast = node1Id == mNodes.count() - 1 ||
                           node2Id == mNodes.count() - 1;
    if(mClosed && oneIsFirst && oneIsLast) return true;
    return false;
}

void gCubicTo(const Node& prevNode, const Node& nextNode,
              QList<qreal>& dissolvedTs, SkPath& result) {
    qCubicSegment2D seg(prevNode.p1(), prevNode.c2(),
                        nextNode.c0(), nextNode.p1());
    qreal lastT = 0;
    for(const qreal t : dissolvedTs) {
        const qreal mappedT = gMapTToFragment(lastT, 1, t);
        auto div = seg.dividedAtT(mappedT);
        const auto& first = div.first;
        result.cubicTo(toSkPoint(first.c1()),
                       toSkPoint(first.c2()),
                       toSkPoint(first.p3()));
        seg = div.second;
        lastT = t;
    }
    result.cubicTo(toSkPoint(seg.c1()),
                   toSkPoint(seg.c2()),
                   toSkPoint(seg.p3()));
    dissolvedTs.clear();
}

SkPath NodeList::toSkPath() const {
    SkPath result;

    if(mNodes.isEmpty()) return result;

    if(at(0)->isDissolved()) {
        auto copy = *this;
        copy.promoteDissolvedNodeToNormal(0);
        return copy.toSkPath();
    }

    const Node * firstNode = nullptr;
    const Node * prevNormalNode = nullptr;

    QList<qreal> dissolvedTs;

    bool move = true;
    for(const auto& node : mNodes) {
        if(node->isDissolved()) {
            dissolvedTs << node->t();
        } else if(node->isNormal()) {
            if(move) {
                firstNode = node.get();
                result.moveTo(toSkPoint(node->p1()));
                move = false;
            } else {
                gCubicTo(*prevNormalNode, *node,
                         dissolvedTs, result);
            }
            prevNormalNode = node.get();
        } else {
            RuntimeThrow("Unrecognized node type");
        }
    }
    if(isClosed()) {
        gCubicTo(*prevNormalNode, *firstNode, dissolvedTs, result);
        result.close();
    }
    return result;
}

void NodeList::setPath(const SkPath &path) {
    mNodes.clear();
    Node * firstNode = nullptr;
    Node * prevNode = nullptr;
    SkPath::RawIter iter = SkPath::RawIter(path);

    SkPoint pts[4];
    int verbId = 0;

    // for converting conics to quads
    SkAutoConicToQuads conicToQuads;
    int quadsCount = 0;
    int quadId = 0;
    SkPoint *ptsT = nullptr;

    SkPath::Verb verb = iter.next(pts);
    for(;;) {
        switch(verb) {
            case SkPath::kMove_Verb: {
                if(firstNode) return;
                const auto qPt = toQPointF(pts[0]);
                firstNode = appendAndGetNode(Node(qPt));
                prevNode = firstNode;
            }
                break;
            case SkPath::kLine_Verb: {
                const auto qPt = toQPointF(pts[1]);

                prevNode->setC2Enabled(false);
                prevNode->mC2 = prevNode->mP1;

                bool appendNode;
                if(iter.peek() == SkPath::kClose_Verb) {
                    firstNode->setC0Enabled(false);
                    firstNode->mC0 = firstNode->mP1;
                    appendNode = !isZero4Dec(pointToLen(firstNode->p1() - qPt));
                } else appendNode = true;
                if(appendNode) prevNode = appendAndGetNode(Node(qPt));
            }
                break;
            case SkPath::kConic_Verb: {
                const QPointF p0 = toQPointF(pts[0]);
                const QPointF p1 = toQPointF(pts[1]);
                const QPointF p2 = toQPointF(pts[2]);
                const qreal weight = SkScalarToDouble(iter.conicWeight());

                const auto seg = qCubicSegment2D::sFromConic(p0, p1, p2, weight);
                pts[1] = toSkPoint(seg.c1());
                pts[2] = toSkPoint(seg.c2());
                pts[3] = toSkPoint(seg.p3());
                verb = SkPath::kCubic_Verb;
                continue;
            }
            case SkPath::kQuad_Verb: {
                const SkPoint ctrlPtT = pts[1];
                pts[1] = pts[0] + (ctrlPtT - pts[0])*0.66667f;
                pts[3] = pts[2];
                pts[2] = pts[3] + (ctrlPtT - pts[3])*0.66667f;
                verb = SkPath::kCubic_Verb;
                continue;
            }
            case SkPath::kCubic_Verb: {
                const QPointF c0Pt = toQPointF(pts[1]);
                const QPointF c1Pt = toQPointF(pts[2]);
                const QPointF p2Pt = toQPointF(pts[3]);

                prevNode->setC2Enabled(true);
                prevNode->mC2 = c0Pt;
                prevNode->disableUnnecessaryCtrls();
                prevNode->guessCtrlsMode();

                bool appendNode;
                if(iter.peek() == SkPath::kClose_Verb && quadsCount == 0) {
                    appendNode = !isZero4Dec(pointToLen(firstNode->p1() - p2Pt));
                    if(!appendNode) {
                        firstNode->setC0Enabled(true);
                        firstNode->mC0 = c1Pt;
                    }
                } else appendNode = true;
                if(appendNode) prevNode = appendAndGetNode(Node(c1Pt, p2Pt, p2Pt));
            }
                break;
            case SkPath::kClose_Verb:
                setClosed(true);
                break;
            case SkPath::kDone_Verb:
                return;
        }
        if(quadsCount > 0) {
            const int firstPtId = quadId*2;
            pts[0] = ptsT[firstPtId];
            pts[1] = ptsT[firstPtId + 1];
            pts[2] = ptsT[firstPtId + 2];
            verb = SkPath::kQuad_Verb;
            quadId++;
            quadsCount--;
        } else {
            verb = iter.next(pts);
            verbId++;
        }
    }
}

qreal NodeList::prevT(const int nodeId) const {
    const Node * const node = prevNode(nodeId);
    if(!node) return 0;
    if(node->isNormal()) return 0;
    return node->t();
}

qreal NodeList::nextT(const int nodeId) const {
    const Node * const node = nextNode(nodeId);
    if(!node) return 1;
    if(node->isNormal()) return 1;
    return node->t();
}

int NodeList::normalCount() const {
    int result = 0;
    for(const auto& node : mNodes) {
        if(node->isNormal()) result++;
    }
    return result;
}

void NodeList::setNodeType(const int nodeId, const NodeType type) const {
    if(nodeId < 0 || nodeId >= mNodes.count()) return;
    setNodeType(mNodes[nodeId], type);
}

void NodeList::setNodeType(Node * const node, const NodeType type) const {
    node->setType(type);
}

void NodeList::setNodeCtrlsMode(const int nodeId, const CtrlsMode ctrlsMode) {
    if(nodeId < 0 || nodeId >= mNodes.count()) return;
    setNodeCtrlsMode(mNodes[nodeId], ctrlsMode);
}

void NodeList::setNodeCtrlsMode(Node * const node, const CtrlsMode ctrlsMode) {
    node->setCtrlsMode(ctrlsMode);
}

void NodeList::setNodeC0Enabled(const int nodeId, const bool enabled) {
    if(nodeId < 0 || nodeId >= mNodes.count()) return;
    setNodeC0Enabled(mNodes[nodeId], enabled);
}

void NodeList::setNodeC0Enabled(Node * const node, const bool enabled) {
    node->setC0Enabled(enabled);
}

void NodeList::setNodeC2Enabled(const int nodeId, const bool enabled) {
    if(nodeId < 0 || nodeId >= mNodes.count()) return;
    setNodeC2Enabled(mNodes[nodeId], enabled);
}

void NodeList::setNodeC2Enabled(Node * const node, const bool enabled) {
    node->setC2Enabled(enabled);
}

Node *NodeList::prevNode(const Node * const node) const {
    return prevNode(node->getNodeId());
}

Node *NodeList::nextNode(const Node * const node) const {
    return nextNode(node->getNodeId());
}

Node *NodeList::prevNode(const int nodeId) const {
    if(mNodes.count() <= 1) return nullptr;
    if(nodeId > 0) return mNodes[nodeId - 1];
    if(mClosed) return mNodes.last();
    return nullptr;
}

Node *NodeList::nextNode(const int nodeId) const {
    if(mNodes.count() <= 1) return nullptr;
    if(nodeId < mNodes.count() - 1) return mNodes[nodeId + 1];
    if(mClosed) return mNodes.first();
    return nullptr;
}

Node *NodeList::prevNormal(const Node * const node) const {
    return prevNormal(node->getNodeId());
}

Node *NodeList::nextNormal(const Node * const node) const {
    return nextNormal(node->getNodeId());
}

Node * NodeList::prevNormal(const int nodeId) const {
    if(mNodes.count() <= 1) return nullptr;
    Node * currNode = mNodes.at(nodeId);
    while(prevNode(currNode)) {
        currNode = prevNode(currNode);
        if(currNode->getNodeId() == nodeId) return nullptr;
        if(currNode->isNormal()) return currNode;
    }
    return nullptr;
}

Node * NodeList::nextNormal(const int nodeId) const {
    if(mNodes.count() <= 1) return nullptr;
    Node * currNode = mNodes.at(nodeId);
    while(nextNode(currNode)) {
        currNode = nextNode(currNode);
        if(currNode->getNodeId() == nodeId) return nullptr;
        if(currNode->isNormal()) return currNode;
    }
    return nullptr;
}

NodeList NodeList::sInterpolate(const NodeList &list1,
                                const NodeList &list2,
                                const qreal weight2) {
    if(list1.count() != list2.count())
        RuntimeThrow("Cannot interpolate paths with different node count");
    if(list1.isClosed() != list2.isClosed())
        RuntimeThrow("Cannot interpolate a closed path with an open path.");
    NodeList list1Cpy = list1;
    NodeList list2Cpy = list2;
    const bool closed = list1Cpy.isClosed();
    NodeList result;
    result.setClosed(closed);
    ListOfNodes& resultList = result.getList();
    const int listCount = list1Cpy.count();
    for(int i = 0; i < listCount; i++) {
        const Node * const node1 = list1Cpy.at(i);
        const Node * const node2 = list2Cpy.at(i);
        if(node1->getType() == node2->getType()) continue;
        if(node1->isDissolved()) {
            list1Cpy.promoteDissolvedNodeToNormal(i);
        } else if(node2->isDissolved()) {
            list2Cpy.promoteDissolvedNodeToNormal(i);
        } else RuntimeThrow("Nodes with different type should not happen");
    }
    for(int i = 0; i < listCount; i++) {
        const Node * const node1 = list1Cpy.at(i);
        const Node * const node2 = list2Cpy.at(i);
        if(node1->isNormal() && node2->isNormal()) {
            const auto normalInter = Node::sInterpolateNormal(
                        *node1, *node2, weight2);
            resultList.append(normalInter);
        } else if(node1->isDissolved() && node2->isDissolved()) {
            const auto dissInter = Node::sInterpolateDissolved(
                        *node1, *node2, weight2);
            resultList.append(dissInter);
        } else RuntimeThrow("Nodes with different type should not happen");
    }

    return result;
}
