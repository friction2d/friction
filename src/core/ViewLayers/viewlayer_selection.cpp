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


#include "viewlayer_selection.h"

#include <QColor>
#include <QString>
#include <QRectF>
#include <QPointF>

#include "basecanvas.h"
#include "skia/skiaincludes.h"
#include "Drawables/rectangle_drawable.h"


ViewLayerSelection::ViewLayerSelection(BaseCanvas *canvas)
    : ViewLayer("selection_layer")
    , _baseCanvas(canvas) {
    Q_ASSERT(!sInstance);
    sInstance = this;
};

void ViewLayerSelection::repaint(SkCanvas * const canvas) {
    canvas->restore();

    if (_isCurrentlySelecting) {
        auto selectionRectangle = RectangleDrawable();

        // Set the rectangle's size and position
        selectionRectangle.setSize(_selectionRect.x(), _selectionRect.y());
        selectionRectangle.setPosition(_cursorPosition.x(), _cursorPosition.y());

        // Style the rectangle (at each's liking)
        auto fillColor = QColor(0, 0, 255, 150);
        auto strokeColor = QColor(0, 0, 255);
        auto strokeWidth = 3;

        selectionRectangle.setFillColor(fillColor);
        selectionRectangle.setStrokeColor(strokeColor);
        selectionRectangle.setStrokeSize(strokeWidth);

        selectionRectangle.drawToCanvas(canvas);
    }

    canvas->save();
};

void ViewLayerSelection::mousePressEvent(QMouseEvent *e) {};

void ViewLayerSelection::mouseReleaseEvent(QMouseEvent *e) {};

void ViewLayerSelection::mouseMoveEvent(QMouseEvent *e) {};

void ViewLayerSelection::mouseDoubleClickEvent(QMouseEvent *e) {};
