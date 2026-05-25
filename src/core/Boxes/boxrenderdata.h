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

#ifndef BOXRENDERDATA_H
#define BOXRENDERDATA_H
#include "skia/skiaincludes.h"

#include <QWeakPointer>
#include "Tasks/updatable.h"
#include "Animators/animator.h"
#include <QMatrix>
class BoundingBox;
class ShaderProgramCallerBase;
#include "smartPointers/ememory.h"
#include "effectsrenderer.h"

class RenderDataCustomizerFunctor;
struct CORE_EXPORT BoxRenderData : public eTask {
    e_OBJECT
protected:
    enum class Step { BOX_IMAGE, EFFECTS };

    BoxRenderData(BoundingBox * const parent);

    virtual void drawSk(SkCanvas * const canvas) = 0;
    virtual void updateRelBoundingRect() = 0;
    virtual void setupRenderData() {}
    virtual void transformRenderCanvas(SkCanvas& canvas) const;
    virtual void copyFrom(BoxRenderData *src);
    virtual void updateGlobalRect();

    HardwareSupport hardwareSupport() const;

    void afterCanceled() {}
    void beforeProcessing(const Hardware hw) final;
    void afterProcessing();
    void afterQued();
    void queTaskNow() final;

    virtual SkColor eraseColor() const {
        return SK_ColorTRANSPARENT;
    }
public:
    virtual void drawOnParentLayer(SkCanvas * const canvas,
                                   SkPaint& paint);
    void drawOnParentLayer(SkCanvas * const canvas);

    virtual QPointF getCenterPosition() {
        return fRelBoundingRect.center();
    }

    void delayDataSet() {
        mDelayDataSet = true;
    }

    bool nextStep();

    void processGpu(QGL33 * const gl, SwitchableContext &context);
    void process();

    stdsptr<BoxRenderData> makeCopy();
    sk_sp<SkImage> requestImageCopy();

    bool fForceRasterize = false;

    uint fBoxStateId = 0;

    QMatrix fResolutionScale;
    QMatrix fScaledTransform;
    QMatrix fInheritedTransform;
    QMatrix fRelTransform;
    QMatrix fTotalTransform;
    QMatrix fRenderTransform;

    bool fRelBoundingRectSet = false;
    QRectF fRelBoundingRect;
    QRect fGlobalRect;
    QList<QRectF> fOtherGlobalRects;
    QRect fMaxBoundsRect;

    QMargins fBaseMargin;

    qreal fOpacity = 1;
    qreal fResolution;
    qreal fRelFrame;

    // for motion blur
    stdptr<BoxRenderData> fMotionBlurTarget;
    // for motion blur

    SkBlendMode fBlendMode = SkBlendMode::kSrcOver;
    const SkFilterQuality fFilterQuality;
    bool fAntiAlias = false;
    bool fUseRenderTransform = false;

    bool fParentIsTarget = true;
    // When true, this render includes a camera transform and must not update
    // mDrawRenderContainer — doing so would double-apply the camera in drawContained.
    bool fCompositionOnly = false;
    qptr<BoundingBox> fParentBox;
    BoundingBox* fBlendEffectIdentifier;
    sk_sp<SkImage> fRenderedImage;

    void dataSet();

    void addEffect(const stdsptr<RasterEffectCaller>& effect) {
        mEffectsRenderer.add(effect);
    }
protected:
    bool hasEffects() const { return !mEffectsRenderer.isEmpty(); }

    void setBaseGlobalRect(const QRectF &baseRectF);

    //! @brief For use with mypaint based outlines
    SkBitmap mBitmap;
    bool mDelayDataSet = false;
    bool mDataSet = false;
private:
    void addImageCopy(const sk_sp<SkImage>& img) {
        mImageCopies << img;
    }

    Step mStep = Step::BOX_IMAGE;
    EffectsRenderer mEffectsRenderer;
    stdptr<BoxRenderData> mCopySource;
    QList<sk_sp<SkImage>> mImageCopies;
};

#endif // BOXRENDERDATA_H
