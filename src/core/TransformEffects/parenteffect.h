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

#ifndef PARENTEFFECT_H
#define PARENTEFFECT_H

#include "followobjecteffectbase.h"
#include "transformvalues.h"

class ParentEffect : public FollowObjectEffectBase
{
public:
    ParentEffect();

    void prp_drawCanvasControls(SkCanvas * const canvas,
                                const CanvasMode mode,
                                const float invScale,
                                const bool ctrlPressed) override;

    void applyEffect(const qreal relFrame,
                     qreal &pivotX,
                     qreal &pivotY,
                     qreal &posX,
                     qreal &posY,
                     qreal &rot,
                     qreal &scaleX,
                     qreal &scaleY,
                     qreal &shearX,
                     qreal &shearY,
                     QMatrix& postTransform,
                     BoundingBox* const parent) override;

private:
    void captureBindState(const qreal relFrame);
    bool ensureBindState(const qreal relFrame);

    bool computeEffectTransform(const qreal relFrame,
                                const TransformValues& baseValues,
                                const qreal posXInfl,
                                const qreal posYInfl,
                                const qreal scaleXInfl,
                                const qreal scaleYInfl,
                                const qreal rotInfl,
                                QMatrix& outPostTransform,
                                const bool updateState);

    void handleInfluenceChanged();
    void updatePrevInfluences(const qreal relFrame);

    bool validateInfluenceValues(const qreal posXInfl,
                                 const qreal posYInfl,
                                 const qreal scaleXInfl,
                                 const qreal scaleYInfl,
                                 const qreal rotInfl) const;

    void applyInfluenceToTransform(TransformValues& values,
                                   const TransformValues& targetValues,
                                   const qreal posXInfl,
                                   const qreal posYInfl,
                                   const qreal scaleXInfl,
                                   const qreal scaleYInfl) const;

    QPointF mPrevPosInfluence;
    QPointF mPrevScaleInfluence;
    qreal mPrevRotInfluence = 0.0;
    bool mPrevInfluenceValid = false;
    QPointF mBindTargetPivotInParent;
    QPointF mBindObjectPivotInParent;
    QMatrix mBindTargetParentToParentSpace;
    QMatrix mBindTargetLinearInParent;
    bool mBindStateValid = false;
    qreal mAccumDeltaAngleRad = 0.0;
    bool mDeltaAngleStateValid = false;
    QPointF mNoFollowPivotState;
    QMatrix mNoFollowLinearState;
    bool mNoFollowStateValid = false;
    QPointF mLastBaseMove;
    bool mLastBaseMoveValid = false;
};

#endif // PARENTEFFECT_H
