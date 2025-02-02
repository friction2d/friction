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

#include "patheffect.h"
#include "pointhelpers.h"
#include "skia/skqtconversions.h"
#include "Properties/boolproperty.h"
#include "Animators/dynamiccomplexanimator.h"
#include "typemenu.h"

#include <QDrag>

PathEffect::PathEffect(const QString &name,
                       const PathEffectType type) :
    eEffect(name), mType(type) {}

void PathEffect::writeIdentifier(eWriteStream& dst) const {
    dst.write(&mType, sizeof(PathEffectType));
}

void PathEffect::writeIdentifierXML(QDomElement& ele) const {
    ele.setAttribute("type", static_cast<int>(mType));
}

void PathEffect::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    eEffect::prp_setupTreeViewMenu(menu);
    const PropertyMenu::PlainSelectedOp<PathEffect> dOp =
    [](PathEffect* const eff) {
        const auto parent = eff->getParent<DynamicComplexAnimatorBase<PathEffect>>();
        parent->removeChild(eff->ref<PathEffect>());
    };
    menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete Effect(s)"), dOp);
}

void PathEffect::apply(const qreal relFrame,
                       const qreal influence,
                       SkPath& path) const {
    getEffectCaller(relFrame, influence)->apply(path);
}

PathEffectType PathEffect::getEffectType() {
    return mType;
}

QMimeData *PathEffect::SWT_createMimeData() {
    return new eMimeData(QList<PathEffect*>() << this);
}
