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

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <set>
#include <QDomDocument>

#include "smartPointers/ememory.h"
#include "singlewidgettarget.h"
#include "paintsettings.h"
#include "Paint/brushcontexedwrapper.h"
#include "actions.h"
#include "Tasks/taskscheduler.h"
#include "clipboardcontainer.h"
#include "conncontextptr.h"
#include "zipfilesaver.h"
#include "zipfileloader.h"
#include "XML/runtimewriteid.h"
#include "XML/xevzipfilesaver.h"
#include "Boxes/videobox.h"
#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"
#include "gizmos.h"
#include "appsupport.h"

class SceneBoundGradient;
class FileDataCacheHandler;
class Canvas;
enum class CanvasMode : short;

enum class NodeVisiblity {
    dissolvedAndNormal,
    dissolvedOnly,
    normalOnly
};

enum class PaintMode {
    normal, erase, lockAlpha, colorize,
    move, crop
};

class CORE_EXPORT Document : public SingleWidgetTarget {
    Q_OBJECT
public:
    Document(TaskScheduler& taskScheduler);

    static Document* sInstance;

    stdsptr<Clipboard> fClipboardContainer;

    QString fEvFile;

    NodeVisiblity fNodeVisibility = NodeVisiblity::dissolvedAndNormal;

    bool fLocalPivot = true;

    bool fGizmoPositionVisibility = AppSupport::getSettings("gizmos", "Position", true).toBool();
    bool fGizmoRotateVisibility = AppSupport::getSettings("gizmos", "Rotate", true).toBool();
    bool fGizmoScaleVisibility = AppSupport::getSettings("gizmos", "Scale", false).toBool();
    bool fGizmoShearVisibility = AppSupport::getSettings("gizmos", "Shear", false).toBool();

    CanvasMode fCanvasMode;

    // bookmarked
    QList<QColor> fColors;
    QList<SimpleBrushWrapper*> fBrushes;

    Qt::Alignment fTextAlignment = Qt::AlignLeft;
    Qt::Alignment fTextVAlignment = Qt::AlignTop;
    QString fFontFamily = "Arial";
    SkFontStyle fFontStyle;
    qreal fFontSize = 72;

    FillSettings fFill;
    StrokeSettings fStroke;

    SimpleBrushWrapper* fOutlineBrush = nullptr;

    bool fDrawPathManual = false;
    int fDrawPathSmooth = 25;
    qreal fDrawPathMaxError = 50;

    QColor fBrushColor;
    SimpleBrushWrapper* fBrush = nullptr;
    bool fOnionVisible = false;
    PaintMode fPaintMode = PaintMode::normal;

    QList<qsptr<Canvas>> fScenes;
    std::map<Canvas*, int> fVisibleScenes;
    ConnContextPtr<Canvas> fActiveScene;
    qptr<BoundingBox> fCurrentBox;

    // gizmo pivot state
    QPointF fPivotPosForGizmos = QPointF(0,0);
    bool fPivotPosForGizmosValid = false;

    void updateScenes();
    void actionFinished();

    void replaceClipboard(const stdsptr<Clipboard>& container);
    DynamicPropsClipboard* getDynamicPropsClipboard() const;
    PropertyClipboard* getPropertyClipboard() const;
    KeysClipboard* getKeysClipboard() const;
    BoxesClipboard* getBoxesClipboard() const;
    SmartPathClipboard* getSmartPathClipboard() const;

    void setPath(const QString& path);
    QString projectDirectory() const;

    void setCanvasMode(const CanvasMode mode);

    void setGizmoVisibility(const Friction::Core::Gizmos::Interact &ti,
                            const bool visibility);
    bool getGizmoVisibility(const Friction::Core::Gizmos::Interact &ti);

    Canvas * createNewScene(const bool emitCreated = true);
    bool removeScene(const qsptr<Canvas>& scene);
    bool removeScene(const int id);

    void addVisibleScene(Canvas * const scene);
    bool removeVisibleScene(Canvas * const scene);

    void setActiveScene(Canvas * const scene);
    void clearActiveScene();
    int getActiveSceneFrame() const;
    void setActiveSceneFrame(const int frame);
    void incActiveSceneFrame();
    void decActiveSceneFrame();

    void addBookmarkBrush(SimpleBrushWrapper* const brush);

    void removeBookmarkBrush(SimpleBrushWrapper* const brush);

    void addBookmarkColor(const QColor& color);

    void removeBookmarkColor(const QColor& color);
//
    void setBrush(BrushContexedWrapper * const brush);

    void setBrushColor(const QColor &color);

    void incBrushRadius();
    void decBrushRadius();

    void setOnionDisabled(const bool disabled);
    void setPaintMode(const PaintMode mode);
//
    void clear();
//
    void writeScenes(eWriteStream &dst) const;
    void readScenes(eReadStream &src);

    void writeXEV(const std::shared_ptr<XevZipFileSaver>& xevFileSaver,
                  const RuntimeIdToWriteId& objListIdConv) const;
    void writeDoxumentXEV(QDomDocument& doc) const;
    void writeScenesXEV(const std::shared_ptr<XevZipFileSaver>& xevFileSaver,
                        const RuntimeIdToWriteId& objListIdConv) const;

    void readDocumentXEV(ZipFileLoader& fileLoader,
                         QList<Canvas*>& scenes);
    void readScenesXEV(XevReadBoxesHandler& boxReadHandler,
                       ZipFileLoader& fileLoader,
                       const QList<Canvas*>& scenes,
                       const RuntimeIdToWriteId& objListIdConv);

    void SWT_setupAbstraction(SWT_Abstraction * const abstraction,
                              const UpdateFuncs &updateFuncs,
                              const int visiblePartWidgetId);
private:
    void readDocumentXEV(const QDomDocument& doc,
                         QList<Canvas*>& scenes);

    Clipboard *getClipboard(const ClipboardType type) const;

    void writeBookmarked(eWriteStream &dst) const;
    void readBookmarked(eReadStream &src);

    void readGradients(eReadStream& src);
signals:
    void canvasModeSet(CanvasMode);

    void gizmoVisibilityChanged(const Friction::Core::Gizmos::Interact &ti,
                                const bool &visible);

    void sceneCreated(Canvas*);
    void sceneRemoved(Canvas*);
    void sceneRemoved(int);
//
    void activeSceneSet(Canvas*);

    void activeSceneFrameSet(int);
//
    void currentBoxChanged(BoundingBox*);
//
    void selectedPaintSettingsChanged();
//
    void brushChanged(BrushContexedWrapper* brush);
    void brushColorChanged(QColor color);
    void brushSizeChanged(float size);

    void paintModeChanged(const PaintMode mode);
//
    void bookmarkColorAdded(QColor color);
    void bookmarkColorRemoved(QColor color);
    void bookmarkBrushAdded(SimpleBrushWrapper* brush);
    void bookmarkBrushRemoved(SimpleBrushWrapper* brush);
//
    void evFilePathChanged(QString);
    void documentChanged();
    void openTextEditor();
    void openMarkerEditor();
    void openExpressionDialog(QrealAnimator* const target);
    void openApplyExpressionDialog(QrealAnimator* const target);
    void newVideo(const VideoBox::VideoSpecs specs);
    void currentPixelColor(const QColor &color);
};

#endif // DOCUMENT_H
