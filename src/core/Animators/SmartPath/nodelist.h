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

#ifndef NODELIST_H
#define NODELIST_H
#include "listofnodes.h"
#include "smartPointers/stdselfref.h"
#include "smartPointers/stdpointer.h"

class SkPath;
class CORE_EXPORT NodeList {
    friend class SmartPath;
protected:
    NodeList() = default;
public:
    NodeList(ListOfNodes&& other);
    NodeList(const NodeList& other);

    Node* operator[](const int i) const
    { return mNodes[i]; }

    typedef QList<stdsptr<Node>>::const_iterator const_iterator;
    typedef QList<stdsptr<Node>>::iterator iterator;

    const_iterator begin() const
    { return mNodes.begin(); }

    const_iterator end() const
    { return mNodes.end(); }

    Node* at(const int i) const
    { return mNodes[i]; }

    int count() const
    { return mNodes.count(); }

    int normalCount() const;

    void setNodeType(const int nodeId, const NodeType type) const;
    void setNodeType(Node * const node, const NodeType type) const;

    void setNodeCtrlsMode(const int nodeId, const CtrlsMode ctrlsMode);
    void setNodeCtrlsMode(Node * const node, const CtrlsMode ctrlsMode);

    void setNodeC0Enabled(const int nodeId, const bool enabled);
    void setNodeC0Enabled(Node * const node, const bool enabled);
    void setNodeC2Enabled(const int nodeId, const bool enabled);
    void setNodeC2Enabled(Node * const node, const bool enabled);

    Node * prevNode(const Node * const node) const;
    Node * nextNode(const Node * const node) const;
    Node * prevNode(const int nodeId) const;
    Node * nextNode(const int nodeId) const;
    Node * prevNormal(const Node * const node) const;
    Node * nextNormal(const Node * const node) const;
    Node * prevNormal(const int nodeId) const;
    Node * nextNormal(const int nodeId) const;

    SkPath toSkPath() const;
    void setPath(const SkPath &path);
    void removeNodeFromList(const int nodeId);
    void reverse()
    { mNodes.reverse(); }
    bool isClosed() const
    { return mClosed; }
    void setClosed(const bool closed)
    { mClosed = closed; }
    bool isEmpty() const
    { return mNodes.isEmpty(); }

    void reset();
    void clear()
    { reset(); }

    int insertFirstNode(const Node &nodeBlueprint);

    void promoteDissolvedNodeToNormal(const int nodeId);
    void promoteDissolvedNodeToNormal(Node * const node);
    void promoteDissolvedNodeToNormal(const int nodeId, Node * const node);

    void removeNode(const int nodeId, const bool approx);
    void removeNode(const int nodeId, Node * const node,
                    const bool approx);

    void demoteNormalNodeToDissolved(const int nodeId, const bool approx);
    void demoteNormalNodeToDissolved(const int nodeId, Node * const node,
                                     const bool approx);

    void splitNode(const int nodeId);
    void splitNodeAndDisconnect(const int nodeId);
    void mergeNodes(const int node1Id, const int node2Id);
    bool nodesConnected(const int node1Id, const int node2Id) const;

    void moveNode(const int fromId, const int toId);
    void updateDissolvedNodePosition(const int nodeId);
    void updateDissolvedNodePosition(const int nodeId, Node * const node);

    void applyTransform(const QTransform &transform)
    { mNodes.applyTransform(transform); }

    bool read(eReadStream& src);
    bool write(eWriteStream &dst) const;

    void normalize();
    Q_REQUIRED_RESULT NodeList normalized() const;

    NodeList mid(int first, int last) const;
protected:
    void append(NodeList&& other);
    void prepend(NodeList&& other);

    void moveNodesToFrontStartingWith(const int first);

    NodeList detachNodesStartingWith(const int first);

    void swap(NodeList& other);

    ListOfNodes& getList()
    { return mNodes; }

    const ListOfNodes& getList() const
    { return mNodes; }

    int insertNodeBefore(const int nextId, const Node &nodeBlueprint);
    int insertNodeAfter(const int prevId, const Node &nodeBlueprint);
    int prependNode(const Node &nodeBlueprint);
    int appendNode(const Node &nodeBlueprint);
    Node * appendAndGetNode(const Node &nodeBlueprint);

    static NodeList sInterpolate(const NodeList &list1,
                                 const NodeList &list2,
                                 const qreal weight2);
private:
    qreal prevT(const int nodeId) const;
    qreal nextT(const int nodeId) const;
    Node *insertNodeToList(const int nodeId, const Node &node);
    void approximateBeforeDemoteOrRemoval(const qreal nodeT,
                                          Node * const node,
                                          Node * const prevNormalV,
                                          Node * const nextNormalV);

    ListOfNodes mNodes;
    bool mClosed = false;
};

#endif // NODELIST_H
