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

#ifndef RASTEREFFECTCOLLECTION_H
#define RASTEREFFECTCOLLECTION_H
#include "Animators/dynamiccomplexanimator.h"
#include "smartPointers/ememory.h"
#include "RasterEffects/rastereffect.h"
#include "ShaderEffects/shadereffect.h"

class BoundingBox;
struct BoxRenderData;

CORE_EXPORT
qsptr<RasterEffect> readIdCreateRasterEffect(eReadStream& src);
CORE_EXPORT
void writeRasterEffectType(RasterEffect* const obj, eWriteStream& dst);

CORE_EXPORT
qsptr<RasterEffect> readIdCreateRasterEffectXML(const QDomElement& ele);
CORE_EXPORT
void writeRasterEffectTypeXML(RasterEffect* const obj, QDomElement& ele);

typedef DynamicComplexAnimator<
    RasterEffect,
    writeRasterEffectType,
    readIdCreateRasterEffect,
    writeRasterEffectTypeXML,
    readIdCreateRasterEffectXML> RasterEffectCollectionBase;

class CORE_EXPORT RasterEffectCollection : public RasterEffectCollectionBase {
    e_OBJECT
    Q_OBJECT
protected:
    RasterEffectCollection();
public:
    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    bool hasEffects();

    void addEffects(const qreal relFrame,
                    BoxRenderData * const data,
                    const qreal influence = 1);

    void updateIfUsesProgram(const ShaderEffectProgram * const program);
    //void readRasterEffect(QIODevice *target);
    const QMargins& getMaxForcedMargin() const {
        return mMaxForcedMargin;
    }

    QDomElement saveEffectsSVG(SvgExporter& exp, const FrameRange& visRange,
                               const QDomElement& child) const;

signals:
    void forcedMarginChanged();
private:
    void updateMaxForcedMargin();

    QMargins mMaxForcedMargin;

    bool mUnbound = false;
};

#endif // RASTEREFFECTCOLLECTION_H
