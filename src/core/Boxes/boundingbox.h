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

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "Animators/eboxorsound.h"
#include "boxrendercontainer.h"
#include "skia/skiaincludes.h"
#include "renderdatahandler.h"
#include "smartPointers/ememory.h"
#include "colorhelpers.h"
#include "MovablePoints/segment.h"
#include "Animators/qcubicsegment1danimator.h"
#include "BlendEffects/blendeffect.h"
#include "TransformEffects/transformeffect.h"
#include "Tasks/domeletask.h"

class Canvas;

enum class AlignRelativeTo;

class QrealAction;
class MovablePoint;

class PathEffect;
class TimelineMovable;
class FillSettingsAnimator;
class OutlineSettingsAnimator;
class PaintSettingsApplier;
class RasterEffectCollection;
struct ShaderEffectProgram;
class BoxTransformAnimator;
class BasicTransformAnimator;
class CustomProperties;
class BlendEffectCollection;
class TransformEffectCollection;

class ContainerBox;
class SmartVectorPath;
class DurationRectangle;
struct ContainerBoxRenderData;
class ShaderEffect;
class RasterEffect;
struct ChildRenderData;
enum class CanvasMode : short;

class SimpleBrushWrapper;

enum class eBoxType {
    vectorPath,
    circle,
    image,
    rectangle,
    text,
    layer,
    canvas,
    internalLink,
    internalLinkGroup,
    internalLinkCanvas,
    svgLink,
    video,
    imageSequence,
    paint,
    group,
    custom,
    deprecated0, // sculptPath,
    nullObject,

    count
};

class BoundingBox;
template<typename T> class TypeMenu;
typedef TypeMenu<Property> PropertyMenu;

class CORE_EXPORT BoundingBox : public eBoxOrSound {
    Q_OBJECT
    e_OBJECT
    e_DECLARE_TYPE(BoundingBox)
    typedef qCubicSegment1DAnimator::Action SegAction;
protected:
    BoundingBox(const QString& name, const eBoxType type);
public:
    ~BoundingBox();

    virtual stdsptr<BoxRenderData> createRenderData() = 0;

    static BoundingBox *sGetBoxByDocumentId(const int documentId);

    static void sClearWriteBoxes();

    template <typename B, typename T>
    static void sWriteReadMember(const B* const from, B* const to, const T member);
private:
    static int sNextDocumentId;
    static QList<BoundingBox*> sDocumentBoxes;

    static int sNextWriteId;
    static QList<const BoundingBox*> sBoxesWithWriteIds;
protected:
    virtual void getMotionBlurProperties(QList<Property*> &list) const;

    void prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp);
    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const;
public:
    virtual bool isGroup() const { return false; }
    virtual bool isLayer() const { return false; }

    virtual qsptr<BoundingBox> createLink(const bool inner);

    virtual SmartVectorPath *objectToVectorPathBox()
    { return nullptr; }
    virtual SmartVectorPath *strokeToVectorPathBox()
    { return nullptr; }

    void centerPivotPositionAction();
    void centerPivotPosition();
    virtual QPointF getRelCenterPosition();

    virtual void selectAndAddContainedPointsToList(
            const QRectF &absRect,
            const MovablePoint::PtOp& adder,
            const CanvasMode mode);

    virtual bool relPointInsidePath(const QPointF &relPos) const;

    virtual void setFontSize(const qreal fontSize)
    { Q_UNUSED(fontSize) }
    virtual void setFontFamilyAndStyle(const QString &family,
                                       const SkFontStyle& style)
    { Q_UNUSED(family) Q_UNUSED(style) }

    virtual void setTextVAlignment(const Qt::Alignment alignment)
    { Q_UNUSED(alignment) }
    virtual void setTextHAlignment(const Qt::Alignment alignment)
    { Q_UNUSED(alignment) }

    virtual void drawPixmapSk(SkCanvas * const canvas,
                              const SkFilterQuality filter, int &drawId,
                              QList<BlendEffect::Delayed> &delayed) const;
    virtual void drawHoveredSk(SkCanvas *canvas, const float invScale);

    virtual BoundingBox *getBoxAtFromAllDescendents(const QPointF &absPos);

    virtual FillSettingsAnimator *getFillSettings() const
    { return nullptr; }
    virtual OutlineSettingsAnimator *getStrokeSettings() const
    { return nullptr; }

    virtual void applyStrokeBrushWidthAction(const SegAction& action);
    virtual void applyStrokeBrushPressureAction(const SegAction& action);
    virtual void applyStrokeBrushSpacingAction(const SegAction& action);
    virtual void applyStrokeBrushTimeAction(const SegAction& action);

    virtual void setStrokeCapStyle(const SkPaint::Cap capStyle);
    virtual void setStrokeJoinStyle(const SkPaint::Join joinStyle);
    virtual void setStrokeBrush(SimpleBrushWrapper * const brush);

    virtual void setOutlineCompositionMode(
            const QPainter::CompositionMode compositionMode);

    virtual void strokeWidthAction(const QrealAction& action);
    virtual void startSelectedStrokeColorTransform();
    virtual void startSelectedFillColorTransform();

    virtual void updateAllBoxes(const UpdateReason reason);

    virtual QMatrix getRelativeTransformAtCurrentFrame() const;
    virtual QMatrix getRelativeTransformAtFrame(const qreal relFrame) const;
    virtual QMatrix getInheritedTransformAtFrame(const qreal relFrame) const;
    virtual QMatrix getTotalTransformAtFrame(const qreal relFrame) const;
    virtual QPointF mapAbsPosToRel(const QPointF &absPos);

    virtual void applyPaintSetting(const PaintSettingsApplier &setting);
    virtual void addPathEffect(const qsptr<PathEffect>& effect)
    { Q_UNUSED(effect) }
    virtual void addFillPathEffect(const qsptr<PathEffect>& effect)
    { Q_UNUSED(effect) }
    virtual void addOutlineBasePathEffect(const qsptr<PathEffect>& effect)
    { Q_UNUSED(effect) }
    virtual void addOutlinePathEffect(const qsptr<PathEffect>& effect)
    { Q_UNUSED(effect) }

    virtual void setupCanvasMenu(PropertyMenu * const menu);

    virtual void setupRenderData(const qreal relFrame, const QMatrix& parentM,
                                 BoxRenderData * const data,
                                 Canvas * const scene);
    virtual void renderDataFinished(BoxRenderData *renderData);
    virtual void updateCurrentPreviewDataFromRenderData(
            BoxRenderData* renderData);

    virtual FrameRange getMotionBlurIdenticalRange(
            const qreal relFrame, const bool inheritedTransform);

    virtual HardwareSupport hardwareSupport() const {
        return HardwareSupport::cpuPreffered;
    }

    virtual bool shouldScheduleUpdate() { return true; }
    virtual void queTasks();

    virtual void writeIdentifier(eWriteStream& dst) const;

    virtual void writeBoundingBox(eWriteStream& dst) const;
    virtual void readBoundingBox(eReadStream& src);

    virtual SkBlendMode getBlendMode() const
    { return mBlendMode; }

    virtual qreal getOpacity(const qreal relFrame) const;

    virtual void saveSVG(SvgExporter& exp, DomEleTask* const task) const {
        Q_UNUSED(exp)
        Q_UNUSED(task)
    }

    virtual void updateIfUsesProgram(const ShaderEffectProgram * const program) const;

    bool SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                             const bool parentSatisfies,
                             const bool parentMainTarget) const;
    bool SWT_visibleOnlyIfParentDescendant() const
    { return false; }

    bool SWT_dropSupport(const QMimeData* const data);
    bool SWT_drop(const QMimeData* const data);
protected:
    void prp_updateCanvasProps();
public:
    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    void prp_afterChangedAbsRange(const FrameRange &range,
                                  const bool clip = true);

    void anim_setAbsFrame(const int frame);

    void ca_childIsRecordingChanged();

    BasicTransformAnimator *getTransformAnimator() const;

    stdsptr<BoxRenderData> createRenderData(const qreal relFrame);
    stdsptr<BoxRenderData> queRender(const qreal relFrame,
                                     const QMatrix& parentM);
    stdsptr<BoxRenderData> queExternalRender(
            const qreal relFrame, const bool forceRasterize);

    void setupWithoutRasterEffects(const qreal relFrame,
                                   const QMatrix& parentM,
                                   BoxRenderData * const data,
                                   Canvas* const scene);
    void setupRasterEffects(const qreal relFrame,
                            BoxRenderData * const data,
                            Canvas* const scene);

    void drawAllCanvasControls(SkCanvas * const canvas,
                               const CanvasMode mode,
                               const float invScale,
                               const bool ctrlPressed);

    void moveByRel(const QPointF &trans);
    void moveByAbs(const QPointF &trans);
    void rotateBy(const qreal rot);
    void scale(const qreal scaleBy);
    void scale(const qreal scaleXBy, const qreal scaleYBy);
    void setScale(const qreal scale);
    void setRotate(const qreal rot);
    void saveTransformPivotAbsPos(const QPointF &absPivot);

    void startPosTransform();
    void startRotTransform();
    void startScaleTransform();

    void startTransform();
    void finishTransform();
    void cancelTransform();

    void alignGeometry(const Qt::Alignment align, const QRectF& to);
    void alignPivot(const Qt::Alignment align, const QRectF& to);
    void alignPivotItself(const Qt::Alignment align,
                          const QRectF& to,
                          const AlignRelativeTo relativeTo,
                          const QPointF lastPivotAbsPos);

    QMatrix getTotalTransform() const;

    MovablePoint *getPointAtAbsPos(const QPointF &absPos,
                                   const CanvasMode mode,
                                   const qreal invScale) const;
    NormalSegment getNormalSegment(const QPointF &absPos,
                                   const qreal invScale) const;
    void drawBoundingRect(SkCanvas * const canvas,
                          const float invScale);

    void selectAllCanvasPts(const MovablePoint::PtOp &adder,
                            const CanvasMode mode);

    int getDocumentId() const { return mDocumentId; }

    int assignWriteId() const;
    void clearWriteId() const;
    int getWriteId() const;

    void clearParent();
    void setParentTransform(BasicTransformAnimator *parent);

    bool isContainedIn(const QRectF &absRect) const;

    QPointF getPivotRelPos(const qreal relFrame);
    QPointF getPivotAbsPos();
    QPointF getPivotAbsPos(const qreal relFrame);

    QPointF getAbsolutePos() const;
    bool absPointInsidePath(const QPointF &absPos);

    void setPivotAbsPos(const QPointF &absPos);
    void setPivotRelPos(const QPointF &relPos);

    bool isAnimated() const;

    void rotateRelativeToSavedPivot(const qreal rot);
    void scaleRelativeToSavedPivot(const qreal scaleBy);
    void setAbsolutePos(const QPointF &pos);
    void setRelativePos(const QPointF &relPos);
    void setOpacity(const qreal opacity);

    void scaleRelativeToSavedPivot(const qreal scaleXBy,
                                   const qreal scaleYBy);
    void startPivotTransform();
    void finishPivotTransform();
    void resetScale();
    void resetTranslation();
    void resetRotation();

    void applyParentTransform();
    bool isTransformationStatic() const;
    BoxTransformAnimator *getBoxTransformAnimator() const;
    const QRectF getAbsBoundingRect() const;
    const QRectF& getRelBoundingRect() const
    { return mRelRect; }
    const SkPath &getRelBoundingRectPath() const
    { return mSkRelBoundingRectPath; }
    void drawHoveredPathSk(SkCanvas *canvas, const SkPath &path,
                           const float invScale);

    void setRasterEffectsEnabled(const bool enable);
    bool getRasterEffectsEnabled() const;

    void clearRasterEffects();

    void addRasterEffect(const qsptr<RasterEffect> &rasterEffect);
    void removeRasterEffect(const qsptr<RasterEffect> &effect);

    void addBlendEffect(const qsptr<BlendEffect> &blendEffect);
    void addTransformEffect(const qsptr<TransformEffect> &transformEffect);

    void setBlendModeSk(const SkBlendMode blendMode);

    QPointF mapRelPosToAbs(const QPointF &relPos) const;

    void copyTransformationTo(BoundingBox * const targetBox) const;
    void copyRasterEffectsTo(BoundingBox * const targetBox) const;
    void copyBoundingBoxDataTo(BoundingBox * const targetBox) const;

//    int prp_getParentFrameShift() const;

    bool diffsIncludingInherited(const int relFrame1, const int relFrame2) const;
    bool diffsIncludingInherited(const qreal relFrame1, const qreal relFrame2) const;

    bool hasCurrentRenderData(const qreal relFrame) const;
    stdsptr<BoxRenderData> getCurrentRenderData(const qreal relFrame) const;
    BoxRenderData *updateCurrentRenderData(const qreal relFrame);

    void updateDrawRenderContainerTransform();

    void incReasonsNotToApplyUglyTransform();
    void decReasonsNotToApplyUglyTransform();

    eBoxType getBoxType() const;

    void requestGlobalPivotUpdateIfSelected();
    void requestGlobalFillStrokeUpdateIfSelected();

    void planUpdate(const UpdateReason reason);

    void planCenterPivotPosition();

    bool visibleForScene() const
    { return mVisibleInScene; }
    void setVisibleForScene(const bool visible)
    { mVisibleInScene = visible; }

    virtual void blendSetup(ChildRenderData& data,
                    const int index, const qreal relFrame,
                    QList<ChildRenderData>& delayed) const;
    void drawPixmapSk(SkCanvas * const canvas,
                      const SkFilterQuality filter) const;
    void detachedBlendUISetup(int& drawId,
            QList<BlendEffect::UIDelayed> &delayed) const;
    virtual void detachedBlendSetup(
            SkCanvas * const canvas,
            const SkFilterQuality filter, int& drawId,
            QList<BlendEffect::Delayed> &delayed) const;

    bool blendEffectsEnabled() const;
    bool hasBlendEffects() const;
    bool hasEnabledBlendEffects() const
    { return blendEffectsEnabled() && hasBlendEffects(); }

    const QStringList checkRasterEffectsForSVGSupport();

    void applyTransformEffects(const qreal relFrame,
                               qreal& pivotX, qreal& pivotY, qreal& pivotZ,
                               qreal& posX, qreal& posY, qreal& posZ,
                               qreal& rotX, qreal& rotY, qreal& rotZ,
                               qreal& scaleX, qreal& scaleY, qreal& scaleZ,
                               qreal& shearX, qreal& shearY, qreal& shearZ,
                               QMatrix& postTransform);

    bool hasTransformEffects() const;
    const QStringList checkTransformEffectsForSVGSupport();

    ContainerBox* getFirstParentLayer() const;

    eTask* saveSVGWithTransform(SvgExporter& exp, QDomElement& parent,
                                const FrameRange& parentVisRange,
                                const QString &maskId = QString()) const;
private:
    void cancelWaitingTasks();
    void afterTotalTransformChanged(const UpdateReason reason);
signals:
    void globalPivotInfluenced();
    void fillStrokeSettingsChanged();
    void blendModeChanged(SkBlendMode);
    void brushChanged(SimpleBrushWrapper* brush);
    void blendEffectChanged();
protected:
    void setRelBoundingRect(const QRectF& relRect);

    uint mStateId = 0;

    int mNReasonsNotToApplyUglyTransform = 0;
protected:
    bool getUpdatePlanned() const
    { return mUpdatePlanned; }

    const int mDocumentId;

    eBoxType mType;

    RenderDataHandler mRenderDataHandler;

    const qsptr<CustomProperties> mCustomProperties;
    const qsptr<BlendEffectCollection> mBlendEffectCollection;
    const qsptr<TransformEffectCollection> mTransformEffectCollection;
    const qsptr<BoxTransformAnimator> mTransformAnimator;
    const qsptr<RasterEffectCollection> mRasterEffectsAnimators;
private:
    void alignGeometry(const QRectF& geometry,
                       const Qt::Alignment align,
                       const QRectF& to);

    void setCustomPropertiesVisible(const bool visible);
    void setBlendEffectsVisible(const bool visible);
    void setTransformEffectsVisible(const bool visible);
    void setSVGPropertiesVisible(const bool visible);
    bool getSVGPropertiesVisible();

    SkBlendMode mBlendMode = SkBlendMode::kSrcOver;

    mutable int mWriteId = -1;

    bool mVisibleInScene = true;
    bool mCenterPivotPlanned = false;
    bool mUpdatePlanned = false;
    UpdateReason mPlannedReason;

    QPointF mSavedTransformPivot;

    QRectF mRelRect;
    SkRect mRelRectSk;
    SkPath mSkRelBoundingRectPath;

    BasicTransformAnimator* mParentTransform = nullptr;

    QList<Property*> mCanvasProps;

    RenderContainer mDrawRenderContainer;
};

#include "clipboardcontainer.h"
template <typename B, typename T>
void BoundingBox::sWriteReadMember(const B * const from, B* const to, const T member) {
    PropertyClipboard::sCopyAndPaste(from->*member, to->*member);
}

#endif // BOUNDINGBOX_H
