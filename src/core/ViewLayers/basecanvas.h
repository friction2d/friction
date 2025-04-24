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


#include <QWidget>

#include "viewlayer.h"

class SkCanvas;
class QRect;


typedef Bounds = (int x, int y, int width, int height);

// CanvasBase
//
// ONLY ONE RESPONSIBILITY!!!
// A widget which renders the ViewLayers content:
// shapes, movable points, selection...
class CanvasBase : public QWidget {
public:
    CanvasBase() {};
    ~CanvasBase() = default;

    void repaint(SkCanvas *canvas)

private: /* Data */
    std::map<string, ViewLayer> viewLayers;

    // Bounds
    //
    // The canvas looks like a widget in the UI. But internally,
    // it's a usual window and we must update its size and position
    // every frame.
    int boundX, boundY = 0
    int boundWidth, boundHeight = 0

    // Zoom & translation
    QRect translationVector;
    float zoomMultiplier = 1;

    // Resolution 25% - 100%
    float resolution = 1;

public: /* Getters / setters */
    void addViewLayer(ViewLayer &viewLayer) {
        auto layerId = viewLayer.layerId();
        
        viewLayers[layerId] = viewLayer;
    }
    void removeViewLayer(string layerId) {
        viewLayers[layerId] = delete;
    }

    Bounds bounds() {
        return (boundX, boundY, boundWidth, boundHeight);
    }
    void setBounds(int x, int y, int width, int height) {
        boundX = x;
        boundY = y;
        boundWidth = width;
        boundHeight = height;
    }

    float zoom() { return zoomMultiplier; }
    void setZoom(float zoom) { zoomMultiplier = zoom; }

    QRect translation() { return translationVector; }
    void setTranslation(QRect translationVector) { translationVector = translationVector; }
    
    int resolution() { return resolution; }
    void setResolution(int resolution) { resolution = resolution; }
}

#endif // BASE_CANVAS_H