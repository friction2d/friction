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

#ifndef CIRCLE_H
#define CIRCLE_H

#include "MovablePoints/animatedpoint.h"

enum class CanvasMode : short;

class CORE_EXPORT CircleRadiusPoint : public AnimatedPoint {
    e_OBJECT
protected:
    CircleRadiusPoint(QPointFAnimator * const associatedAnimator,
                      BasicTransformAnimator * const parent,
                      AnimatedPoint * const centerPoint,
                      const MovablePointType &type,
                      const bool blockX);
public:
    QPointF getRelativePos() const;
    void setRelativePos(const QPointF &relPos);
private:
    const bool mXBlocked = false;
    const stdptr<AnimatedPoint> mCenterPoint;
};

#include "Boxes/pathbox.h"

class CORE_EXPORT Circle : public PathBox {
    e_OBJECT
protected:
    Circle();
public:
    SkPath getRelativePath(const qreal relFrame) const;

    bool differenceInEditPathBetweenFrames(
                const int frame1, const int frame2) const;
    void saveSVG(SvgExporter& exp, DomEleTask* const task) const;

    void setCenter(const QPointF& center);
    void setVerticalRadius(const qreal verticalRadius);
    void setHorizontalRadius(const qreal horizontalRadius);
    void setRadius(const qreal radius);
    void moveRadiusesByAbs(const QPointF &absTrans);

    qreal getCurrentXRadius();
    qreal getCurrentYRadius();
protected:
    void getMotionBlurProperties(QList<Property*> &list) const;
private:
    stdsptr<AnimatedPoint> mCenterPoint;
    stdsptr<CircleRadiusPoint> mHorizontalRadiusPoint;
    stdsptr<CircleRadiusPoint> mVerticalRadiusPoint;

    qsptr<QPointFAnimator> mCenterAnimator;
    qsptr<QPointFAnimator> mHorizontalRadiusAnimator;
    qsptr<QPointFAnimator> mVerticalRadiusAnimator;
};

#endif // CIRCLE_H
