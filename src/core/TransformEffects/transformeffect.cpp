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

#include "transformeffect.h"
#include "typemenu.h"
#include "Animators/dynamiccomplexanimator.h"

TransformEffect::TransformEffect(const QString &name,
                                 const TransformEffectType type) :
    eEffect(name), mType(type)  {

}

void TransformEffect::writeIdentifier(eWriteStream& dst) const {
    dst.write(&mType, sizeof(TransformEffectType));
}

void TransformEffect::writeIdentifierXML(QDomElement& ele) const {
    ele.setAttribute("type", static_cast<int>(mType));
}

void TransformEffect::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<TransformEffect>()) return;
    menu->addedActionsForType<TransformEffect>();
    {
        const PropertyMenu::PlainSelectedOp<TransformEffect> dOp =
        [](TransformEffect* const prop) {
            const auto parent = prop->template getParent<
                    DynamicComplexAnimatorBase<TransformEffect>>();
            parent->removeChild(prop->template ref<TransformEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete"), dOp);
    }

    menu->addSeparator();
    StaticComplexAnimator::prp_setupTreeViewMenu(menu);
}
