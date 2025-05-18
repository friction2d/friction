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

#ifndef FRICTION_CORE_BOXES_SCENE_H
#define FRICTION_CORE_BOXES_SCENE_H

#include <string>
#include <QThread>
#include <QString>

#include "smartPointers/selfref.h"
#include "Boxes/containerbox.h"


// Represents a scene in a Document (what Canvas used to be)
// Has:
//  - Keys + FrameRange
//  - Objects (_currentGroup)
//  - read/write
class Scene {
public:
    Scene(QString sceneName, qreal canvasWidth, qreal canvasHeight, qreal fps);
    ~Scene() = default;

    ContainerBox *getCurrentGroup() { return _currentGroup; };

    QString name() const { return _name; };
    qreal fps() const { return _fps; };
    qreal canvasWidth() const { return _canvasWidth; };
    qreal canvasHeight() const { return _canvasHeight; };

    int currentFrame() const { return _currentFrame; };

    void setName(QString name) { _name = name; };
    void setFps(qreal fps) { _fps = fps; }
    void setCanvasWidth(qreal width) { _canvasWidth = width; };
    void setCanvasHeight(qreal height) { _canvasHeight = height; };

    void setCurrentFrame(int frame) { _currentFrame = frame; };

private:
    qptr<ContainerBox> _currentGroup;

    QString _name;
    qreal _resolution = 0.5;

    qreal _fps;
    qreal _canvasWidth;
    qreal _canvasHeight;

    int _currentFrame;
};

#endif // FRICTION_CORE_BOXES_SCENE_H
