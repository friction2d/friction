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

#ifndef NODE_H
#define NODE_H

#include "pointhelpers.h"
#include "exceptions.h"
#include "nodepointvalues.h"

struct CORE_EXPORT NormalNodeData {
    NormalNodeData(const bool c0Enabled,
                   const bool c2Enabled,
                   const CtrlsMode ctrlsMode,
                   const QPointF c0,
                   const QPointF p1,
                   const QPointF c2) {
        fC0Enabled = c0Enabled;
        fC2Enabled = c2Enabled;
        fC0 = c0;
        fP1 = p1;
        fC2 = c2;
        fCtrlsMode = ctrlsMode;
    }

    NormalNodeData(const NodePointValues& values) {
        fC0Enabled = true;
        fC2Enabled = true;
        fC0 = values.fC0;
        fP1 = values.fP1;
        fC2 = values.fC2;
        fCtrlsMode = gGuessCtrlsMode(fC0, fP1, fC2, true, true);
    }

    bool fC0Enabled;
    bool fC2Enabled;
    CtrlsMode fCtrlsMode;
    QPointF fC0;
    QPointF fP1;
    QPointF fC2;
};

enum class NodeType : char {
    dissolved, normal, none
};

struct CORE_EXPORT Node {
    friend class NodeList;
    friend class ListOfNodes;

    Node();
    Node(const QPointF& p1);
    Node(const QPointF& c0, const QPointF& p1, const QPointF& c2);
    Node(const NormalNodeData& data);
    Node(const qreal t);

    bool isNormal() const { return mType == NodeType::normal; }
    bool isDissolved() const { return mType == NodeType::dissolved; }

    int getNodeId() const { return mId; }

    void setNormalData(const NormalNodeData& data);

    static Node sInterpolateNormal(const Node &node1, const Node &node2,
                                   const qreal weight2);

    static Node sInterpolateDissolved(const Node &node1, const Node &node2,
                                      const qreal weight2);

    QPointF c0() const { return mC0Enabled ? mC0 : mP1; }
    QPointF p1() const { return mP1; }
    QPointF c2() const { return mC2Enabled ? mC2 : mP1; }
    qreal t() const { return mT; }

    void setC0(const QPointF& c0) { mC0 = c0; }
    void setC2(const QPointF& c2) { mC2 = c2; }
    void setP1(const QPointF& p1) { mP1 = p1; }
    void setT(const qreal t) { mT = t; }

    NodeType getType() const { return mType; }
    CtrlsMode getCtrlsMode() const { return mCtrlsMode; }
    bool getC0Enabled() const { return mC0Enabled; }
    bool getC2Enabled() const { return mC2Enabled; }

    NormalNodeData normalData() const;

    void applyTransform(const QTransform &transform);

    void disableUnnecessaryCtrls();

    void guessCtrlsMode();

    void setC0Enabled(const bool enabled)
    { mC0Enabled = enabled; }

    void setC2Enabled(const bool enabled)
    { mC2Enabled = enabled; }

    void assignCtrlsMode(const CtrlsMode ctrlsMode)
    { mCtrlsMode = ctrlsMode; }
protected:
    void setNodeId(const int nodeId)
    { mId = nodeId; }

    void setType(const NodeType type)
    { mType = type; }

    void setCtrlsMode(const CtrlsMode ctrlsMode);

    bool mC0Enabled = true;
    bool mC2Enabled = true;
    NodeType mType;
    CtrlsMode mCtrlsMode = CtrlsMode::corner;
    int mId = -1;
    //! @brief T value for segment defined by previous and next normal node
    qreal mT;
    QPointF mC0;
    QPointF mP1;
    QPointF mC2;
};

#endif // NODE_H
