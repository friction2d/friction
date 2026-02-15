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

#include "boxrenderdata.h"
#include "boundingbox.h"
#include "skia/skiahelpers.h"
#include "efiltersettings.h"
#include "Private/Tasks/taskscheduler.h"
#include "Private/Tasks/gputaskexecutor.h"

BoxRenderData::BoxRenderData(BoundingBox * const parent) :
    fFilterQuality(eFilterSettings::sRender()) {
    fParentBox = parent;
    fBlendEffectIdentifier = parent;
}

void BoxRenderData::transformRenderCanvas(SkCanvas &canvas) const {
    canvas.translate(toSkScalar(-fGlobalRect.x()),
                     toSkScalar(-fGlobalRect.y()));
    canvas.concat(toSkMatrix(fScaledTransform));
}

void BoxRenderData::copyFrom(BoxRenderData *src) {
    mCopySource = src;
    fRelTransform = src->fRelTransform;
    fInheritedTransform = src->fInheritedTransform;
    fTotalTransform = src->fTotalTransform;
    fScaledTransform = src->fScaledTransform;
    fRelFrame = src->fRelFrame;
    fRelBoundingRect = src->fRelBoundingRect;
    fRenderTransform = src->fRenderTransform;
    fAntiAlias = src->fAntiAlias;
    fUseRenderTransform = src->fUseRenderTransform;
    fBlendMode = src->fBlendMode;
    fGlobalRect = src->fGlobalRect;
    fOpacity = src->fOpacity;
    fResolution = src->fResolution;
    fResolutionScale = src->fResolutionScale;
    fRenderedImage = src->requestImageCopy();
    fBoxStateId = src->fBoxStateId;
    mState = eTaskState::finished;
    fRelBoundingRectSet = true;
}

stdsptr<BoxRenderData> BoxRenderData::makeCopy() {
    if(!fParentBox) return nullptr;
    const auto copy = fParentBox->createRenderData();
    copy->copyFrom(this);
    return copy;
}

sk_sp<SkImage> BoxRenderData::requestImageCopy() {
    if(mImageCopies.isEmpty()) return SkiaHelpers::makeCopy(fRenderedImage);
    else return mImageCopies.takeLast();
}

void BoxRenderData::drawOnParentLayer(SkCanvas * const canvas) {
    SkPaint paint;
    if(fUseRenderTransform) paint.setFilterQuality(fFilterQuality);
    drawOnParentLayer(canvas, paint);
}

void BoxRenderData::drawOnParentLayer(SkCanvas * const canvas,
                                      SkPaint& paint) {
    if(isZero4Dec(fOpacity) || !fRenderedImage) return;
    if(fUseRenderTransform) canvas->concat(toSkMatrix(fRenderTransform));
    if(fBlendMode == SkBlendMode::kDstIn ||
       fBlendMode == SkBlendMode::kSrcIn ||
       fBlendMode == SkBlendMode::kDstATop ||
       fBlendMode == SkBlendMode::kModulate ||
       fBlendMode == SkBlendMode::kSrcOut) {
        canvas->save();
        auto rect = SkRect::MakeXYWH(fGlobalRect.x(), fGlobalRect.y(),
                                     fRenderedImage->width(),
                                     fRenderedImage->height());
        rect.inset(1, 1);
        canvas->clipRect(rect, SkClipOp::kDifference, false);
        canvas->clear(SK_ColorTRANSPARENT);
        canvas->restore();
    }
    paint.setAlpha(static_cast<U8CPU>(qRound(fOpacity*2.55)));
    paint.setBlendMode(fBlendMode);
    paint.setAntiAlias(fAntiAlias);
    canvas->drawImage(fRenderedImage, fGlobalRect.x(), fGlobalRect.y(), &paint);
}

void BoxRenderData::processGpu(QGL33 * const gl,
                               SwitchableContext &context) {
    if(mStep == Step::EFFECTS)
        return mEffectsRenderer.processGpu(gl, context, this);
    updateGlobalRect();
    if(isZero4Dec(fOpacity)) return;
    if(fGlobalRect.width() <= 0 || fGlobalRect.height() <= 0) return;

    context.switchToSkia();
    const auto grContext = context.grContext();

    const auto grTex = grContext->createBackendTexture(
                fGlobalRect.width(), fGlobalRect.height(),
                kRGBA_8888_SkColorType, GrMipMapped::kNo,
                GrRenderable::kYes);
    if(!grTex.isValid()) return;
    const auto surf = SkSurface::MakeFromBackendTexture(
                grContext, grTex, kTopLeft_GrSurfaceOrigin, 0,
                kRGBA_8888_SkColorType, nullptr, nullptr);

    const auto canvas = surf->getCanvas();
    transformRenderCanvas(*canvas);
    canvas->clear(eraseColor());
    drawSk(canvas);
    canvas->flush();
    fRenderedImage = SkImage::MakeFromAdoptedTexture(grContext, grTex,
                                                     kTopLeft_GrSurfaceOrigin,
                                                     kRGBA_8888_SkColorType);
    if(mEffectsRenderer.isEmpty() ||
       mEffectsRenderer.nextHardwareSupport() == HardwareSupport::cpuOnly)
        fRenderedImage = fRenderedImage->makeRasterImage();
    else mEffectsRenderer.processGpu(gl, context, this);
//    if(mEffectsRenderer.isEmpty()) return;
//    const auto nextEffectHw = mEffectsRenderer.nextHardwareSupport();
//    if(nextEffectHw != HardwareSupport::cpuOnly) {
//        mEffectsRenderer.processGpu(gl, context, this);
//    }
}
#include "textboxrenderdata.h"
void BoxRenderData::process() {
    if(mStep == Step::EFFECTS) return;
    updateGlobalRect();
    if(isZero4Dec(fOpacity)) return;
    if(fGlobalRect.width() <= 0 || fGlobalRect.height() <= 0) return;

    const auto info = SkiaHelpers::getPremulRGBAInfo(fGlobalRect.width(),
                                                     fGlobalRect.height());
    mBitmap.allocPixels(info);
    mBitmap.eraseColor(eraseColor());
    SkCanvas canvas(mBitmap);
    transformRenderCanvas(canvas);

    drawSk(&canvas);

    fRenderedImage = SkiaHelpers::transferDataToSkImage(mBitmap);
}

void BoxRenderData::beforeProcessing(const Hardware hw) {
    Q_UNUSED(hw)
    Q_ASSERT(mStep != Step::EFFECTS);
    setupRenderData();
    if(!mDataSet) dataSet();
    if(isZero4Dec(fOpacity)) finishedProcessing();
}

void BoxRenderData::afterProcessing() {
    if(fMotionBlurTarget) {
        fMotionBlurTarget->fOtherGlobalRects << fGlobalRect;
    }
    if(fParentBox && fParentIsTarget) {
        fParentBox->renderDataFinished(this);
    } else if(mCopySource) {
        mCopySource->addImageCopy(std::move(fRenderedImage));
    }
}

void BoxRenderData::afterQued() {
    if(mDataSet) return;
    if(!mDelayDataSet) dataSet();
}

#include "Private/esettings.h"

HardwareSupport BoxRenderData::hardwareSupport() const {
    if(mStep == Step::EFFECTS) {
        return mEffectsRenderer.nextHardwareSupport();
    } else {
        if(fParentBox) return fParentBox->hardwareSupport();
        return HardwareSupport::cpuPreffered;
    }
}

void BoxRenderData::queTaskNow() {
    TaskScheduler::instance()->queCpuTask(ref<eTask>());
}

bool BoxRenderData::nextStep() {
    const bool result = !mEffectsRenderer.isEmpty() &&
                        fRenderedImage;
    if(result) {
        mStep = Step::EFFECTS;
        if(hardwareSupport() == HardwareSupport::cpuOnly) {
            mEffectsRenderer.processCpu(this);
        } else {
            GpuTaskExecutor::sAddTask(ref<eTask>());
        }
    }
    return result;
}

void BoxRenderData::dataSet() {
    if(mDataSet) return;
    mDataSet = true;
    if(!fRelBoundingRectSet) {
        fRelBoundingRectSet = true;
        updateRelBoundingRect();
    }
    if(!fParentBox || !fParentIsTarget) return;
    fParentBox->updateCurrentPreviewDataFromRenderData(this);
}

#include "Boxes/textboxrenderdata.h"
void BoxRenderData::updateGlobalRect() {
    fScaledTransform = fTotalTransform*fResolutionScale;
    QRectF baseRectF = fScaledTransform.mapRect(fRelBoundingRect);
    for(const QRectF &rectT : qAsConst(fOtherGlobalRects)) {
        baseRectF = baseRectF.united(rectT);
    }
    baseRectF.adjust(-fBaseMargin.left(), -fBaseMargin.top(),
                     fBaseMargin.right(), fBaseMargin.bottom());
    setBaseGlobalRect(baseRectF);
}

void BoxRenderData::setBaseGlobalRect(const QRectF& baseRectF) {
    const auto clampedBaseRect = baseRectF.intersected(fMaxBoundsRect);
    SkIRect currRect = toSkRect(clampedBaseRect).roundOut();
    if(!mEffectsRenderer.isEmpty()) {
        const SkIRect skMaxBounds = toSkIRect(fMaxBoundsRect);
        mEffectsRenderer.setBaseGlobalRect(currRect, skMaxBounds);
    }
    fGlobalRect = toQRect(currRect);
}
