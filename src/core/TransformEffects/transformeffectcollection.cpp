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

#include "transformeffectcollection.h"
#include "ReadWrite/evformat.h"

#include "trackeffect.h"
#include "followpatheffect.h"
#include "followobjecteffect.h"
#include "followobjectrelativeeffect.h"
#include "parenteffect.h"

TransformEffectCollection::TransformEffectCollection() :
    TransformEffectCollectionBase("transform effects") {}

void TransformEffectCollection::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<TransformEffectCollection>()) return;
    menu->addedActionsForType<TransformEffectCollection>();
    {
        const PropertyMenu::PlainSelectedOp<TransformEffectCollection> aOp =
        [](TransformEffectCollection * coll) {
            coll->addChild(enve::make_shared<TrackEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("effect"), tr("Add Track Effect"), aOp);
    }
    {
        const PropertyMenu::PlainSelectedOp<TransformEffectCollection> aOp =
        [](TransformEffectCollection * coll) {
            coll->addChild(enve::make_shared<FollowPathEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("effect"), tr("Add Follow Path Effect"), aOp);
    }
    {
        const PropertyMenu::PlainSelectedOp<TransformEffectCollection> aOp =
        [](TransformEffectCollection * coll) {
            coll->addChild(enve::make_shared<FollowObjectEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("effect"), tr("Add Follow Object Effect"), aOp);
    }
    {
        const PropertyMenu::PlainSelectedOp<TransformEffectCollection> aOp =
        [](TransformEffectCollection * coll) {
            coll->addChild(enve::make_shared<FollowObjectRelativeEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("effect"), tr("Add Follow Object Relative Effect"), aOp);
    }
    {
        const PropertyMenu::PlainSelectedOp<TransformEffectCollection> aOp =
        [](TransformEffectCollection * coll) {
            coll->addChild(enve::make_shared<ParentEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("effect"), tr("Add Parent Effect"), aOp);
    }
    menu->addSeparator();
    TransformEffectCollection::prp_setupTreeViewMenu(menu);
}

void TransformEffectCollection::prp_readProperty(eReadStream &src) {
    if(src.evFileVersion() < EvFormat::transformEffects) return;
    TransformEffectCollectionBase::prp_readProperty(src);
}

void TransformEffectCollection::prp_readProperty_impl(eReadStream &src) {
    if(src.evFileVersion() == EvFormat::transformEffects) {
        TransformEffectCollectionBase::prp_readProperty_impl(src);
        bool visible; src >> visible;
    } else {
        TransformEffectCollectionBase::prp_readProperty_impl(src);
    }
}

bool TransformEffectCollection::hasEffectsSVG()
{
    const auto& children = ca_getChildren();
    for (const auto& effect : children) {
        if (const auto followPath = enve_cast<FollowPathEffect*>(effect.get())) {
            return true;
        }
    }
    return false;
}

QDomElement TransformEffectCollection::saveEffectsSVG(SvgExporter &exp,
                                                      const FrameRange &visRange,
                                                      QDomElement &childElement,
                                                      const QDomElement &parentElement) const
{
    QDomElement result = parentElement;
    const auto& children = ca_getChildren();
    for (const auto& effect : children) {
        if (const auto path = enve_cast<FollowPathEffect*>(effect.get())) {
            result = path->saveFollowPathSVG(exp,
                                             visRange,
                                             childElement,
                                             result);
        }
    }
    return result;
}

void TransformEffectCollection::applyEffects(
        const qreal relFrame,
        qreal& pivotX, qreal& pivotY, qreal& pivotZ,
        qreal& posX, qreal& posY, qreal& posZ,
        qreal& rotX, qreal& rotY, qreal& rotZ,
        qreal& scaleX, qreal& scaleY, qreal& scaleZ,
        qreal& shearX, qreal& shearY, qreal& shearZ,
        QMatrix& postTransform,
        BoundingBox* const parent) const {
    const int iMax = ca_getNumberOfChildren();
    for(int i = 0; i < iMax; i++) {
        const auto effect = getChild(i);
        if(!effect->isVisible()) continue;
        effect->applyEffect(relFrame,
                            pivotX, pivotY, pivotZ,
                            posX, posY, posZ,
                            rotX, rotY, rotZ,
                            scaleX, scaleY, scaleZ,
                            shearX, shearY, shearZ,
                            postTransform,
                            parent);
    }
}

qsptr<TransformEffect> createTransformEffectForType(
        const TransformEffectType type) {
    switch(type) {
        case(TransformEffectType::track):
            return enve::make_shared<TrackEffect>();
        case(TransformEffectType::followPath):
            return enve::make_shared<FollowPathEffect>();
        case(TransformEffectType::followObject):
            return enve::make_shared<FollowObjectEffect>();
        case(TransformEffectType::followObjectRelative):
            return enve::make_shared<FollowObjectRelativeEffect>();
        case(TransformEffectType::parent):
            return enve::make_shared<ParentEffect>();
        default: RuntimeThrow("Invalid transform effect type '" +
                              QString::number(int(type)) + "'");
    }
}

qsptr<TransformEffect> readIdCreateTransformEffect(eReadStream &src) {
    TransformEffectType type;
    src.read(&type, sizeof(TransformEffectType));
    return createTransformEffectForType(type);
}

void writeTransformEffectType(TransformEffect * const obj, eWriteStream &dst) {
    obj->writeIdentifier(dst);
}

qsptr<TransformEffect> readIdCreateTransformEffectXEV(const QDomElement& ele) {
    const QString typeStr = ele.attribute("type");
    const int typeInt = XmlExportHelpers::stringToInt(typeStr);
    const auto type = static_cast<TransformEffectType>(typeInt);
    return createTransformEffectForType(type);
}

void writeTransformEffectTypeXEV(TransformEffect* const obj, QDomElement& ele) {
    obj->writeIdentifierXEV(ele);
}
