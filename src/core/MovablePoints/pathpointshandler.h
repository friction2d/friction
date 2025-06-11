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

#ifndef PATHPOINTSHANDLER_H
#define PATHPOINTSHANDLER_H
#include "Animators/SmartPath/smartpath.h"
#include "smartnodepoint.h"
#include "pointshandler.h"
#include "simpletask.h"
class Scene;
class SmartPathCollectionHandler;

class CORE_EXPORT PathPointsHandler : public PointsHandler {
    e_OBJECT
protected:
    PathPointsHandler(SmartPathAnimator * const targetAnimator);
public:
    NormalSegment getNormalSegment(const QPointF &absPos,
                                   const qreal invScale) const;

    void scheduleRemoveNode(const int nodeId);

    // actions on NORMAL
    void setCtrlsMode(const int nodeId, const CtrlsMode mode);
    void removeNode(const int nodeId, const bool approx);
    SmartNodePoint *addNewAtStart(const QPointF &relPos);
    SmartNodePoint *addNewAtEnd(const QPointF &relPos);
    // actions on DISSOLVED

    // actions on DUMMY and DISSOLVED
    void promoteToNormal(const int nodeId);
    void demoteToDissolved(const int nodeId, const bool approx);
    int moveToClosestSegment(const int nodeId, const QPointF& relPos);

    void mergeNodes(const int nodeId1, const int nodeId2);

    // actions on normal-normal segments
    SmartNodePoint *divideSegment(const int node1Id, const int node2Id,
                                  const qreal t);
    void createSegment(const int node1Id, const int node2Id);
    void removeSegment(const NormalSegment &segment);

    int getPrevNodeId(const int startId) const;
    int getNextNodeId(const int startId) const;

    SmartNodePoint* getPrevNode(const int startId) const;
    SmartNodePoint* getNextNode(const int startId) const;
    SmartNodePoint* getPrevNormalNode(const int startId) const;
    SmartNodePoint* getNextNormalNode(const int startId) const;
    SmartNodePoint* getClosestNode(const QPointF& absPos, const qreal& maxDist) const;

    void updateAllPoints();
    void updateAllPointsRadius();

    const SmartPathAnimator * getAnimator() {
        return mTargetAnimator;
    }
private:
    SimpleTaskScheduler scheduleNodesRemoval;

    void flushNodesRemoval();

    void updatePoints(int min, int max);
    void updatePoint(const int nodeId);
    void updatePoint(SmartNodePoint * const pt, const int nodeId);

    SmartNodePoint* createNewNodePoint(const int nodeId);
    SmartNodePoint* createAndAssignNewNodePoint(const int nodeId);
    SmartPath* targetPath() const;

    SmartPathAnimator * const mTargetAnimator;
    bool mKeyOnCurrentFrame = false;

    QList<int> mRemoveNodes;
};

#endif // PATHPOINTSHANDLER_H
