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

#ifndef PATHEFFECT_H
#define PATHEFFECT_H
#include "../Animators/eeffect.h"
#include "../skia/skiaincludes.h"
#include "../Animators/staticcomplexanimator.h"
#include "patheffectcaller.h"
class PathEffectCaller;
class BoolProperty;
class PathBox;
class BoundingBox;
enum class PathEffectType : short {
    DISPLACE,
    DASH,
    Duplicate,
    SOLIDIFY,
    SUM,
    SUB,
    LINES,
    ZIGZAG,
    SPATIAL_DISPLACE,
    SUBDIVIDE,
    CUSTOM,
    TYPE_COUNT
};
class PathEffect;

class CORE_EXPORT PathEffect : public eEffect {
    Q_OBJECT
protected:
    PathEffect(const QString& name, const PathEffectType type);
public:
    virtual stdsptr<PathEffectCaller>
        getEffectCaller(const qreal relFrame,
                        const qreal influence) const = 0;

    QMimeData *SWT_createMimeData() final;

    void writeIdentifier(eWriteStream &dst) const;
    void writeIdentifierXML(QDomElement& ele) const;

    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    void apply(const qreal relFrame,
               const qreal influence,
               SkPath& path) const;
    PathEffectType getEffectType();
protected:
    const PathEffectType mType;
};

#endif // PATHEFFECT_H
