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

#include <QMatrix>

#include "viewlayer.h"

class SkCanvas;
class Document;


// CanvasBase
//
// ONLY ONE RESPONSIBILITY!!!
// A widget which renders the ViewLayers content:
// shapes, movable points, selection... to the CanvasWindow.
class CanvasBase {
public:
    CanvasBase();
    ~CanvasBase() = default;

    void repaint(SkCanvas *canvas);

private:
    std::map<std::string, std::unique_ptr<ViewLayer>> _viewLayers;

    // Zoom & translation
    QMatrix _translationVector;
    qreal _zoomMultiplier = 1;

    // Resolution 25% - 100%
    qreal _resolution = 1;

public:
    void addViewLayer(std::unique_ptr<ViewLayer> viewLayer) {
        auto layerId = viewLayer->layerId();

        _viewLayers[layerId] = std::move(viewLayer);
    };
    void removeViewLayer(std::string layerId) {
        // Clear item from std::map using key-value
        _viewLayers.erase(layerId);
    };

    float zoom() { return _zoomMultiplier; };
    void setZoom(qreal zoom) { _zoomMultiplier = zoom; };

    QMatrix translation() { return _translationVector; };
    void setTranslation(QMatrix translationVector) { _translationVector = translationVector; };

    int resolution() { return _resolution; };
    void setResolution(qreal resolution) { _resolution = resolution; };
};

#endif // BASE_CANVAS_H
