#include "blendeffect.h"
#include "Boxes/pathbox.h"

BlendEffect::BlendEffect(const QString& name, const BlendEffectType type)
    : eEffect(name)
    , mType(type)
{
    mClipPath = enve::make_shared<BoxTargetProperty>("clip path");
    mClipPath->setValidator<BoundingBox>();

    connect(mClipPath.get(), &BoxTargetProperty::targetSet,
            this, [this](BoundingBox* const newClipBox) {
        if (mClipBox.get()) {
            mClipBox.get()->removeMaskUser(this);
            mClipBox.get()->planUpdate(UpdateReason::userChange);
        }

        auto& conn = mClipBox.assign(newClipBox);
        if (newClipBox) {
            newClipBox->addMaskUser(this);
            newClipBox->planUpdate(UpdateReason::userChange);

            conn << connect(newClipBox, &Property::prp_absFrameRangeChanged,
                            this, &Property::prp_afterChangedAbsRange);
            conn << connect(newClipBox, &eBoxOrSound::visibilityChanged,
                            [this, newClipBox](bool visible) {
                if (visible) { newClipBox->addMaskUser(this); }
                else { newClipBox->removeMaskUser(this); }
                newClipBox->planUpdate(UpdateReason::userChange);
            });
        }
    });

    ca_addChild(mClipPath);
}

BlendEffect::~BlendEffect()
{
    if (mClipBox.get()) {
        mClipBox.get()->removeMaskUser(this);
        mClipBox.get()->planUpdate(UpdateReason::userChange);
    }
}

void BlendEffect::prp_readProperty_impl(eReadStream& src)
{
    if (src.evFileVersion() < 13) {
        StaticComplexAnimator::prp_readProperty_impl(src);
    } else { eEffect::prp_readProperty_impl(src); }
}

void BlendEffect::writeIdentifier(eWriteStream& dst) const
{
    dst.write(&mType, sizeof(BlendEffectType));
}

void BlendEffect::writeIdentifierXEV(QDomElement& ele) const
{
    ele.setAttribute("type", static_cast<int>(mType));
}

void BlendEffect::prp_setupTreeViewMenu(PropertyMenu * const menu)
{
    if (menu->hasActionsForType<BlendEffect>()) { return; }
    menu->addedActionsForType<BlendEffect>();
    {
        const PropertyMenu::PlainSelectedOp<BlendEffect> dOp =
        [this](BlendEffect* const prop) {
            if (mClipBox.get()) {
                mClipBox.get()->removeMaskUser(this);
                mClipBox.get()->planUpdate(UpdateReason::userChange);
            }
            const auto parent = prop->template getParent<
                    DynamicComplexAnimatorBase<BlendEffect>>();
            parent->removeChild(prop->template ref<BlendEffect>());
        };
        menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete"), dOp);
    }

    menu->addSeparator();
    StaticComplexAnimator::prp_setupTreeViewMenu(menu);
}

bool BlendEffect::isPathValid() const
{
    return clipPathSource();
}

SkPath BlendEffect::clipPath(const qreal relFrame) const
{
    const auto target = clipPathSource();
    if (!target) { return SkPath(); }
    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal tRelFrame = target->prp_absFrameToRelFrameF(absFrame);
    return target->getAbsolutePath(tRelFrame);
}

BoxTargetProperty *BlendEffect::clipPathProperty() const
{
    return mClipPath.get();
}

PathBox *BlendEffect::clipPathSource() const
{
    const auto target = mClipPath->getTarget();
    return enve_cast<PathBox*>(target);
}
