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

#ifndef OUTLINESETTINGSANIMATOR_H
#define OUTLINESETTINGSANIMATOR_H

#include "paintsettingsanimator.h"
#include "Animators/brushsettingsanimator.h"
#include "Private/esettings.h"

class CORE_EXPORT OutlineSettingsAnimator : public PaintSettingsAnimator {
    typedef qCubicSegment1DAnimator::Action SegAction;
    e_OBJECT
protected:
    OutlineSettingsAnimator(BoundingBox * const parent);

    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const;
    void prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp);
public:
    void showHideChildrenBeforeChangingPaintType(const PaintType newPaintType);

    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);

    void setPaintType(const PaintType paintType);

    void strokeWidthAction(const QrealAction& action);
    void setCurrentStrokeWidth(const qreal newWidth);
    void setCapStyle(const SkPaint::Cap capStyle);
    void setJoinStyle(const SkPaint::Join joinStyle);
    void setStrokerSettingsSk(SkStroke * const stroker);

    void applyStrokeBrushWidthAction(const SegAction& action)
    { mBrushSettings->applyWidthAction(action); }

    void applyStrokeBrushPressureAction(const SegAction& action)
    { mBrushSettings->applyPressureAction(action); }

    void applyStrokeBrushSpacingAction(const SegAction& action)
    { mBrushSettings->applySpacingAction(action); }

    void applyStrokeBrushTimeAction(const SegAction& action)
    { mBrushSettings->applyTimeAction(action); }

    void setStrokeBrushSpacingCurve(const qCubicSegment1D& curve) {
        mBrushSettings->setStrokeBrushSpacingCurve(curve);
    }

    void setStrokeBrushPressureCurve(const qCubicSegment1D& curve) {
        mBrushSettings->setStrokeBrushPressureCurve(curve);
    }

    void setStrokeBrushTimeCurve(const qCubicSegment1D& curve) {
        mBrushSettings->setStrokeBrushTimeCurve(curve);
    }

    void setStrokeBrushWidthCurve(const qCubicSegment1D& curve) {
        mBrushSettings->setStrokeBrushWidthCurve(curve);
    }

    void setStrokeBrush(SimpleBrushWrapper* const brush) {
        mBrushSettings->setBrush(brush);
    }

    qreal getCurrentStrokeWidth() const;

    SkPaint::Cap getCapStyle() const;
    SkPaint::Join getJoinStyle() const;

    QrealAnimator *getStrokeWidthAnimator();
    BrushSettingsAnimator *getBrushSettings() {
        return mBrushSettings.get();
    }

    void setOutlineCompositionMode(
            const QPainter::CompositionMode compositionMode);

    QPainter::CompositionMode getOutlineCompositionMode();
    void setLineWidthUpdaterTarget(PathBox * const path);
    bool nonZeroLineWidth();

    QrealAnimator *getLineWidthAnimator();

    void setStrokerSettingsForRelFrameSk(const qreal relFrame,
                                         SkStroke * const stroker);

    void setupStrokeSettings(const qreal relFrame,
                             UpdateStrokeSettings& settings);
    void duplicateStrokeSettingsNotAnim(
            OutlineSettingsAnimator * const settings);

    void saveSVG(SvgExporter& exp,
                 QDomElement& parent,
                 const FrameRange& visRange,
                 const bool asFill = false) const;
    void saveCapsSVG(QDomElement& parent) const;

protected:
    QDomElement writeBrushPaint(const XevExporter& exp) const;
    void readBrushPaint(const QDomElement& ele,
                        const XevImporter& imp);

    QString prp_tagNameXEV() const { return "Outline"; }
private:
    SkPaint::Cap mCapStyle = SkPaint::kRound_Cap;
    SkPaint::Join mJoinStyle = SkPaint::kRound_Join;
    QPainter::CompositionMode mOutlineCompositionMode =
            QPainter::CompositionMode_Source;
    qsptr<BrushSettingsAnimator> mBrushSettings =
            enve::make_shared<BrushSettingsAnimator>();
    qsptr<QrealAnimator> mLineWidth =
            enve::make_shared<QrealAnimator>(eSettings::instance().fLastUsedStrokeWidth,
                                             0, 999, 1, "thickness");
};
#endif // OUTLINESETTINGSANIMATOR_H
