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

#ifndef GRADIENTPOINTS_H
#define GRADIENTPOINTS_H

#include "Animators/staticcomplexanimator.h"
#include "skia/skiaincludes.h"

class GradientPoint;
class MovablePoint;
class PathBox;
class QVector3DAnimator;
class QPointFAnimator;

class CORE_EXPORT GradientPoints : public StaticComplexAnimator {
    e_OBJECT
protected:
    GradientPoints(BoundingBox * const parent);
public:
    void prp_drawCanvasControls(
            SkCanvas *const canvas, const CanvasMode mode,
            const float invScale, const bool ctrlPressed);

    void enable();
    void disable();

    void setColors(const QColor &startColor, const QColor &endColor);

    void setPositions(const QPointF &startPos, const QPointF &endPos);
    void applyTransform(const QMatrix &transform);;

    QPointF getStartPointAtRelFrame(const int relFrame);
    QPointF getEndPointAtRelFrame(const int relFrame);
    QPointF getStartPoint(const qreal relFrame);
    QPointF getEndPoint(const qreal relFrame);

    bool enabled() const { return mEnabled; }

    QPointFAnimator* startAnimator() const { return mStartAnimator.get(); }
    QPointFAnimator* endAnimator() const { return mEndAnimator.get(); }
private:
    bool mEnabled;

    qsptr<QPointFAnimator> mStartAnimator;
    qsptr<QPointFAnimator> mEndAnimator;

    stdsptr<GradientPoint> mStartPoint;
    stdsptr<GradientPoint> mEndPoint;
};

#endif // GRADIENTPOINTS_H
