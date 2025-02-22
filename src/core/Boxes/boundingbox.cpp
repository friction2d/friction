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

#include "Boxes/boundingbox.h"
#include "Boxes/containerbox.h"
#include "TransformEffects/followpatheffect.h"
#include "canvas.h"
#include "swt_abstraction.h"
#include "Timeline/durationrectangle.h"
#include "pointhelpers.h"
#include "skia/skqtconversions.h"
#include "GUI/global.h"
#include "MovablePoints/movablepoint.h"
#include "Private/Tasks/taskscheduler.h"
#include "RasterEffects/rastereffectcollection.h"
#include "Animators/transformanimator.h"
#include "RasterEffects/rastereffect.h"
#include "RasterEffects/customrastereffectcreator.h"
#include "internallinkbox.h"
#include "Animators/qpointfanimator.h"
#include "MovablePoints/pathpointshandler.h"
#include "typemenu.h"
#include "patheffectsmenu.h"
#include "RasterEffects/rastereffectsinclude.h"
#include "RasterEffects/rastereffectmenucreator.h"
#include "matrixdecomposition.h"
#include "paintsettingsapplier.h"
#include "Animators/customproperties.h"
#include "GUI/propertynamedialog.h"
#include "BlendEffects/blendeffectcollection.h"
#include "TransformEffects/transformeffectcollection.h"
#include "GUI/dialogsinterface.h"
#include "svgexporter.h"
#include "svgexporthelpers.h"
#include "internallinkcanvas.h"

#include <QInputDialog>
#include <QMessageBox>

int BoundingBox::sNextDocumentId = 0;
QList<BoundingBox*> BoundingBox::sDocumentBoxes;
int BoundingBox::sNextWriteId;
QList<const BoundingBox*> BoundingBox::sBoxesWithWriteIds;

BoundingBox::BoundingBox(const QString& name, const eBoxType type) :
    eBoxOrSound(name),
    mDocumentId(sNextDocumentId++), mType(type),
    mCustomProperties(enve::make_shared<CustomProperties>()),
    mBlendEffectCollection(enve::make_shared<BlendEffectCollection>()),
    mTransformEffectCollection(enve::make_shared<TransformEffectCollection>()),
    mTransformAnimator(enve::make_shared<BoxTransformAnimator>()),
    mRasterEffectsAnimators(enve::make_shared<RasterEffectCollection>()) {
    sDocumentBoxes << this;

    ca_addChild(mCustomProperties);
    mCustomProperties->SWT_setVisible(false);

    mTransformAnimator->ca_addChild(mTransformEffectCollection);
    setSVGPropertiesVisible(false);

    ca_addChild(mBlendEffectCollection);
    mBlendEffectCollection->SWT_hide();

    ca_addChild(mTransformAnimator);
    const auto pivotAnim = mTransformAnimator->getPivotAnimator();
    connect(pivotAnim, &Property::prp_currentFrameChanged,
            this, &BoundingBox::requestGlobalPivotUpdateIfSelected);

    ca_addChild(mRasterEffectsAnimators);
    mRasterEffectsAnimators->SWT_hide();

    connect(mTransformAnimator.get(),
            &BoxTransformAnimator::totalTransformChanged,
            this, &BoundingBox::afterTotalTransformChanged);
    connect(this, &eBoxOrSound::parentChanged, this, [this]() {
        const auto parent = getParentGroup();
        if(parent) {
            const auto trans = parent->getTransformAnimator();
            setParentTransform(trans);
        } else setParentTransform(nullptr);
    });
    connect(mBlendEffectCollection.get(), &Property::prp_currentFrameChanged,
            this, &BoundingBox::blendEffectChanged);
}

BoundingBox::~BoundingBox() {
    sDocumentBoxes.removeOne(this);
}

void BoundingBox::writeBoundingBox(eWriteStream& dst) const {
    if(mWriteId < 0) assignWriteId();
    eBoxOrSound::prp_writeProperty_impl(dst);
    dst << mWriteId;
    dst.write(&mBlendMode, sizeof(SkBlendMode));
}

void BoundingBox::readBoundingBox(eReadStream& src) {
    eBoxOrSound::prp_readProperty_impl(src);
    if(src.evFileVersion() < 10) {
        QString name; src >> name;
        prp_setName(name);
    }
    int readId; src >> readId;
    src.read(&mBlendMode, sizeof(SkBlendMode));

    src.addReadBox(readId, this);
}

qreal BoundingBox::getOpacity(const qreal relFrame) const {
    return mTransformAnimator->getOpacity(relFrame);
}

void BoundingBox::prp_readPropertyXEV_impl(const QDomElement& ele,
                                           const XevImporter& imp) {
    const auto readIdStr = ele.attribute("id");
    const int readId = XmlExportHelpers::stringToInt(readIdStr);
    auto& handler = imp.getXevReadBoxesHandler();
    handler.addReadBox(readId, this);

    eBoxOrSound::prp_readPropertyXEV_impl(ele, imp);
}

QDomElement BoundingBox::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    if(mWriteId < 0) assignWriteId();
    auto result = eBoxOrSound::prp_writePropertyXEV_impl(exp);
    result.setAttribute("id", mWriteId);
    return result;
}

BoundingBox *BoundingBox::sGetBoxByDocumentId(const int documentId) {
    for(const auto& box : sDocumentBoxes) {
        if(box->getDocumentId() == documentId) return box;
    }
    return nullptr;
}

void BoundingBox::prp_afterChangedAbsRange(const FrameRange &range, const bool clip) {
    const auto croppedRange = clip ? prp_absInfluenceRange()*range : range;
    StaticComplexAnimator::prp_afterChangedAbsRange(croppedRange, clip);
    if(croppedRange.inRange(anim_getCurrentAbsFrame())) {
        planUpdate(UpdateReason::userChange);
    }
}

void BoundingBox::ca_childIsRecordingChanged() {
    ComplexAnimator::ca_childIsRecordingChanged();
    SWT_scheduleContentUpdate(SWT_BoxRule::animated);
    SWT_scheduleContentUpdate(SWT_BoxRule::notAnimated);
}

qsptr<BoundingBox> BoundingBox::createLink(const bool inner) {
    auto linkBox = enve::make_shared<InternalLinkBox>(this, inner);
    copyTransformationTo(linkBox.get());
    return std::move(linkBox);
}

void BoundingBox::clearRasterEffects() {
    mRasterEffectsAnimators->clear();
}

QPointF BoundingBox::getRelCenterPosition() {
    return mRelRect.center();
}

void BoundingBox::centerPivotPosition() {
    const auto center = getRelCenterPosition();
    mTransformAnimator->setPivotFixedTransform(center);
    requestGlobalPivotUpdateIfSelected();
}

void BoundingBox::centerPivotPositionAction() {
    const auto pos = mTransformAnimator->getPosAnimator();
    const auto pivot = mTransformAnimator->getPivotAnimator();
    pos->prp_startTransform();
    pivot->prp_startTransform();
    centerPivotPosition();
    pos->prp_finishTransform();
    pivot->prp_finishTransform();
}

void BoundingBox::planCenterPivotPosition() {
    mCenterPivotPlanned = true;
}

void BoundingBox::blendSetup(ChildRenderData &data,
                             const int index, const qreal relFrame,
                             QList<ChildRenderData> &delayed) const {
    if(!blendEffectsEnabled()) return;
    mBlendEffectCollection->blendSetup(data, index, relFrame, delayed);
}

void BoundingBox::updateIfUsesProgram(
        const ShaderEffectProgram * const program) const {
    mRasterEffectsAnimators->updateIfUsesProgram(program);
}

void BoundingBox::copyTransformationTo(BoundingBox * const targetBox) const {
    sWriteReadMember(this, targetBox, &BoundingBox::mTransformAnimator);
}

void BoundingBox::copyRasterEffectsTo(BoundingBox * const targetBox) const {
    sWriteReadMember(this, targetBox, &BoundingBox::mRasterEffectsAnimators);
}

void BoundingBox::copyBoundingBoxDataTo(BoundingBox * const targetBox) const {
    copyTransformationTo(targetBox);
    copyRasterEffectsTo(targetBox);
}

void BoundingBox::drawHoveredSk(SkCanvas *canvas, const float invScale) {
    drawHoveredPathSk(canvas, mSkRelBoundingRectPath, invScale);
}

void BoundingBox::drawHoveredPathSk(SkCanvas *canvas,
                                    const SkPath &path,
                                    const float invScale) {
    canvas->save();
    SkPath mappedPath = path;
    mappedPath.transform(toSkMatrix(
                             mTransformAnimator->getTotalTransform()));
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);
    paint.setStrokeWidth(2*invScale);
    paint.setStyle(SkPaint::kStroke_Style);
    canvas->drawPath(mappedPath, paint);

    paint.setColor(SK_ColorRED);
    paint.setStrokeWidth(invScale);
    canvas->drawPath(mappedPath, paint);
    canvas->restore();
}

void BoundingBox::setRasterEffectsEnabled(const bool enable) {
    mRasterEffectsAnimators->SWT_setEnabled(enable);
    mRasterEffectsAnimators->SWT_setVisible(
                mRasterEffectsAnimators->ca_hasChildren() || enable);
}

bool BoundingBox::getRasterEffectsEnabled() const {
    return mRasterEffectsAnimators->SWT_isEnabled();
}

void BoundingBox::updateAllBoxes(const UpdateReason reason) {
    planUpdate(reason);
}

void BoundingBox::drawAllCanvasControls(SkCanvas * const canvas,
                                        const CanvasMode mode,
                                        const float invScale,
                                        const bool ctrlPressed) {
    for(const auto& prop : mCanvasProps)
        prop->prp_drawCanvasControls(canvas, mode, invScale, ctrlPressed);
}

MovablePoint *BoundingBox::getPointAtAbsPos(const QPointF &absPos,
                                            const CanvasMode mode,
                                            const qreal invScale) const {
    for(int i = mCanvasProps.count() - 1; i >= 0; i--) {
        const auto& prop = mCanvasProps.at(i);
        const auto handler = prop->getPointsHandler();
        if(!handler) continue;
        const auto pt = handler->getPointAtAbsPos(absPos, mode, invScale);
        if(pt) return pt;
    }
    return nullptr;
}

NormalSegment BoundingBox::getNormalSegment(const QPointF &absPos,
                                            const qreal invScale) const {
    for(int i = mCanvasProps.count() - 1; i >= 0; i--) {
        const auto& prop = mCanvasProps.at(i);
        const auto handler = prop->getPointsHandler();
        if(!handler) continue;
        const auto pathHandler = enve_cast<PathPointsHandler*>(handler);
        if(!pathHandler) continue;
        const auto seg = pathHandler->getNormalSegment(absPos, invScale);
        if(seg.isValid()) return seg;
    }
    return NormalSegment();
}

#include "efiltersettings.h"

void BoundingBox::drawPixmapSk(SkCanvas * const canvas,
                               const SkFilterQuality filter) const {
    const qreal opacity = getOpacity(anim_getCurrentRelFrame());
    if(isZero4Dec(opacity) || !mVisibleInScene) return;
    mDrawRenderContainer.drawSk(canvas, filter);
}

bool BoundingBox::blendEffectsEnabled() const {
    return mBlendEffectCollection->SWT_isVisible();
}

bool BoundingBox::hasBlendEffects() const {
    return mBlendEffectCollection->ca_hasChildren();
}

const QStringList BoundingBox::checkRasterEffectsForSVGSupport()
{
    QStringList result;
    const int totalEffects = mRasterEffectsAnimators->ca_getNumberOfChildren();
    for (int i = 0; i < totalEffects; ++i) {
        const auto effect = enve_cast<RasterEffect*>(mRasterEffectsAnimators->getChild(i));
        if (!effect) { continue; }
        if (!effect->isVisible()) { continue; }
        bool isSafeForSVG = false;
        if (const auto blur = enve_cast<BlurEffect*>(effect)) {
            isSafeForSVG = true;
        } else if (const auto shadow = enve_cast<ShadowEffect*>(effect)) {
            isSafeForSVG = true;
        }
        if (!isSafeForSVG) { result.append(effect->prp_getName()); }
    }
    return result;
}

void BoundingBox::applyTransformEffects(
        const qreal relFrame,
        qreal& pivotX, qreal& pivotY, qreal& pivotZ,
        qreal& posX, qreal& posY, qreal& posZ,
        qreal& rotX, qreal& rotY, qreal& rotZ,
        qreal& scaleX, qreal& scaleY, qreal& scaleZ,
        qreal& shearX, qreal& shearY, qreal& shearZ,
        QMatrix& postTransform) {
    mTransformEffectCollection->applyEffects(relFrame,
                                             pivotX, pivotY, pivotZ,
                                             posX, posY, posZ,
                                             rotX, rotY, rotZ,
                                             scaleX, scaleY, scaleZ,
                                             shearX, shearY, shearZ,
                                             postTransform, this);
}

bool BoundingBox::hasTransformEffects() const {
    return mTransformEffectCollection->ca_hasChildren();
}

const QStringList BoundingBox::checkTransformEffectsForSVGSupport()
{
    QStringList result;
    const int totalEffects = mTransformEffectCollection->ca_getNumberOfChildren();
    for (int i = 0; i < totalEffects; ++i) {
        const auto effect = enve_cast<TransformEffect*>(mTransformEffectCollection->getChild(i));
        if (!effect) { continue; }
        if (!effect->isVisible()) { continue; }
        bool isSafeForSVG = false;
        if (const auto followPath = enve_cast<FollowPathEffect*>(effect)) {
            isSafeForSVG = true;
        }
        if (!isSafeForSVG) { result.append(effect->prp_getName()); }
    }
    return result;
}

ContainerBox *BoundingBox::getFirstParentLayer() const {
    const auto parent = getParentGroup();
    if(!parent) return nullptr;
    if(parent->isLayer()) return parent;
    return parent->getFirstParentLayer();
}

void BoundingBox::detachedBlendUISetup(int& drawId,
        QList<BlendEffect::UIDelayed> &delayed) const {
    if(!blendEffectsEnabled()) return;
    mBlendEffectCollection->detachedBlendUISetup(drawId, delayed);
}

void BoundingBox::detachedBlendSetup(
        SkCanvas * const canvas,
        const SkFilterQuality filter, int& drawId,
        QList<BlendEffect::Delayed> &delayed) const {
    if(!blendEffectsEnabled()) return;
    mBlendEffectCollection->detachedBlendSetup(
                this, canvas, filter, drawId, delayed);
}

void BoundingBox::drawPixmapSk(SkCanvas * const canvas,
                               const SkFilterQuality filter, int& drawId,
                               QList<BlendEffect::Delayed> &delayed) const {
    Q_UNUSED(drawId)
    Q_UNUSED(delayed)
    canvas->save();
    if(blendEffectsEnabled()) {
        mBlendEffectCollection->drawBlendSetup(canvas);
    }
    drawPixmapSk(canvas, filter);
    canvas->restore();
}

void BoundingBox::setBlendModeSk(const SkBlendMode blendMode) {
    if(mBlendMode == blendMode) return;
    {
        prp_pushUndoRedoName("Set Blend Mode");
        UndoRedo ur;
        const auto oldValue = mBlendMode;
        const auto newValue = blendMode;
        ur.fUndo = [this, oldValue]() {
            setBlendModeSk(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setBlendModeSk(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mBlendMode = blendMode;
    prp_afterWholeInfluenceRangeChanged();
    emit blendModeChanged(blendMode);
}

void BoundingBox::resetScale() {
    mTransformAnimator->resetScale();
}

void BoundingBox::resetTranslation() {
    mTransformAnimator->resetTranslation();
}

void BoundingBox::resetRotation() {
    mTransformAnimator->resetRotation();
}

void BoundingBox::anim_setAbsFrame(const int frame) {
    const int oldRelFrame = anim_getCurrentRelFrame();
    ComplexAnimator::anim_setAbsFrame(frame);
    const int newRelFrame = anim_getCurrentRelFrame();

    if(prp_differencesBetweenRelFrames(oldRelFrame, newRelFrame)) {
        planUpdate(UpdateReason::frameChange);
    }
}

bool BoundingBox::diffsIncludingInherited(
        const int relFrame1, const int relFrame2) const {
    const bool diffThis = prp_differencesBetweenRelFrames(relFrame1, relFrame2);
    const auto parent = getParentGroup();
    if(!parent || diffThis) return diffThis;
    const int absFrame1 = prp_relFrameToAbsFrame(relFrame1);
    const int absFrame2 = prp_relFrameToAbsFrame(relFrame2);
    const int parentRelFrame1 = parent->prp_absFrameToRelFrame(absFrame1);
    const int parentRelFrame2 = parent->prp_absFrameToRelFrame(absFrame2);

    const bool diffInherited = parent->diffsAffectingContainedBoxes(
                parentRelFrame1, parentRelFrame2);
    return diffThis || diffInherited;
}

bool BoundingBox::diffsIncludingInherited(const qreal relFrame1,
                                          const qreal relFrame2) const {
    const int prevFrame = qFloor(qMin(relFrame1, relFrame2));
    const int nextFrame = qCeil(qMax(relFrame1, relFrame2));
    return diffsIncludingInherited(prevFrame, nextFrame);
}

void BoundingBox::setParentTransform(BasicTransformAnimator *parent) {
    if(parent == mParentTransform) return;
    mParentTransform = parent;
    mTransformAnimator->setParentTransformAnimator(mParentTransform);
}

void BoundingBox::afterTotalTransformChanged(const UpdateReason reason) {
    updateDrawRenderContainerTransform();
    planUpdate(reason);
    requestGlobalPivotUpdateIfSelected();
}

void BoundingBox::clearParent() {
    const auto parent = getParentGroup();
    if(parent) setParentTransform(parent->getTransformAnimator());
    else setParentTransform(nullptr);
}

void BoundingBox::setPivotRelPos(const QPointF &relPos) {
    mTransformAnimator->setPivotFixedTransform(relPos);
    requestGlobalPivotUpdateIfSelected();
}

void BoundingBox::startPivotTransform() {
    mTransformAnimator->startPivotTransform();
}

void BoundingBox::finishPivotTransform() {
    mTransformAnimator->finishPivotTransform();
}

void BoundingBox::setPivotAbsPos(const QPointF &absPos) {
    setPivotRelPos(mapAbsPosToRel(absPos));
}

QPointF BoundingBox::getPivotAbsPos() {
    return mTransformAnimator->getPivotAbs();
}

QPointF BoundingBox::getPivotRelPos(const qreal relFrame) {
    return mTransformAnimator->getPivot(relFrame);
}

QPointF BoundingBox::getPivotAbsPos(const qreal relFrame) {
    return mTransformAnimator->getPivotAbs(relFrame);
}

void BoundingBox::setRelBoundingRect(const QRectF& relRect) {
    mRelRect = relRect;
    mRelRectSk = toSkRect(mRelRect);
    mSkRelBoundingRectPath.reset();
    mSkRelBoundingRectPath.addRect(mRelRectSk);

    if(mCenterPivotPlanned) {
        mCenterPivotPlanned = false;
        centerPivotPosition();
    }
}

void BoundingBox::prp_updateCanvasProps() {
    mCanvasProps.clear();
    ca_execOnDescendants([this](Property * prop) {
        if(prop->prp_drawsOnCanvas()) mCanvasProps.append(prop);
    });
    if(prp_drawsOnCanvas()) mCanvasProps.append(this);
    const auto parentScene = getParentScene();
    if(parentScene) parentScene->requestUpdate();
}

void BoundingBox::updateCurrentPreviewDataFromRenderData(
        BoxRenderData* renderData) {
    setRelBoundingRect(renderData->fRelBoundingRect);
}

void BoundingBox::planUpdate(const UpdateReason reason) {
    if(mUpdatePlanned && mPlannedReason == UpdateReason::userChange) return;
    if(!isVisibleAndInVisibleDurationRect()) return;
    const auto parent = getParentGroup();
    if(parent) parent->planUpdate(reason);
    else if(!enve_cast<Canvas*>(this)) return;
    if(reason == UpdateReason::userChange) {
        mStateId++;
        mRenderDataHandler.clear();
    }

    mDrawRenderContainer.setExpired(true);
    if(mUpdatePlanned) {
        mPlannedReason = qMax(reason, mPlannedReason);
    } else {
        mUpdatePlanned = true;
        mPlannedReason = reason;
    }
}


stdsptr<BoxRenderData> BoundingBox::queExternalRender(
        const qreal relFrame, const bool forceRasterize) {
    const auto renderData = createRenderData(relFrame);
    if(!renderData) return nullptr;
    renderData->fParentIsTarget = false;
    renderData->fForceRasterize = forceRasterize;
    const auto parentM = getInheritedTransformAtFrame(relFrame);
    setupRenderData(relFrame, parentM, renderData.get(), getParentScene());
    renderData->queTask();
    return renderData;
}

stdsptr<BoxRenderData> BoundingBox::queRender(
        const qreal relFrame, const QMatrix& parentM) {
    const auto renderData = updateCurrentRenderData(relFrame);
    if(!renderData) return nullptr;
    setupRenderData(relFrame, parentM, renderData, getParentScene());
    const auto renderDataSPtr = enve::shared(renderData);
    renderDataSPtr->queTask();
    return renderDataSPtr;
}

void BoundingBox::queTasks() {
    if(!mUpdatePlanned) return;
    mUpdatePlanned = false;
    if(!shouldScheduleUpdate()) return;
    const int relFrame = anim_getCurrentRelFrame();
    if(hasCurrentRenderData(relFrame)) return;
    const auto parentM = getInheritedTransformAtFrame(relFrame);
    queRender(relFrame, parentM);
}

stdsptr<BoxRenderData> BoundingBox::createRenderData(const qreal relFrame) {
    const auto renderData = createRenderData();
    if(!renderData) return nullptr;
    renderData->fRelFrame = relFrame;
    return renderData;
}

BoxRenderData *BoundingBox::updateCurrentRenderData(const qreal relFrame) {
    const auto renderData = createRenderData(relFrame);
    if(!renderData) return nullptr;
    mRenderDataHandler.addItemAtRelFrame(renderData);
    return renderData.get();
}

bool BoundingBox::hasCurrentRenderData(const qreal relFrame) const {
    const auto currentRenderData = mRenderDataHandler.getItemAtRelFrame(relFrame);
    if(currentRenderData) return true;
    if(mDrawRenderContainer.isExpired()) return false;
    const auto drawData = mDrawRenderContainer.getSrcRenderData();
    if(!drawData) return false;
    return !diffsIncludingInherited(drawData->fRelFrame, relFrame);
}

stdsptr<BoxRenderData> BoundingBox::getCurrentRenderData(const qreal relFrame) const {
    const auto currentRenderData =
            mRenderDataHandler.getItemAtRelFrame(relFrame);
    if(currentRenderData) return currentRenderData->ref<BoxRenderData>();
    if(mDrawRenderContainer.isExpired()) return nullptr;
    const auto drawData = mDrawRenderContainer.getSrcRenderData();
    if(!drawData) return nullptr;
    if(!diffsIncludingInherited(drawData->fRelFrame, relFrame)) {
        const auto copy = drawData->makeCopy();
        copy->fRelFrame = relFrame;
        return copy;
    }
    return nullptr;
}

bool BoundingBox::isContainedIn(const QRectF &absRect) const {
    return absRect.contains(getAbsBoundingRect());
}

BoundingBox *BoundingBox::getBoxAtFromAllDescendents(const QPointF &absPos) {
    if(absPointInsidePath(absPos)) return this;
    return nullptr;
}

void BoundingBox::setStrokeCapStyle(const SkPaint::Cap capStyle) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->setCapStyle(capStyle);
    prp_afterWholeInfluenceRangeChanged();
    planUpdate(UpdateReason::userChange);
}

void BoundingBox::setStrokeJoinStyle(const SkPaint::Join joinStyle) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->setJoinStyle(joinStyle);
    prp_afterWholeInfluenceRangeChanged();
    planUpdate(UpdateReason::userChange);
}

void BoundingBox::setOutlineCompositionMode(
        const QPainter::CompositionMode compositionMode) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->setOutlineCompositionMode(compositionMode);
    prp_afterWholeInfluenceRangeChanged();
    planUpdate(UpdateReason::userChange);
}

void BoundingBox::setStrokeBrush(SimpleBrushWrapper * const brush) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->setStrokeBrush(brush);
    requestGlobalFillStrokeUpdateIfSelected();
}

void BoundingBox::applyStrokeBrushWidthAction(const SegAction &action) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->applyStrokeBrushWidthAction(action);
}

void BoundingBox::applyStrokeBrushPressureAction(const SegAction &action) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->applyStrokeBrushPressureAction(action);
}

void BoundingBox::applyStrokeBrushSpacingAction(const SegAction &action) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->applyStrokeBrushSpacingAction(action);
}

void BoundingBox::applyStrokeBrushTimeAction(const SegAction &action) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->applyStrokeBrushTimeAction(action);
}

QPointF BoundingBox::mapAbsPosToRel(const QPointF &absPos) {
    return mTransformAnimator->mapAbsPosToRel(absPos);
}

void BoundingBox::strokeWidthAction(const QrealAction& action) {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->strokeWidthAction(action);
}

void BoundingBox::startSelectedStrokeColorTransform() {
    const auto strokeSettings = getStrokeSettings();
    if(!strokeSettings) return;
    strokeSettings->getColorAnimator()->prp_startTransform();
}

void BoundingBox::startSelectedFillColorTransform() {
    const auto fillSettings = getStrokeSettings();
    if(!fillSettings) return;
    fillSettings->getColorAnimator()->prp_startTransform();
}

void BoundingBox::applyPaintSetting(const PaintSettingsApplier &setting)
{ setting.apply(this); }

void BoundingBox::drawBoundingRect(SkCanvas * const canvas,
                                   const float invScale) {
    SkiaHelpers::drawOutlineOverlay(canvas, mSkRelBoundingRectPath,
                                    invScale, toSkMatrix(getTotalTransform()),
                                    true, eSizesUI::widget*0.25f);
}

QMatrix BoundingBox::getTotalTransform() const {
    return mTransformAnimator->getTotalTransform();
}

QMatrix BoundingBox::getRelativeTransformAtCurrentFrame() const {
    return getRelativeTransformAtFrame(anim_getCurrentRelFrame());
}

void BoundingBox::scale(const qreal scaleBy) {
    scale(scaleBy, scaleBy);
}

void BoundingBox::scale(const qreal scaleXBy, const qreal scaleYBy) {
    mTransformAnimator->scale(scaleXBy, scaleYBy);
}

void BoundingBox::setScale(const qreal scale)
{
    mTransformAnimator->setScale(scale, scale);
}

void BoundingBox::setRotate(const qreal rot)
{
    mTransformAnimator->setRotation(rot);
}

void BoundingBox::rotateBy(const qreal rot) {
    mTransformAnimator->rotateRelativeToSavedValue(rot, rot);
}

void BoundingBox::rotateRelativeToSavedPivot(const qreal rot) {
    mTransformAnimator->rotateRelativeToSavedValue(rot,
                                                   mSavedTransformPivot);
}

void BoundingBox::scaleRelativeToSavedPivot(const qreal scaleXBy,
                                            const qreal scaleYBy) {
    mTransformAnimator->scaleRelativeToSavedValue(scaleXBy, scaleYBy,
                                                 mSavedTransformPivot);
}

void BoundingBox::scaleRelativeToSavedPivot(const qreal scaleBy) {
    scaleRelativeToSavedPivot(scaleBy, scaleBy);
}

QPointF BoundingBox::mapRelPosToAbs(const QPointF &relPos) const {
    return mTransformAnimator->mapRelPosToAbs(relPos);
}

void BoundingBox::setupCanvasMenu(PropertyMenu * const menu)
{
    if (menu->hasActionsForType<BoundingBox>()) { return; }
    menu->addedActionsForType<BoundingBox>();

    const auto pScene = getParentScene();
    Q_ASSERT(pScene);

    menu->addPlainAction(QIcon::fromTheme("linked"), tr("Create Link"), [pScene]() {
        pScene->createLinkBoxForSelected();
    });
    menu->addPlainAction(QIcon::fromTheme("pivot-align-center"), tr("Center Pivot"), [pScene]() {
        pScene->centerPivotForSelected();
    });

    menu->addSeparator();

    menu->addPlainAction(QIcon::fromTheme("copy"), tr("Copy"), [pScene]() {
        pScene->copyAction();
    })->setShortcut(Qt::CTRL + Qt::Key_C);

    menu->addPlainAction(QIcon::fromTheme("cut"), tr("Cut"), [pScene]() {
        pScene->cutAction();
    })->setShortcut(Qt::CTRL + Qt::Key_X);

    menu->addPlainAction(QIcon::fromTheme("duplicate"), tr("Duplicate"), [pScene]() {
        pScene->duplicateAction();
    })->setShortcut(Qt::CTRL + Qt::Key_D);

    menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete"), [pScene]() {
        pScene->removeSelectedBoxesAndClearList();
    })->setShortcut(Qt::Key_Delete);

    menu->addSeparator();

    menu->addPlainAction(QIcon::fromTheme("group"), tr("Group"), [pScene]() {
        pScene->groupSelectedBoxes();
    })->setShortcut(Qt::CTRL + Qt::Key_G);

    menu->addSeparator();

    const auto rasterEffectsMenu = menu->addMenu(QIcon::fromTheme("effect"), tr("Raster Effects"));
    RasterEffectMenuCreator::addEffects(
                rasterEffectsMenu, &BoundingBox::addRasterEffect);
}

void BoundingBox::alignGeometry(const QRectF& geometry,
                                const Qt::Alignment align,
                                const QRectF& to) {
    QPointF moveBy{0., 0.};
    if(align & Qt::AlignLeft) {
        moveBy.setX(to.left() - geometry.left());
    } else if(align & Qt::AlignHCenter) {
        moveBy.setX(to.center().x() - geometry.center().x());
    } else if(align & Qt::AlignRight) {
        moveBy.setX(to.right() - geometry.right());
    }
    if(align & Qt::AlignTop) {
        moveBy.setY(to.top() - geometry.top());
    } else if(align & Qt::AlignVCenter) {
        moveBy.setY(to.center().y() - geometry.center().y());
    } else if(align & Qt::AlignBottom) {
        moveBy.setY(to.bottom() - geometry.bottom());
    }
    if(moveBy.isNull()) return;
    startPosTransform();
    moveByAbs(moveBy);
    finishTransform();
}

void BoundingBox::alignGeometry(const Qt::Alignment align, const QRectF& to) {
    alignGeometry(getAbsBoundingRect(), align, to);
}

void BoundingBox::alignPivot(const Qt::Alignment align, const QRectF& to) {
    const QPointF pivot = getPivotAbsPos();
    alignGeometry(QRectF(pivot, pivot), align, to);
}

void BoundingBox::alignPivotItself(const Qt::Alignment align,
                                   const QRectF& to,
                                   const AlignRelativeTo relativeTo,
                                   const QPointF lastPivotAbsPos)
{
    QPointF currentPivot = mTransformAnimator->getPivot();
    QPointF currentPivotAbsPos = getPivotAbsPos();

    QPointF lastSelectedPivotAbsPos = lastPivotAbsPos;

    QPointF center = getRelCenterPosition();

    qreal currentScaleX = mTransformAnimator->sx();
    qreal currentScaleY = mTransformAnimator->sy();

    switch (relativeTo) {
    case AlignRelativeTo::scene:
        switch (align) {
        case Qt::AlignVCenter:
            center.setX(currentPivot.x() * currentScaleX);
            center.setY(currentPivot.y() * currentScaleY - currentPivotAbsPos.y() + to.bottomRight().y()/2);
            break;
        case Qt::AlignHCenter:
            center.setX(currentPivot.x() * currentScaleX - currentPivotAbsPos.x() + to.bottomRight().x()/2);
            center.setY(currentPivot.y() * currentScaleY);
            break;
        case Qt::AlignLeft:
            center.setX(currentPivot.x() * currentScaleX - currentPivotAbsPos.x());
            center.setY(currentPivot.y() * currentScaleY);
            break;
        case Qt::AlignRight:
            center.setX(currentPivot.x() * currentScaleX + (to.topRight().x() - currentPivotAbsPos.x()));
            center.setY(currentPivot.y() * currentScaleY);
            break;
        case Qt::AlignTop:
            center.setX(currentPivot.x() * currentScaleX);
            center.setY(currentPivot.y() * currentScaleY - currentPivotAbsPos.y());
            break;
        case Qt::AlignBottom:
            center.setX(currentPivot.x() * currentScaleX);
            center.setY(currentPivot.y() * currentScaleY + (to.bottomRight().y() - currentPivotAbsPos.y()));
            break;
        }

        if (currentScaleX != 0.0) {
            center.setX(center.x() / currentScaleX);
        }
        if (currentScaleY != 0.0) {
            center.setY(center.y() / currentScaleY);
        }

        break;
    case AlignRelativeTo::lastSelected:
        switch (align) {
        case Qt::AlignVCenter:
            center.setX(currentPivot.x());
            center.setY(currentPivot.y() - currentPivotAbsPos.y() + to.center().y());
            break;
        case Qt::AlignHCenter:
            center.setX(currentPivot.x() - currentPivotAbsPos.x() + to.center().x());
            center.setY(currentPivot.y());
            break;
        case Qt::AlignLeft:
            center.setX(currentPivot.x() - currentPivotAbsPos.x() + to.topLeft().x());
            center.setY(currentPivot.y());
            break;
        case Qt::AlignRight:
            center.setX(currentPivot.x() + (to.topRight().x() - currentPivotAbsPos.x()));
            center.setY(currentPivot.y());
            break;
        case Qt::AlignTop:
            center.setX(currentPivot.x());
            center.setY(currentPivot.y() - currentPivotAbsPos.y() + to.topLeft().y());
            break;
        case Qt::AlignBottom:
            center.setX(currentPivot.x());
            center.setY(currentPivot.y() + (to.bottomRight().y() - currentPivotAbsPos.y()));
            break;
        }
        break;
    case AlignRelativeTo::lastSelectedPivot:
        switch (align) {
        case Qt::AlignVCenter:
            center.setX(currentPivot.x());
            center.setY(currentPivot.y() - currentPivotAbsPos.y() + lastSelectedPivotAbsPos.y());
            break;
        case Qt::AlignHCenter:
            center.setX(currentPivot.x() - currentPivotAbsPos.x() + lastSelectedPivotAbsPos.x());
            center.setY(currentPivot.y());
            break;
        default:
            center.setX(currentPivot.x());
            center.setY(currentPivot.y());
            break;
        }
        break;
    case AlignRelativeTo::boundingBox:
        switch (align) {
        case Qt::AlignVCenter:
            center.setX(currentPivot.x());
            break;
        case Qt::AlignHCenter:
            center.setY(currentPivot.y());
            break;
        case Qt::AlignLeft:
            center.setX(mRelRect.topLeft().x());
            center.setY(currentPivot.y());
            break;
        case Qt::AlignRight:
            center.setX(mRelRect.topRight().x());
            center.setY(currentPivot.y());
            break;
        case Qt::AlignTop:
            center.setX(currentPivot.x());
            center.setY(mRelRect.topLeft().y());
            break;
        case Qt::AlignBottom:
            center.setX(currentPivot.x());
            center.setY(mRelRect.bottomLeft().y());
            break;
        }
        break;
    }

    startPivotTransform();
    setPivotRelPos(center);
    finishPivotTransform();
}

void BoundingBox::moveByAbs(const QPointF &trans) {
    mTransformAnimator->moveByAbs(trans);
}

void BoundingBox::moveByRel(const QPointF &trans) {
    mTransformAnimator->moveRelativeToSavedValue(trans.x(), trans.y());
}

void BoundingBox::setAbsolutePos(const QPointF &pos) {
    setRelativePos(mParentTransform->mapAbsPosToRel(pos));
}

void BoundingBox::setRelativePos(const QPointF &relPos) {
    mTransformAnimator->setPosition(relPos.x(), relPos.y());
}

void BoundingBox::setOpacity(const qreal opacity) {
    mTransformAnimator->setOpacity(opacity);
}

void BoundingBox::saveTransformPivotAbsPos(const QPointF &absPivot) {
    mSavedTransformPivot = mParentTransform->mapAbsPosToRel(absPivot) -
                           mTransformAnimator->getPivot();
}

void BoundingBox::startPosTransform() {
    mTransformAnimator->startPosTransform();
}

void BoundingBox::startRotTransform() {
    mTransformAnimator->startRotTransform();
}

void BoundingBox::startScaleTransform() {
    mTransformAnimator->startScaleTransform();
}

void BoundingBox::startTransform() {
    mTransformAnimator->prp_startTransform();
}

void BoundingBox::finishTransform() {
    mTransformAnimator->prp_finishTransform();
    //updateTotalTransform();
}

void BoundingBox::setupRenderData(const qreal relFrame,
                                  const QMatrix& parentM,
                                  BoxRenderData * const data,
                                  Canvas* const scene) {
    setupWithoutRasterEffects(relFrame, parentM, data, scene);
    setupRasterEffects(relFrame, data, scene);
}

void BoundingBox::setupWithoutRasterEffects(const qreal relFrame,
                                            const QMatrix& parentM,
                                            BoxRenderData * const data,
                                            Canvas* const scene) {
    //Q_ASSERT(scene);
    if(!scene) return;

    data->fBoxStateId = mStateId;
    data->fRelFrame = relFrame;

    const auto thisRelM = getRelativeTransformAtFrame(relFrame);
    data->fRelTransform = thisRelM;
    data->fInheritedTransform = parentM;
    data->fTotalTransform = thisRelM*parentM;

    data->fResolution = scene->getResolution();
    data->fResolutionScale.reset();
    data->fResolutionScale.scale(data->fResolution, data->fResolution);
    data->fOpacity = getOpacity(relFrame);
    data->fBaseMargin = QMargins() + 2;
    data->fBlendMode = getBlendMode();

    {
        const auto parent = getParentGroup();
        QRectF maxBoundsF;
        if(parent) maxBoundsF = parent->currentGlobalBounds();
        else maxBoundsF = scene->getCurrentBounds();
        const QRectF scaledMaxBoundsF = data->fResolutionScale.mapRect(maxBoundsF);
        data->fMaxBoundsRect = scaledMaxBoundsF.toAlignedRect();
    }
}

void BoundingBox::setupRasterEffects(const qreal relFrame,
                                     BoxRenderData * const data,
                                     Canvas* const scene) {
    //Q_ASSERT(scene);
    if(!scene) return;
    const bool effectsVisible = scene->getRasterEffectsVisible();
    if(data->fOpacity > 0.001 && effectsVisible) {
        mRasterEffectsAnimators->addEffects(relFrame, data);
    }
}

void BoundingBox::incReasonsNotToApplyUglyTransform() {
    mNReasonsNotToApplyUglyTransform++;
}

void BoundingBox::decReasonsNotToApplyUglyTransform() {
    mNReasonsNotToApplyUglyTransform--;
}

bool BoundingBox::relPointInsidePath(const QPointF &relPos) const {
    return mRelRect.contains(relPos.toPoint());
}

bool BoundingBox::absPointInsidePath(const QPointF &absPoint) {
    return relPointInsidePath(mapAbsPosToRel(absPoint));
}

void BoundingBox::cancelTransform() {
    mTransformAnimator->prp_cancelTransform();
    //updateTotalTransform();
}

QPointF BoundingBox::getAbsolutePos() const {
    return QPointF(mTransformAnimator->getTotalTransform().dx(),
                   mTransformAnimator->getTotalTransform().dy());
}

void BoundingBox::updateDrawRenderContainerTransform() {
    if(mNReasonsNotToApplyUglyTransform == 0) {
        mDrawRenderContainer.updatePaintTransformGivenNewTotalTransform(
                    getTotalTransformAtFrame(anim_getCurrentRelFrame()));
    }
}

eBoxType BoundingBox::getBoxType() const { return mType; }

void BoundingBox::requestGlobalFillStrokeUpdateIfSelected() {
    if(isSelected()) emit fillStrokeSettingsChanged();
}

void BoundingBox::requestGlobalPivotUpdateIfSelected() {
    if(isSelected()) emit globalPivotInfluenced();
}

void BoundingBox::applyParentTransform() {
    if(!mParentTransform) return;
    const auto parentTransform = mParentTransform->getRelativeTransform();
    const auto thisTransform = mTransformAnimator->getRelativeTransform();
    const auto newTransform = thisTransform*parentTransform;
    const auto pivot = mTransformAnimator->getPivot();
    const auto dec = MatrixDecomposition::decomposePivoted(newTransform, pivot);

    mTransformAnimator->startTransformSkipOpacity();
    mTransformAnimator->setValues(dec);
    mTransformAnimator->prp_finishTransform();
}

bool BoundingBox::isTransformationStatic() const {
    return !mTransformAnimator->anim_isRecording();
}

BasicTransformAnimator *BoundingBox::getTransformAnimator() const {
    return getBoxTransformAnimator();
}

BoxTransformAnimator *BoundingBox::getBoxTransformAnimator() const {
    return mTransformAnimator.get();
}

const QRectF BoundingBox::getAbsBoundingRect() const {
    return getTotalTransform().mapRect(mRelRect);
}

bool BoundingBox::isAnimated() const {
    return anim_isDescendantRecording();
}

void BoundingBox::addRasterEffect(const qsptr<RasterEffect>& rasterEffect) {
    mRasterEffectsAnimators->addChild(rasterEffect);
}

void BoundingBox::removeRasterEffect(const qsptr<RasterEffect> &effect) {
    mRasterEffectsAnimators->removeChild(effect);
}

void BoundingBox::addBlendEffect(const qsptr<BlendEffect> &blendEffect)
{
    mBlendEffectCollection->addChild(blendEffect);
}

void BoundingBox::addTransformEffect(const qsptr<TransformEffect> &transformEffect)
{
    mTransformEffectCollection->addChild(transformEffect);
}

//int BoundingBox::prp_getParentFrameShift() const {
//    if(!mParentGroup) {
//        return 0;
//    } else {
//        return mParentGroup->prp_getFrameShift();
//    }
//}

QMatrix BoundingBox::getRelativeTransformAtFrame(const qreal relFrame) const {
    if(isZero6Dec(relFrame - anim_getCurrentRelFrame()))
        return mTransformAnimator->getRelativeTransform();
    return mTransformAnimator->getRelativeTransformAtFrame(relFrame);
}

QMatrix BoundingBox::getInheritedTransformAtFrame(const qreal relFrame) const {
    if(isZero6Dec(relFrame - anim_getCurrentRelFrame()))
        return mTransformAnimator->getInheritedTransform();
    return mTransformAnimator->getInheritedTransformAtFrame(relFrame);
}

QMatrix BoundingBox::getTotalTransformAtFrame(const qreal relFrame) const {
    if(isZero6Dec(relFrame - anim_getCurrentRelFrame()))
        return mTransformAnimator->getTotalTransform();
    return mTransformAnimator->getTotalTransformAtFrame(relFrame);
}

void BoundingBox::setCustomPropertiesVisible(const bool visible) {
    if(mCustomProperties->SWT_isVisible() == visible) return;
    {
        prp_pushUndoRedoName("Custom Properties");
        UndoRedo ur;
        const auto oldValue = !visible;
        const auto newValue = visible;
        ur.fUndo = [this, oldValue]() {
            mCustomProperties->SWT_setVisible(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            mCustomProperties->SWT_setVisible(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mCustomProperties->SWT_setVisible(visible);
}

void BoundingBox::setBlendEffectsVisible(const bool visible) {
    if(mBlendEffectCollection->SWT_isVisible() == visible) return;
    {
        prp_pushUndoRedoName("Blend Effects");
        UndoRedo ur;
        const auto oldValue = !visible;
        const auto newValue = visible;
        ur.fUndo = [this, oldValue]() {
            setBlendEffectsVisible(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setBlendEffectsVisible(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mBlendEffectCollection->SWT_setVisible(visible);
    const auto pLayer = getFirstParentLayer();
    if(pLayer) {
        if(visible) {
            pLayer->addBoxWithBlendEffects(this);
        } else {
            pLayer->removeBoxWithBlendEffects(this);
        }
    }
    prp_afterWholeInfluenceRangeChanged();
}

void BoundingBox::setSVGPropertiesVisible(const bool visible)
{
    const auto& children = mTransformAnimator->ca_getNumberOfChildren();
    const auto properties = QStringList() << "begin event" << "end event";
    for (int i = 0; i < children; i++) {
        const auto child = mTransformAnimator->ca_getChildAt(i);
        if (!child) { continue; }
        if (!properties.contains(child->prp_getName())) { continue; }
        child->SWT_setVisible(visible);
    }
}

bool BoundingBox::getSVGPropertiesVisible()
{
    const auto& children = mTransformAnimator->ca_getNumberOfChildren();
    const auto properties = QStringList() << "begin event" << "end event";
    for (int i = 0; i < children; i++) {
        const auto child = mTransformAnimator->ca_getChildAt(i);
        if (!child) { continue; }
        if (!properties.contains(child->prp_getName())) { continue; }
        return child->SWT_isVisible();
    }
    return false;
}

void BoundingBox::prp_setupTreeViewMenu(PropertyMenu * const menu)
{
    if (menu->hasActionsForType<BoundingBox>()) { return; }
    menu->addedActionsForType<BoundingBox>();

    const auto parentWidget = menu->getParentWidget();
    menu->addPlainAction(QIcon::fromTheme("dialog-information"), tr("Rename"), [this, parentWidget]() {
        PropertyNameDialog::sRenameBox(this, parentWidget);
    });

    const auto pScene = getParentScene();
    if (pScene) {
        menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete"), [pScene]() {
            /*const int ask = QMessageBox::question(nullptr,
                                                  tr("Delete?"),
                                                  tr("Are you sure you want to delete selected item(s)?"));
            if (ask != QMessageBox::Yes) { return; }*/
            pScene->removeSelectedBoxesAndClearList();
        })->setShortcut(Qt::Key_Delete);
    }

    menu->addSeparator();
    {
        const PropertyMenu::CheckSelectedOp<BoundingBox> visRangeOp =
            [](BoundingBox* const box, const bool checked) {
                box->setSVGPropertiesVisible(checked);
            };
        menu->addCheckableAction(tr("SVG Properties"),
                                 getSVGPropertiesVisible(),
                                 visRangeOp);
    }
    {
        const PropertyMenu::CheckSelectedOp<BoundingBox> visRangeOp =
        [](BoundingBox* const box, const bool checked) {
            box->setCustomPropertiesVisible(checked);
        };
        menu->addCheckableAction(tr("Custom Properties"),
                                 mCustomProperties->SWT_isVisible(),
                                 visRangeOp);
    }
    {
        const PropertyMenu::CheckSelectedOp<BoundingBox> visRangeOp =
        [](BoundingBox* const box, const bool checked) {
            box->setBlendEffectsVisible(checked);
        };
        menu->addCheckableAction(tr("Blend Effects"),
                                 mBlendEffectCollection->SWT_isVisible(),
                                 visRangeOp);
    }
    menu->addSeparator();

    const PropertyMenu::CheckSelectedOp<BoundingBox> visRangeOp =
    [](BoundingBox* const box, const bool checked) {
        if (box->durationRectangleLocked()) { return; }
        const bool hasDur = box->hasDurationRectangle();
        if (hasDur == checked) { return; }
        if (checked) { box->createDurationRectangle(); }
        else { box->setDurationRectangle(nullptr); }
    };
    menu->addCheckableAction(tr("Visibility Range"),
                             hasDurationRectangle(),
                             visRangeOp)->setEnabled(!durationRectangleLocked());

    menu->addPlainAction(QIcon::fromTheme("visible"), tr("Visibility Range Settings"),
                         [this]() {
        const auto durRect = getDurationRectangle();
        if (!durRect) { return; }
        const auto& instance = DialogsInterface::instance();
        instance.showDurationSettingsDialog(durRect);
    })->setEnabled(hasDurationRectangle());

    menu->addSeparator();
    setupCanvasMenu(menu->addMenu(QIcon::fromTheme("preferences"), tr("Actions")));
}

void BoundingBox::getMotionBlurProperties(QList<Property*> &list) const {
    list.append(mTransformAnimator->getScaleAnimator());
    list.append(mTransformAnimator->getPosAnimator());
    list.append(mTransformAnimator->getPivotAnimator());
    list.append(mTransformAnimator->getRotAnimator());
    list.append(mTransformAnimator->getShearAnimator());
}

FrameRange BoundingBox::getMotionBlurIdenticalRange(
        const qreal relFrame, const bool inheritedTransform) {
    FrameRange range(FrameRange::EMINMAX);
    if(isVisible()) {
        const auto durRect = getDurationRectangle();
        if(isFrameFInDurationRect(relFrame)) {
            QList<Property*> props;
            getMotionBlurProperties(props);
            for(const auto& child : props) {
                if(range.isUnary()) break;
                auto childRange = child->prp_getIdenticalRelRange(relFrame);
                range *= childRange;
            }

            if(durRect) range *= durRect->getRelFrameRange();
        } else {
            if(relFrame > durRect->getMaxRelFrame()) {
                return durRect->getAbsFrameRangeToTheRight();
            } else if(relFrame < durRect->getMinRelFrame()) {
                return durRect->getAbsFrameRangeToTheLeft();
            }
        }
    } else {
        return FrameRange::EMINMAX;
    }
    const auto parent = getParentGroup();
    if(!parent || !inheritedTransform) return range;
    if(range.isUnary()) return range;
    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal parentRel = parent->prp_absFrameToRelFrameF(absFrame);
    auto parentRange = parent->BoundingBox::getMotionBlurIdenticalRange(
                           parentRel, inheritedTransform);

    return range*parentRange;
}

void BoundingBox::writeIdentifier(eWriteStream &dst) const {
    dst.write(&mType, sizeof(eBoxType));
}

int BoundingBox::getWriteId() const {
    if(mWriteId < 0) assignWriteId();
    return mWriteId;
}

int BoundingBox::assignWriteId() const {
    mWriteId = sNextWriteId++;
    sBoxesWithWriteIds << this;
    return mWriteId;
}

void BoundingBox::clearWriteId() const {
    mWriteId = -1;
}

void BoundingBox::sClearWriteBoxes() {
    for(const auto& box : sBoxesWithWriteIds) {
        box->clearWriteId();
    }
    sNextWriteId = 0;
    sBoxesWithWriteIds.clear();
}

#include "simpletask.h"

void BoundingBox::selectAndAddContainedPointsToList(
        const QRectF &absRect,
        const MovablePoint::PtOp &adder,
        const CanvasMode mode) {
    for(const auto& desc : mCanvasProps) {
        const auto handler = desc->getPointsHandler();
        if(!handler) continue;
        handler->addInRectForSelection(absRect, adder, mode);
    }
}

void BoundingBox::selectAllCanvasPts(const MovablePoint::PtOp &adder,
                                     const CanvasMode mode) {
    for(const auto& desc : mCanvasProps) {
        const auto handler = desc->getPointsHandler();
        if(!handler) continue;
        handler->addAllPointsToSelection(adder, mode);
    }
}

bool BoundingBox::SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                                      const bool parentSatisfies,
                                      const bool parentMainTarget) const {
    const SWT_BoxRule rule = rules.fRule;
    bool satisfies = false;
    const bool alwaysShowChildren = rules.fAlwaysShowChildren;
    if(rules.fType == SWT_Type::sound) return false;
    if(alwaysShowChildren) {
        if(rule == SWT_BoxRule::all) {
            satisfies = parentSatisfies;
        } else if(rule == SWT_BoxRule::selected) {
            satisfies = isSelected() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::animated) {
            satisfies = isAnimated() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::notAnimated) {
            satisfies = !isAnimated() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::visible) {
            satisfies = isVisible() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::hidden) {
            satisfies = !isVisible() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::locked) {
            satisfies = isLocked() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::unlocked) {
            satisfies = !isLocked() ||
                    (parentSatisfies && !parentMainTarget);
        }
    } else {
        if(rule == SWT_BoxRule::all) {
            satisfies = parentSatisfies;
        } else if(rule == SWT_BoxRule::selected) {
            satisfies = isSelected();
        } else if(rule == SWT_BoxRule::animated) {
            satisfies = isAnimated();
        } else if(rule == SWT_BoxRule::notAnimated) {
            satisfies = !isAnimated();
        } else if(rule == SWT_BoxRule::visible) {
            satisfies = isVisible() && parentSatisfies;
        } else if(rule == SWT_BoxRule::hidden) {
            satisfies = !isVisible() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::locked) {
            satisfies = isLocked() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_BoxRule::unlocked) {
            satisfies = !isLocked() && parentSatisfies;
        }
    }
    if(satisfies) {
        if(!rules.fSearchString.isEmpty()) {
            satisfies = prp_getName().contains(rules.fSearchString);
        }
    }
    return satisfies;
}

bool BoundingBox::SWT_dropSupport(const QMimeData * const data) {
    return mRasterEffectsAnimators->SWT_dropSupport(data);
}

bool BoundingBox::SWT_drop(const QMimeData * const data) {
    if(mRasterEffectsAnimators->SWT_dropSupport(data))
        return mRasterEffectsAnimators->SWT_drop(data);
    return false;
}

void BoundingBox::renderDataFinished(BoxRenderData *renderData) {
    const bool currentState = renderData->fBoxStateId == mStateId;
    const qreal relFrame = renderData->fRelFrame;
    if(currentState) mRenderDataHandler.removeItemAtRelFrame(relFrame);
    auto currentRenderData = mDrawRenderContainer.getSrcRenderData();
    bool newerSate = true;
    bool closerFrame = true;
    if(currentRenderData) {
        newerSate = currentRenderData->fBoxStateId < renderData->fBoxStateId;
        const qreal finishedFrameDist =
                qAbs(anim_getCurrentRelFrame() - relFrame);
        const qreal oldFrameDist =
                qAbs(anim_getCurrentRelFrame() - currentRenderData->fRelFrame);
        closerFrame = finishedFrameDist < oldFrameDist;
    }
    if(newerSate || closerFrame) {
        mDrawRenderContainer.setSrcRenderData(renderData);
        const bool currentFrame = isZero4Dec(relFrame - anim_getCurrentRelFrame());
        const bool expired = !currentState || !currentFrame;
        mDrawRenderContainer.setExpired(expired);
        if(expired) updateDrawRenderContainerTransform();
    }
}

//QString skBlendModeToSVG(const SkBlendMode mode) {
//    switch(mode) {
//    case SkBlendMode::kMultiply:    return "multiply";
//    case SkBlendMode::kScreen:      return "screen";
//    case SkBlendMode::kOverlay:     return "overlay";
//    case SkBlendMode::kDarken:      return "darken";
//    case SkBlendMode::kLighten:     return "lighten";
//    case SkBlendMode::kColorDodge:  return "color-dodge";
//    case SkBlendMode::kColorBurn:   return "color-burn";
//    case SkBlendMode::kHardLight:   return "hard-light";
//    case SkBlendMode::kSoftLight:   return "soft-light";
//    case SkBlendMode::kDifference:  return "difference";
//    case SkBlendMode::kExclusion:   return "exclusion";
//    case SkBlendMode::kHue:         return "hue";
//    case SkBlendMode::kSaturation:  return "saturation";
//    case SkBlendMode::kColor:       return "color";
//    case SkBlendMode::kLuminosity:  return "luminosity";
//    default: return "";
//    }
//}

eTask* BoundingBox::saveSVGWithTransform(SvgExporter& exp,
                                         QDomElement& parent,
                                         const FrameRange& parentVisRange,
                                         const QString &maskId) const
{
    const auto visRange = parentVisRange*prp_absInfluenceRange();
    const auto task = enve::make_shared<DomEleTask>(exp, visRange);
    exp.addNextTask(task);
    const auto taskPtr = task.get();
    const QPointer<const BoundingBox> ptr = this;
    const auto expPtr = &exp;
    const auto parentPtr = &parent;
    taskPtr->addDependent({[ptr, taskPtr, expPtr, parentPtr, visRange, maskId]() {
        auto& ele = taskPtr->element();
        if (ptr) {
            ele.setAttribute("id", AppSupport::filterId(ptr->prp_getName()));
            SvgExportHelpers::assignVisibility(*expPtr, ele, visRange);

            const auto transformEffects = ptr->mTransformEffectCollection.get();
            const bool hasTransformEffects = transformEffects->hasEffectsSVG();

            const auto transform = ptr->mTransformAnimator.get();
            const auto transformed = transform->saveSVG(*expPtr, visRange, ele);
            const auto effects = ptr->mRasterEffectsAnimators.get();
            const auto withEffects = hasTransformEffects ?
                                         transformEffects->saveEffectsSVG(*expPtr,
                                                                          visRange,
                                                                          ele,
                                                                          effects->saveEffectsSVG(*expPtr,
                                                                                                  visRange,
                                                                                                  transformed)) :
                                         effects->saveEffectsSVG(*expPtr,
                                                                 visRange,
                                                                 transformed);

            if (maskId == ptr->prp_getName()) { // move mask to defs
                auto& eleMask = taskPtr->initialize("mask");
                eleMask.setAttribute("id", QString("%1Mask").arg(AppSupport::filterId(ptr->prp_getName())));
                eleMask.appendChild(withEffects);
                expPtr->addToDefs(eleMask);
            } else {
                parentPtr->appendChild(withEffects);
            }
        }
    }, nullptr});
    saveSVG(exp, taskPtr);
    taskPtr->queTask();
    return task.get();
}
