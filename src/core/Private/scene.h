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

#include "smartPointers/selfref.h"
#include "Boxes/containerbox.h"
#include "Boxes/boundingbox.h"
// Selected properties
#include "Properties/property.h"
#include "conncontextobjlist.h"
#include "conncontextptr.h"
// Undo/redo
#include "undoredo.h"

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
public:
    Scene(QString sceneName, qreal canvasWidth, qreal canvasHeight, qreal fps, ContainerBox defaultGroup);
    ~Scene();

    // We contain all Scene objects in an object called "ContainerBox"
    // This is basically an object that can contain other objects.
    ContainerBox *getCurrentGroup() const { return _currentGroup; };
    void setCurrentGroup(ContainerBox* containerBox) { _currentGroup = containerBox; };

    // Access the objects that the ContainerBox has
    const QList<BoundingBox*> &getContainedBoxes() const { return getCurrentGroup()->getContainedBoxes(); };
    void setCurrentGroupParentAsCurrentGroup();

    // Scene properties (name, width, fps...)

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


    // Performs tasks in the children objects
    // What kind of tasks? Is it QUEUE tasks?
    //
    // TODO(kaixoo): I don't like these kind of mutability in Scene
    // Scene should only contain data.
    void queTasks();

    // Frames of the timeline

    // Sets which frame is currently selected in the timeline
    void setSceneFrame(const int relFrame);
    void setSceneFrame(const stdsptr<SceneFrameContainer> &cont);
    void setLoadingSceneFrame(const stdsptr<SceneFrameContainer> &cont);

    FrameRange getFrameRange() const { return _range; };
    void setFrameRange(const FrameRange& range);

    int currentFrame() const { return _currentFrame; };
    int getMinFrame() const
    {
        return _range.fMin;
    }
    int getMaxFrame() const
    {
        return _range.fMax;
    }

    // Gradients
    // What is a "Scene Bound Gradient"?

    SceneBoundGradient * createNewGradient();
    bool removeGradient(const qsptr<SceneBoundGradient> &gradient);

    SceneBoundGradient * getGradientWithRWId(const int rwId) const;
    SceneBoundGradient * getGradientWithDocumentId(const int id) const;
    SceneBoundGradient * getGradientWithDocumentSceneId(const int id) const;

    // Undo/redo
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

    // Setters

    void setName(QString name) { _name = name; };
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

    // Selected `Properties/property.h`
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
    // Import / export

    // TODO(kaixoo):
    // Ideally these should form part of different interfaces (SVG, XEV...)
    // That one can ask for without having to care of implementation details

    // Converts Scene objects to SVG and saves them to a SvgExporter.
    void saveSVG(SvgExporter& exp, DomEleTask* const eleTask) const;

    // Writes user data to an eWriteStream (such as last tool used, last fill color used...)
    void writeSettings(eWriteStream &dst) const;
    void readSettings(eReadStream &src);

    // What does "writeBoundingBox" mean?
    // Does it mean "import these files as bounding boxes in the scene?"
    void writeBoundingBox(eWriteStream& dst) const;
    void readBoundingBox(eReadStream& src);

    // What are markers?
    // As in "tags" or "labels" in the timeline?
    void writeMarkers(eWriteStream &dst) const;
    void readMarkers(eReadStream &src);

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

    // Checks if the object name is being used elsewhere, and gives it a unique name if necessary
    // We do this to avoid two objects having the same name
    QString makeNameUniqueForDescendants(
            const QString& name, eBoxOrSound * const skip = nullptr);
    // Checks if the object name is being used elsewhere, and gives it a unique name if necessary
    // We do this to avoid two objects having the same name
    QString makeNameUniqueForContained(
            const QString& name, eBoxOrSound * const skip = nullptr);

    // What is a "display time code?"
    // Is it some kind of per-region hour format thing?
    bool getDisplayTimecode() { return _displayTimeCode; }
    void setDisplayTimecode(bool timecode)
    {
        _displayTimeCode = timecode;
        emit displayTimeCodeChanged(timecode);
    }

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

protected:
    qsptr<UndoRedoStack> _undoRedoStack;

private:
    qptr<ContainerBox> _currentGroup;

    QString _name;
    bool _clipToCanvas;

    qreal _fps;
    qreal _canvasWidth;
    qreal _canvasHeight;

    qsptr<SoundComposition> _soundComposition;

    int _currentFrame;
    FrameRange _range{0, 200};

    bool _displayTimeCode;

private:
    // Selected properties
    ConnContextObjList<Property*> _selectedProperties;
};

#endif // FRICTION_CORE_BOXES_SCENE_H
