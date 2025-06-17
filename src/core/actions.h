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

#ifndef ACTIONS_H
#define ACTIONS_H

#include <QString>

class QDropEvent;

#include "colorhelpers.h"
#include "skia/skiaincludes.h"
#include "action.h"
#include "conncontextptr.h"

class PaintSettingsApplier;
class Document;
class Scene;
class ViewLayerPreview;
class ViewLayerSelection;
class QrealAction;
class eBoxOrSound;
class ContainerBox;
class ExternalLinkBox;

// This is a class that allows you to interface directly with Scene or ViewLayers
// So instead of calling actions on them directly, you call them through this class
//
// auto actions = Actions::sGetInstance();
// actions->setTextAlignment();
// actions->selectAllAction();
class CORE_EXPORT Actions : public QObject {
public:
    Actions(Document& document);

    static Actions* sInstance;

    void setTextAlignment(const Qt::Alignment alignment) const;
    void setTextVAlignment(const Qt::Alignment alignment) const;
    void setFontFamilyAndStyle(const QString& family,
                               const SkFontStyle& style) const;
    void setFontSize(const qreal size) const;
    void setFontText(const QString &text);

    void connectPointsSlot() const;
    void disconnectPointsSlot() const;
    void mergePointsSlot() const;
    void subdivideSegments() const;

    void makePointCtrlsSymmetric() const;
    void makePointCtrlsSmooth() const;
    void makePointCtrlsCorner() const;

    void makeSegmentLine() const;
    void makeSegmentCurve() const;

    void newEmptyPaintFrame() const;

    void selectAllAction() const;
    void invertSelectionAction() const;
    void clearSelectionAction() const;

    void startSelectedStrokeColorTransform() const;
    void startSelectedFillColorTransform() const;

    void strokeCapStyleChanged(const SkPaint::Cap capStyle) const;
    void strokeJoinStyleChanged(const SkPaint::Join joinStyle) const;

    void strokeWidthAction(const QrealAction& action) const;

    void applyPaintSettingToSelected(
            const PaintSettingsApplier &setting) const;

    void updateAfterFrameChanged(const int currentFrame) const;

    void setClipToCanvas(const bool clip);

    void setRasterEffectsVisible(const bool bT);
    void setPathEffectsVisible(const bool bT);

    eBoxOrSound* handleDropEvent(QDropEvent * const event,
                                 const QPointF &relDropPos = QPointF(0, 0),
                                 const int frame = 0);
    eBoxOrSound* importFile(const QString &path);
    eBoxOrSound* importFile(const QString &path,
                            ContainerBox * const target,
                            const int insertId = 0,
                            const QPointF &relDropPos = QPointF(0, 0),
                            const int frame = 0);
    eBoxOrSound* importClipboard(const QString &content);
    eBoxOrSound* importClipboard(const QString &content,
                                 ContainerBox * const target,
                                 const int insertId = 0,
                                 const QPointF &relDropPos = QPointF(0, 0),
                                 const int frame = 0);
    eBoxOrSound *linkFile(const QString &path);
//
    void setMovePathMode();
    void setMovePointMode();
    void setAddPointMode();
    void setDrawPathMode();

    void setPaintMode();
    void setRectangleMode();
    void setCircleMode();
    void setTextMode();

    void setNullMode();
    void setPickPaintSettingsMode();
//
    bool smoothChange() const { return mSmoothChange; }
    void startSmoothChange() { mSmoothChange = true; }
    void finishSmoothChange();

    Action* deleteSceneAction;
    Action* sceneSettingsAction;

    UndoableAction* raiseAction;
    UndoableAction* lowerAction;
    UndoableAction* raiseToTopAction;
    UndoableAction* lowerToBottomAction;

    UndoableAction* objectsToPathAction;
    UndoableAction* strokeToPathAction;

    UndoableAction* groupAction;
    UndoableAction* ungroupAction;

    UndoableAction* pathsUnionAction;
    UndoableAction* pathsDifferenceAction;
    UndoableAction* pathsIntersectionAction;
    UndoableAction* pathsDivisionAction;
    UndoableAction* pathsExclusionAction;
    UndoableAction* pathsCombineAction;
    UndoableAction* pathsBreakApartAction;

    UndoableAction* deleteAction;
    UndoableAction* copyAction;
    UndoableAction* pasteAction;
    UndoableAction* cutAction;
    UndoableAction* duplicateAction;

    UndoableAction* rotate90CWAction;
    UndoableAction* rotate90CCWAction;
    UndoableAction* flipHorizontalAction;
    UndoableAction* flipVerticalAction;

    Action* undoAction;
    Action* redoAction;
private:
    void connectToActiveScene(Scene* const scene);
    void connectToViewLayerSelection(ViewLayerSelection* const viewLayerSelection);

    void afterAction() const;

    bool mSmoothChange = false;
    Document& mDocument;
    ConnContextPtr<ViewLayerSelection> mActiveCanvasSelection;
    ConnContextPtr<Scene> mActiveScene;
};

#endif // ACTIONS_H
