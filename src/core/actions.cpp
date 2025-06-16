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

#include "actions.h"
#include "Private/document.h"
#include "Private/scene.h"
#include "ViewLayers/viewlayer_preview.h"
#include "ViewLayers/viewlayer_selection.h"
#include "Paint/simplebrushwrapper.h"
#include "paintsettingsapplier.h"
#include "Sound/eindependentsound.h"
#include "Boxes/externallinkboxt.h"
#include "GUI/dialogsinterface.h"
#include "svgimporter.h"

#include <QMessageBox>
#include <QStandardItemModel>

Actions* Actions::sInstance = nullptr;

Actions::Actions(Document &document)
    : mDocument(document)
    , mActiveCanvas(ViewLayerPreview::sGetInstance())
    , mActiveCanvasSelection(ViewLayerSelection::sGetInstance()) {
    Q_ASSERT(!sInstance);
    sInstance = this;

    // Call once!!
    connectToViewLayerSelection();

    connect(&document, &Document::activeSceneSet,
            this, &Actions::connectToActiveScene);

    const auto pushName = [this](const QString& name) {
        if(!mActiveScene) return;
        mActiveScene->pushUndoRedoName(name);
    };

    { // deleteSceneAction
        const auto deleteSceneActionCan = [this]() {
            return static_cast<bool>(mActiveScene);
        };
        const auto deleteSceneActionExec = [this]() {
            if(!mActiveScene) return false;
            const auto sceneName = mActiveScene->name();
            const int buttonId = QMessageBox::question(
                        nullptr, "Delete " + sceneName,
                        QString("Are you sure you want to delete "
                        "%1? This action cannot be undone.").arg(sceneName),
                        "Cancel", "Delete");
            if(buttonId == 0) return false;
            return mDocument.removeScene(mActiveScene->ref<Scene>());
        };
        const auto deleteSceneActionText = [this]() {
            if(!mActiveScene) return QStringLiteral("Delete Scene");
            return "Delete " + mActiveScene->name();
        };
        deleteSceneAction = new Action(deleteSceneActionCan,
                                       deleteSceneActionExec,
                                       deleteSceneActionText,
                                       this);
    }

    { // sceneSettingsAction
        const auto sceneSettingsActionCan = [this]() {
            return static_cast<bool>(mActiveScene);
        };
        const auto sceneSettingsActionExec = [this]() {
            if(!mActiveScene) return;
            const auto& intr = DialogsInterface::instance();
            intr.showSceneSettingsDialog(mActiveScene);
        };

        sceneSettingsAction = new Action(sceneSettingsActionCan,
                                         sceneSettingsActionExec,
                                         tr("Scene Properties"),
                                         this);
    }

    { // undoAction
        const auto undoActionCan = [this]() {
            if(!mActiveScene) return false;
            return mActiveScene->undoRedoStack()->canUndo();
        };
        const auto undoActionExec = [this]() {
            mActiveScene->undo();
            afterAction();
        };
        const auto undoActionText = [this]() {
            if(!mActiveScene) return QStringLiteral("Undo");
            return mActiveScene->undoRedoStack()->undoText();
        };
        undoAction = new Action(undoActionCan, undoActionExec,
                                undoActionText, this);
    }

    { // redoAction
        const auto redoActionCan = [this]() {
            if(!mActiveScene) return false;
            return mActiveScene->undoRedoStack()->canRedo();
        };
        const auto redoActionExec = [this]() {
            mActiveScene->redo();
            afterAction();
        };
        const auto redoActionText = [this]() {
            if(!mActiveScene) return QStringLiteral("Redo");
            return mActiveScene->undoRedoStack()->redoText();
        };
        redoAction = new Action(redoActionCan, redoActionExec,
                                redoActionText, this);
    }

    { // raiseAction
        const auto raiseActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto raiseActionExec = [this]() {
            mActiveCanvasSelection->raiseSelectedBoxes();
            afterAction();
        };
        raiseAction = new UndoableAction(raiseActionCan, raiseActionExec,
                                         "Raise", pushName, this);
    }

    { // lowerAction
        const auto lowerActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto lowerActionExec = [this]() {
            mActiveCanvasSelection->lowerSelectedBoxes();
            afterAction();
        };
        lowerAction = new UndoableAction(lowerActionCan, lowerActionExec,
                                         "Lower", pushName, this);
    }

    { // raiseToTopAction
        const auto raiseToTopActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto raiseToTopActionExec = [this]() {
            mActiveCanvasSelection->raiseSelectedBoxesToTop();
            afterAction();
        };
        raiseToTopAction = new UndoableAction(raiseToTopActionCan,
                                              raiseToTopActionExec,
                                              "Raise to Top", pushName,
                                              this);
    }


    { // lowerToBottomAction
        const auto lowerToBottomActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto lowerToBottomActionExec = [this]() {
            mActiveCanvasSelection->lowerSelectedBoxesToBottom();
            afterAction();
        };
        lowerToBottomAction = new UndoableAction(lowerToBottomActionCan,
                                                 lowerToBottomActionExec,
                                                 "Lower to Bottom", pushName,
                                                 this);
    }

    { // objectsToPathAction
        const auto objectsToPathActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto objectsToPathActionExec = [this]() {
            mActiveCanvasSelection->convertSelectedBoxesToPath();
            afterAction();
        };
        objectsToPathAction = new UndoableAction(objectsToPathActionCan,
                                                 objectsToPathActionExec,
                                                 "Object to Path", pushName,
                                                 this);
    }

    { // strokeToPathAction
        const auto strokeToPathActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto strokeToPathActionExec = [this]() {
            mActiveCanvasSelection->convertSelectedPathStrokesToPath();
            afterAction();
        };
        strokeToPathAction = new UndoableAction(strokeToPathActionCan,
                                                strokeToPathActionExec,
                                                "Stroke to Path", pushName,
                                                this);
    }

    { // groupAction
        const auto groupActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto groupActionExec = [this]() {
            mActiveCanvasSelection->groupSelectedBoxes();
            afterAction();
        };
        groupAction = new UndoableAction(groupActionCan,
                                         groupActionExec,
                                         "Group", pushName,
                                         this);
    }

    { // ungroupAction
        const auto ungroupActionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto ungroupActionExec = [this]() {
            mActiveCanvasSelection->ungroupSelectedBoxes();
            afterAction();
        };
        ungroupAction = new UndoableAction(ungroupActionCan,
                                           ungroupActionExec,
                                           "Ungroup", pushName,
                                           this);
    }

    { // pathsUnionAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->selectedPathsUnion();
            afterAction();
        };
        pathsUnionAction = new UndoableAction(actionCan, actionExec,
                                              "Union", pushName, this);
    }

    { // pathsDifferenceAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->selectedPathsDifference();
            afterAction();
        };
        pathsDifferenceAction = new UndoableAction(actionCan, actionExec,
                                                   "Difference", pushName, this);
    }

    { // pathsIntersectionAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->selectedPathsIntersection();
            afterAction();
        };
        pathsIntersectionAction = new UndoableAction(actionCan, actionExec,
                                                     "Intersection", pushName, this);
    }

    { // pathsDivisionAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->selectedPathsDivision();
            afterAction();
        };
        pathsDivisionAction = new UndoableAction(actionCan, actionExec,
                                                 "Division", pushName, this);
    }

    { // pathsExclusionAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->selectedPathsExclusion();
            afterAction();
        };
        pathsExclusionAction = new UndoableAction(actionCan, actionExec,
                                                  "Exclusion", pushName, this);
    }

    { // pathsCombineAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->selectedPathsCombine();
            afterAction();
        };
        pathsCombineAction = new UndoableAction(actionCan, actionExec,
                                                "Combine", pushName, this);
    }

    { // pathsBreakApartAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->selectedPathsBreakApart();
            afterAction();
        };
        pathsBreakApartAction = new UndoableAction(actionCan, actionExec,
                                                   "Break Apart", pushName, this);
    }

    { // deleteAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty() ||
                   !mActiveCanvasSelection->isPointSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->deleteAction();
            afterAction();
        };
        deleteAction = new UndoableAction(actionCan, actionExec,
                                          "Delete", pushName, this);
    }

    { // copyAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->copyAction();
            afterAction();
        };
        copyAction = new UndoableAction(actionCan, actionExec,
                                        "Copy", pushName, this);
    }

    { // pasteAction
        const auto actionCan = [this]() {
            return !!mActiveCanvasSelection;
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->pasteAction();
            afterAction();
        };
        pasteAction = new UndoableAction(actionCan, actionExec,
                                        "Paste", pushName, this);
    }

    { // cutAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->cutAction();
            afterAction();
        };
        cutAction = new UndoableAction(actionCan, actionExec,
                                       "Cut", pushName, this);
    }

    { // duplicateAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->duplicateAction();
            afterAction();
        };
        duplicateAction = new UndoableAction(actionCan, actionExec,
                                             "Duplicate", pushName, this);
    }

    { // rotate90CWAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->rotateSelectedBoxesStartAndFinish(90);
            afterAction();
        };
        rotate90CWAction = new UndoableAction(actionCan, actionExec,
                                              "Rotate 90° CW", pushName, this);
    }

    { // rotate90CCWAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->rotateSelectedBoxesStartAndFinish(-90);
            afterAction();
        };
        rotate90CCWAction = new UndoableAction(actionCan, actionExec,
                                               "Rotate 90° CCW", pushName, this);
    }

    { // flipHorizontalAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->flipSelectedBoxesHorizontally();
            afterAction();
        };
        flipHorizontalAction = new UndoableAction(actionCan, actionExec,
                                                  "Flip Horizontal", pushName, this);
    }

    { // flipVerticalAction
        const auto actionCan = [this]() {
            if(!mActiveCanvasSelection) return false;
            return !mActiveCanvasSelection->isBoxSelectionEmpty();
        };
        const auto actionExec = [this]() {
            mActiveCanvasSelection->flipSelectedBoxesVertically();
            afterAction();
        };
        flipVerticalAction = new UndoableAction(actionCan, actionExec,
                                                "Flip Vertical", pushName, this);
    }
}

void Actions::setTextAlignment(const Qt::Alignment alignment) const {
    if(!mActiveCanvasSelection) return;
    mDocument.fTextAlignment = alignment;
    mActiveCanvasSelection->setSelectedTextAlignment(alignment);
    afterAction();
}

void Actions::setTextVAlignment(const Qt::Alignment alignment) const {
    if(!mActiveCanvasSelection) return;
    mDocument.fTextVAlignment = alignment;
    mActiveCanvasSelection->setSelectedTextVAlignment(alignment);
    afterAction();
}

void Actions::setFontFamilyAndStyle(const QString& family,
                                    const SkFontStyle& style) const {
    if(!mActiveCanvasSelection) return;
    mDocument.fFontFamily = family;
    mDocument.fFontStyle = style;
    mActiveCanvasSelection->setSelectedFontFamilyAndStyle(family, style);
    afterAction();
}

void Actions::setFontSize(const qreal size) const {
    if(!mActiveCanvasSelection) return;
    mDocument.fFontSize = size;
    mActiveCanvasSelection->setSelectedFontSize(size);
    afterAction();
}

void Actions::setFontText(const QString &text)
{
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->setSelectedFontText(text);
    afterAction();
}

void Actions::connectPointsSlot() const
{
    qDebug() << "connectPointsSlot";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->connectPoints();
    afterAction();
}

void Actions::disconnectPointsSlot() const
{
    qDebug() << "disconnectPointsSlot";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->disconnectPoints();
    afterAction();
}

void Actions::mergePointsSlot() const
{
    qDebug() << "mergePointsSlot";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->mergePoints();
    afterAction();
}

void Actions::subdivideSegments() const
{
    qDebug() << "subdivideSegments";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->subdivideSegments();
    afterAction();
}

void Actions::makePointCtrlsSymmetric() const
{
    qDebug() << "makePointCtrlsSymmetric";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->makePointCtrlsSymmetric();
    afterAction();
}

void Actions::makePointCtrlsSmooth() const
{
    qDebug() << "makePointCtrlsSmooth";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->makePointCtrlsSmooth();
    afterAction();
}

void Actions::makePointCtrlsCorner() const
{
    qDebug() << "makePointCtrlsCorner";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->makePointCtrlsCorner();
    afterAction();
}

void Actions::makeSegmentLine() const
{
    qDebug() << "makeSegmentLine";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->makeSegmentLine();
    afterAction();
}

void Actions::makeSegmentCurve() const
{
    qDebug() << "makeSegmentCurve";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->makeSegmentCurve();
    afterAction();
}

void Actions::newEmptyPaintFrame() const
{
    qDebug()<< "newEmptyPaintFrame";
    if (!mActiveCanvasSelection) { return; }
    mActiveCanvasSelection->newEmptyPaintFrameAction();
    afterAction();
}

void Actions::selectAllAction() const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->selectAllAction();
}

void Actions::invertSelectionAction() const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->invertSelectionAction();
}

void Actions::clearSelectionAction() const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->clearSelectionAction();
}

void Actions::startSelectedStrokeColorTransform() const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->startSelectedStrokeColorTransform();
    afterAction();
}

void Actions::startSelectedFillColorTransform() const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->startSelectedFillColorTransform();
    afterAction();
}

void Actions::strokeCapStyleChanged(const SkPaint::Cap capStyle) const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->setSelectedCapStyle(capStyle);
    afterAction();
}

void Actions::strokeJoinStyleChanged(const SkPaint::Join joinStyle) const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->setSelectedJoinStyle(joinStyle);
    afterAction();
}

void Actions::strokeWidthAction(const QrealAction &action) const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->strokeWidthAction(action);
    afterAction();
}

void Actions::applyPaintSettingToSelected(
        const PaintSettingsApplier &setting) const {
    if(!mActiveCanvasSelection) return;
    mActiveCanvasSelection->applyPaintSettingToSelected(setting);
    afterAction();
}

void Actions::updateAfterFrameChanged(const int currentFrame) const {
    if(!mActiveScene) return;
    mActiveScene->getCurrentGroup()->anim_setAbsFrame(currentFrame);
    afterAction();
}

void Actions::setClipToCanvas(const bool clip) {
    if(!mActiveScene) return;
    if(mActiveScene->clipToCanvas() == clip) return;
    mActiveScene->setClipToCanvas(clip);
    mActiveScene->updateAllBoxes(UpdateReason::userChange);
    mActiveScene->sceneFramesUpToDate();
    afterAction();
}

void Actions::setRasterEffectsVisible(const bool bT) {
    if(!mActiveScene) return;
    mActiveScene->setRasterEffectsVisible(bT);
    mActiveScene->updateAllBoxes(UpdateReason::userChange);
    afterAction();
}

void Actions::setPathEffectsVisible(const bool bT) {
    if(!mActiveScene) return;
    mActiveScene->setPathEffectsVisible(bT);
    mActiveScene->updateAllBoxes(UpdateReason::userChange);
    afterAction();
}

#include "filesourcescache.h"
//#include "svgimporter.h"
#include "Boxes/videobox.h"
#include "Boxes/imagebox.h"
#include "importhandler.h"

eBoxOrSound* Actions::handleDropEvent(QDropEvent * const event,
                                      const QPointF& relDropPos,
                                      const int frame)
{
    if (!mActiveScene) { return nullptr; }
    const QMimeData* mimeData = event->mimeData();

    QList<QUrl> internalUrls;
    if (mimeData->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QStandardItemModel dummyModel;
        if (dummyModel.dropMimeData(event->mimeData(),
                                    event->dropAction(),
                                    0,
                                    0,
                                    QModelIndex()))
        {
            QModelIndex index = dummyModel.index(0, 0);
            QString path = index.data(Qt::UserRole).toString();
            if (QFile::exists(path)) { internalUrls << QUrl::fromUserInput(path); }
        }
    }

    if (mimeData->hasUrls() || internalUrls.count() > 0) {
        event->acceptProposedAction();
        const QList<QUrl> urlList = internalUrls.count() > 0 ? internalUrls : mimeData->urls();
        for (int i = 0; i < urlList.size() && i < 32; i++) {
            try {
                return importFile(urlList.at(i).toLocalFile(),
                                  mActiveScene->getCurrentGroup(),
                                  0, relDropPos, frame);
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
        }
    }
    return nullptr;
}


qsptr<ImageBox> createImageBox(const QString &path) {
    const auto img = enve::make_shared<ImageBox>(path);
    return img;
}

#include "Boxes/imagesequencebox.h"
qsptr<ImageSequenceBox> createImageSequenceBox(const QString &folderPath) {
    const auto aniBox = enve::make_shared<ImageSequenceBox>();
    aniBox->setFolderPath(folderPath);
    return aniBox;
}

#include "Boxes/videobox.h"
qsptr<VideoBox> createVideoForPath(const QString &path) {
    const auto vidBox = enve::make_shared<VideoBox>();
    vidBox->setFilePath(path);
    return vidBox;
}

qsptr<eIndependentSound> createSoundForPath(const QString &path) {
    const auto result = enve::make_shared<eIndependentSound>();
    result->setFilePath(path);
    return result;
}

eBoxOrSound *Actions::importFile(const QString &path) {
    if(!mActiveScene) return nullptr;
    return importFile(path, mActiveScene->getCurrentGroup());
}

eBoxOrSound *Actions::importFile(const QString &path,
                                 ContainerBox* const target,
                                 const int insertId,
                                 const QPointF &relDropPos,
                                 const int frame) {
    const auto scene = target->getParentScene();
    auto block = scene ? scene->blockUndoRedo() :
                         UndoRedoStack::StackBlock();
    qsptr<eBoxOrSound> result;
    const QFile file(path);
    if(!file.exists())
        RuntimeThrow("File " + path + " does not exit.");

    QFileInfo fInfo(path);

    if (fInfo.dir().absolutePath() != QDir::homePath()) {
        AppSupport::setSettings("files",
                                "recentImportDir",
                                fInfo.dir().absolutePath());
    }

    if(fInfo.isDir()) {
        result = createImageSequenceBox(path);
        target->insertContained(insertId, result);
    } else { // is file
        const QString extension = fInfo.suffix();
        if(isSoundExt(extension)) {
            result = createSoundForPath(path);
            target->insertContained(insertId, result);
        } else {
            try {
                if(isImageExt(extension)) {
                    result = createImageBox(path);
                } else if(isVideoExt(extension)) {
                    result = createVideoForPath(path);
                } else {
                    result = ImportHandler::sInstance->import(path, scene);
                }
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
        }
    }
    if(result) {
        if(frame) result->shiftAll(frame);
        block.reset();
        target->prp_pushUndoRedoName("Import File");
        target->insertContained(insertId, result);
        if(const auto importedBox = enve_cast<BoundingBox*>(result)) {
            importedBox->planCenterPivotPosition();
            importedBox->startPosTransform();
            importedBox->moveByAbs(relDropPos);
            importedBox->finishTransform();
        }
        if (const auto videoBox = enve_cast<VideoBox*>(result)) {
            Document::sInstance->newVideo(videoBox->getSpecs());
        }
    }
    afterAction();
    return result.get();
}

eBoxOrSound *Actions::importClipboard(const QString &content)
{
    if (!mActiveScene) { return nullptr; }
    return importClipboard(content, mActiveScene->getCurrentGroup());
}

eBoxOrSound *Actions::importClipboard(const QString &content,
                                      ContainerBox * const target,
                                      const int insertId,
                                      const QPointF &relDropPos,
                                      const int frame)
{
    const auto scene = target->getParentScene();
    auto block = scene ? scene->blockUndoRedo() : UndoRedoStack::StackBlock();
    qsptr<eBoxOrSound> result;

    if (!content.contains("<svg")) { RuntimeThrow(tr("Unable to parse SVG")); }

    try {
        const auto gradientCreator = [scene]() {
            return scene->createNewGradient();
        };
        result =  ImportSVG::loadSVGFile(content.toUtf8(),
                                         gradientCreator);
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
    }

    if (result) {
        if (frame) { result->shiftAll(frame); }
        block.reset();
        target->prp_pushUndoRedoName("Import from Clipboard");
        target->insertContained(insertId, result);
        if (const auto importedBox = enve_cast<BoundingBox*>(result)) {
            importedBox->planCenterPivotPosition();
            importedBox->startPosTransform();
            importedBox->moveByAbs(relDropPos);
            importedBox->finishTransform();
        }
    }
    afterAction();
    return result.get();
}

#include "Boxes/internallinkbox.h"
#include "Boxes/svglinkbox.h"

eBoxOrSound* Actions::linkFile(const QString &path)
{
    qsptr<eBoxOrSound> result;
    const QFileInfo info(path);
    const QString suffix = info.suffix();
    if (suffix == "svg") {
        const auto svg = enve::make_shared<SvgLinkBox>();
        svg->setFilePath(path);
        result = svg;
    } /*else if (suffix == "ora") {
        const auto ora = enve::make_shared<ImageBox>();
        ora->setFilePath(path);
        result = ora;
    }*/ else { RuntimeThrow(tr("Cannot link file format %1").arg(path)); }
    mActiveScene->getCurrentGroup()->addContained(result);
    mDocument.actionFinished();
    return result.get();
}

void Actions::setMovePathMode() {
    mDocument.setSceneMode(CanvasMode::boxTransform);
}

void Actions::setMovePointMode() {
    mDocument.setSceneMode(CanvasMode::pointTransform);
}

void Actions::setAddPointMode() {
    mDocument.setSceneMode(CanvasMode::pathCreate);
}

void Actions::setDrawPathMode() {
    mDocument.setSceneMode(CanvasMode::drawPath);
}

void Actions::setRectangleMode() {
    mDocument.setSceneMode(CanvasMode::rectCreate);
}

void Actions::setPickPaintSettingsMode() {
    mDocument.setSceneMode(CanvasMode::pickFillStroke);
}

void Actions::setCircleMode() {
    mDocument.setSceneMode(CanvasMode::circleCreate);
}

void Actions::setTextMode() {
    mDocument.setSceneMode(CanvasMode::textCreate);
}

void Actions::setPaintMode() {
    mDocument.setSceneMode(CanvasMode::paint);
}

void Actions::setNullMode() {
    mDocument.setSceneMode(CanvasMode::nullCreate);
}

void Actions::finishSmoothChange() {
    mSmoothChange = false;
    //    mDocument.actionFinished();
}

void Actions::connectToActiveScene(Scene* const scene) {
    auto& conn = mActiveScene.assign(scene);

    deleteSceneAction->raiseCanExecuteChanged();
    deleteSceneAction->raiseTextChanged();
    if(mActiveScene) {
        conn << connect(mActiveScene, &Scene::nameChanged,
                        deleteSceneAction, &Action::raiseTextChanged);
                        }
    sceneSettingsAction->raiseCanExecuteChanged();

    undoAction->raiseCanExecuteChanged();
    undoAction->raiseTextChanged();
    redoAction->raiseCanExecuteChanged();
    redoAction->raiseTextChanged();
    if(mActiveScene) {
        const auto urStack = mActiveScene->undoRedoStack();
        conn << connect(urStack, &UndoRedoStack::canUndoChanged,
                        undoAction, &Action::raiseCanExecuteChanged);
        conn << connect(urStack, &UndoRedoStack::undoTextChanged,
                        undoAction, &Action::raiseTextChanged);

        conn << connect(urStack, &UndoRedoStack::canRedoChanged,
                        redoAction, &Action::raiseCanExecuteChanged);
        conn << connect(urStack, &UndoRedoStack::redoTextChanged,
                        redoAction, &Action::raiseTextChanged);
                        }
}

// Call once!!
void Actions::connectToViewLayerSelection() {
    raiseAction->raiseCanExecuteChanged();
    lowerAction->raiseCanExecuteChanged();
    raiseToTopAction->raiseCanExecuteChanged();
    lowerToBottomAction->raiseCanExecuteChanged();
    if(mActiveCanvasSelection) {
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        raiseAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        lowerAction, &Scene::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        raiseToTopAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        lowerToBottomAction, &Action::raiseCanExecuteChanged);
                        }

    objectsToPathAction->raiseCanExecuteChanged();
    strokeToPathAction->raiseCanExecuteChanged();
    if(mActiveCanvasSelection) {
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        objectsToPathAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        strokeToPathAction, &Action::raiseCanExecuteChanged);
                        }

    groupAction->raiseCanExecuteChanged();
    ungroupAction->raiseCanExecuteChanged();
    if(mActiveCanvasSelection) {
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        groupAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        ungroupAction, &Action::raiseCanExecuteChanged);
    }


    pathsUnionAction->raiseCanExecuteChanged();
    pathsDifferenceAction->raiseCanExecuteChanged();
    pathsIntersectionAction->raiseCanExecuteChanged();
    pathsDivisionAction->raiseCanExecuteChanged();
    pathsExclusionAction->raiseCanExecuteChanged();
    pathsCombineAction->raiseCanExecuteChanged();
    pathsBreakApartAction->raiseCanExecuteChanged();
    if(mActiveCanvasSelection) {
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pathsUnionAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pathsDifferenceAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pathsIntersectionAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pathsDivisionAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pathsExclusionAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pathsCombineAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pathsBreakApartAction, &Action::raiseCanExecuteChanged);
                        }

    deleteAction->raiseCanExecuteChanged();
    copyAction->raiseCanExecuteChanged();
    pasteAction->raiseCanExecuteChanged();
    cutAction->raiseCanExecuteChanged();
    duplicateAction->raiseCanExecuteChanged();
    if(mActiveCanvasSelection) {
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        deleteAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerSelection::pointSelectionChanged,
                        deleteAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        copyAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        pasteAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        cutAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        duplicateAction, &Action::raiseCanExecuteChanged);
                        }

    rotate90CWAction->raiseCanExecuteChanged();
    rotate90CCWAction->raiseCanExecuteChanged();
    flipHorizontalAction->raiseCanExecuteChanged();
    flipVerticalAction->raiseCanExecuteChanged();
    if(mActiveCanvasSelection) {
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        rotate90CWAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        rotate90CCWAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        flipHorizontalAction, &Action::raiseCanExecuteChanged);
        conn << connect(mActiveCanvasSelection, &ViewLayerPreview::objectSelectionChanged,
                        flipVerticalAction, &Action::raiseCanExecuteChanged);
    }
}

void Actions::afterAction() const {
    Document::sInstance->actionFinished();
}
