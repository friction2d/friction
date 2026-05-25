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

#ifndef CAMERABOX_H
#define CAMERABOX_H

#include "boundingbox.h"

class CORE_EXPORT CameraBox : public BoundingBox {
    e_OBJECT
public:
    CameraBox();

    stdsptr<BoxRenderData> createRenderData() { return nullptr; }
    bool shouldScheduleUpdate() { return false; }
    bool relPointInsidePath(const QPointF &relPos) const;
    void queTasks();

    void drawCameraBox(SkCanvas* canvas, float invScale);
    QRectF getWorldBoundsAtFrame(qreal relFrame) const;
};

#endif // CAMERABOX_H
