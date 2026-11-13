#include "blendeffectboxshadow.h"

#include "Boxes/boundingbox.h"
#include "Boxes/pathbox.h"

BlendEffectBoxShadow::BlendEffectBoxShadow(BoundingBox* const box,
                                           BlendEffect * const effect)
    : eBoxOrSound(tr("%1").arg(effect->prp_getName()))
    , mBox(box)
    , mEffect(effect)
{
    const auto clipPath = mEffect->clipPathSource();
    const auto clipProp = mEffect->clipPathProperty();

    if (clipPath && !clipPath->prp_getName().isEmpty()) {
        prp_setName(tr("%1 (%2)").arg(mEffect->prp_getName(),
                                      clipPath->prp_getName()));
    }

    if (clipProp) {
        connect(clipProp, &Property::prp_nameChanged,
                [this](const QString &name) {
            if (!name.isEmpty()) {
                prp_setName(tr("%1 (%2)").arg(mEffect->prp_getName(),
                                              name));
            } else { prp_setName(mEffect->prp_getName()); }
        });
    }

    connect(mEffect, &Property::prp_nameChanged,
            [this](QString name) {
        const auto clipPath = mEffect->clipPathSource();
        if (clipPath && !clipPath->prp_getName().isEmpty()) {
            prp_setName(tr("%1 (%2)").arg(name,
                                          clipPath->prp_getName()));
        } else { prp_setName(name); }
    });

    setVisible(effect->isVisible());

    connect(effect, &eEffect::effectVisibilityChanged,
            this, &eBoxOrSound::setVisible);
    connect(this, &eBoxOrSound::visibilityChanged,
            effect, &eEffect::setVisible);
}

bool BlendEffectBoxShadow::SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                                               const bool parentSatisfies,
                                               const bool parentMainTarget) const
{
    const SWT_BoxRule rule = rules.fRule;
    if (rule == SWT_BoxRule::selected) { return false; }
    return mBox->SWT_shouldBeVisible(rules,
                                     parentSatisfies,
                                     parentMainTarget);
}

void BlendEffectBoxShadow::prp_drawTimelineControls(QPainter * const p,
                                                    const qreal pixelsPerFrame,
                                                    const FrameRange &absFrameRange,
                                                    const int rowHeight)
{
    return mBox->drawDurationRectangle(p,
                                       pixelsPerFrame,
                                       absFrameRange,
                                       rowHeight);
}

qsptr<BlendEffectBoxShadow> BlendEffectBoxShadow::createLink() const
{
    return enve::make_shared<BlendEffectBoxShadow>(mBox, mEffect);
}
