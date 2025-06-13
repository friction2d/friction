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
#include "conncontextptr.h"
#include "Properties/property.h"
#include "conncontextobjlist.h"
// Undo/redo
#include "undoredo.h"

class UndoRedoStack;


// Represents a scene in a Document (what Canvas used to be)
// Has:
//  - Keys + FrameRange
//  - Objects (_currentGroup)
//  - read/write
//
// I don't like some things (undoRedoStack, gradients, selectedProperties...)
class Scene {
public:
    Scene(QString sceneName, qreal canvasWidth, qreal canvasHeight, qreal fps);
    ~Scene();

    ContainerBox *getCurrentGroup() { return _currentGroup; };

    // Properties (name, width, fps...)

    QString name() const { return _name; };
    qreal fps() const { return _fps; };
    qreal canvasWidth() const { return _canvasWidth; };
    qreal canvasHeight() const { return _canvasHeight; };
    QSize canvasSize() { return QSize(canvasWidth(), canvasHeight()); };

    bool clipToCanvas() { return _clipToCanvas; };
    void setClipToCanvas(bool clipToCanvas) { _clipToCanvas = clipToCanvas; };

    // Frames of the timeline

    int currentFrame() const { return _currentFrame; };
    FrameRange getFrameRange() const { return _range; };
    int getMinFrame() const
    {
        return _range.fMin;
    }
    int getMaxFrame() const
    {
        return _range.fMax;
    }

    const QList<BoundingBox*> &getContainedBoxes() { return getCurrentGroup()->getContainedBoxes(); };

    // Gradients

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
        return mUndoRedoStack.get();
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

    // Selected properties
    // This is MAGIC, DO NOT TOUCH

    void addToSelectedProps(Property* const prop) {
        auto& conn = mSelectedProps.addObj(prop);
        conn << connect(prop, &Property::prp_parentChanged,
                        this, [this, prop]() { removeFromSelectedProps(prop); });
        prop->prp_setSelected(true);
    }

    void removeFromSelectedProps(Property* const prop) {
        mSelectedProps.removeObj(prop);
        prop->prp_setSelected(false);
    }

    void clearSelectedProps() {
        const auto selected = mSelectedProps.getList();
        for(const auto prop : selected) {
            removeFromSelectedProps(prop);
        }
    }

public:
    bool getRasterEffectsVisible() const { return _rasterEffectsVisible; };
public:
    // Import / export

    // TODO(kaixoo):
    // Ideally these should form part of different interfaces (SVG, XEV...)
    // That one can ask for without having to care of implementation details

    void saveSVG(SvgExporter& exp, DomEleTask* const eleTask) const;

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

protected:
    qsptr<UndoRedoStack> mUndoRedoStack;

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
    bool _rasterEffectsVisible;

    // Selected properties
    ConnContextObjList<Property*> mSelectedProps;
};

#endif // FRICTION_CORE_BOXES_SCENE_H
