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

#include "Boxes/svglinkbox.h"
#include "Boxes/videobox.h"
#include "canvas.h"
#include "MovablePoints/pathpivot.h"
#include "PathEffects/patheffectsinclude.h"
#include "Boxes/smartvectorpath.h"
#include "Animators/SmartPath/smartpathcollection.h"
#include "Private/document.h"
#include "eevent.h"
#include "Boxes/textbox.h"

void Canvas::groupSelectedBoxes() {
    if(mSelectedBoxes.isEmpty()) return;
    const auto newGroup = enve::make_shared<ContainerBox>(eBoxType::group);
    mCurrentContainer->addContained(newGroup);
    for(int i = mSelectedBoxes.count() - 1; i >= 0; i--) {
        const auto boxSP = mSelectedBoxes.at(i)->ref<BoundingBox>();
        boxSP->removeFromParent_k();
        newGroup->addContained(boxSP);
    }
    clearBoxesSelectionList();
    newGroup->planCenterPivotPosition();
    schedulePivotUpdate();
    addBoxToSelection(newGroup.get());
}

bool Canvas::anim_nextRelFrameWithKey(const int relFrame,
                                     int &nextRelFrame) {
    int thisNext;
    const bool thisHasNext = BoundingBox::anim_nextRelFrameWithKey(
                relFrame, thisNext);
    int minNextFrame = FrameRange::EMAX;
    for(const auto &box : mSelectedBoxes) {
        const int boxRelFrame = box->prp_absFrameToRelFrame(relFrame);
        int boxNext;
        if(box->anim_nextRelFrameWithKey(boxRelFrame, boxNext)) {
            const int absNext = box->prp_relFrameToAbsFrame(boxNext);
            if(minNextFrame > absNext) minNextFrame = absNext;
        }
    }
    if(minNextFrame == FrameRange::EMAX) {
        if(thisHasNext) nextRelFrame = thisNext;
        return thisHasNext;
    }
    if(thisHasNext) nextRelFrame = qMin(minNextFrame, thisNext);
    else nextRelFrame = minNextFrame;
    return true;
}

bool Canvas::anim_prevRelFrameWithKey(const int relFrame,
                                     int &prevRelFrame) {
    int thisPrev;
    const bool thisHasPrev = BoundingBox::anim_prevRelFrameWithKey(
                relFrame, thisPrev);
    int minPrevFrame = FrameRange::EMIN;
    for(const auto &box : mSelectedBoxes) {
        const int boxRelFrame = box->prp_absFrameToRelFrame(relFrame);
        int boxPrev;
        if(box->anim_prevRelFrameWithKey(boxRelFrame, boxPrev)) {
            const int absPrev = box->prp_relFrameToAbsFrame(boxPrev);
            if(minPrevFrame < absPrev) minPrevFrame = absPrev;
        }
    }
    if(minPrevFrame == FrameRange::EMIN) {
        if(thisHasPrev) prevRelFrame = thisPrev;
        return thisHasPrev;
    }
    if(thisHasPrev) prevRelFrame = qMax(minPrevFrame, thisPrev);
    else prevRelFrame = minPrevFrame;
    return true;
}

void Canvas::shiftAllPointsForAllKeys(const int by) {
    Q_UNUSED(by)
    for(const auto &box : mSelectedBoxes) {
        if(const auto svp = enve_cast<SmartVectorPath*>(box)) {
//            svp->shiftAllPointsForAllKeys(by);
        }
    }
}

void Canvas::revertAllPointsForAllKeys() {
    for(const auto &box : mSelectedBoxes) {
        if(const auto svp = enve_cast<SmartVectorPath*>(box)) {
            //svp->revertAllPointsForAllKeys();
        }
    }
}

void Canvas::shiftAllPoints(const int by) {
    Q_UNUSED(by)
    for(const auto &box : mSelectedBoxes) {
        if(const auto svp = enve_cast<SmartVectorPath*>(box)) {
            //svp->shiftAllPoints(by);
        }
    }
}

void Canvas::revertAllPoints() {
    for(const auto &box : mSelectedBoxes) {
        if(const auto svp = enve_cast<SmartVectorPath*>(box)) {
            //svp->revertAllPoints();
        }
    }
}

void Canvas::flipSelectedBoxesHorizontally() {
    for(const auto &box : mSelectedBoxes) {
        box->startScaleTransform();
        box->scale(-1, 1);
        box->finishTransform();
    }
}

void Canvas::flipSelectedBoxesVertically() {
    for(const auto &box : mSelectedBoxes) {
        box->startScaleTransform();
        box->scale(1, -1);
        box->finishTransform();
    }
}

void Canvas::convertSelectedBoxesToPath() {
    for(const auto &box : mSelectedBoxes)
        box->objectToVectorPathBox();
}

void Canvas::convertSelectedPathStrokesToPath() {
    for(const auto &box : mSelectedBoxes) {
        box->strokeToVectorPathBox();
    }
}

void Canvas::setSelectedTextAlignment(const Qt::Alignment alignment) const {
    pushUndoRedoName("Change Text Alignment");
    for(const auto &box : mSelectedBoxes) {
        box->setTextHAlignment(alignment);
    }
}

void Canvas::setSelectedTextVAlignment(const Qt::Alignment alignment) const {
    pushUndoRedoName("Change Text Alignment");
    for(const auto &box : mSelectedBoxes) {
        box->setTextVAlignment(alignment);
    }
}

void Canvas::setSelectedFontFamilyAndStyle(const QString& family,
                                           const SkFontStyle& style)
{
    pushUndoRedoName("Change Font");
    for(const auto &box : mSelectedBoxes) {
        box->setFontFamilyAndStyle(family, style);
    }
}

void Canvas::setSelectedFontSize(const qreal size) {
    pushUndoRedoName("Change Font Size");
    for(const auto &box : mSelectedBoxes) {
        box->setFontSize(size);
    }
}

void Canvas::setSelectedFontText(const QString &text)
{
    pushUndoRedoName("Change Text Value");
    for (const auto &box : mSelectedBoxes) {
        if (const auto txtBox = enve_cast<TextBox*>(box)) {
            txtBox->prp_startTransform();
            txtBox->setCurrentValue(text);
            txtBox->prp_finishTransform();
        }
    }
}

void Canvas::resetSelectedTranslation() {
    pushUndoRedoName("Reset Translation");
    for(const auto &box : mSelectedBoxes)
        box->resetTranslation();
}

void Canvas::resetSelectedScale() {
    pushUndoRedoName("Reset Scale");
    for(const auto &box : mSelectedBoxes)
        box->resetScale();
}

void Canvas::resetSelectedRotation() {
    pushUndoRedoName("Reset Rotation");
    for(const auto &box : mSelectedBoxes)
        box->resetRotation();
}

void Canvas::applyPaintSettingToSelected(const PaintSettingsApplier &setting) {
    for(const auto &box : mSelectedBoxes) {
        box->applyPaintSetting(setting);
    }
}

void Canvas::setSelectedCapStyle(const SkPaint::Cap capStyle) {
    pushUndoRedoName("Set Cap Style");
    for(const auto &box : mSelectedBoxes) {
        box->setStrokeCapStyle(capStyle);
    }
}

void Canvas::setSelectedJoinStyle(const SkPaint::Join joinStyle) {
    pushUndoRedoName("Set Join Style");
    for(const auto &box : mSelectedBoxes) {
        box->setStrokeJoinStyle(joinStyle);
    }
}

void Canvas::setSelectedStrokeBrush(SimpleBrushWrapper * const brush) {
    pushUndoRedoName("Set Stroke Brush");
    for(const auto &box : mSelectedBoxes) {
        box->setStrokeBrush(brush);
    }
}

void Canvas::applyStrokeBrushWidthActionToSelected(const SegAction& action) {
    for(const auto &box : mSelectedBoxes) {
        box->applyStrokeBrushWidthAction(action);
    }
}

void Canvas::applyStrokeBrushPressureActionToSelected(const SegAction& action) {
    for(const auto &box : mSelectedBoxes) {
        box->applyStrokeBrushPressureAction(action);
    }
}

void Canvas::applyStrokeBrushSpacingActionToSelected(const SegAction& action) {
    for(const auto &box : mSelectedBoxes) {
        box->applyStrokeBrushSpacingAction(action);
    }
}

void Canvas::applyStrokeBrushTimeActionToSelected(const SegAction& action) {
    for(const auto &box : mSelectedBoxes) {
        box->applyStrokeBrushTimeAction(action);
    }
}

void Canvas::strokeWidthAction(const QrealAction& action) {
    for(const auto &box : mSelectedBoxes)
        box->strokeWidthAction(action);
}

void Canvas::startSelectedStrokeColorTransform() {
    for(const auto &box : mSelectedBoxes) {
        box->startSelectedStrokeColorTransform();
    }
}

void Canvas::startSelectedFillColorTransform() {
    for(const auto &box : mSelectedBoxes) {
        box->startSelectedFillColorTransform();
    }
}

#include "Boxes/smartvectorpath.h"
NormalSegment Canvas::getSegment(const eMouseEvent& e) const {
    const qreal zoomInv = 1/e.fScale;
    for(const auto &box : mSelectedBoxes) {
        const auto pathEdge = box->getNormalSegment(e.fPos, zoomInv);
        if(pathEdge.isValid()) return pathEdge;
    }
    return NormalSegment();
}

void Canvas::rotateSelectedBoxesStartAndFinish(const qreal rotBy,
                                               bool inc) {
    if(mDocument.fLocalPivot) {
        for(const auto &box : mSelectedBoxes) {
            box->startRotTransform();
            if (inc) { box->rotateBy(rotBy); }
            else { box->setRotate(rotBy); }
            box->finishTransform();
        }
    } else {
        for(const auto &box : mSelectedBoxes) {
            box->startRotTransform();
            box->startPosTransform();
            box->saveTransformPivotAbsPos(mRotPivot->getAbsolutePos());
            box->rotateRelativeToSavedPivot(rotBy);
            box->finishTransform();
        }
    }
}

void Canvas::scaleSelectedBoxesStartAndFinish(const qreal scaleBy)
{
    if (mDocument.fLocalPivot) {
        for(const auto &box : mSelectedBoxes) {
            box->startScaleTransform();
            box->setScale(scaleBy);
            box->finishTransform();
        }
    } else {
        for (const auto &box : mSelectedBoxes) {
            box->startRotTransform();
            box->startPosTransform();
            box->saveTransformPivotAbsPos(mRotPivot->getAbsolutePos());
            box->scaleRelativeToSavedPivot(scaleBy);
            box->finishTransform();
        }
    }
}

void Canvas::moveSelectedBoxesStartAndFinish(const QPointF moveBy)
{
    for (const auto &box : mSelectedBoxes) {
        box->startPosTransform();
        box->moveByAbs(moveBy);
        box->finishTransform();
    }
}

void Canvas::rotateSelectedBy(const qreal rotBy,
                              const QPointF &absOrigin,
                              const bool startTrans) {
    if(mDocument.fLocalPivot) {
        if(startTrans) {
            for(const auto &box : mSelectedBoxes) {
                box->startRotTransform();
                box->rotateBy(rotBy);
            }
        } else {
            for(const auto &box : mSelectedBoxes) {
                box->rotateBy(rotBy);
            }
        }
    } else {
        if(startTrans) {
            for(const auto &box : mSelectedBoxes) {
                box->startRotTransform();
                box->startPosTransform();
                box->saveTransformPivotAbsPos(absOrigin);
                box->rotateRelativeToSavedPivot(rotBy);
            }
        } else {
            for(const auto &box : mSelectedBoxes) {
                box->rotateRelativeToSavedPivot(rotBy);
            }
        }
    }
}

void Canvas::scaleSelectedBy(const qreal scaleBy,
                             const QPointF &absOrigin,
                             const bool startTrans) {
    scaleSelectedBy(scaleBy, scaleBy, absOrigin, startTrans);
}

void Canvas::scaleSelectedBy(const qreal scaleXBy,
                             const qreal scaleYBy,
                             const QPointF& absOrigin,
                             const bool startTrans) {
    if(mDocument.fLocalPivot) {
        if(startTrans) {
            for(const auto &box : mSelectedBoxes) {
                box->startScaleTransform();
                box->scale(scaleXBy, scaleYBy);
            }
        } else {
            for(const auto &box : mSelectedBoxes) {
                box->scale(scaleXBy, scaleYBy);
            }
        }
    } else {
        if(startTrans) {
            for(const auto &box : mSelectedBoxes) {
                box->startScaleTransform();
                box->startPosTransform();
                box->saveTransformPivotAbsPos(absOrigin);
                box->scaleRelativeToSavedPivot(scaleXBy, scaleYBy);
            }
        } else {
            for(const auto &box : mSelectedBoxes) {
                box->scaleRelativeToSavedPivot(scaleXBy, scaleYBy);
            }
        }
    }
}

QPointF Canvas::getSelectedBoxesAbsPivotPos() {
    if(mSelectedBoxes.isEmpty()) return QPointF(0, 0);
    QPointF posSum(0, 0);
    const int count = mSelectedBoxes.count();
    for(const auto &box : mSelectedBoxes)
        posSum += box->getPivotAbsPos();
    return posSum/count;
}

bool Canvas::isBoxSelectionEmpty() const {
    return mSelectedBoxes.isEmpty();
}

void Canvas::ungroupSelectedBoxes() {
    for(const auto &box : mSelectedBoxes) {
        if(const auto cont = enve_cast<ContainerBox*>(box)) {
            if(cont->isLink()) continue;
            cont->ungroupAction_k();
        }
    }
}

void Canvas::centerPivotForSelected() {
    pushUndoRedoName("Center pivot");
    for(const auto &box : mSelectedBoxes)
        box->centerPivotPositionAction();
}

void Canvas::removeSelectedBoxesAndClearList() {
    while(!mSelectedBoxes.isEmpty()) {
        const auto &box = mSelectedBoxes.last();
        removeBoxFromSelection(box);
        box->removeFromParent_k();
    }
    emit objectSelectionChanged();
}

void Canvas::setCurrentBox(BoundingBox* const box) {
    mCurrentBox = box;
    emit currentBoxChanged(box);
}

//#include "Boxes/paintbox.h"
void Canvas::addBoxToSelection(BoundingBox * const box)
{
    if (box->isSelected() || box->isLocked()) { return; }
    auto& connCtx = mSelectedBoxes.addObj(box);
    mLastSelectedBox = box;
    connCtx << connect(box, &BoundingBox::globalPivotInfluenced,
                       this, &Canvas::schedulePivotUpdate);
    connCtx << connect(box, &BoundingBox::fillStrokeSettingsChanged,
                       this, &Canvas::selectedPaintSettingsChanged);
    connCtx << connect(box, &BoundingBox::visibilityChanged,
                       this, [this, box](const bool visible) {
        if (!visible) { removeBoxFromSelection(box); }
    });
    connCtx << connect(box, &BoundingBox::parentChanged,
                       this, [this, box]() {
        removeBoxFromSelection(box);
    });

    box->setSelected(true);
    schedulePivotUpdate();

    sortSelectedBoxesDesc();
    //setCurrentFillStrokeSettingsFromBox(box);
    setCurrentBox(box);

    /*if(mCurrentMode == CanvasMode::paint) {
        if(const auto pBox = enve_cast<PaintBox*>(box)) {
            mPaintTarget.setPaintBox(pBox);
        }
    }*/
    emit selectedPaintSettingsChanged();
    emit objectSelectionChanged();
}

void Canvas::removeBoxFromSelection(BoundingBox * const box) {
    if(!box->isSelected()) return;
    mSelectedBoxes.removeObj(box);
    box->setSelected(false);
    schedulePivotUpdate();
    //if(mCurrentMode == CanvasMode::paint) updatePaintBox();
    if (mSelectedBoxes.isEmpty()) { setCurrentBox(nullptr); }
    else { setCurrentBox(mSelectedBoxes.last()); }
    emit selectedPaintSettingsChanged();
    emit objectSelectionChanged();
}

void Canvas::clearBoxesSelection() {
    for(const auto &box : mSelectedBoxes) box->setSelected(false);
    clearBoxesSelectionList();
    schedulePivotUpdate();
    setCurrentBox(nullptr);
//    if(mLastPressedBox) {
//        mLastPressedBox->setSelected(false);
//        mLastPressedBox = nullptr;
    //    }
}

void Canvas::clearBoxesSelectionList() {
    //if(mCurrentMode == CanvasMode::paint)
        //mPaintTarget.setPaintBox(nullptr);
    mSelectedBoxes.clear();
    emit selectedPaintSettingsChanged();
    emit objectSelectionChanged();
}

const QString Canvas::checkForUnsupportedBoxSVG(BoundingBox * const box)
{
    QString result;
    if (!box) { return result; }
    qDebug() << "check" << box->prp_getName() << "for SVG support";

    const auto transformEffects = box->checkTransformEffectsForSVGSupport();
    if (transformEffects.size() > 0) {
        result.append(QString("- %1 => %2 : %3 %4\n").arg(prp_getName(),
                                                          box->prp_getName(),
                                                          transformEffects.join(", "),
                                                          tr("is unsupported")));
    }
    if (box->hasEnabledBlendEffects()) {
        result.append(QString("- %1 => %2 : %3\n").arg(prp_getName(),
                                                       box->prp_getName(),
                                                       tr("Blend effects are unsupported")));
    }
    const auto rasterEffects = box->checkRasterEffectsForSVGSupport();
    if (rasterEffects.size() > 0) {
        result.append(QString("- %1 => %2 : %3 %4\n").arg(prp_getName(),
                                                          box->prp_getName(),
                                                          rasterEffects.join(", "),
                                                          tr("is unsupported")));
    }
    if (const auto bbox = enve_cast<TextBox*>(box)) {
        if (bbox->hasTextEffects()) {
            result.append(QString("- %1 => %2 : %3\n").arg(prp_getName(),
                                                           box->prp_getName(),
                                                           tr("Text effects are unsupported")));
        }
        result.append(QString("- %1 => %2 : %3\n").arg(prp_getName(),
                                                       box->prp_getName(),
                                                       tr("For best compatibility convert text to path")));
    }
    return result;
}

const QString Canvas::checkForUnsupportedBoxesSVG(const QList<BoundingBox *> boxes)
{
    QString result;
    for (const auto &box : boxes) {
        if (!box->isVisible()) { continue; }
        if (const auto bbox = enve_cast<ContainerBox*>(box)) {
            const auto warnings = checkForUnsupportedBoxesSVG(bbox->getContainedBoxes());
            if (!warnings.isEmpty()) { result.append(warnings); }
        }
        const auto warnings = checkForUnsupportedBoxSVG(box);
        if (!warnings.isEmpty()) { result.append(warnings); }
    }
    return result;
}

const QString Canvas::checkForUnsupportedSVG()
{
    return checkForUnsupportedBoxesSVG(getContainedBoxes());
}

void Canvas::applyCurrentTransformToSelected() {
}

//bool zAsc(BoundingBox* const box1, BoundingBox* const box2) {
//    return box1->getZIndex() > box2->getZIndex();
//}

//void Canvas::sortSelectedBoxesAsc() {
//    mSelectedBoxes.sort(zAsc);
//}

bool zDesc(BoundingBox* const box1, BoundingBox* const box2) {
    return box1->getZIndex() < box2->getZIndex();
}

void Canvas::sortSelectedBoxesDesc() {
    mSelectedBoxes.sort(zDesc);
}

void Canvas::raiseSelectedBoxesToTop() {
    const auto begin = mSelectedBoxes.rbegin();
    const auto end = mSelectedBoxes.rend();
    for(auto it = begin; it != end; it++) {
        (*it)->bringToFront();
    }
    sortSelectedBoxesDesc();
}

void Canvas::lowerSelectedBoxesToBottom() {
    for(const auto &box : mSelectedBoxes) {
        box->bringToEnd();
    }
    sortSelectedBoxesDesc();
}

void Canvas::lowerSelectedBoxes() {
    int lastZ = -10000;
    bool lastBoxChanged = true;
    const auto begin = mSelectedBoxes.rbegin();
    const auto end = mSelectedBoxes.rend();
    for(auto it = begin; it != end; it++) {
        const auto box = *it;
        const int boxZ = box->getZIndex();
        if(boxZ + 1 != lastZ || lastBoxChanged) box->moveDown();
        lastZ = boxZ;
        lastBoxChanged = boxZ - box->getZIndex() != 0;
    }
    sortSelectedBoxesDesc();
}

void Canvas::raiseSelectedBoxes() {
    int lastZ = -10000;
    bool lastBoxChanged = true;
    for(const auto &box : mSelectedBoxes) {
        const int boxZ = box->getZIndex();
        if(boxZ - 1 != lastZ || lastBoxChanged) box->moveUp();
        lastZ = boxZ;
        lastBoxChanged = boxZ - box->getZIndex() != 0;
    }
    sortSelectedBoxesDesc();
}

void Canvas::deselectAllBoxes() {
    for(const auto &box : mSelectedBoxes)
        removeBoxFromSelection(box);
}

MovablePoint *Canvas::getPointAtAbsPos(const QPointF &absPos,
                                       const CanvasMode mode,
                                       const qreal invScale) {
    if(mode == CanvasMode::boxTransform || mode == CanvasMode::pointTransform) {
        if(mRotPivot->isPointAtAbsPos(absPos, mode, invScale)) {
            return mRotPivot.get();
        }
    }
    if(mode == CanvasMode::pointTransform || mode == CanvasMode::pathCreate ||
       mode == CanvasMode::drawPath || mode == CanvasMode::boxTransform) {
        for(const auto &box : mSelectedBoxes) {
            const auto pointAtPos = box->getPointAtAbsPos(absPos, mode, invScale);
            if(pointAtPos) return pointAtPos;
        }
    }
    return nullptr;
}

void Canvas::finishSelectedBoxesTransform() {
    for(const auto &box : mSelectedBoxes) {
        box->finishTransform();
    }
}

void Canvas::cancelSelectedBoxesTransform() {
    for(const auto &box : mSelectedBoxes) {
        box->cancelTransform();
    }
}

void Canvas::moveSelectedBoxesByAbs(const QPointF &by,
                                    const bool startTransform) {
    if(startTransform) {
        for(const auto &box : mSelectedBoxes) {
            box->startPosTransform();
            box->moveByAbs(by);
        }
    } else {
        for(const auto &box : mSelectedBoxes) {
            box->moveByAbs(by);
        }
    }
}

//QPointF BoxesGroup::getRelCenterPosition() {
//    QPointF posSum = QPointF(0., 0.);
//    if(mChildren.isEmpty()) return posSum;
//    int count = mChildren.length();
//    for(const auto& box : mChildren) {
//        posSum += box->getPivotAbsPos();
//    }
//    return mapAbsPosToRel(posSum/count);
//}

#include "Boxes/internallinkbox.h"
void Canvas::createLinkBoxForSelected() {
    pushUndoRedoName("Create Link");
    for(const auto& selectedBox : mSelectedBoxes)
        mCurrentContainer->addContained(selectedBox->createLink(false));
}

SmartVectorPath *Canvas::getPathResultingFromOperation(const SkPathOp& pathOp)
{
    const auto newPath = enve::make_shared<SmartVectorPath>();
    newPath->planCenterPivotPosition();
    SkOpBuilder builder;
    bool first = true;
    const QList<BoundingBox*> boxes(mSelectedBoxes.rbegin(),
                                    mSelectedBoxes.rend());
    for (const auto &box : boxes) {
        if (const auto pBox = enve_cast<PathBox*>(box)) {
            SkPath boxPath = pBox->getRelativePath();
            const QMatrix boxTrans = box->getRelativeTransformAtCurrentFrame();
            boxPath.transform(toSkMatrix(boxTrans));
            if (first) {
                builder.add(boxPath, SkPathOp::kUnion_SkPathOp);
                first = false;
                pBox->copyDataToOperationResult(newPath.get());
            } else {
                builder.add(boxPath, pathOp);
            }
        }
    }
    SkPath resultPath;
    builder.resolve(&resultPath);
    if (resultPath.isEmpty()) {
        return getPathResultingFromCombine();
    } else {
        newPath->loadSkPath(resultPath);
    }
    mCurrentContainer->addContained(newPath);
    return newPath.get();
}

SmartVectorPath *Canvas::getPathResultingFromCombine() {
    SmartVectorPath *newPath = nullptr;
    for(const auto &box : mSelectedBoxes) {
        if(const auto path = enve_cast<SmartVectorPath*>(box)) {
            newPath = path;
            break;
        }
    }
    if(!newPath) {
        const auto newPathT = enve::make_shared<SmartVectorPath>();
        newPathT->planCenterPivotPosition();
        mCurrentContainer->addContained(newPathT);
        newPath = newPathT.get();
    }

    const auto targetVP = newPath->getPathAnimator();
    const QMatrix firstTranf = newPath->getTotalTransform();
    for(const auto &box : mSelectedBoxes) {
        if(box == newPath) continue;
        if(const auto boxPath = enve_cast<SmartVectorPath*>(box)) {
            const QMatrix relTransf = boxPath->getTotalTransform()*
                    firstTranf.inverted();
            const auto srcVP = boxPath->getPathAnimator();
            srcVP->applyTransform(relTransf);
            targetVP->moveAllFrom(srcVP);
            box->removeFromParent_k();
        }
    }
    return newPath;
}

void Canvas::selectedPathsDifference() {
    if(mSelectedBoxes.isEmpty()) return;

    SmartVectorPath * const newPath = getPathResultingFromOperation(
                SkPathOp::kDifference_SkPathOp);

    clearBoxesSelection();
    addBoxToSelection(newPath);
}

void Canvas::selectedPathsIntersection() {
    if(mSelectedBoxes.isEmpty()) return;

    SmartVectorPath * const newPath = getPathResultingFromOperation(
                SkPathOp::kIntersect_SkPathOp);

    clearBoxesSelection();
    addBoxToSelection(newPath);
}

void Canvas::selectedPathsDivision() {
    if(mSelectedBoxes.isEmpty()) return;

    SmartVectorPath * const newPath1 = getPathResultingFromOperation(
                SkPathOp::kDifference_SkPathOp);

    SmartVectorPath * const newPath2 = getPathResultingFromOperation(
                SkPathOp::kIntersect_SkPathOp);

    clearBoxesSelection();
    addBoxToSelection(newPath1);
    addBoxToSelection(newPath2);
}

void Canvas::selectedPathsExclusion() {
    if(mSelectedBoxes.isEmpty()) return;

    SmartVectorPath * const newPath1 = getPathResultingFromOperation(
                SkPathOp::kDifference_SkPathOp);
    SmartVectorPath * const newPath2 = getPathResultingFromOperation(
                SkPathOp::kReverseDifference_SkPathOp);

    clearBoxesSelection();
    addBoxToSelection(newPath1);
    addBoxToSelection(newPath2);
}


void Canvas::selectedPathsBreakApart() {
    if(mSelectedBoxes.isEmpty()) return;

    QList<qsptr<SmartVectorPath>> created;
    for(const auto &box : mSelectedBoxes) {
        if(const auto path = enve_cast<SmartVectorPath*>(box)) {
            created << path->breakPathsApart_k();
        }
    }
    for(const auto& path : created) {
        mCurrentContainer->addContained(path);
        addBoxToSelection(path.get());
    }
}

void Canvas::selectedPathsUnion() {
    if(mSelectedBoxes.isEmpty()) return;

    SmartVectorPath * const newPath = getPathResultingFromOperation(
                SkPathOp::kUnion_SkPathOp);

    clearBoxesSelection();
    addBoxToSelection(newPath);
}

void Canvas::selectedPathsCombine() {
    if(mSelectedBoxes.isEmpty()) return;

    SmartVectorPath * const newPath = getPathResultingFromCombine();

    clearBoxesSelection();
    addBoxToSelection(newPath);
}

void Canvas::alignSelectedBoxes(const Qt::Alignment align,
                                const AlignPivot pivot,
                                const AlignRelativeTo relativeTo) {
    if(mSelectedBoxes.isEmpty()) return;
    QRectF geometry;
    BoundingBox* skip = nullptr;
    switch(relativeTo) {
    case AlignRelativeTo::scene:
        geometry = QRectF(0., 0., mWidth, mHeight);
        break;
    case AlignRelativeTo::lastSelected:
        if(!mLastSelectedBox) return;
        skip = mLastSelectedBox;
        geometry = mLastSelectedBox->getAbsBoundingRect();
        break;
    case AlignRelativeTo::lastSelectedPivot:
        if(!mLastSelectedBox) return;
        skip = mLastSelectedBox;
        geometry = QRectF(mLastSelectedBox->getPivotAbsPos(),mLastSelectedBox->getPivotAbsPos());
        break;
    case AlignRelativeTo::boundingBox:
        geometry = QRectF(0., 0., mWidth, mHeight); // TODO: not using it?
        break;
    }

    pushUndoRedoName("align");
    for(const auto &box : mSelectedBoxes) {
        if(box == skip) continue;
        switch(pivot) {
        case AlignPivot::pivot:
            box->alignPivot(align, geometry);
            break;
        case AlignPivot::geometry:
            box->alignGeometry(align, geometry);
            break;
        case AlignPivot::pivotItself:
            box->alignPivotItself(align, geometry, relativeTo, mLastSelectedBox->getPivotAbsPos());
            break;
        }
    }
}
