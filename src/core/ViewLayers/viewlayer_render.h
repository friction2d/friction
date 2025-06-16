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

#ifndef FRICTION_UI_VIEW_LAYER_RENDER_H
#define FRICTION_UI_VIEW_LAYER_RENDER_H

#include "viewlayer.h"

#include "framerange.h"
#include "Private/document.h"
#include "skia/skiaincludes.h"

class BaseCanvas;


// This is the class that renders canvas' objects when in render mode
// When ViewLayerRender is shown, ViewLayerPreview and ViewLayerSelection are disabled
// (you're not supposed to be able to move things around when in render mode)
class ViewLayerRender : public ViewLayer {
public:
    ViewLayerRender(Document &document, BaseCanvas *canvas);
    ~ViewLayerRender() = default;

    static ViewLayerRender* sGetInstance() { return sInstance; };

    void repaint(SkCanvas * const canvas) override;

    qreal getResolution() const;
    void setResolution(const qreal percent);

    void setIsRenderingToOutput(const bool bT) { _isRenderingOutput = bT; };

    // Mouse events
    void mousePressEvent(QMouseEvent *e) override {};
    void mouseReleaseEvent(QMouseEvent *e) override {};
    void mouseMoveEvent(QMouseEvent *e) override {};
    void mouseDoubleClickEvent(QMouseEvent *e) override {};

signals:
    void currentFrameChanged(int);
    void requestUpdate();

private:
    BaseCanvas *_baseCanvas;
    Document &_document;
    static ViewLayerRender *sInstance;

    bool _isRenderingOutput;
};

#endif // FRICTION_UI_VIEW_LAYER_RENDER_H
