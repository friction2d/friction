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
#include "../ui_global.h"
#include "../widgets/glwindow.h"
#include "../../core/skia/skiaincludes.h"
#include "../../core/Private/document.h"
#include "../../core/eevent.h"

// CanvasBase
//
// ONLY ONE RESPONSIBILITY!!!
// A widget which renders the ViewLayers content:
// shapes, movable points, selection... to the CanvasWindow.
class UI_EXPORT BaseCanvas : public GLWindow {
public:
    explicit BaseCanvas(
        int width,
        int height,
        QColor backgroundColor = QColor(0, 0, 0),
        QWidget * parent = nullptr)
            : GLWindow(parent)
            , _width(width)
            , _height(height)
            , _backgroundColor(backgroundColor) {};
    ~BaseCanvas() = default;

    void renderSk(SkCanvas * const canvas) override;

    void setBackgroundColor(QColor color) { _backgroundColor = color; };
    QColor backgroundColor() { return _backgroundColor; };

    int width() { return _width; };
    int height() { return _height; };

    void setSize(int width, int height) { _width = width; _height = height; };

    // Scene id
    // Used internally in XEV format for identification purposes
    int getWriteId() { return -1; };
    int getDocumentId() { return -1; };

private:
    std::map<std::string, ViewLayer&> _viewLayers;

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

    // Mouse events
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
};

#endif // BASE_CANVAS_H
