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

#ifndef SMARTPATHANIMATOR_H
#define SMARTPATHANIMATOR_H

#include "../../MovablePoints/segment.h"
#include "../../skia/skiahelpers.h"
#include "../interoptimalanimatort.h"
#include "differsinterpolate.h"
#include "smartpath.h"

using SmartPathKey = InterpolationKeyT<SmartPath>;

using SmartPathAnimatorBase = InterOptimalAnimatorT<SmartPath>;

class CORE_EXPORT SmartPathAnimator : public SmartPathAnimatorBase {
    e_OBJECT
    Q_OBJECT
protected:
    SmartPathAnimator();
    SmartPathAnimator(const SkPath& path);
    SmartPathAnimator(const SmartPath& baseValue);

    void prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp);
    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const;
public:
    enum class Mode {
        normal,
        add, remove, removeReverse,
        intersect, exclude,
        divide
    };

    QMimeData *SWT_createMimeData();

    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    void prp_drawCanvasControls(
            SkCanvas * const canvas, const CanvasMode mode,
            const float invScale, const bool ctrlPressed);

    void prp_readProperty_impl(eReadStream& src);
    void prp_writeProperty_impl(eWriteStream& dst) const;

    SkPath getPathAtAbsFrame(const qreal frame)
    { return getPathAtRelFrame(prp_absFrameToRelFrameF(frame)); }
    SkPath getPathAtRelFrame(const qreal frame);

    bool isClosed() const
    { return baseValue().isClosed(); }

    void actionSetNormalNodeCtrlsMode(const int nodeId, const CtrlsMode mode);
    void actionDemoteToDissolved(const int nodeId, const bool approx);
    void actionPromoteToNormal(const int nodeId);

    void removeNode(const int nodeId, const bool approx);
    void actionRemoveNode(const int nodeId, const bool approx);
    int actionAddNewAtStart(const QPointF &relPos);
    int actionAddNewAtStart(const NormalNodeData &data);
    int actionAddNewAtEnd(const QPointF &relPos);
    int actionAddNewAtEnd(const NormalNodeData &data);
    int actionInsertNodeBetween(const int node1Id, const int node2Id, const qreal t);
    int insertNodeBetween(const int node1Id, const int node2Id, const qreal t);
    int actionInsertNodeBetween(const int node1Id, const int node2Id,
                                const qreal t, const NodePointValues& vals);
    void connectNodes(const int node1Id, const int node2Id);
    void actionConnectNodes(const int node1Id, const int node2Id);
    void actionMergeNodes(const int node1Id, const int node2Id);
    void actionSetFirstNode(const int nodeId);
    void actionMoveNodeBetween(const int nodeId, const int prevNodeId,
                               const int nextNodeId);
    void close();
    void actionClose();
    void actionDisconnectNodes(const int node1Id, const int node2Id);
    void actionReverseCurrent();
    void actionReverseAll();

    void actionAppendMoveAllFrom(SmartPathAnimator * const other);
    void actionPrependMoveAllFrom(SmartPathAnimator * const other);

    void actionReplaceSegments(const int beginNodeId, int endNodeId,
                               const QList<qCubicSegment2D>& with);
    void actionReplaceSegments(const int beginNodeId, int endNodeId,
                               const QList<qCubicSegment2D>& with,
                               const bool reverse);

    bool hasDetached() const
    { return baseValue().hasDetached(); }

    qsptr<SmartPathAnimator> createFromDetached();

    void applyTransform(const QMatrix &transform);

    const SkPath& getCurrentPath();

    void setMode(const Mode mode);
    Mode getMode() const { return mMode; }

    void pastePath(const SmartPath& path)
    { return pastePath(anim_getCurrentRelFrame(), path); }
    void pastePath(const int frame, SmartPath path);

    void setPathColor(const QColor& color)
    { mPathColor = color; }
    const QColor& getPathColor() const
    { return mPathColor; }
signals:
    void pathBlendModeChagned(SmartPathAnimator::Mode);
    void emptied();
private:
    int actionAddFirstNode(const QPointF &relPos);
    int actionAddFirstNode(const NormalNodeData &data);

    void updateAllPoints();

    SkPath mResultPath;
    Mode mMode = Mode::normal;
    QColor mPathColor = Qt::white;
};

#endif // SMARTPATHANIMATOR_H
