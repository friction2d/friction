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

#ifndef SMARTCTRLPOINT_H
#define SMARTCTRLPOINT_H
#include "MovablePoints/nonanimatedmovablepoint.h"
class SmartNodePoint;
class CORE_EXPORT SmartCtrlPoint : public NonAnimatedMovablePoint {
    e_OBJECT
public:
    enum Type : char { C0, C2 };
protected:
    SmartCtrlPoint(SmartNodePoint * const parentPoint,
                   const Type &type);
public:
    SmartNodePoint* getParentPoint() const { return mParentPoint_k; }
    void drawSk(SkCanvas* const canvas,
                const CanvasMode mode,
                const float invScale,
                const bool keyOnCurrent,
                const bool ctrlPressed);
    void setRelativePos(const QPointF &relPos);
    void rotateRelativeToSavedPivot(const qreal rotate);

    void startTransform();
    void finishTransform();
    void cancelTransform();

    void remove();

    bool isVisible(const CanvasMode mode) const;
    void scale(const qreal sx, const qreal sy);

    void updateRadius();

    bool enabled() const;

    void setOtherCtrlPt(SmartCtrlPoint * const ctrlPt);
private:
    const Type mCtrlType;
    SmartNodePoint* const mParentPoint_k;
    stdptr<SmartCtrlPoint> mOtherCtrlPt_cv;
};

#endif // SMARTCTRLPOINT_H
