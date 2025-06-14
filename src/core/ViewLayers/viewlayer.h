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

#ifndef VIEW_LAYER_H
#define VIEW_LAYER_H

#include <string>
#include <QObject>

#include "skia/skiaincludes.h"
#include "eevent.h"

// A ViewLayer is an abstract class that can receive input from the user
//
// (mouse clicks, keyboard...) and draw to a 2d canvas.
// We do this in order to separate the pieces of code logically
// (one view layer for rendering objects, one for selection...)
//
// We inherit from QObject because that allows us to do signals
class ViewLayer : public QObject {
    Q_OBJECT
public:
    ViewLayer(std::string layerId)
        : QObject()
        , _layerId(layerId)
        , _isVisible(true) {};
    ~ViewLayer() = default;

    virtual void repaint(SkCanvas * const canvas) = 0;

public:
    std::string layerId() const { return _layerId; }

    void show() { setVisible(true); };
    void hide() { setVisible(false); };
    void setVisible(bool isVisible) { _isVisible = isVisible; };
    bool isVisible() { return _isVisible; };

    // Mouse events
    virtual void mousePressEvent(QMouseEvent *e) = 0;
    virtual void mouseReleaseEvent(QMouseEvent *e) = 0;
    virtual void mouseMoveEvent(QMouseEvent *e) = 0;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) = 0;

private:
    std::string const _layerId;
    bool _isVisible;
};

#endif // VIEW_LAYER_H
