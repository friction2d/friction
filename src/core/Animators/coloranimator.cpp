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

#include "Animators/coloranimator.h"
#include "colorhelpers.h"
#include "pointtypemenu.h"
#include "svgexporter.h"

ColorAnimator::ColorAnimator(const QString &name) : StaticComplexAnimator(name) {
    setColorMode(ColorMode::rgb);
    mAlphaAnimator->setCurrentBaseValue(1);
    ca_addChild(mVal1Animator);
    ca_addChild(mVal2Animator);
    ca_addChild(mVal3Animator);
    ca_addChild(mAlphaAnimator);

    const auto emitColorChange = [this]() {
        emit colorChanged(getColor());
    };

    connect(mVal1Animator.get(), &QrealAnimator::effectiveValueChanged,
            this, emitColorChange);
    connect(mVal2Animator.get(), &QrealAnimator::effectiveValueChanged,
            this, emitColorChange);
    connect(mVal3Animator.get(), &QrealAnimator::effectiveValueChanged,
            this, emitColorChange);
    connect(mAlphaAnimator.get(), &QrealAnimator::effectiveValueChanged,
            this, emitColorChange);
}

QJSValue toArray(QJSEngine& e, const QColor& value) {
    auto array = e.newArray(4);
    array.setProperty(0, value.redF());
    array.setProperty(1, value.greenF());
    array.setProperty(2, value.blueF());
    array.setProperty(3, value.alphaF());
    return array;
}

QJSValue ColorAnimator::prp_getBaseJSValue(QJSEngine& e) const {
    return toArray(e, getBaseColor());
}

QJSValue ColorAnimator::prp_getBaseJSValue(QJSEngine& e, const qreal relFrame) const {
    return toArray(e, getBaseColor(relFrame));
}

QJSValue ColorAnimator::prp_getEffectiveJSValue(QJSEngine& e) const {
    return toArray(e, getColor());
}

QJSValue ColorAnimator::prp_getEffectiveJSValue(QJSEngine& e, const qreal relFrame) const {
    return toArray(e, getColor(relFrame));
}

void ColorAnimator::prp_writeProperty_impl(eWriteStream& dst) const {
    StaticComplexAnimator::prp_writeProperty_impl(dst);
    dst.write(&mColorMode, sizeof(ColorMode));
}

void ColorAnimator::prp_readProperty_impl(eReadStream& src) {
    StaticComplexAnimator::prp_readProperty_impl(src);
    src.read(&mColorMode, sizeof(ColorMode));
    setColorMode(mColorMode);
}

QColor valuesToColor(const qreal val1, const qreal val2,
                     const qreal val3, const qreal alpha,
                     const ColorMode colorMode) {
    QColor color;
    switch(colorMode) {
    case ColorMode::rgb: {
        color.setRgbF(val1, val2, val3, alpha);
    } break;
    case ColorMode::hsv: {
        color.setHsvF(val1, val2, val3, alpha);
    } break;
    case ColorMode::hsl: {
        color.setHslF(val1, val2, val3, alpha);
    } break;
    }
    return color;
}

QColor ColorAnimator::getBaseColor() const {
    const qreal val1 = mVal1Animator->getCurrentBaseValue();
    const qreal val2 = mVal2Animator->getCurrentBaseValue();
    const qreal val3 = mVal3Animator->getCurrentBaseValue();
    const qreal alpha = mAlphaAnimator->getCurrentBaseValue();

    return valuesToColor(val1, val2, val3, alpha, mColorMode);
}

QColor ColorAnimator::getBaseColor(const qreal relFrame) const {
    const qreal val1 = mVal1Animator->getBaseValue(relFrame);
    const qreal val2 = mVal2Animator->getBaseValue(relFrame);
    const qreal val3 = mVal3Animator->getBaseValue(relFrame);
    const qreal alpha = mAlphaAnimator->getBaseValue(relFrame);

    return valuesToColor(val1, val2, val3, alpha, mColorMode);
}

QColor ColorAnimator::getColor() const {
    const qreal val1 = mVal1Animator->getEffectiveValue();
    const qreal val2 = mVal2Animator->getEffectiveValue();
    const qreal val3 = mVal3Animator->getEffectiveValue();
    const qreal alpha = mAlphaAnimator->getEffectiveValue();

    return valuesToColor(val1, val2, val3, alpha, mColorMode);
}

QColor ColorAnimator::getColor(const qreal relFrame) const {
    const qreal val1 = mVal1Animator->getEffectiveValue(relFrame);
    const qreal val2 = mVal2Animator->getEffectiveValue(relFrame);
    const qreal val3 = mVal3Animator->getEffectiveValue(relFrame);
    const qreal alpha = mAlphaAnimator->getEffectiveValue(relFrame);

    return valuesToColor(val1, val2, val3, alpha, mColorMode);
}

void ColorAnimator::setColor(const QColor &col) {
    qreal val1, val2, val3;
    const qreal alpha = col.alphaF();
    if(mColorMode == ColorMode::rgb) {
        val1 = col.redF();
        val2 = col.greenF();
        val3 = col.blueF();
    } else if(mColorMode == ColorMode::hsv) {
        val1 = col.hueF();
        val2 = col.hsvSaturationF();
        val3 = col.valueF();
    } else { // HSLMODE
        val1 = col.hueF();
        val2 = col.hslSaturationF();
        val3 = col.lightnessF();
    }

    mVal1Animator->setCurrentBaseValue(val1);
    mVal2Animator->setCurrentBaseValue(val2);
    mVal3Animator->setCurrentBaseValue(val3);
    mAlphaAnimator->setCurrentBaseValue(alpha);
}

void ColorAnimator::setColorMode(const ColorMode colorMode)
{
    if (colorMode == ColorMode::rgb) {
        mVal1Animator->prp_setName("red");
        mVal2Animator->prp_setName("green");
        mVal3Animator->prp_setName("blue");
    } else if (colorMode == ColorMode::hsv) {
        mVal1Animator->prp_setName("hue");
        mVal2Animator->prp_setName("saturation");
        mVal3Animator->prp_setName("value");
    } else { // HSL
        mVal1Animator->prp_setName("hue");
        mVal2Animator->prp_setName("saturation");
        mVal3Animator->prp_setName("lightness");
    }
    if (mColorMode == colorMode) { return; }

    void (*foo)(qreal&, qreal&, qreal&);
    if (mColorMode == ColorMode::rgb && colorMode == ColorMode::hsv) {
        foo = &qrgb_to_hsv;
    } else if (mColorMode == ColorMode::rgb && colorMode == ColorMode::hsl) {
        foo = &qrgb_to_hsl;
    } else if (mColorMode == ColorMode::hsv && colorMode == ColorMode::rgb) {
        foo = &qhsv_to_rgb;
    } else if (mColorMode == ColorMode::hsv && colorMode == ColorMode::hsl) {
        foo = &qhsv_to_hsl;
    } else if (mColorMode == ColorMode::hsl && colorMode == ColorMode::rgb) {
        foo = &qhsl_to_rgb;
    } else if (mColorMode == ColorMode::hsl && colorMode == ColorMode::hsv) {
        foo = &qhsl_to_hsv;
    } else {
        return;
    }

    const auto& keys = anim_getKeys();
    for (const auto &key : keys) {
        const int frame = key->getAbsFrame();

        qreal rF = mVal1Animator->getBaseValueAtAbsFrame(frame);
        qreal gF = mVal2Animator->getBaseValueAtAbsFrame(frame);
        qreal bF = mVal3Animator->getBaseValueAtAbsFrame(frame);

        foo(rF, gF, bF);

        mVal1Animator->saveValueToKey(frame, rF);
        mVal2Animator->saveValueToKey(frame, gF);
        mVal3Animator->saveValueToKey(frame, bF);
    }

    if (!keys.isEmpty()) {
        mVal1Animator->anim_setRecordingWithoutChangingKeys(true);
        mVal2Animator->anim_setRecordingWithoutChangingKeys(true);
        mVal3Animator->anim_setRecordingWithoutChangingKeys(true);
    } else {
        qreal crF = mVal1Animator->getCurrentBaseValue();
        qreal cgF = mVal2Animator->getCurrentBaseValue();
        qreal cbF = mVal3Animator->getCurrentBaseValue();

        foo(crF, cgF, cbF);

        mVal1Animator->setCurrentBaseValue(crF);
        mVal2Animator->setCurrentBaseValue(cgF);
        mVal3Animator->setCurrentBaseValue(cbF);
    }

    mColorMode = colorMode;

    emit colorModeChanged(mColorMode);
}

void ColorAnimator::startVal1Transform() {
    mVal1Animator->prp_startTransform();
}

void ColorAnimator::startVal2Transform() {
    mVal2Animator->prp_startTransform();
}

void ColorAnimator::startVal3Transform() {
    mVal3Animator->prp_startTransform();
}

void ColorAnimator::startAlphaTransform() {
    mAlphaAnimator->prp_startTransform();
}

void ColorAnimator::setCurrentVal1Value(const qreal val1) {
    mVal1Animator->setCurrentBaseValue(val1);
}

void ColorAnimator::setCurrentVal2Value(const qreal val2) {
    mVal2Animator->setCurrentBaseValue(val2);
}

void ColorAnimator::setCurrentVal3Value(const qreal val3) {
    mVal3Animator->setCurrentBaseValue(val3);
}

void ColorAnimator::setCurrentAlphaValue(const qreal alpha) {
    mAlphaAnimator->setCurrentBaseValue(alpha);
}

void ColorAnimator::saveColorSVG(SvgExporter &exp,
                                 QDomElement &parent,
                                 const FrameRange &visRange,
                                 const QString &name,
                                 bool rgba,
                                 bool a) const
{
    qDebug() << "save color for SVG" << name;
    Animator::saveSVG(exp, parent, visRange, name,
                      [this, &rgba, &a, &exp, &name, &parent](const qreal relFrame) {
        const auto color = getColor(relFrame);
        if (exp.fColors11) {
            if (color.alphaF() != 1. && (name == "fill" || name == "stroke" || name == "stop-color")) {
                QString type = QString("%1-opacity").arg(name == "stop-color" ? "stop" : name);
                parent.setAttribute(type, QString::number(color.alphaF()));
            }
            return color.name();
        }
        if (a) { return QString::number(color.alphaF()); }
        if (!rgba) { return color.name(); }
        return QString("rgba(%1, %2, %3, %4)").arg(QString::number(color.red()),
                                                   QString::number(color.green()),
                                                   QString::number(color.blue()),
                                                   QString::number(color.alphaF()));
    });
}

void ColorAnimator::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    const auto colorModeMenu = menu->addMenu(QIcon::fromTheme("color"), tr("Color Mode"));

    const PropertyMenu::CheckSelectedOp<ColorAnimator> rgbOp =
    [](ColorAnimator * anim, bool checked) {
        Q_UNUSED(checked)
        anim->setColorMode(ColorMode::rgb);
    };
    colorModeMenu->addCheckableAction("RGB", mColorMode == ColorMode::rgb, rgbOp);

    const PropertyMenu::CheckSelectedOp<ColorAnimator> hsvOp =
    [](ColorAnimator * anim, bool checked) {
        Q_UNUSED(checked)
        anim->setColorMode(ColorMode::hsv);
    };
    colorModeMenu->addCheckableAction("HSV", mColorMode == ColorMode::hsv, hsvOp);

    const PropertyMenu::CheckSelectedOp<ColorAnimator> hslOp =
    [](ColorAnimator * anim, bool checked) {
        Q_UNUSED(checked)
        anim->setColorMode(ColorMode::hsl);
    };
    colorModeMenu->addCheckableAction("HSL", mColorMode == ColorMode::hsl, hslOp);
}

QDomElement ColorAnimator::prp_writePropertyXEV_impl(
        const XevExporter& exp) const {
    auto result = exp.createElement("Color");
    result.setAttribute("mode", static_cast<int>(mColorMode));

    writeChildPropertiesXEV(result, exp);

    return result;
}

void ColorAnimator::prp_readPropertyXEV_impl(
        const QDomElement& ele, const XevImporter& imp) {
    const auto modeStr = ele.attribute("mode", "0");
    const int modeInt = XmlExportHelpers::stringToInt(modeStr);
    setColorMode(static_cast<ColorMode>(modeInt));
    StaticComplexAnimator::prp_readPropertyXEV_impl(ele, imp);
}
