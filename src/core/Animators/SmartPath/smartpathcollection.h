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

#ifndef SMARTPATHCOLLECTION_H
#define SMARTPATHCOLLECTION_H
#include <QList>
#include "../dynamiccomplexanimator.h"
#include "smartpathanimator.h"
#include "../../MovablePoints/segment.h"

class SmartPathAnimator;
class SmartNodePoint;
typedef DynamicComplexAnimator<SmartPathAnimator> SmartPathCollectionBase;

class CORE_EXPORT SmartPathCollection : public SmartPathCollectionBase {
    Q_OBJECT
    e_OBJECT
protected:
    SmartPathCollection();
public:
    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);

    using EffectApplier = std::function<void(const qreal relFrame,
                                             SkPath& path)>;
    void savePathsSVG(SvgExporter& exp,
                      QDomElement& parent,
                      const EffectApplier& applier,
                      const bool forceDumbIncrement,
                      const FrameRange& visRange,
                      const QList<Animator*> extInfl);

    SmartNodePoint * createNewSubPathAtRelPos(const QPointF &relPos);
    SmartNodePoint * createNewSubPathAtPos(const QPointF &absPos);

    SkPath getPathAtRelFrame(const qreal relFrame) const;

    void applyTransform(const QMatrix &transform) const;

    void loadSkPath(const SkPath& path);

    void setFillType(const SkPathFillType fillType);
    SkPathFillType getFillType() const
    { return mFillType; }
signals:
    void fillTypeChanged(SkPathFillType);
private:
    void updateVisibleChildren();

    SmartPathAnimator *createNewPath();
    SmartPathAnimator *createNewPath(const SkPath& path);

    void updatePathColors();

    SkPathFillType mFillType = SkPathFillType::kWinding;
};

#endif // SMARTPATHCOLLECTION_H
