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
#include <QObject>
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

class UndoRedoStack;
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
class Scene : public QObject {
    Q_OBJECT
public:
    Scene(QString sceneName, qreal canvasWidth, qreal canvasHeight, qreal fps);
    ~Scene();

    static Scene* fromContainerBox(ContainerBox *containerBox);

    ContainerBox *getCurrentGroup() const { return _currentGroup; };
    void setCurrentGroup(ContainerBox* containerBox) { _currentGroup = containerBox; };

    void setCurrentGroupParentAsCurrentGroup();

    // Properties (name, width, fps...)

    QString name() const { return _name; };
    qreal fps() const { return _fps; };
    qreal canvasWidth() const { return _canvasWidth; };
    qreal canvasHeight() const { return _canvasHeight; };
    QSize canvasSize() { return QSize(canvasWidth(), canvasHeight()); };

    bool clipToCanvas() const { return _clipToCanvas; };
    void setClipToCanvas(bool clipToCanvas) { _clipToCanvas = clipToCanvas; };

    void queTasks();

    // Frames of the timeline

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

    const QList<BoundingBox*> &getContainedBoxes() const { return getCurrentGroup()->getContainedBoxes(); };

    // Gradients

    SceneBoundGradient * createNewGradient();
    bool removeGradient(const qsptr<SceneBoundGradient> &gradient);

    SceneBoundGradient * getGradientWithRWId(const int rwId) const;
    SceneBoundGradient * getGradientWithDocumentId(const int id) const;
    SceneBoundGradient * getGradientWithDocumentSceneId(const int id) const;

    // Undo/redo
    bool newUndoRedoSet();

    void undo();
    void redo();

    UndoRedoStack::StackBlock blockUndoRedo();
    void unblockUndoRedo();

    void addUndoRedo(const QString &name,
                     const stdfunc<void ()> &undo,
                     const stdfunc<void ()> &redo);
    void pushUndoRedoName(const QString &name) const;

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

    // Selected Propertys
    // This is MAGIC, DO NOT TOUCH

    template <class T = MovablePoint>
    [[deprecated]]
    void executeOperationOnSelectedPoints(const std::function<void(T*)> &op) {}

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

    void saveSVG(SvgExporter& exp, DomEleTask* const eleTask) const;

    void writeSettings(eWriteStream &dst) const;
    void readSettings(eReadStream &src);
    void writeBoundingBox(eWriteStream& dst) const;
    void readBoundingBox(eReadStream& src);
    void writeMarkers(eWriteStream &dst) const;
    void readMarkers(eReadStream &src);

    void writeBoxOrSoundXEV(const stdsptr<XevZipFileSaver>& xevFileSaver,
                            const RuntimeIdToWriteId& objListIdConv,
                            const QString& path) const;
    void readBoxOrSoundXEV(XevReadBoxesHandler& boxReadHandler,
                           ZipFileLoader& fileLoader, const QString& path,
                           const RuntimeIdToWriteId& objListIdConv);

    void writeAllContained(eWriteStream &dst) const;
    void writeAllContainedXEV(const stdsptr<XevZipFileSaver>& fileSaver,
                              const RuntimeIdToWriteId& objListIdConv,
                              const QString& path) const;

    QString makeNameUniqueForDescendants(
            const QString& name, eBoxOrSound * const skip = nullptr);
    QString makeNameUniqueForContained(
            const QString& name, eBoxOrSound * const skip = nullptr);

signals:
    void requestUpdate();
    void destroyed();
    void dimensionsChanged(int, int);
    void fpsChanged(qreal);
    void nameChanged(const QString&, QPrivateSignal);

protected:
    qsptr<UndoRedoStack> _undoRedoStack;

private:
    qptr<ContainerBox> _currentGroup;

    QString _name;
    bool _clipToCanvas;

    qreal _fps;
    qreal _canvasWidth;
    qreal _canvasHeight;

    int _currentFrame;
    FrameRange _range{0, 200};

private:
    // Selected properties
    ConnContextObjList<Property*> _selectedProperties;
};

#endif // FRICTION_CORE_BOXES_SCENE_H
