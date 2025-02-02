#include "blendeffectcollection.h"

#include "Boxes/layerboxrenderdata.h"
#include "typemenu.h"

#include "moveblendeffect.h"
#include "targetedblendeffect.h"

BlendEffectCollection::BlendEffectCollection() :
    BlendEffectCollectionBase("blend effects") {}

void BlendEffectCollection::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<BlendEffectCollection>()) return;
    menu->addedActionsForType<BlendEffectCollection>();
    {
        const PropertyMenu::PlainSelectedOp<BlendEffectCollection> aOp =
        [](BlendEffectCollection * coll) {
            coll->addChild(enve::make_shared<MoveBlendEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("effect"), tr("New Move Effect"), aOp);
    }
    {
        const PropertyMenu::PlainSelectedOp<BlendEffectCollection> aOp =
        [](BlendEffectCollection * coll) {
            coll->addChild(enve::make_shared<TargetedBlendEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("effect"), tr("New Targeted Effect"), aOp);
    }
    menu->addSeparator();
    BlendEffectCollectionBase::prp_setupTreeViewMenu(menu);
}

void BlendEffectCollection::blendSetup(
        ChildRenderData &data,
        const int index, const qreal relFrame,
        QList<ChildRenderData> &delayed) const {
    const int iMax = ca_getNumberOfChildren();
    for(int i = 0; i < iMax; i++) {
        const auto effect = getChild(i);
        if(!effect->isVisible()) continue;
        effect->blendSetup(data, index, relFrame, delayed);
    }
}

void BlendEffectCollection::drawBlendSetup(SkCanvas * const canvas) {
    const qreal relFrame = anim_getCurrentRelFrame();
    const int iMax = ca_getNumberOfChildren();
    for(int i = 0; i < iMax; i++) {
        const auto effect = getChild(i);
        if(!effect->isVisible()) continue;
        effect->drawBlendSetup(relFrame, canvas);
    }
}

void BlendEffectCollection::detachedBlendUISetup(
        int &drawId,
        QList<BlendEffect::UIDelayed> &delayed) const {
    const qreal relFrame = anim_getCurrentRelFrame();
    const int iMax = ca_getNumberOfChildren();
    for(int i = 0; i < iMax; i++) {
        const auto effect = getChild(i);
        effect->detachedBlendUISetup(relFrame, drawId, delayed);
    }
}

void BlendEffectCollection::detachedBlendSetup(
        const BoundingBox* const boxToDraw,
        SkCanvas * const canvas,
        const SkFilterQuality filter, int &drawId,
        QList<BlendEffect::Delayed> &delayed) const {
    const qreal relFrame = anim_getCurrentRelFrame();
    const int iMax = ca_getNumberOfChildren();
    for(int i = 0; i < iMax; i++) {
        const auto effect = getChild(i);
        if(!effect->isVisible()) continue;
        effect->detachedBlendSetup(boxToDraw, relFrame, canvas, filter, drawId, delayed);
    }
}

void BlendEffectCollection::prp_writeProperty_impl(eWriteStream &dst) const {
    BlendEffectCollectionBase::prp_writeProperty_impl(dst);
    dst << SWT_isVisible();
}

void BlendEffectCollection::prp_readProperty_impl(eReadStream &src) {
    if(src.evFileVersion() < 12) return;
    BlendEffectCollectionBase::prp_readProperty_impl(src);
    bool visible; src >> visible;
    SWT_setVisible(visible);
}

qsptr<BlendEffect> createBlendEffectForType(const BlendEffectType type) {
    switch(type) {
        case(BlendEffectType::move):
            return enve::make_shared<MoveBlendEffect>();
        case(BlendEffectType::targeted):
            return enve::make_shared<TargetedBlendEffect>();
        default: RuntimeThrow("Invalid blend effect type '" +
                              QString::number(int(type)) + "'");
    }
}

qsptr<BlendEffect> readIdCreateBlendEffect(eReadStream &src) {
    BlendEffectType type;
    src.read(&type, sizeof(BlendEffectType));
    return createBlendEffectForType(type);
}

void writeBlendEffectType(BlendEffect * const obj, eWriteStream &dst) {
    obj->writeIdentifier(dst);
}

qsptr<BlendEffect> readIdCreateBlendEffectXEV(const QDomElement& ele) {
    const QString typeStr = ele.attribute("type");
    const int typeInt = Friction::Core::XmlExportHelpers::stringToInt(typeStr);
    const auto type = static_cast<BlendEffectType>(typeInt);
    return createBlendEffectForType(type);
}

void writeBlendEffectTypeXEV(BlendEffect* const obj, QDomElement& ele) {
    obj->writeIdentifierXEV(ele);
}
