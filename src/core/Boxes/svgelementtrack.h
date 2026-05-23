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

#ifndef SVGELEMENTTRACK_H
#define SVGELEMENTTRACK_H

#include "Animators/staticcomplexanimator.h"
#include <QLoggingCategory>
#include <QSet>

Q_DECLARE_LOGGING_CATEGORY(lcSvgElementTrack)

class ContainerBox;
class BoundingBox;

class CORE_EXPORT SvgElementTrack : public StaticComplexAnimator {
    Q_OBJECT
    e_OBJECT
public:
    explicit SvgElementTrack(const QString& targetId);

    bool isOrphaned() const { return mOrphaned; }
    void setOrphaned(const bool orphaned);

    bool isChildOrphaned(Property* child) const {
        return mOrphanedChildren.contains(child);
    }

    // Returns the found target box (or nullptr if not found).
    BoundingBox* resolveTarget(ContainerBox* svgRoot);
    BoundingBox* resolvedTarget() const { return mResolvedTarget; }

    void reconcileWithTarget(BoundingBox* target);
    void initFromTarget(BoundingBox* target);
    void syncToTarget(BoundingBox* target);

    void writeTrack(eWriteStream& dst) const;
    void readTrack(eReadStream& src);

    void prp_drawTimelineControls(
            QPainter* p, qreal pixelsPerFrame,
            const FrameRange& absFrameRange, int rowHeight) override;

    void prp_setupTreeViewMenu(PropertyMenu* menu) override;

signals:
    void orphanedChanged();
    void deleteRequested();

protected:
    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const override;
    void prp_readPropertyXEV_impl(const QDomElement& ele,
                                  const XevImporter& imp) override;

    void captureFromTarget(BoundingBox* target);

private:
    bool mOrphaned = true;
    bool mSyncingToTarget = false;
    QSet<Property*> mOrphanedChildren;
    BoundingBox* mResolvedTarget = nullptr;
    QMetaObject::Connection mTargetConn;
};

#endif // SVGELEMENTTRACK_H
