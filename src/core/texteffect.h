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

#ifndef TEXTEFFECT_H
#define TEXTEFFECT_H
#include "Animators/eeffect.h"
#include "Animators/qrealanimator.h"
#include "Properties/comboboxproperty.h"
#include "Animators/transformanimator.h"
#include "RasterEffects/rastereffectcollection.h"
#include "PathEffects/patheffectcollection.h"

class LetterRenderData;
class WordRenderData;
class LineRenderData;
class TextBoxRenderData;
class AnimatedPoint;

enum class TextFragmentType : short;

class CORE_EXPORT TextEffect : public eEffect {
    Q_OBJECT
public:
    TextEffect();

    bool SWT_dropSupport(const QMimeData * const data);
    bool SWT_drop(const QMimeData * const data);
    QMimeData *SWT_createMimeData();

    void prp_setupTreeViewMenu(PropertyMenu * const menu);
    void prp_drawCanvasControls(SkCanvas * const canvas,
                                const CanvasMode mode,
                                const float invScale,
                                const bool ctrlPressed);

    void writeIdentifier(eWriteStream& dst) const
    { Q_UNUSED(dst) }

    void writeIdentifierXML(QDomElement& ele) const
    { Q_UNUSED(ele) }

    void apply(TextBoxRenderData * const textData) const;
    TextFragmentType target() const;

    qreal getGuideLineWidth() const;
    qreal getGuideLineHeight() const;
private:
    QMatrix getTransform(const qreal relFrame,
                         const qreal influence,
                         const QPointF &addPivot) const;

    void applyToLetter(LetterRenderData * const letterData,
                       const qreal influence) const;
    void applyToWord(WordRenderData * const wordData,
                     const qreal influence) const;
    void applyToLine(LineRenderData * const lineData,
                     const qreal influence) const;

    qsptr<QrealAnimator> mInfluence;
    qsptr<ComboBoxProperty> mTarget;
    qsptr<QrealAnimator> mMinInfluence;

    qsptr<StaticComplexAnimator> mDiminishCont;
    qsptr<QrealAnimator> mDiminishInfluence;

    qsptr<QPointFAnimator> mP1Anim;
    qsptr<QPointFAnimator> mP2Anim;
    qsptr<QPointFAnimator> mP3Anim;
    qsptr<QPointFAnimator> mP4Anim;

    stdsptr<AnimatedPoint> mP1Pt;
    stdsptr<AnimatedPoint> mP2Pt;
    stdsptr<AnimatedPoint> mP3Pt;
    stdsptr<AnimatedPoint> mP4Pt;

    qsptr<QrealAnimator> mDiminishSmoothness;

    qsptr<StaticComplexAnimator> mPeriodicCont;
    qsptr<QrealAnimator> mPeriodicInfluence;
    qsptr<QrealAnimator> mPeriod;
    qsptr<QrealAnimator> mPeriodicShift;
    qsptr<QrealAnimator> mPeriodicSmoothness;

    qsptr<AdvancedTransformAnimator> mTransform;

    qsptr<PathEffectCollection> mBasePathEffects;
    qsptr<PathEffectCollection> mFillPathEffects;
    qsptr<PathEffectCollection> mOutlineBasePathEffects;
    qsptr<PathEffectCollection> mOutlinePathEffects;

    qsptr<RasterEffectCollection> mRasterEffects;
};

#endif // TEXTEFFECT_H
