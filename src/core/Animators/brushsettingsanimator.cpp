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

#include "brushsettingsanimator.h"
#include "Paint/brushescontext.h"
#include "paintsettingsanimator.h"

BrushSettingsAnimator::BrushSettingsAnimator() :
    StaticComplexAnimator("brush settings") {
    ca_addChild(mWidthCurve);
    ca_addChild(mPressureCurve);
    ca_addChild(mSpacingCurve);
    ca_addChild(mTimeCurve);

    mWidthCurve->setCurrentValue(qCubicSegment1D(1));
    mPressureCurve->setCurrentValue(qCubicSegment1D(1));
    mSpacingCurve->setCurrentValue(qCubicSegment1D(1));
    mTimeCurve->setCurrentValue(qCubicSegment1D(1));
}

void BrushSettingsAnimator::prp_writeProperty_impl(eWriteStream& dst) const {
    StaticComplexAnimator::prp_writeProperty_impl(dst);
    dst << (mBrush ? mBrush->getCollectionName() : "");
    dst << (mBrush ? mBrush->getBrushName() : "");
}

void BrushSettingsAnimator::prp_readProperty_impl(eReadStream& src) {
    StaticComplexAnimator::prp_readProperty_impl(src);
    QString brushCollection; src >> brushCollection;
    QString brushName; src >> brushName;
    mBrush = BrushCollectionData::sGetBrush(brushCollection, brushName);
}

void BrushSettingsAnimator::prp_readPropertyXML_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    StaticComplexAnimator::prp_readPropertyXML_impl(ele, imp);
    const auto brush = ele.firstChildElement("Brush");
    mBrush = Friction::Core::XmlExportHelpers::brushFromElement(brush);
}

QDomElement BrushSettingsAnimator::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = StaticComplexAnimator::prp_writePropertyXML_impl(exp);
    const auto brush = Friction::Core::XmlExportHelpers::brushToElement(mBrush.get(), exp.doc());
    result.appendChild(brush);
    return result;
}

void BrushSettingsAnimator::setBrush(SimpleBrushWrapper * const brush) {
    if(mBrush == brush) return;
    {
        UndoRedo ur;
        const stdptr<SimpleBrushWrapper> oldValue = mBrush;
        const stdptr<SimpleBrushWrapper> newValue = brush;
        ur.fUndo = [this, oldValue]() {
            setBrush(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setBrush(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mBrush = brush;
    prp_afterWholeInfluenceRangeChanged();
    emit brushChanged(brush);
}

void BrushSettingsAnimator::setupStrokeSettings(const qreal relFrame,
                                                UpdateStrokeSettings &settings) {
    if(!mBrush) return;
    settings.fStrokeBrush = mBrush->createDuplicate();
    settings.fTimeCurve = mTimeCurve->getValueAtRelFrame(relFrame);
    settings.fWidthCurve = mWidthCurve->getValueAtRelFrame(relFrame)*
                           settings.fOutlineWidth;
    settings.fPressureCurve = mPressureCurve->getValueAtRelFrame(relFrame);
    settings.fSpacingCurve = mSpacingCurve->getValueAtRelFrame(relFrame);
}
