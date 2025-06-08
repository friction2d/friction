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

#ifndef CANVAS_WIDGET_H
#define CANVAS_WIDGET_H

#include "ui_global.h"
#include "ViewLayers/basecanvas.h"
#include "glwindow.h"

class UI_EXPORT CanvasWidget : public GLWindow {
public:
    CanvasWidget(BaseCanvas *baseCanvas)
        : _baseCanvas(baseCanvas) {};

    // Mouse events
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);

protected:
    void renderSk(SkCanvas* canvas);

private:
    BaseCanvas* _baseCanvas;
}

#endif // CANVAS_WIDGET_H
