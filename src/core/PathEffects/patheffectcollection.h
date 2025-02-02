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

#ifndef PATHEFFECTCOLLECTION_H
#define PATHEFFECTCOLLECTION_H
#include "PathEffects/patheffect.h"
#include "Animators/dynamiccomplexanimator.h"
#include "skia/skiaincludes.h"
class PathBox;
class PathEffectsTask;

CORE_EXPORT
qsptr<PathEffect> readIdCreatePathEffect(eReadStream& src);
CORE_EXPORT
void writePathEffectType(PathEffect* const obj, eWriteStream& dst);

CORE_EXPORT
qsptr<PathEffect> readIdCreatePathEffectXML(const QDomElement& ele);
CORE_EXPORT
void writePathEffectTypeXML(PathEffect* const obj, QDomElement& ele);

typedef DynamicComplexAnimator<
    PathEffect,
    writePathEffectType,
    readIdCreatePathEffect,
    writePathEffectTypeXML,
    readIdCreatePathEffectXML> PathEffectCollectionBase;

class CORE_EXPORT PathEffectCollection : public PathEffectCollectionBase {
    e_OBJECT
protected:
    PathEffectCollection();
public:
    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    bool hasEffects();
    void readPathEffect(eReadStream &target);

    void addEffects(const qreal relFrame,
                    QList<stdsptr<PathEffectCaller>>& list,
                    const qreal influence = 1) const;
private:
};


#endif // PATHEFFECTCOLLECTION_H
