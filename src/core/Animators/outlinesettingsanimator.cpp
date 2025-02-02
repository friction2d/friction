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

#include "outlinesettingsanimator.h"
#include "Boxes/pathbox.h"
#include "Private/document.h"

OutlineSettingsAnimator::OutlineSettingsAnimator(BoundingBox * const parent) :
    PaintSettingsAnimator("outline", parent) {

    ca_addChild(mLineWidth);
}

void OutlineSettingsAnimator::prp_writeProperty_impl(eWriteStream& dst) const {
    PaintSettingsAnimator::prp_writeProperty_impl(dst);
    mLineWidth->prp_writeProperty_impl(dst);
    mBrushSettings->prp_writeProperty_impl(dst);
    dst.write(&mCapStyle, sizeof(SkPaint::Cap));
    dst.write(&mJoinStyle, sizeof(SkPaint::Join));
    dst.write(&mOutlineCompositionMode, sizeof(QPainter::CompositionMode));
}

void OutlineSettingsAnimator::prp_readProperty_impl(eReadStream& src) {
    PaintSettingsAnimator::prp_readProperty_impl(src);
    mLineWidth->prp_readProperty_impl(src);
    mBrushSettings->prp_readProperty_impl(src);
    src.read(&mCapStyle, sizeof(SkPaint::Cap));
    src.read(&mJoinStyle, sizeof(SkPaint::Join));
    src.read(&mOutlineCompositionMode, sizeof(QPainter::CompositionMode));
}

QDomElement OutlineSettingsAnimator::prp_writePropertyXEV_impl(const Friction::Core::XevExporter& exp) const {
    auto props = PaintSettingsAnimator::prp_writePropertyXEV_impl(exp);

    const auto lineWidth = mLineWidth->prp_writeNamedPropertyXEV("Width", exp);
    props.appendChild(lineWidth);

    switch(mCapStyle) {
    case SkPaint::kButt_Cap:
        props.setAttribute("stroke-linecap", "butt"); break;
    case SkPaint::kRound_Cap:
        props.setAttribute("stroke-linecap", "round"); break;
    default: // SkPaint::kSquare_Cap
        props.setAttribute("stroke-linecap", "square"); break;
    }

    switch(mJoinStyle) {
    case SkPaint::kMiter_Join:
        props.setAttribute("stroke-linejoin", "miter"); break;
    case SkPaint::kRound_Join:
        props.setAttribute("stroke-linejoin", "round"); break;
    default: // SkPaint::kBevel_Join
        props.setAttribute("stroke-linejoin", "bevel"); break;
    }

    return props;
}

void OutlineSettingsAnimator::prp_readPropertyXEV_impl(const QDomElement& ele,
                                                  const Friction::Core::XevImporter& imp) {
    PaintSettingsAnimator::prp_readPropertyXEV_impl(ele, imp);

    const auto lineWidth = ele.firstChildElement("Width");
    mLineWidth->prp_readPropertyXEV(lineWidth, imp);

    const auto capStyle = ele.attribute("stroke-linecap");
    if(capStyle == "butt") mCapStyle = SkPaint::kButt_Cap;
    else if(capStyle == "round") mCapStyle = SkPaint::kRound_Cap;
    else /*if(capStyle == "square")*/ mCapStyle = SkPaint::kSquare_Cap;

    const auto joinStyle = ele.attribute("stroke-linejoin");
    if(capStyle == "miter") mJoinStyle = SkPaint::kMiter_Join;
    else if(capStyle == "round") mJoinStyle = SkPaint::kRound_Join;
    else /*if(capStyle == "bevel")*/ mJoinStyle = SkPaint::kBevel_Join;
}

void OutlineSettingsAnimator::setPaintType(const PaintType paintType) {
    PaintSettingsAnimator::setPaintType(paintType);
    const auto defaultBrush = Document::sInstance->fOutlineBrush;
    if(paintType == BRUSHPAINT && !mBrushSettings->getBrush()) {
        mBrushSettings->setBrush(defaultBrush);
    }
}

void OutlineSettingsAnimator::showHideChildrenBeforeChangingPaintType(
        const PaintType newPaintType) {
    PaintSettingsAnimator::showHideChildrenBeforeChangingPaintType(newPaintType);
    if(getPaintType() == BRUSHPAINT) ca_removeChild(mBrushSettings);
    if(newPaintType == BRUSHPAINT) ca_addChild(mBrushSettings);
}

void OutlineSettingsAnimator::strokeWidthAction(const QrealAction& action) {
    action.apply(mLineWidth.get());
}

void OutlineSettingsAnimator::setCurrentStrokeWidth(const qreal newWidth) {
    mLineWidth->setCurrentBaseValue(newWidth);
}

void OutlineSettingsAnimator::setCapStyle(const SkPaint::Cap capStyle) {
    if(mCapStyle == capStyle) return;
    {
        UndoRedo ur;
        const auto oldValue = mCapStyle;
        const auto newValue = capStyle;
        ur.fUndo = [this, oldValue]() {
            setCapStyle(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setCapStyle(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mCapStyle = capStyle;
    prp_afterWholeInfluenceRangeChanged();

    // workaround!
    // we need to trigger a value changed to apply style
    // a better solution is welcome ;)
    qreal baseValue = mLineWidth->getCurrentBaseValue();
    mLineWidth->setCurrentBaseValue(baseValue + 0.1);
    mLineWidth->setCurrentBaseValue(baseValue);
}

void OutlineSettingsAnimator::setJoinStyle(const SkPaint::Join joinStyle) {
    if(mJoinStyle == joinStyle) return;
    {
        UndoRedo ur;
        const auto oldValue = mJoinStyle;
        const auto newValue = joinStyle;
        ur.fUndo = [this, oldValue]() {
            setJoinStyle(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setJoinStyle(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mJoinStyle = joinStyle;
    prp_afterWholeInfluenceRangeChanged();

    // workaround!
    // we need to trigger a value changed to apply style
    // a better solution is welcome ;)
    qreal baseValue = mLineWidth->getCurrentBaseValue();
    mLineWidth->setCurrentBaseValue(baseValue + 0.1);
    mLineWidth->setCurrentBaseValue(baseValue);
}

void OutlineSettingsAnimator::setStrokerSettingsSk(SkStroke * const stroker) {
    stroker->setWidth(toSkScalar(mLineWidth->getEffectiveValue()));
    stroker->setCap(mCapStyle);
    stroker->setJoin(mJoinStyle);
}

void OutlineSettingsAnimator::setStrokerSettingsForRelFrameSk(
        const qreal relFrame, SkStroke * const stroker) {
    const qreal widthT = mLineWidth->getEffectiveValue(relFrame);
    stroker->setWidth(toSkScalar(widthT));
    stroker->setCap(mCapStyle);
    stroker->setJoin(mJoinStyle);
}

void OutlineSettingsAnimator::duplicateStrokeSettingsNotAnim(
        OutlineSettingsAnimator * const settings) {
    duplicatePaintSettingsNotAnim(settings);
    if(settings) {
        const qreal width = settings->getCurrentStrokeWidth();
        mLineWidth->setCurrentBaseValue(width);
    }
}

void OutlineSettingsAnimator::saveSVG(SvgExporter& exp,
                                      QDomElement& parent,
                                      const FrameRange& visRange,
                                      const bool asFill) const
{
    PaintSettingsAnimator::saveSVG(exp, parent, visRange,
                                   asFill ? "fill" : "stroke");
    if (asFill) { return; }

    mLineWidth->saveQrealSVG(exp, parent, visRange, "stroke-width");
    saveCapsSVG(parent);
}

void OutlineSettingsAnimator::saveCapsSVG(QDomElement &parent) const
{
    QString linecap;
    switch(mCapStyle) {
    case SkPaint::kRound_Cap:
        linecap = "round";
        break;
    case SkPaint::kSquare_Cap:
        linecap = "square";
        break;
    default:
        linecap = "butt";
    }
    parent.setAttribute("stroke-linecap", linecap);

    QString linejoin;
    switch(mJoinStyle) {
    case SkPaint::kRound_Join:
        linejoin = "round";
        break;
    case SkPaint::kBevel_Join:
        linejoin = "bevel";
        break;
    default:
        linejoin = "miter";
    }
    parent.setAttribute("stroke-linejoin", linejoin);
}

QDomElement OutlineSettingsAnimator::writeBrushPaint(const Friction::Core::XevExporter& exp) const {
    return mBrushSettings->prp_writePropertyXEV(exp);
}

void OutlineSettingsAnimator::readBrushPaint(const QDomElement& ele,
                                             const Friction::Core::XevImporter& imp) {
    const auto brushSettings = ele.firstChildElement("BrushSettings");
    mBrushSettings->prp_readPropertyXEV(brushSettings, imp);
}

void OutlineSettingsAnimator::setupStrokeSettings(const qreal relFrame,
                                                  UpdateStrokeSettings &settings) {
    setupPaintSettings(relFrame, settings);
    settings.fOutlineWidth = mLineWidth->getEffectiveValue(relFrame);
}

qreal OutlineSettingsAnimator::getCurrentStrokeWidth() const {
    return mLineWidth->getEffectiveValue();
}

SkPaint::Cap OutlineSettingsAnimator::getCapStyle() const {
    return mCapStyle;
}

SkPaint::Join OutlineSettingsAnimator::getJoinStyle() const {
    return mJoinStyle;
}

QrealAnimator *OutlineSettingsAnimator::getStrokeWidthAnimator() {
    return mLineWidth.data();
}

void OutlineSettingsAnimator::setOutlineCompositionMode(
        const QPainter::CompositionMode compositionMode) {
    mOutlineCompositionMode = compositionMode;
}

QPainter::CompositionMode OutlineSettingsAnimator::getOutlineCompositionMode() {
    return mOutlineCompositionMode;
}

bool OutlineSettingsAnimator::nonZeroLineWidth() {
    return !isZero4Dec(mLineWidth->getEffectiveValue());
}

QrealAnimator *OutlineSettingsAnimator::getLineWidthAnimator() {
    return mLineWidth.data();
}

