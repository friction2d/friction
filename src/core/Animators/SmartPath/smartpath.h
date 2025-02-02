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

#ifndef SMARTPATH_H
#define SMARTPATH_H
#include "../../simplemath.h"
#include "../../skia/skiaincludes.h"
#include "../../exceptions.h"
#include "../../pointhelpers.h"
#include "nodelist.h"
#include "nodepointvalues.h"
#include "../../framerange.h"
#include "../../smartPointers/ememory.h"

class CORE_EXPORT SmartPath {
public:
    SmartPath() = default;
    SmartPath(const SkPath& path);
    SmartPath(const NodeList& path);

    void actionOpen();

    void actionRemoveNode(const int nodeId, const bool approx);

    int actionAddFirstNode(const QPointF& c0,
                           const QPointF& p1,
                           const QPointF& c2);
    int actionAddFirstNode(const NormalNodeData &data);

    void actionMoveNodeBetween(const int movedNodeId,
                               const int prevNodeId,
                               const int nextNodeId);

    int actionPrependNode();
    int actionPrependNode(const NodePointValues &values);

    int actionAppendNodeAtEndNode(const int endNodeId);
    int actionAppendNodeAtEndNode(const NodePointValues &values);
    int actionAppendNodeAtEndNode();

    int actionInsertNodeBetween(const int prevId,
                                const int nextId,
                                const QPointF &c0,
                                const QPointF &p1,
                                const QPointF &c2);
    int actionInsertNodeBetween(const int prevId, const int nextId,
                                const NodePointValues &vals);
    int actionInsertNodeBetween(const int prevId,
                                const int nextId,
                                const qreal t);

    void actionPromoteDissolvedNodeToNormal(const int nodeId);
    void actionDemoteToDissolved(const int nodeId, const bool approx);

    void actionDisconnectNodes(const int node1Id, const int node2Id);
    void actionConnectNodes(const int node1Id, const int node2Id);

    void actionSetDissolvedNodeT(const int nodeId, const qreal t);

    void actionSetNormalNodeValues(const int nodeId, const QPointF& c0,
                                   const QPointF& p1, const QPointF& c2);
    void actionSetNormalNodeValues(const int nodeId, const NormalNodeData& data);

    void actionSetNormalNodeP1(const int nodeId, const QPointF& p1);
    void actionSetNormalNodeC0(const int nodeId, const QPointF& c0);
    void actionSetNormalNodeC2(const int nodeId, const QPointF& c2);

    void actionSetNormalNodeCtrlsMode(const int nodeId, const CtrlsMode mode);

    void actionSetNormalNodeC0Enabled(const int nodeId, const bool enabled);
    void actionSetNormalNodeC2Enabled(const int nodeId, const bool enabled);

    void actionReversePath();

    void actionAppendMoveAllFrom(SmartPath&& other);
    void actionPrependMoveAllFrom(SmartPath&& other);

    void actionMergeNodes(const int node1Id, const int node2Id);

    void reset();
    void clear();

    bool isEmpty() const
    { return mNodesList.isEmpty(); }

    SkPath getPathAt() const;
    void setPath(const SkPath& path);

    const Node * getNodePtr(const int id) const;

    int prevNodeId(const int nodeId) const;
    int nextNodeId(const int nodeId) const;
    int prevNormalId(const int nodeId) const;
    int nextNormalId(const int nodeId) const;

    qValueRange dissolvedTRange(const int nodeId);

    void updateDissolvedNodePosition(const int nodeId);
    void updateDissolvedNodePosition(const int nodeId, Node * const node);

    bool isClosed() const
    { return mNodesList.isClosed(); }

    void setClosed(const bool closed)
    { mNodesList.setClosed(closed); }

    int getNodeCount() const
    { return mNodesList.count(); }

    int normalNodeCount() const
    { return mNodesList.normalCount(); }

    void addDissolvedNodes(const int add);

    static void sInterpolate(const SmartPath &path1,
                             const SmartPath &path2,
                             const qreal path2Weight,
                             SmartPath& target);

    bool hasDetached() const
    { return mLastDetached.count() > 0; }

    NodeList getAndClearLastDetached();

    void applyTransform(const QMatrix &transform)
    { mNodesList.applyTransform(transform); }

    bool read(eReadStream& src)
    { return mNodesList.read(src); }
    bool write(eWriteStream& dst) const
    { return mNodesList.write(dst); }

    QString toXML() const;
    void loadXML(const QStringRef& xev);

    const NodeList& getNodesRef() const
    { return mNodesList; }

    bool isClockwise() const;

    NodeList mid(const int first, const int last) const;
private:
    int insertNodeBetween(const int prevId, const int nextId,
                          const Node &nodeBlueprint);

    NodeList mNodesList;

    NodeList mLastDetached;
};

eWriteStream& operator<<(eWriteStream& dst, const SmartPath& path);
eReadStream& operator>>(eReadStream& src, SmartPath& path);

#endif // SMARTPATH_H
