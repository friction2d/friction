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

#ifndef PATHPIVOT_H
#define PATHPIVOT_H
#include "MovablePoints/nonanimatedmovablepoint.h"
#include "smartPointers/ememory.h"

class Canvas;

enum class CanvasMode : short;

class CORE_EXPORT PathPivot : public NonAnimatedMovablePoint {
    e_OBJECT
protected:
    PathPivot(const Canvas* const parent);
public:
    void drawSk(SkCanvas * const canvas, const CanvasMode mode,
                const float invScale, const bool keyOnCurrent,
                const bool ctrlPressed);
    void drawTransforming(SkCanvas * const canvas,
                          const CanvasMode mode,
                          const float invScale,
                          const float interval);
    bool isVisible(const CanvasMode mode) const;

    void setMousePos(const QPointF& pos) { mMousePos = pos; }
private:
    const Canvas * const mCanvas = nullptr;
    QPointF mMousePos;
};

#endif // PATHPIVOT_H
