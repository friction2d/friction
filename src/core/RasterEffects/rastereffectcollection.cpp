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

#include "rastereffectcollection.h"
#include "RasterEffects/rastereffect.h"
#include "Boxes/boundingbox.h"
#include "Boxes/containerbox.h"
#include "RasterEffects/rastereffectsinclude.h"
#include "RasterEffects/customrastereffectcreator.h"
#include "rastereffectmenucreator.h"

RasterEffectCollection::RasterEffectCollection() :
    RasterEffectCollectionBase("raster effects") {
    ca_setHiddenWhenEmpty(true);

    connect(this, &ComplexAnimator::ca_childAdded,
            this, &RasterEffectCollection::updateMaxForcedMargin);
    connect(this, &ComplexAnimator::ca_childRemoved,
            this, &RasterEffectCollection::updateMaxForcedMargin);
}

void RasterEffectCollection::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<RasterEffectCollection>()) return;
    menu->addedActionsForType<RasterEffectCollection>();
    const auto rasterEffectsMenu = menu->addMenu(QIcon::fromTheme("effect"), "Add Effect");
    RasterEffectMenuCreator::addEffects(
                rasterEffectsMenu, &RasterEffectCollection::addChild);
    menu->addSeparator();
    RasterEffectCollectionBase::prp_setupTreeViewMenu(menu);
}

void RasterEffectCollection::updateMaxForcedMargin() {
    QMargins newMargins;
    const auto& children = ca_getChildren();
    for(const auto& effect : children) {
        const auto rasterEffect = static_cast<RasterEffect*>(effect.get());
        if(!rasterEffect->forceMargin()) continue;
        const auto reMargin = rasterEffect->getMargin();
        newMargins.setTop(qMax(newMargins.top(), reMargin.top()));
        newMargins.setLeft(qMax(newMargins.left(), reMargin.left()));
        newMargins.setBottom(qMax(newMargins.bottom(), reMargin.bottom()));
        newMargins.setRight(qMax(newMargins.right(), reMargin.right()));
    }
    newMargins.setTop(qCeil(newMargins.top()*0.1)*10);
    newMargins.setLeft(qCeil(newMargins.left()*0.1)*10);
    newMargins.setBottom(qCeil(newMargins.bottom()*0.1)*10);
    newMargins.setRight(qCeil(newMargins.right()*0.1)*10);

    const bool changed =
            newMargins.top() > mMaxForcedMargin.top() ||
            newMargins.left() > mMaxForcedMargin.left() ||
            newMargins.bottom() > mMaxForcedMargin.bottom() ||
            newMargins.right() > mMaxForcedMargin.right();

    mMaxForcedMargin = newMargins;

    if(changed) forcedMarginChanged();
}

void RasterEffectCollection::addEffects(const qreal relFrame,
                                       BoxRenderData * const data,
                                       const qreal influence) {
    const bool zeroInfluence = isZero4Dec(influence);
    const auto& children = ca_getChildren();
    for(const auto& effect : children) {
        const auto rEffect = static_cast<RasterEffect*>(effect.get());
        if(!rEffect->isVisible()) continue;
        if(zeroInfluence && rEffect->skipZeroInfluence(relFrame)) continue;
        const auto effectRenderData = rEffect->getEffectCaller(
                    relFrame, data->fResolution, influence, data);
        if(effectRenderData) data->addEffect(effectRenderData);
    }
}

void RasterEffectCollection::updateIfUsesProgram(
        const ShaderEffectProgram * const program) {
    const auto& children = ca_getChildren();
    for(const auto& effect : children) {
        const auto shaderEffect = enve_cast<ShaderEffect*>(effect.get());
        if(!shaderEffect) continue;
        shaderEffect->updateIfUsesProgram(program);
    }
}

QDomElement RasterEffectCollection::saveEffectsSVG(
        SvgExporter& exp, const FrameRange& visRange,
        const QDomElement& child) const {
    QDomElement result = child;
    const auto& children = ca_getChildren();
    for(const auto& effect : children) {
        if(const auto blur = enve_cast<BlurEffect*>(effect.get())) {
            result = blur->saveBlurSVG(exp, visRange, result);
        } else if(const auto shadow = enve_cast<ShadowEffect*>(effect.get())) {
            result = shadow->saveShadowSVG(exp, visRange, result);
        }
    }
    return result;
}

bool RasterEffectCollection::hasEffects() {
    return ca_hasChildren();
}

#include "GUI/dialogsinterface.h"

qsptr<ShaderEffect> createShaderEffect(const ShaderEffectCreator::Identifier id) {
    const auto best = ShaderEffectCreator::sGetBestCompatibleEffects(id);
    if(best.isEmpty()) RuntimeThrow("No compatible ShaderEffect found for " + id.fName);
    std::shared_ptr<ShaderEffectCreator> creator;
    if(best.count() == 1) {
        creator = best.first();
    } else if(!best.isEmpty()) {
        creator = DialogsInterface::instance().execShaderChooser(id.fName, best);
    }
    if(creator) return creator->create();
    RuntimeThrow("No compatible ShaderEffect found for " + id.fName);
}

qsptr<ShaderEffect> readIdCreateShaderEffect(eReadStream& src) {
    const auto id = ShaderEffectCreator::sReadIdentifier(src);
    return createShaderEffect(id);
}

qsptr<ShaderEffect> readIdCreateShaderEffectXML(const QDomElement& ele) {
    const auto id = ShaderEffectCreator::sReadIdentifierXML(ele);
    return createShaderEffect(id);
}

#include "customidentifier.h"
#include "RasterEffects/customrastereffectcreator.h"

qsptr<RasterEffect> createRasterEffectForNonCustomType(const RasterEffectType type) {
    switch(type) {
        case(RasterEffectType::BLUR):
            return enve::make_shared<BlurEffect>();
        case(RasterEffectType::SHADOW):
            return enve::make_shared<ShadowEffect>();
        case(RasterEffectType::MOTION_BLUR):
            return enve::make_shared<MotionBlurEffect>();
        case(RasterEffectType::WIPE):
            return enve::make_shared<WipeEffect>();
        case(RasterEffectType::NOISE_FADE):
            return enve::make_shared<NoiseFadeEffect>();
        case(RasterEffectType::COLORIZE):
            return enve::make_shared<ColorizeEffect>();
        case(RasterEffectType::BRIGHTNESS_CONTRAST):
            return enve::make_shared<BrightnessContrastEffect>();
        default: return nullptr;
    }
}

qsptr<RasterEffect> readIdCreateRasterEffect(eReadStream &src) {
    RasterEffectType type;
    src.read(&type, sizeof(RasterEffectType));
    auto result = createRasterEffectForNonCustomType(type);
    if(result) return result;
    if(type == RasterEffectType::CUSTOM) {
        const auto id = CustomIdentifier::sRead(src);
        const auto eff = CustomRasterEffectCreator::sCreateForIdentifier(id);
        if(eff) return eff;
        RuntimeThrow("Unrecognized CustomRasterEffect identifier " + id.toString());
    } else if(type == RasterEffectType::CUSTOM_SHADER) {
        return readIdCreateShaderEffect(src);
    } else RuntimeThrow("Invalid RasterEffect type '" +
                        QString::number(int(type)) + "'");
}

void writeRasterEffectType(RasterEffect * const obj, eWriteStream &dst) {
    obj->writeIdentifier(dst);
}

qsptr<RasterEffect> readIdCreateRasterEffectXML(const QDomElement& ele) {
    const int typeInt = Friction::Core::XmlExportHelpers::stringToInt(ele.attribute("type"));
    const RasterEffectType type = static_cast<RasterEffectType>(typeInt);

    auto result = createRasterEffectForNonCustomType(type);
    if(result) return result;
    if(type == RasterEffectType::CUSTOM) {
        const auto id = CustomIdentifier::sReadXML(ele);
        const auto eff = CustomRasterEffectCreator::sCreateForIdentifier(id);
        if(eff) return eff;
        RuntimeThrow("Unrecognized CustomRasterEffect identifier " + id.toString());
    } else if(type == RasterEffectType::CUSTOM_SHADER) {
        return readIdCreateShaderEffectXML(ele);
    } else RuntimeThrow("Invalid RasterEffect type '" +
                        QString::number(int(type)) + "'");
}

void writeRasterEffectTypeXML(RasterEffect* const obj, QDomElement& ele) {
    obj->writeIdentifierXML(ele);
}
