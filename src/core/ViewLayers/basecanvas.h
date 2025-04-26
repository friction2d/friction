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

#include <QScrollArea>
#include <QRect>

#include "viewlayer.h"

class SkCanvas;
class Document;


// CanvasBase
//
// ONLY ONE RESPONSIBILITY!!!
// A widget which renders the ViewLayers content:
// shapes, movable points, selection... to the CanvasWindow.
class CanvasBase : public QScrollArea {
public:
    explicit CanvasBase(Document& document,
                        QWidget *parent = nullptr);
    ~CanvasBase() = default;

    void repaint(SkCanvas *canvas);

private: /* Data */
    std::map<std::string, ViewLayer> _viewLayers;

    // Zoom & translation
    QRect _translationVector;
    qreal _zoomMultiplier = 1;

    // Resolution 25% - 100%
    qreal _resolution = 1;

    Document &_document;

public: /* Getters / setters */
    void addViewLayer(ViewLayer &viewLayer) {
        auto layerId = viewLayer.layerId();

        _viewLayers[layerId] = viewLayer;
    }
    void removeViewLayer(std::string layerId) {
        _viewLayers[layerId] = delete;
    }

    float zoom() { return _zoomMultiplier; }
    void setZoom(qreal zoom) { _zoomMultiplier = zoom; }

    QRect translation() { return _translationVector; }
    void setTranslation(QRect translationVector) { _translationVector = translationVector; }

    int resolution() { return _resolution; }
    void setResolution(qreal resolution) { _resolution = resolution; }
};

#endif // BASE_CANVAS_H
