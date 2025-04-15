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

#include "Boxes/pathbox.h"

#include "MovablePoints/gradientpoint.h"
#include "Animators/gradientpoints.h"
#include "skia/skiaincludes.h"
#include "PathEffects/patheffect.h"
#include "PathEffects/patheffectcollection.h"
#include "canvas.h"
#include "Animators/transformanimator.h"
#include "paintsettingsapplier.h"
#include "Animators/gradient.h"
#include "RasterEffects/rastereffectcollection.h"
#include "Animators/outlinesettingsanimator.h"
#include "PathEffects/patheffectstask.h"
#include "Private/Tasks/taskscheduler.h"
#include "clipboardcontainer.h"
#include "circle.h"
#include "Boxes/smartvectorpath.h"
#include "themesupport.h"

PathBox::PathBox(const QString &name,
                 const eBoxType type)
    : BoxWithPathEffects(name, type)
{
    connect(this, &eBoxOrSound::parentChanged,
            this, [this]() {
        setPathsOutdated(UpdateReason::userChange);
    });

    connect(mPathEffectsAnimators.get(),
            &Property::prp_currentFrameChanged,
            this, [this](const UpdateReason reason) {
        setPathsOutdated(reason);
    });

    connect(mFillPathEffectsAnimators.get(),
            &Property::prp_currentFrameChanged,
            this, [this](const UpdateReason reason) {
        setFillPathOutdated(reason);
    });

    connect(mOutlineBasePathEffectsAnimators.get(),
            &Property::prp_currentFrameChanged,
            this, [this](const UpdateReason reason) {
        setOutlinePathOutdated(reason);
    });

    connect(mOutlinePathEffectsAnimators.get(),
            &Property::prp_currentFrameChanged,
            this, [this](const UpdateReason reason) {
        setOutlinePathOutdated(reason);
    });

    mFillSettings = enve::make_shared<FillSettingsAnimator>(this);
    mFillGradientPoints = mFillSettings->getGradientPoints();
    mStrokeSettings = enve::make_shared<OutlineSettingsAnimator>(this);
    mStrokeGradientPoints = mFillSettings->getGradientPoints();

    bool fillFlat = eSettings::instance().fLastFillFlatEnabled;
    bool strokeFlat = eSettings::instance().fLastStrokeFlatEnabled;
    if (!fillFlat && !strokeFlat) { strokeFlat = true; }

    switch (type) {
    case eBoxType::circle:
    case eBoxType::rectangle:
        mFillSettings->setPaintType(fillFlat ?
                                    PaintType::FLATPAINT : PaintType::NOPAINT);
        mStrokeSettings->setPaintType(strokeFlat ?
                                      PaintType::FLATPAINT : PaintType::NOPAINT);
        break;
    default:
        mStrokeSettings->setPaintType(PaintType::FLATPAINT);
    }

    mFillSettings->setCurrentColor(eSettings::instance().fLastUsedFillColor);
    mStrokeSettings->setCurrentColor(eSettings::instance().fLastUsedStrokeColor);

    ca_prependChild(mPathEffectsAnimators.get(), mFillSettings);
    ca_prependChild(mPathEffectsAnimators.get(), mStrokeSettings);

    ca_moveChildBelow(mRasterEffectsAnimators.data(),
                      mOutlinePathEffectsAnimators.data());

    const auto lineWidthAnim = mStrokeSettings->getStrokeWidthAnimator();
    connect(lineWidthAnim, &Property::prp_currentFrameChanged,
            this, &PathBox::setOutlinePathOutdated);

    const auto brushSettings = mStrokeSettings->getBrushSettings();
    connect(brushSettings, &BrushSettingsAnimator::brushChanged,
            this, &BoundingBox::brushChanged);

    connect(mFillSettings.get(),
            &PaintSettingsAnimator::paintTypeChanged,
            this, [this](const PaintType &type) {
        switch (getBoxType()) {
        case eBoxType::circle:
        case eBoxType::rectangle:
            eSettings::sInstance->fLastFillFlatEnabled = (type == PaintType::FLATPAINT);
            break;
        default:;
        }
    });
    connect(mStrokeSettings.get(),
            &PaintSettingsAnimator::paintTypeChanged,
            this, [this](const PaintType &type) {
        switch (getBoxType()) {
        case eBoxType::circle:
        case eBoxType::rectangle:
            eSettings::sInstance->fLastStrokeFlatEnabled = (type == PaintType::FLATPAINT);
            break;
        default:;
        }
    });
}

HardwareSupport PathBox::hardwareSupport() const {
    if(!eSettings::sInstance->fPathGpuAcc ||
       mStrokeSettings->getPaintType() == PaintType::BRUSHPAINT) {
        return HardwareSupport::cpuOnly;
    }
    return HardwareSupport::cpuPreffered;
}

void PathBox::setupRenderData(const qreal relFrame, const QMatrix& parentM,
                              BoxRenderData * const data,
                              Canvas* const scene) {
    if(!scene) return;
    BoundingBox::setupRenderData(relFrame, parentM, data, scene);

    bool currentEditPathCompatible = false;
    bool currentPathCompatible = false;
    bool currentOutlinePathCompatible = false;
    bool currentFillPathCompatible = false;

    if(!mCurrentPathsOutdated) {
        const int prevFrame = qFloor(qMin(data->fRelFrame, mCurrentPathsFrame));
        const int nextFrame = qCeil(qMax(data->fRelFrame, mCurrentPathsFrame));

        const bool sameFrame = isZero4Dec(data->fRelFrame - mCurrentPathsFrame);

        currentEditPathCompatible = sameFrame || !differenceInEditPathBetweenFrames(prevFrame, nextFrame);
        if(currentEditPathCompatible) {
            currentPathCompatible = sameFrame || !differenceInPathBetweenFrames(prevFrame, nextFrame);
            if(currentPathCompatible && !mCurrentOutlinePathOutdated) {
                currentOutlinePathCompatible = sameFrame || !differenceInOutlinePathBetweenFrames(prevFrame, nextFrame);
            }
            if(currentPathCompatible && !mCurrentFillPathOutdated) {
                currentFillPathCompatible = sameFrame || !differenceInFillPathBetweenFrames(prevFrame, nextFrame);
            }
        }
    }

    const auto pathData = static_cast<PathBoxRenderData*>(data);
    if(currentEditPathCompatible) {
        pathData->fEditPath = mEditPathSk;
    } else {
        pathData->fEditPath = getRelativePath(relFrame);
    }

    QList<stdsptr<PathEffectCaller>> pathEffects;
    if(currentPathCompatible) {
        pathData->fPath = mPathSk;
    } else {
        if(scene->getPathEffectsVisible())
            addBasePathEffects(relFrame, pathEffects);
        pathData->fPath = pathData->fEditPath;
    }

    QList<stdsptr<PathEffectCaller>> fillEffects;
    if(currentFillPathCompatible) {
        pathData->fFillPath = mFillPathSk;
    } else {
        if(scene->getPathEffectsVisible()) {
            addFillEffects(relFrame, fillEffects);
        }
        if(pathEffects.isEmpty() && fillEffects.isEmpty()) {
            pathData->fFillPath = pathData->fPath;
        }
    }

    QList<stdsptr<PathEffectCaller>> outlineBaseEffects;
    QList<stdsptr<PathEffectCaller>> outlineEffects;
    if(currentOutlinePathCompatible) {
        pathData->fOutlineBasePath = mOutlineBasePathSk;
        pathData->fOutlinePath = mOutlinePathSk;
    } else {
        setupStrokerSettings(pathData, relFrame);

        if(scene->getPathEffectsVisible()) {
            addOutlineBaseEffects(relFrame, outlineBaseEffects);
            addOutlineEffects(relFrame, outlineEffects);
        }

        if(pathEffects.isEmpty() && outlineBaseEffects.isEmpty()) {
            pathData->fOutlineBasePath = pathData->fPath;
            pathData->fStroker.strokePath(pathData->fOutlineBasePath,
                                          &pathData->fOutlinePath);
        }
    }

    if(!pathEffects.isEmpty() || !fillEffects.isEmpty() ||
       !outlineBaseEffects.isEmpty() || !outlineEffects.isEmpty()) {
        const auto pathTask = enve::make_shared<PathEffectsTask>(
                    pathData, std::move(pathEffects), std::move(fillEffects),
                    std::move(outlineBaseEffects), std::move(outlineEffects));
        pathTask->addDependent(pathData);
        pathData->delayDataSet();
        pathTask->queTask();
    }

    if(currentOutlinePathCompatible && currentFillPathCompatible) {
        data->fRelBoundingRectSet = true;
        data->fRelBoundingRect = getRelBoundingRect();
    }
    setupPaintSettings(pathData, relFrame);
}

void PathBox::addPathEffects(
        const qreal relFrame, Canvas* const scene,
        PathEffectsCList& pathEffects,
        PathEffectsCList& fillEffects,
        PathEffectsCList& outlineBaseEffects,
        PathEffectsCList& outlineEffects) {
    if(scene->getPathEffectsVisible()) {
        addBasePathEffects(relFrame, pathEffects);
        addFillEffects(relFrame, fillEffects);
        addOutlineBaseEffects(relFrame, outlineBaseEffects);
        addOutlineEffects(relFrame, outlineEffects);
    }
}

void PathBox::setupStrokerSettings(PathBoxRenderData * const pathData,
                                   const qreal relFrame) {
    mStrokeSettings->setStrokerSettingsForRelFrameSk(
                relFrame, &pathData->fStroker);
}

void PathBox::setupPaintSettings(PathBoxRenderData * const pathData,
                                 const qreal relFrame) {
    UpdatePaintSettings &fillSettings = pathData->fPaintSettings;
    UpdateStrokeSettings &strokeSettings = pathData->fStrokeSettings;

    mFillSettings->setupPaintSettings(relFrame, fillSettings);
    mStrokeSettings->setupStrokeSettings(relFrame, strokeSettings);

    const auto brushSettings = mStrokeSettings->getBrushSettings();
    if(strokeSettings.fPaintType == PaintType::BRUSHPAINT && brushSettings) {
        brushSettings->setupStrokeSettings(relFrame, strokeSettings);
    }
}

void PathBox::duplicatePaintSettingsFrom(
        FillSettingsAnimator * const fillSettings,
        OutlineSettingsAnimator * const strokeSettings) {
    duplicateFillSettingsFrom(fillSettings);
    duplicateStrokeSettingsFrom(strokeSettings);
}

void PathBox::duplicateFillSettingsFrom(
        FillSettingsAnimator * const fillSettings) {
    if(!fillSettings) {
        mFillSettings->setPaintType(NOPAINT);
    } else {
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        eWriteStream writeStream(&buffer);
        fillSettings->prp_writeProperty_impl(writeStream);
        if(buffer.reset()) {
            eReadStream readStream(&buffer);
            mFillSettings->prp_readProperty_impl(readStream);
        }
        buffer.close();
    }
}

void PathBox::duplicateStrokeSettingsFrom(
        OutlineSettingsAnimator * const strokeSettings) {
    if(!strokeSettings) {
        mStrokeSettings->setPaintType(NOPAINT);
    } else {
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        eWriteStream writeStream(&buffer);
        strokeSettings->prp_writeProperty_impl(writeStream);
        if(buffer.reset()) {
            eReadStream readStream(&buffer);
            mStrokeSettings->prp_readProperty_impl(readStream);
        }
        buffer.close();
    }
}

void PathBox::duplicateFillSettingsNotAnimatedFrom(
        FillSettingsAnimator * const fillSettings) {
    mFillSettings->duplicatePaintSettingsNotAnim(fillSettings);
}

void PathBox::duplicateStrokeSettingsNotAnimatedFrom(
        OutlineSettingsAnimator * const strokeSettings) {
    mStrokeSettings->duplicateStrokeSettingsNotAnim(strokeSettings);
}

void PathBox::drawHoveredSk(SkCanvas *canvas, const float invScale) {
    drawHoveredPathSk(canvas, mPathSk, invScale);
}

void PathBox::copyDataToOperationResult(PathBox * const targetBox) const {
    copyRasterEffectsTo(targetBox);
    sWriteReadMember(this, targetBox, &PathBox::mFillSettings);
    sWriteReadMember(this, targetBox, &PathBox::mStrokeSettings);
    sWriteReadMember(this, targetBox, &PathBox::mFillPathEffectsAnimators);
    sWriteReadMember(this, targetBox, &PathBox::mOutlineBasePathEffectsAnimators);
    sWriteReadMember(this, targetBox, &PathBox::mOutlinePathEffectsAnimators);
}

void PathBox::copyPathBoxDataTo(PathBox * const targetBox) const {
    copyBoundingBoxDataTo(targetBox);
    sWriteReadMember(this, targetBox, &PathBox::mFillSettings);
    sWriteReadMember(this, targetBox, &PathBox::mStrokeSettings);
    sWriteReadMember(this, targetBox, &PathBox::mPathEffectsAnimators);
    sWriteReadMember(this, targetBox, &PathBox::mFillPathEffectsAnimators);
    sWriteReadMember(this, targetBox, &PathBox::mOutlineBasePathEffectsAnimators);
    sWriteReadMember(this, targetBox, &PathBox::mOutlinePathEffectsAnimators);
}

bool PathBox::differenceInOutlinePathBetweenFrames(const int frame1, const int frame2) const {
    const auto width = mStrokeSettings->getLineWidthAnimator();
    if(width->prp_differencesBetweenRelFrames(frame1, frame2)) return true;
    return BoxWithPathEffects::differenceInOutlinePathBetweenFrames(frame1, frame2);
}

void PathBox::setPathsOutdated(const UpdateReason reason) {
    mCurrentPathsOutdated = true;
    planUpdate(reason);
}

void PathBox::setOutlinePathOutdated(const UpdateReason reason) {
    mCurrentOutlinePathOutdated = true;
    planUpdate(reason);
}

void PathBox::setFillPathOutdated(const UpdateReason reason) {
    mCurrentFillPathOutdated = true;
    planUpdate(reason);
}

void PathBox::saveFillSettingsSVG(SvgExporter& exp, QDomElement& ele,
                                  const FrameRange& visRange) const {
    mFillSettings->saveSVG(exp, ele, visRange);
}

void PathBox::saveStrokeSettingsSVG(SvgExporter& exp, QDomElement& ele,
                                    const FrameRange& visRange,
                                    const bool asFill) const {
    mStrokeSettings->saveSVG(exp, ele, visRange, asFill);
}

void PathBox::savePathBoxSVG(SvgExporter& exp, QDomElement& ele,
                             const FrameRange& visRange) const {
    saveFillSettingsSVG(exp, ele, visRange);
    saveStrokeSettingsSVG(exp, ele, visRange);
}

SmartVectorPath *PathBox::objectToVectorPathBox() {
    if(enve_cast<SmartVectorPath*>(this)) return nullptr;
    const auto newPath = enve::make_shared<SmartVectorPath>();
    newPath->loadSkPath(mEditPathSk);
    copyPathBoxDataTo(newPath.get());
    getParentGroup()->addContained(newPath);
    return newPath.get();
}

SmartVectorPath *PathBox::strokeToVectorPathBox() {
    if(mOutlinePathSk.isEmpty()) return nullptr;
    const auto newPath = enve::make_shared<SmartVectorPath>();
    newPath->loadSkPath(mOutlinePathSk);
    copyPathBoxDataTo(newPath.get());
    getParentGroup()->addContained(newPath);
    return newPath.get();
}


SkPath PathBox::getParentCoordinatesPath(const qreal relFrame) const {
    SkPath result;
    const auto transform = toSkMatrix(getRelativeTransformAtFrame(relFrame));
    getRelativePath(relFrame).transform(transform, &result);
    return result;
}

SkPath PathBox::getAbsolutePath(const qreal relFrame) const {
    SkPath result;
    const auto transform = toSkMatrix(getTotalTransformAtFrame(relFrame));
    getRelativePath(relFrame).transform(transform, &result);
    return result;
}

SkPath PathBox::getAbsolutePath() const {
    SkPath result;
    const auto transform = toSkMatrix(getTotalTransform());
    mPathSk.transform(transform, &result);
    return result;
}

const SkPath &PathBox::getRelativePath() const { return mPathSk; }

void PathBox::updateCurrentPreviewDataFromRenderData(
        BoxRenderData* renderData) {
    const auto pathRenderData = enve_cast<PathBoxRenderData*>(renderData);
    if(pathRenderData) {
        mCurrentPathsFrame = renderData->fRelFrame;
        mEditPathSk = pathRenderData->fEditPath;
        mPathSk = pathRenderData->fPath;
        mOutlineBasePathSk = pathRenderData->fOutlineBasePath;
        mOutlinePathSk = pathRenderData->fOutlinePath;
        mFillPathSk = pathRenderData->fFillPath;
        mCurrentPathsOutdated = false;
        mCurrentOutlinePathOutdated = false;
        mCurrentFillPathOutdated = false;
    }

    BoundingBox::updateCurrentPreviewDataFromRenderData(renderData);
}

bool PathBox::relPointInsidePath(const QPointF &relPos) const {
    const SkPoint relPosSk = toSkPoint(relPos);
    if(getRelBoundingRectPath().contains(relPosSk.x(), relPosSk.y()) ) {
        if(mFillPathSk.contains(relPosSk.x(), relPosSk.y())) {
            return true;
        }
        return mOutlinePathSk.contains(relPosSk.x(), relPosSk.y());
    } else {
        return false;
    }
}

void PathBox::setOutlineAffectedByScale(const bool bT) {
    mOutlineAffectedByScale = bT;
    planUpdate(UpdateReason::userChange);
}

FillSettingsAnimator *PathBox::getFillSettings() const {
    return mFillSettings.data();
}

OutlineSettingsAnimator *PathBox::getStrokeSettings() const {
    return mStrokeSettings.data();
}
