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

#ifndef SVGFLIPBOOKTRACK_H
#define SVGFLIPBOOKTRACK_H

#include "Animators/staticcomplexanimator.h"
#include <QLoggingCategory>
#include <QMap>

Q_DECLARE_LOGGING_CATEGORY(lcSvgFlipbookTrack)

class ContainerBox;
class BoundingBox;
class IntAnimator;

class CORE_EXPORT SvgFlipbookTrack : public StaticComplexAnimator {
    Q_OBJECT
    e_OBJECT
public:
    explicit SvgFlipbookTrack(const QString& ownerElementId);

    void setPageMap(const QMap<int, QString>& pageMap);
    bool isOrphaned() const { return mOrphaned; }

    void resolveTargets(ContainerBox* svgRoot);
    void syncToTargets();

    void writeTrack(eWriteStream& dst) const;
    void readTrack(eReadStream& src);

    void prp_drawTimelineControls(
            QPainter* p, qreal pixelsPerFrame,
            const FrameRange& absFrameRange, int rowHeight) override;

    void prp_setupTreeViewMenu(PropertyMenu* menu) override;

signals:
    void deleteRequested();

private:
    bool mOrphaned = true;
    QMap<int, QString> mPageMap;
    QMap<int, BoundingBox*> mResolvedPages;
    qsptr<IntAnimator> mIndex;
};

#endif // SVGFLIPBOOKTRACK_H
