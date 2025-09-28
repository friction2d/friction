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

#include "Private/document.h"
#include "FileCacheHandlers/filecachehandler.h"
#include "canvas.h"
#include "simpletask.h"
#include "appsupport.h"

Document* Document::sInstance = nullptr;

Document::Document(TaskScheduler& taskScheduler) {
    Q_ASSERT(!sInstance);
    sInstance = this;
    fShowRotateGizmo = AppSupport::getSettings("gizmos", "Rotate", true).toBool();
    fShowPositionGizmo = AppSupport::getSettings("gizmos", "Position", true).toBool();
    fShowScaleGizmo = AppSupport::getSettings("gizmos", "Scale", false).toBool();
    fShowShearGizmo = AppSupport::getSettings("gizmos", "Shear", false).toBool();
    connect(&taskScheduler, &TaskScheduler::finishedAllQuedTasks,
            this, &Document::updateScenes);
}

void Document::updateScenes() {
    SimpleTask::sProcessAll();
    TaskScheduler::instance()->queTasks();

    for(const auto& scene : fVisibleScenes) {
        emit scene.first->requestUpdate();
    }
}

void Document::actionFinished() {
    updateScenes();
    for (const auto& scene : fVisibleScenes) {
        const auto newUndoRedo = scene.first->newUndoRedoSet();
        if (newUndoRedo) {
            qDebug() << "document changed";
            emit documentChanged();
        }
    }
}

void Document::replaceClipboard(const stdsptr<Clipboard> &container) {
    fClipboardContainer = container;
}

Clipboard *Document::getClipboard(const ClipboardType type) const {
    if(!fClipboardContainer) return nullptr;
    if(type == fClipboardContainer->getType())
        return fClipboardContainer.get();
    return nullptr;
}

DynamicPropsClipboard* Document::getDynamicPropsClipboard() const {
    auto contT = getClipboard(ClipboardType::dynamicProperties);
    return static_cast<DynamicPropsClipboard*>(contT);
}

PropertyClipboard* Document::getPropertyClipboard() const {
    auto contT = getClipboard(ClipboardType::property);
    return static_cast<PropertyClipboard*>(contT);
}

KeysClipboard* Document::getKeysClipboard() const {
    auto contT = getClipboard(ClipboardType::keys);
    return static_cast<KeysClipboard*>(contT);
}

BoxesClipboard* Document::getBoxesClipboard() const {
    auto contT = getClipboard(ClipboardType::boxes);
    return static_cast<BoxesClipboard*>(contT);
}

SmartPathClipboard* Document::getSmartPathClipboard() const {
    auto contT = getClipboard(ClipboardType::smartPath);
    return static_cast<SmartPathClipboard*>(contT);
}

void Document::setPath(const QString &path) {
    fEvFile = path;
    emit evFilePathChanged(fEvFile);
}

QString Document::projectDirectory() const {
    if(fEvFile.isEmpty()) {
        return QDir::homePath();
    } else {
        QFileInfo fileInfo(fEvFile);
        return fileInfo.dir().path();
    }
}

void Document::setCanvasMode(const CanvasMode mode) {
    fCanvasMode = mode;
    emit canvasModeSet(mode);
    actionFinished();
}

void Document::setShowRotateGizmo(bool show)
{
    if (fShowRotateGizmo == show) { return; }
    fShowRotateGizmo = show;
    for (const auto &scene : fScenes) {
        if (scene) { scene->setShowRotateGizmo(show); }
    }
    AppSupport::setSettings("gizmos", "Rotate", show);
    emit showRotateGizmoChanged(show);
}

void Document::setShowPositionGizmo(bool show)
{
    if (fShowPositionGizmo == show) { return; }
    fShowPositionGizmo = show;
    for (const auto &scene : fScenes) {
        if (scene) { scene->setShowPositionGizmo(show); }
    }
    AppSupport::setSettings("gizmos", "Position", show);
    emit showPositionGizmoChanged(show);
}

void Document::setShowScaleGizmo(bool show)
{
    if (fShowScaleGizmo == show) { return; }
    fShowScaleGizmo = show;
    for (const auto &scene : fScenes) {
        if (scene) { scene->setShowScaleGizmo(show); }
    }
    AppSupport::setSettings("gizmos", "Scale", show);
    emit showScaleGizmoChanged(show);
}

void Document::setShowShearGizmo(bool show)
{
    if (fShowShearGizmo == show) { return; }
    fShowShearGizmo = show;
    for (const auto &scene : fScenes) {
        if (scene) { scene->setShowShearGizmo(show); }
    }
    AppSupport::setSettings("gizmos", "Shear", show);
    emit showShearGizmoChanged(show);
}

Canvas *Document::createNewScene(const bool emitCreated) {
    const auto newScene = enve::make_shared<Canvas>(*this);
    fScenes.append(newScene);
    SWT_addChild(newScene.get());
    newScene->setShowRotateGizmo(fShowRotateGizmo);
    newScene->setShowPositionGizmo(fShowPositionGizmo);
    newScene->setShowScaleGizmo(fShowScaleGizmo);
    newScene->setShowShearGizmo(fShowShearGizmo);
    if (emitCreated) {
        emit sceneCreated(newScene.get());
    }
    return newScene.get();
}

bool Document::removeScene(const qsptr<Canvas>& scene) {
    const int id = fScenes.indexOf(scene);
    return removeScene(id);
}

bool Document::removeScene(const int id) {
    if(id < 0 || id >= fScenes.count()) return false;
    const auto scene = fScenes.takeAt(id);
    SWT_removeChild(scene.data());
    emit sceneRemoved(scene.data());
    emit sceneRemoved(id);
    return true;
}

void Document::addVisibleScene(Canvas * const scene) {
    fVisibleScenes[scene]++;
    updateScenes();
}

bool Document::removeVisibleScene(Canvas * const scene) {
    const auto it = fVisibleScenes.find(scene);
    if(it == fVisibleScenes.end()) return false;
    if(it->second == 1) fVisibleScenes.erase(it);
    else it->second--;
    return true;
}

void Document::setActiveScene(Canvas * const scene) {
    if(scene == fActiveScene) return;
    auto& conn = fActiveScene.assign(scene);
    if(fActiveScene) {
        conn << connect(fActiveScene, &Canvas::currentBoxChanged,
                        this, &Document::currentBoxChanged);
        conn << connect(fActiveScene, &Canvas::selectedPaintSettingsChanged,
                        this, &Document::selectedPaintSettingsChanged);
        conn << connect(fActiveScene, &Canvas::destroyed,
                        this, &Document::clearActiveScene);
        conn << connect(fActiveScene, &Canvas::openTextEditor,
                        this, [this] () { emit openTextEditor(); });
        conn << connect(fActiveScene, &Canvas::openMarkerEditor,
                        this, [this] () { emit openMarkerEditor(); });
        conn << connect(fActiveScene, &Canvas::openExpressionDialog,
                        this, [this](QrealAnimator* const target) {
            emit openExpressionDialog(target);
        });
        conn << connect(fActiveScene, &Canvas::openApplyExpressionDialog,
                        this, [this](QrealAnimator* const target) {
            emit openApplyExpressionDialog(target);
        });
        conn << connect(fActiveScene, &Canvas::currentHoverColor,
                        this, [this](const QColor &color) {
            emit currentPixelColor(color);
        });
        emit currentBoxChanged(fActiveScene->getCurrentBox());
        emit selectedPaintSettingsChanged();
    }
    emit activeSceneSet(scene);
}

void Document::clearActiveScene() {
    setActiveScene(nullptr);
}

int Document::getActiveSceneFrame() const {
    if(!fActiveScene) return 0;
    return fActiveScene->anim_getCurrentAbsFrame();
}

void Document::setActiveSceneFrame(const int frame) {
    if(!fActiveScene) return;
    if(fActiveScene->anim_getCurrentRelFrame() == frame) return;
    fActiveScene->anim_setAbsFrame(frame);
    emit activeSceneFrameSet(frame);
}

void Document::incActiveSceneFrame() {
    setActiveSceneFrame(getActiveSceneFrame() + 1);
}

void Document::decActiveSceneFrame() {
    setActiveSceneFrame(getActiveSceneFrame() - 1);
}

void Document::addBookmarkBrush(SimpleBrushWrapper * const brush) {
    if(!brush) return;
    removeBookmarkBrush(brush);
    fBrushes << brush;
    emit bookmarkBrushAdded(brush);
}

void Document::removeBookmarkBrush(SimpleBrushWrapper * const brush) {
    if(fBrushes.removeOne(brush))
        emit bookmarkBrushRemoved(brush);
}

void Document::addBookmarkColor(const QColor &color) {
    removeBookmarkColor(color);
    fColors << color;
    emit bookmarkColorAdded(color);
}

void Document::removeBookmarkColor(const QColor &color)
{
    const auto rgba = color.rgba();
    for (int i = 0; i < fColors.count(); i++) {
        if (fColors.at(i).rgba() == rgba) {
            fColors.removeAt(i);
            emit bookmarkColorRemoved(color);
            break;
        }
    }
}

void Document::setBrush(BrushContexedWrapper * const brush) {
    fBrush = brush->getSimpleBrush();
    if(fBrush) {
        fBrush->setColor(fBrushColor);
        switch(fPaintMode) {
        case PaintMode::normal: fBrush->setNormalMode(); break;
        case PaintMode::erase: fBrush->startEraseMode(); break;
        case PaintMode::lockAlpha: fBrush->startAlphaLockMode(); break;
        case PaintMode::colorize: fBrush->startColorizeMode(); break;
        default: break;
        }
    }
    emit brushChanged(brush);
    emit brushSizeChanged(fBrush ? fBrush->getBrushSize() : 0.f);
    emit brushColorChanged(fBrush ? fBrush->getColor() : Qt::white);
}

void Document::setBrushColor(const QColor &color) {
    fBrushColor = color;
    if(fBrush) fBrush->setColor(fBrushColor);
    emit brushColorChanged(color);
}

void Document::incBrushRadius() {
    if(!fBrush) return;
    fBrush->incPaintBrushSize(0.3);
    emit brushSizeChanged(fBrush->getBrushSize());
}

void Document::decBrushRadius() {
    if(!fBrush) return;
    fBrush->decPaintBrushSize(0.3);
    emit brushSizeChanged(fBrush->getBrushSize());
}

void Document::setOnionDisabled(const bool disabled) {
    fOnionVisible = !disabled;
    actionFinished();
}

void Document::setPaintMode(const PaintMode mode) {
    if(mode == fPaintMode) return;
    fPaintMode = mode;
    if(fBrush) {
        switch(fPaintMode) {
        case PaintMode::normal: fBrush->setNormalMode(); break;
        case PaintMode::erase: fBrush->startEraseMode(); break;
        case PaintMode::lockAlpha: fBrush->startAlphaLockMode(); break;
        case PaintMode::colorize: fBrush->startColorizeMode(); break;
        default: break;
        }
    }
    emit paintModeChanged(mode);
}

void Document::clear() {
    setPath("");
    const int nScenes = fScenes.count();
    for(int i = 0; i < nScenes; i++) removeScene(0);
    replaceClipboard(nullptr);
    const auto iBrushes = fBrushes;
    for (const auto brush : iBrushes) {
        removeBookmarkBrush(brush);
    }
    fBrushes.clear();
    const auto iColors = fColors;
    for (const auto& color : iColors) {
        removeBookmarkColor(color);
    }
    fColors.clear();
}

void Document::SWT_setupAbstraction(SWT_Abstraction * const abstraction,
                                    const UpdateFuncs &updateFuncs,
                                    const int visiblePartWidgetId) {
    for(const auto& scene : fScenes) {
        auto abs = scene->SWT_abstractionForWidget(updateFuncs,
                                                   visiblePartWidgetId);
        abstraction->addChildAbstraction(abs->ref<SWT_Abstraction>());
    }
}
