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

#ifndef VIEW_LAYER_RENDER_H
#define VIEW_LAYER_RENDER_H

#include "viewlayer.h"

#include "basecanvas.h"
#include "../../core/Private/document.h"
#include "../../core/skia/skiaincludes.h"


class ViewLayerRender : public ViewLayer {
public:
    ViewLayerRender(Document &document, BaseCanvas *canvas);
    ~ViewLayerRender() = default;

    void repaint(SkCanvas * const canvas) override;

    // Mouse events
    void mousePressEvent(QMouseEvent *e) override {};
    void mouseReleaseEvent(QMouseEvent *e) override {};
    void mouseMoveEvent(QMouseEvent *e) override {};
    void mouseDoubleClickEvent(QMouseEvent *e) override {};

private:
    BaseCanvas *_baseCanvas;
    Document &_document;
};

#endif // VIEW_LAYER_RENDER_H
