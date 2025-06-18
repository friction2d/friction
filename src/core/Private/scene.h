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

#ifndef FRICTION_CORE_BOXES_SCENE_H
#define FRICTION_CORE_BOXES_SCENE_H

#include <string>
#include <QThread>
#include <QString>
#include <QList>
#include <QSize>

#include "Boxes/containerbox.h"
#include "Boxes/boundingbox.h"
#include "Animators/coloranimator.h"
// Cache and memory
#include "smartPointers/selfref.h"
#include "smartPointers/ememory.h"
#include "CacheHandlers/usepointer.h"
#include "CacheHandlers/sceneframecontainer.h"
#include "CacheHandlers/hddcachablecachehandler.h"
// Selected properties
#include "Properties/property.h"
#include "conncontextobjlist.h"
#include "conncontextptr.h"
// Undo/redo
#include "undoredo.h"
// FrameMarker
#include "framerange.h"

class Document;
class SoundComposition;
class SceneFrameContainer;
class SceneBoundGradient;
class eWriteStream;
class eReadStream;


// Represents a scene in a Document (what Canvas used to be)
// Has:
//  - Keys + FrameRange
//  - Objects (_currentGroup)
//  - read/write
//
// I don't like some things (undoRedoStack, gradients, selectedProperties...)
//
// We inherit from QObject because that allows us to do signals
class Scene : public SelfRef {
    Q_OBJECT
    e_OBJECT
public:
    Scene(
        Document& document,
        qsptr<ContainerBox> defaultGroup,
        const QString sceneName = "New scene",
        const int canvasWidth = 1920,
        const int canvasHeight = 1080,
        const qreal fps = 24,
        const int frameCount = 200);

    // TODO: In C++17, make this a single default constructor
    // We currently need to overload constructor because C++14 is dumb
    Scene(
        Document& document,
        const QString sceneName = "New Scene",
        const int canvasWidth = 1920,
        const int canvasHeight = 1080,
        const qreal fps = 24,
        const int frameCount = 200);
    ~Scene();

    /* ========= Children ========= */
    qsptr<ContainerBox> getCurrentGroup() const { return _currentGroup; };
    void setCurrentGroup(qsptr<ContainerBox> containerBox) { _currentGroup = containerBox; };

    // Access the objects that the ContainerBox has
    const QList<BoundingBox*> &getContainedBoxes() const { return getCurrentGroup()->getContainedBoxes(); };
    void setCurrentGroupParentAsCurrentGroup();

    /* ========= Scene properties ========= */
    QString name() const { return _name; };
    qreal fps() const { return _fps; };
    qreal canvasWidth() const { return _canvasWidth; };
    qreal canvasHeight() const { return _canvasHeight; };
    QSize canvasSize() { return QSize(canvasWidth(), canvasHeight()); };
    bool clipToCanvas() const { return _clipToCanvas; };
    void setClipToCanvas(bool clipToCanvas) { _clipToCanvas = clipToCanvas; };

    // A SoundComposition basically contains all of the audio of the Scene
    // I don't know why it's not a regular object of the treeview
    SoundComposition *getSoundComposition() { return _soundComposition.get(); };

    HddCachableCacheHandler &getSceneFramesHandler() { return _sceneFramesHandler; };
    UseSharedPointer<SceneFrameContainer> &getSceneFrame() { return _sceneFrame; }

    // This is what draws the background color
    ColorAnimator *getBgColorAnimator()
    {
        return _backgroundColor.get();
    }

    // Performs tasks in the children objects
    // What kind of tasks? Is it QUEUE tasks?
    //
    // TODO(kaixoo): I don't like these kind of mutability in Scene
    // Scene should only contain data.
    void queTasks();

    // This is used for XEV writing
    // Comes from Property (?)
    int getWriteId() const;
    int getDocumentId() const;

    /* ========= Frames ========= */
    // Sets which frame is currently selected in the timeline
    int getCurrentFrame() const { return _currentFrame; };
    void setSceneFrame(const int relFrame);
    void setSceneFrame(const stdsptr<SceneFrameContainer> &cont);
    void setLoadingSceneFrame(const stdsptr<SceneFrameContainer> &cont);

    // This tells us the video length (range of frames which is supposed to be exported)
    FrameRange getFrameRange() const { return _range; };
    void setFrameRange(const FrameRange& range);

    void setMinFrameUseRange(const int min)
    {
        _sceneFramesHandler.setMinUseRange(min);
    }

    void setMaxFrameUseRange(const int max)
    {
        _sceneFramesHandler.setMaxUseRange(max);
    }

    void clearUseRange()
    {
        _sceneFramesHandler.clearUseRange();
    }

    // FrameMarker
    void setFrameIn(const bool enabled,
                    const int frameIn);
    void setFrameOut(const bool enabled,
                     const int frameOut);

    const FrameMarker getFrameIn() const;
    const FrameMarker getFrameOut() const;

    int getMinFrame() const
    {
        return _range.fMin;
    }
    int getMaxFrame() const
    {
        return _range.fMax;
    }

    //! Used for clip to canvas, when frames are not really changed.
    void sceneFramesUpToDate() const
    {
        // TODO(kaixoo): static_cast to Scene
        /*for (const auto &cont : _sceneFramesHandler) {
            const auto sceneCont = static_cast<SceneFrameContainer*>(cont.second.get());
            sceneCont->fBoxState = getCurrentGroup()->getStateId();
        };*/
    }

    // Display time code
    // What is a "display time code?"
    // Is it some kind of per-region hour format thing?
    bool getDisplayTimecode() { return _displayTimeCode; }
    void setDisplayTimecode(bool timecode)
    {
        _displayTimeCode = timecode;
        emit displayTimeCodeChanged(timecode);
    }

    // Gradients
    // What is a "Scene Bound Gradient"?
    const QList<qsptr<SceneBoundGradient>> &gradients() const
    {
        return _gradients;
    }
    SceneBoundGradient * createNewGradient();
    bool removeGradient(const qsptr<SceneBoundGradient> &gradient);

    SceneBoundGradient * getGradientWithRWId(const int rwId) const;
    SceneBoundGradient * getGradientWithDocumentId(const int id) const;
    SceneBoundGradient * getGradientWithDocumentSceneId(const int id) const;

    /* ========= Undo / Redo ========= */
    bool newUndoRedoSet();

    // Performs an undo action in the current scene
    void undo();
    // Performs a redo action in the current scene
    void redo();

    // Blocks the ability to perform undo/redo
    UndoRedoStack::StackBlock blockUndoRedo();
    void unblockUndoRedo();

    void addUndoRedo(const QString &name,
                     const stdfunc<void ()> &undo,
                     const stdfunc<void ()> &redo);
    void pushUndoRedoName(const QString &name) const;

    // UndoRedoStack is an object that allows us to keep track of all undo/redos that have been done in a scene
    // And go back / forward between them.
    UndoRedoStack* undoRedoStack() const
    {
        return _undoRedoStack.get();
    }

    // What are markers?
    // As in "tags" or "labels" in the timeline?
    void writeMarkers(eWriteStream &dst) const;
    void readMarkers(eReadStream &src);

    void setMarker(const QString &title,
                   const int frame);
    void setMarker(const int frame);
    void setMarkerEnabled(const int frame, const bool &enabled);
    bool hasMarker(const int frame,
                   const bool removeExists = false);
    bool hasMarkerIn(const int frame);
    bool hasMarkerOut(const int frame);
    bool hasMarkerEnabled(const int frame);
    bool removeMarker(const int frame);
    bool editMarker(const int frame,
                    const QString &title,
                    const bool enabled);
    void moveMarkerFrame(const int markerFrame,
                         const int newFrame);
    const QString getMarkerText(int frame);
    int getMarkerIndex(const int frame);
    const std::vector<FrameMarker> getMarkers();
    void clearMarkers();
    void updateMarkers();

    /* ========= Setters ========= */
    void setName(QString name) {
        _name = name;
        getCurrentGroup()->prp_setName(_name);
        // TODO(kaixoo): ???
        //emit nameChanged(_name, QPrivateSignal);
    };
    void setFps(qreal fps) {
        _fps = fps;
        emit fpsChanged(_fps);
    };
    void setCanvasWidth(qreal width) {
        _canvasWidth = width;
        emit dimensionsChanged(_canvasWidth, _canvasHeight);
    };
    void setCanvasHeight(qreal height) {
        _canvasHeight = height;
        emit dimensionsChanged(_canvasWidth, _canvasHeight);
    };

    void setCanvasSize(qreal width, qreal height) {
        _canvasWidth = width;
        _canvasHeight = height;
        emit dimensionsChanged(_canvasWidth, _canvasHeight);
    };

    void setCurrentFrame(int frame) { _currentFrame = frame; };

    /* ========= Selected Properties ========= */
    // What are "Selected Properties"? These are not classical key-value "properties", this is a base class for objects that provides basic things like prp_getName()...
    template <class T = MovablePoint>
    [[deprecated]]
    void executeOperationOnSelectedPoints(const std::function<void(T*)> &operation) {}

    template <class T = Property>
    void executeOperationOnSelectedProperties(const std::function<void(T*)> &operation) {
        for(const auto property : _selectedProperties) {
            const auto t = enve_cast<T*>(property);
            if(t) operation(t);
        }
    }

    template <class T = BoundingBox>
    [[deprecated]]
    void executeOperationOnSelectedBoxes(const std::function<void(const QList<T*>&)> &operation) {}

    template <class T = BoundingBox>
    [[deprecated]]
    void executeOperationOnSelectedBoxes(const std::function<void(T*)> &operation) {}


    void addToSelectedProperties(Property* const property) {
        auto& conn = _selectedProperties.addObj(property);
        conn << connect(property, &Property::prp_parentChanged,
                        this, [this, property]() { removeFromSelectedProperties(property); });
        property->prp_setSelected(true);
    }

    void removeFromSelectedProperties(Property* const property) {
        _selectedProperties.removeObj(property);
        property->prp_setSelected(false);
    }

    void clearSelectedProperties() {
        const auto selected = _selectedProperties.getList();
        for(const auto property : selected) {
            removeFromSelectedProperties(property);
        }
    }

public:
    /* ========= Import / export ========= */

    // Converts Scene objects to SVG and saves them to a SvgExporter.
    void saveSVG(SvgExporter& exp, DomEleTask* const eleTask) const;
    void saveSceneSVG(SvgExporter& exp) const;

    // Writes user data to an eWriteStream (such as last tool used, last fill color used...)
    void writeSettings(eWriteStream &dst) const;
    void readSettings(eReadStream &src);

    // What does "writeBoundingBox" mean?
    // Does it mean "import these files as bounding boxes in the scene?"
    void writeBoundingBox(eWriteStream& dst) const;
    void readBoundingBox(eReadStream& src);

    void writeBoxOrSoundXEV(const stdsptr<XevZipFileSaver>& xevFileSaver,
                            const RuntimeIdToWriteId& objListIdConv,
                            const QString& path) const;
    void readBoxOrSoundXEV(XevReadBoxesHandler& boxReadHandler,
                           ZipFileLoader& fileLoader, const QString& path,
                           const RuntimeIdToWriteId& objListIdConv);

    // Writes contained objects to an eWriteStream
    void writeAllContained(eWriteStream &dst) const;
    // Writes contained objects to XEV file format
    void writeAllContainedXEV(const stdsptr<XevZipFileSaver>& fileSaver,
                              const RuntimeIdToWriteId& objListIdConv,
                              const QString& path) const;

    /* ========= Utility functions ========= */

    // Checks if the object name is being used elsewhere, and gives it a unique name if necessary
    // We do this to avoid two objects having the same name
    QString makeNameUniqueForDescendants(
            const QString& name, eBoxOrSound * const skip = nullptr);
    // Checks if the object name is being used elsewhere, and gives it a unique name if necessary
    // We do this to avoid two objects having the same name
    QString makeNameUniqueForContained(
            const QString& name, eBoxOrSound * const skip = nullptr);

signals:
    // "Intrinsic" signals
    void requestUpdate();
    void destroyed();

    // Scene properties changed
    void dimensionsChanged(int, int);
    void fpsChanged(qreal);
    void nameChanged(const QString&, QPrivateSignal);

    // Frames
    void newFrameRange(FrameRange);
    void currentFrameChanged(int);

    // Others
    void displayTimeCodeChanged(bool displayTimeCode);
    void markersChanged();
    void gradientCreated(SceneBoundGradient*);
    void gradientRemoved(SceneBoundGradient*);
    // TODO(kaixoo) ???
    void currentPickedColor(const QColor &color);
    void currentHoverColor(const QColor &color);
    void requestEasingAction(const QString &easing);
    void currentCotnainerSet(ContainerBox* const container);

protected:
    qsptr<UndoRedoStack> _undoRedoStack;

private:
    qsptr<ContainerBox> _currentGroup;

    QString _name;
    bool _clipToCanvas;

    qreal _fps;
    qreal _canvasWidth;
    qreal _canvasHeight;

    qsptr<SoundComposition> _soundComposition;
    qsptr<ColorAnimator> _backgroundColor = enve::make_shared<ColorAnimator>();

    int _currentFrame;
    FrameRange _range{0, 200};

    FrameMarker _in{tr("In"), false, 0};
    FrameMarker _out{tr("Out"), false, 0};
    std::vector<FrameMarker> _markers;

    UseSharedPointer<SceneFrameContainer> _sceneFrame;
    HddCachableCacheHandler _sceneFramesHandler;

    bool _displayTimeCode;
    QList<qsptr<SceneBoundGradient>> _gradients;

private:
    // Selected properties
    ConnContextObjList<Property*> _selectedProperties;
};

#endif // FRICTION_CORE_BOXES_SCENE_H
