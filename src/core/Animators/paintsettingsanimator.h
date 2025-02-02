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

#ifndef PAINTSETTINGSANIMATOR_H
#define PAINTSETTINGSANIMATOR_H

#include "Animators/qrealanimator.h"
#include "coloranimator.h"
#include "colorhelpers.h"
#include "skia/skiaincludes.h"
#include "smartPointers/ememory.h"
#include "Animators/sceneboundgradient.h"
#include "Animators/staticcomplexanimator.h"
#include "paintsettings.h"
#include "Paint/simplebrushwrapper.h"
#include "conncontextptr.h"
#include "transformvalues.h"

class PathBox;
class SkStroke;

class GradientPoints;
class AdvancedTransformAnimator;

struct UpdatePaintSettings;

class CORE_EXPORT PaintSettingsAnimator : public ComplexAnimator {
protected:
    PaintSettingsAnimator(const QString &name,
                          BoundingBox * const parent);

    virtual void showHideChildrenBeforeChangingPaintType(
            const PaintType newPaintType);

    QDomElement prp_writePropertyXEV_impl(const Friction::Core::XevExporter& exp) const;
    void prp_readPropertyXEV_impl(const QDomElement& ele, const Friction::Core::XevImporter& imp);
public:
    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);

    virtual void setPaintType(const PaintType paintType);

    QColor getColor() const;
    PaintType getPaintType() const;
    Gradient *getGradient() const;
    void setGradient(Gradient *gradient);
    void setCurrentColor(const QColor &color,
                         const bool &history = false);
    ColorAnimator *getColorAnimator();
    void setGradientPointsPos(const QPointF& pt1, const QPointF& pt2);

    void duplicateColorAnimatorFrom(ColorAnimator *source);
    QColor getColor(const qreal relFrame) const;
    GradientType getGradientType() { return mGradientType; }
    void setGradientType(const GradientType type);

    QMatrix getGradientTransform(const qreal relFrame) const;
    void setGradientTransform(const TransformValues& transform);

    GradientPoints* getGradientPoints() const
    { return mGradientPoints.get(); }

    void setupPaintSettings(const qreal relFrame,
                            UpdatePaintSettings& settings);
    void duplicatePaintSettingsNotAnim(PaintSettingsAnimator * const settings);
    void applyTransform(const QMatrix& transform);
protected:
    void saveSVG(SvgExporter& exp,
                 QDomElement& parent,
                 const FrameRange& visRange,
                 const QString& name) const;
    virtual QDomElement writeBrushPaint(const Friction::Core::XevExporter& exp) const
    { Q_UNUSED(exp) return QDomElement(); }
    virtual void readBrushPaint(const QDomElement& ele,
                                const Friction::Core::XevImporter& imp)
    { Q_UNUSED(ele) Q_UNUSED(imp) }
private:
    void setGradientVar(Gradient * const grad);
    void resetGradientPoints();
    void updateGradientPoint();

    GradientType mGradientType = GradientType::LINEAR;
    PaintType mPaintType = NOPAINT;

    BoundingBox * const mTarget_k;
    qsptr<GradientPoints> mGradientPoints;
    qsptr<ColorAnimator> mColor = enve::make_shared<ColorAnimator>();
    ConnContextQPtr<Gradient> mGradient;
    qsptr<AdvancedTransformAnimator> mGradientTransform;
};

class CORE_EXPORT FillSettingsAnimator : public PaintSettingsAnimator {
    e_OBJECT
protected:
    FillSettingsAnimator(BoundingBox * const parent) :
        PaintSettingsAnimator("fill", parent) {}
public:
    void saveSVG(SvgExporter& exp, QDomElement& parent,
                 const FrameRange& visRange) const {
        PaintSettingsAnimator::saveSVG(exp, parent, visRange, "fill");
    }
};

struct CORE_EXPORT UpdatePaintSettings {
    UpdatePaintSettings(const QColor &paintColorT,
                        const PaintType paintTypeT);
    UpdatePaintSettings();

    virtual ~UpdatePaintSettings();

    void applyPainterSettingsSk(SkPaint& paint, const float opacity = 1.f);

    void updateGradient(const QGradientStops &stops,
                        const QPointF &start,
                        const QPointF &finalStop,
                        const GradientType gradientType,
                        const QMatrix &transform);
    PaintType fPaintType;
    QColor fPaintColor;
    sk_sp<SkShader> fGradient;
};

struct CORE_EXPORT UpdateStrokeSettings : UpdatePaintSettings {
    UpdateStrokeSettings(
            const qreal width,
            const QColor &paintColorT,
            const PaintType paintTypeT,
            const QPainter::CompositionMode &outlineCompositionModeT);
    UpdateStrokeSettings();

    QPainter::CompositionMode fOutlineCompositionMode =
            QPainter::CompositionMode_Source;

    qreal fOutlineWidth;
    stdsptr<SimpleBrushWrapper> fStrokeBrush;
    qCubicSegment1D fTimeCurve;
    qCubicSegment1D fPressureCurve;
    qCubicSegment1D fWidthCurve;
    qCubicSegment1D fSpacingCurve;
};

#endif // PAINTSETTINGSANIMATOR_H
