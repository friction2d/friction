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

#include "node.h"
#include "exceptions.h"

Node::Node() { mType = NodeType::none; }

Node::Node(const QPointF &p1) {
    mC0 = p1;
    mP1 = p1;
    mC2 = p1;
    mType = NodeType::normal;
    mCtrlsMode = CtrlsMode::corner;
    mC0Enabled = false;
    mC2Enabled = false;
}

Node::Node(const QPointF &c0, const QPointF &p1, const QPointF &c2) {
    mC0 = c0;
    mP1 = p1;
    mC2 = c2;
    mType = NodeType::normal;

    guessCtrlsMode();
    disableUnnecessaryCtrls();
}

Node::Node(const NormalNodeData &data) {
    mType = NodeType::normal;
    setNormalData(data);
}

Node::Node(const qreal t) {
    mT = t;
    mType = NodeType::dissolved;
}

void Node::setNormalData(const NormalNodeData &data) {
    mC0 = data.fC0;
    mP1 = data.fP1;
    mC2 = data.fC2;
    mC0Enabled = data.fC0Enabled;
    mC2Enabled = data.fC2Enabled;
    mCtrlsMode = data.fCtrlsMode;
}

Node Node::sInterpolateNormal(const Node &node1, const Node &node2,
                              const qreal weight2)
{
    if (!node1.isNormal() || !node2.isNormal()) {
        qWarning() << "InterpolateNormal: Unsupported node type.";
        return (weight2 < 0.5) ? node1 : node2;
    }

    //if(isZero6Dec(weight2)) return node1;
    //if(isOne6Dec(weight2)) return node2;
    const qreal w1 = 1 - weight2;
    Node result(w1*node1.c0() + weight2*node2.c0(),
                w1*node1.mP1 + weight2*node2.mP1,
                w1*node1.c2() + weight2*node2.c2());
    result.setC0Enabled(node1.getC0Enabled() || node2.getC0Enabled());
    result.setC2Enabled(node1.getC2Enabled() || node2.getC2Enabled());
    const CtrlsMode node1Ctrls = node1.getCtrlsMode();
    const CtrlsMode node2Ctrls = node2.getCtrlsMode();
    if(node1Ctrls == node2Ctrls) {
        result.mCtrlsMode = node1Ctrls;
    } else if(node1Ctrls == CtrlsMode::corner ||
              node2Ctrls == CtrlsMode::corner) {
        result.mCtrlsMode = CtrlsMode::corner;
    } else if(node1Ctrls == CtrlsMode::smooth ||
              node2Ctrls == CtrlsMode::smooth) {
        result.mCtrlsMode = CtrlsMode::smooth;
    } else {
        result.mCtrlsMode = CtrlsMode::symmetric;
    }
    return result;
}

Node Node::sInterpolateDissolved(const Node &node1, const Node &node2,
                        const qreal weight2)
{
    if (!node1.isDissolved() || !node2.isDissolved()) {
        qWarning() << "InterpolateDissolved: Unsupported node type.";
        return (weight2 < 0.5) ? node1 : node2;
    }

    const qreal w1 = 1 - weight2;
    return Node(w1*node1.t() + weight2*node2.t());
}

NormalNodeData Node::normalData() const {
    Q_ASSERT(isNormal());
    return {mC0Enabled, mC2Enabled,
            mCtrlsMode, mC0, mP1, mC2};
}

void Node::applyTransform(const QTransform &transform) {
    mC0 = transform.map(mC0);
    mP1 = transform.map(mP1);
    mC2 = transform.map(mC2);
}

void Node::disableUnnecessaryCtrls() {
    if(isZero2Dec(pointToLen(mC0 - mP1))) setC0Enabled(false);
    if(isZero2Dec(pointToLen(mC2 - mP1))) setC2Enabled(false);
}

void Node::guessCtrlsMode() {
    mCtrlsMode = gGuessCtrlsMode(mC0, mP1, mC2, mC0Enabled, mC2Enabled);
}

void Node::setCtrlsMode(const CtrlsMode ctrlsMode) {
    mCtrlsMode = ctrlsMode;
    if(ctrlsMode == CtrlsMode::symmetric) {
        gGetCtrlsSymmetricPos(mC0, mP1, mC2, mC0, mC2);
    } else if(ctrlsMode == CtrlsMode::smooth) {
        gGetCtrlsSmoothPos(mC0, mP1, mC2, mC0, mC2);
    } else return;
    setC0Enabled(true);
    setC2Enabled(true);
}
