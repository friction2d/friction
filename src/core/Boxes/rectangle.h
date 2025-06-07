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

#ifndef RECTANGLE_H
#define RECTANGLE_H
#include "Boxes/pathbox.h"
class AnimatedPoint;
class QPointFAnimator;
class CORE_EXPORT RectangleBox : public PathBox {
    e_OBJECT
protected:
    RectangleBox();
public:
    SkPath getRelativePath(const qreal relFrame) const;
    bool differenceInEditPathBetweenFrames(
                const int frame1, const int frame2) const;

    void saveSVG(SvgExporter& exp, DomEleTask* const task) const;

    void moveSizePointByAbs(const QPointF &absTrans);

    MovablePoint *getBottomRightPoint();

    void setTopLeftPos(const QPointF &pos);
    void setBottomRightPos(const QPointF &pos);
    void setYRadius(const qreal radiusY);
    void setXRadius(const qreal radiusX);

    QPointFAnimator* getTopLeftAnimator();
    QPointFAnimator* getBottomRightAnimator();
    QPointFAnimator* getRadiusAnimator();

private:
    qsptr<QPointFAnimator> mTopLeftAnimator;
    qsptr<QPointFAnimator> mBottomRightAnimator;
    qsptr<QPointFAnimator> mRadiusAnimator;

    stdsptr<AnimatedPoint> mTopLeftPoint;
    stdsptr<AnimatedPoint> mBottomRightPoint;
    stdsptr<AnimatedPoint> mRadiusPoint;

    void getMotionBlurProperties(QList<Property*> &list) const;
};

#endif // RECTANGLE_H
