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

#ifndef PAINTSETTINGSAPPLIER_H
#define PAINTSETTINGSAPPLIER_H
#include "smartPointers/ememory.h"
#include "colorsetting.h"
#include <QPainter>
#include "Boxes/pathbox.h"
#include "Animators/gradient.h"
#include "Animators/outlinesettingsanimator.h"
#include "matrixdecomposition.h"
class ColorAnimator;
class SceneBoundGradient;
class PathBox;
enum PaintType : short;
class PaintSettingsAnimator;
class FillSettingsAnimator;
class OutlineSettingsAnimator;

class CORE_EXPORT PaintSetting {
public:
    enum Target { FILL, OUTLINE };
    virtual ~PaintSetting() {}

    void apply(BoundingBox * const target) const {
        const auto targetSettings = targetPaintSettings(target);
        if(targetSettings) applyToPS(targetSettings);
    }
protected:
    PaintSetting(const Target& target) : mTarget(target) {}
    virtual void applyToPS(PaintSettingsAnimator * const target) const = 0;
private:
    PaintSettingsAnimator * targetPaintSettings(BoundingBox * const target) const {
        if(mTarget == FILL) return target->getFillSettings();
        return target->getStrokeSettings();
    }
    const Target mTarget;
};

class CORE_EXPORT ColorPaintSetting : public PaintSetting {
public:
    ColorPaintSetting(const Target& target,
                      const ColorSetting& colorSettings) :
        PaintSetting(target), mColorSetting(colorSettings) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        mColorSetting.apply(target->getColorAnimator());
    }
private:
    const ColorSetting mColorSetting;
};

class CORE_EXPORT GradientPaintSetting : public PaintSetting {
public:
    GradientPaintSetting(const Target& target,
                         Gradient * const gradient) :
        PaintSetting(target), mGradient(gradient) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        target->setGradient(mGradient);
    }
private:
    Gradient * const mGradient;
};

class CORE_EXPORT GradientPtsPosSetting : public PaintSetting {
public:
    GradientPtsPosSetting(const Target& target,
                          const QPointF& pt1, const QPointF& pt2) :
        PaintSetting(target), mPt1(pt1), mPt2(pt2) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        target->setGradientPointsPos(mPt1, mPt2);
    }
private:
    const QPointF mPt1;
    const QPointF mPt2;
};

class CORE_EXPORT GradientTransformSetting : public PaintSetting {
public:
    GradientTransformSetting(const Target& target,
                             const QTransform& trans) :
        PaintSetting(target), mTransform(trans) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        const auto dec = MatrixDecomposition::decompose(mTransform);
        target->setGradientTransform(dec);
    }
private:
    const QTransform mTransform;
};

class CORE_EXPORT GradientTypePaintSetting : public PaintSetting {
public:
    GradientTypePaintSetting(const Target& target,
                             const GradientType type) :
        PaintSetting(target), mGradientType(type) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        target->setGradientType(mGradientType);
    }
private:
    const GradientType mGradientType;
};

class CORE_EXPORT PaintTypePaintSetting : public PaintSetting {
public:
    PaintTypePaintSetting(const Target& target,
                          const PaintType type) :
        PaintSetting(target), mPaintType(type) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        target->setPaintType(mPaintType);
    }
private:
    const PaintType mPaintType;
};

class CORE_EXPORT OutlineWidthPaintSetting : public PaintSetting {
public:
    OutlineWidthPaintSetting(const qreal width) :
        PaintSetting(OUTLINE), mWidth(width) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        static_cast<OutlineSettingsAnimator*>(target)->setCurrentStrokeWidth(mWidth);
    }
private:
    const qreal mWidth;
};

class CORE_EXPORT StrokeBrushPaintSetting : public PaintSetting {
public:
    StrokeBrushPaintSetting(SimpleBrushWrapper * const brush) :
        PaintSetting(OUTLINE), mBrush(brush) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        static_cast<OutlineSettingsAnimator*>(target)->setStrokeBrush(mBrush);
    }
private:
    SimpleBrushWrapper * const mBrush;
};

class CORE_EXPORT StrokeWidthCurvePaintSetting : public PaintSetting {
public:
    StrokeWidthCurvePaintSetting(const qCubicSegment1D& widthCurve) :
        PaintSetting(OUTLINE), mWidthCurve(widthCurve) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        static_cast<OutlineSettingsAnimator*>(target)->setStrokeBrushWidthCurve(mWidthCurve);
    }
private:
    const qCubicSegment1D mWidthCurve;
};

class CORE_EXPORT StrokePressureCurvePaintSetting : public PaintSetting {
public:
    StrokePressureCurvePaintSetting(const qCubicSegment1D& pressureCurve) :
        PaintSetting(OUTLINE), mPressureCurve(pressureCurve) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        static_cast<OutlineSettingsAnimator*>(target)->setStrokeBrushPressureCurve(mPressureCurve);
    }
private:
    const qCubicSegment1D mPressureCurve;
};

class CORE_EXPORT StrokeTimeCurvePaintSetting : public PaintSetting {
public:
    StrokeTimeCurvePaintSetting(const qCubicSegment1D& timeCurve) :
        PaintSetting(OUTLINE), mTimeCurve(timeCurve) {}
protected:
    void applyToPS(PaintSettingsAnimator * const target) const {
        static_cast<OutlineSettingsAnimator*>(target)->setStrokeBrushTimeCurve(mTimeCurve);
    }
private:
    const qCubicSegment1D mTimeCurve;
};

class CORE_EXPORT PaintSettingsApplier {
public:
    inline PaintSettingsApplier &operator<< (const stdsptr<PaintSetting> &t)
    { mSettings << t; return *this; }

    void apply(BoundingBox * const target) const {
        for(const auto& setting : mSettings)
            setting->apply(target);
    }
private:
    QList<stdsptr<PaintSetting>> mSettings;
};

#endif // PAINTSETTINGSAPPLIER_H
