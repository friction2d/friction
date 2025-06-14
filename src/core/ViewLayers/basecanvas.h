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

#ifndef BASE_CANVAS_H
#define BASE_CANVAS_H

#include <map>

#include <QTransform>
#include <QWidget>
#include <QColor>

#include "viewlayer.h"
#include "skia/skiaincludes.h"
#include "Private/document.h"
#include "eevent.h"

// CanvasBase
//
// ONLY ONE RESPONSIBILITY!!!
// A widget which stores the ViewLayers
//
// Another class `CanvasWidget` then takes those and renders them to a GLWindow.
class BaseCanvas {
public:
    explicit BaseCanvas(
        int width,
        int height,
        QColor backgroundColor = QColor(0, 0, 0),
        QWidget * parent = nullptr)
            : _width(width)
            , _height(height)
            , _backgroundColor(backgroundColor) {
            Q_ASSERT(!sInstance);
            sInstance = this;
        };
    ~BaseCanvas() = default;

    static BaseCanvas *sGetInstance();

    // Must be called by the GLWindow
    void renderSk(SkCanvas * const canvas);

    void setBackgroundColor(QColor color) { _backgroundColor = color; };
    QColor backgroundColor() { return _backgroundColor; };

    int width() { return _width; };
    int height() { return _height; };

    void setSize(int width, int height) { _width = width; _height = height; };

    // Scene id
    // Used internally in XEV format for identification purposes
    int getWriteId() { return -1; };
    int getDocumentId() { return -1; };

    void zoomOutView();
    void zoomInView();
    void fitCanvasToSize();

    void resetTransformation();

private:
    std::map<std::string, ViewLayer&> _viewLayers;

    static BaseCanvas *sInstance;

    int _width;
    int _height;
    QColor _backgroundColor;

    // Zoom & translation
    QTransform _translationVector;
    qreal _zoomMultiplier = 1;

    // Resolution 25% - 100%
    qreal _resolution = 1;

public:
    void addViewLayer(ViewLayer& viewLayer) {
        auto layerId = viewLayer.layerId();

        // We can't use the operator[] here because ViewLayer doesn't have a default empty constructor.
        // stackoverflow.com/questions/6952486/recommended-way-to-insert-elements-into-map
        // wtf is this shit c++
        auto pair = std::pair<std::string, ViewLayer&>(layerId, viewLayer);
        _viewLayers.insert(pair);
    };
    void removeViewLayer(std::string layerId) {
        // Clear item from std::map using key-value
        _viewLayers.erase(layerId);
    };

    float zoom() { return _zoomMultiplier; };
    void setZoom(qreal zoom) { _zoomMultiplier = zoom; };

    QTransform translation() { return _translationVector; };
    void setTranslation(QTransform translationVector) { _translationVector = translationVector; };

    int resolution() { return _resolution; };
    void setResolution(qreal resolution) { _resolution = resolution; };

    QRect canvasBounds() const
    {
        return QRect(0, 0, _width, _height);
    }

    QRect maxBounds() const
    {
        return QRect(-_width/2, - _height/2, 2*_width, 2*_height);
    }

    QRect currentBounds() const
    {
        return maxBounds();
    }

protected: // Key input & Qt events
    bool event(QEvent *e);

    void hideEvent(QHideEvent* e);
    void showEvent(QShowEvent* e);
    void resizeEvent(QResizeEvent* e);

    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
private:
    void tabletEvent(QTabletEvent *e);

    // TODO: This should be delegated to ViewLayers
    bool handleSceneModeChangeKeyPress(QKeyEvent *event);
    bool handleCutCopyPasteKeyPress(QKeyEvent *event);
    bool handleTransformationKeyPress(QKeyEvent *event);
    bool handleZValueKeyPress(QKeyEvent *event);
    bool handleParentChangeKeyPress(QKeyEvent *event);
    bool handleGroupChangeKeyPress(QKeyEvent *event);
    bool handleResetTransformKeyPress(QKeyEvent *event);
    bool handleRevertPathKeyPress(QKeyEvent *event);
    bool handleStartTransformKeyPress(const eKeyEvent &e);
    bool handleSelectAllKeyPress(QKeyEvent *event);
    //bool handleShiftKeysKeyPress(QKeyEvent *event);
#ifdef Q_OS_MAC
    bool handleNativeGestures(QNativeGestureEvent *event);
#endif
};

#endif // BASE_CANVAS_H
