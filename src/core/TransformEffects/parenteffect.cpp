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

#include "parenteffect.h"

#include "Boxes/boundingbox.h"
#include "Animators/transformanimator.h"
#include "Animators/qrealanimator.h"
#include "matrixdecomposition.h"

ParentEffect::ParentEffect() :
    FollowObjectEffectBase("parent", TransformEffectType::parent) {}

void ParentEffect::applyEffect(const qreal relFrame,
                               qreal& pivotX,
                               qreal& pivotY,
                               qreal& posX,
                               qreal& posY,
                               qreal& rot,
                               qreal& scaleX,
                               qreal& scaleY,
                               qreal& shearX,
                               qreal& shearY,
                               QMatrix& postTransform,
                               BoundingBox* const parent)
{
    Q_UNUSED(pivotX)
    Q_UNUSED(pivotY)
    Q_UNUSED(posX)
    Q_UNUSED(posY)
    Q_UNUSED(rot)
    Q_UNUSED(scaleX)
    Q_UNUSED(scaleY)
    Q_UNUSED(shearX)
    Q_UNUSED(shearY)

    if (!isVisible() || !parent) { return; }

    const auto target = targetProperty()->getTarget();
    if (!target) { return; }

    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);
    const auto targetTransAnim = target->getTransformAnimator();

    const QMatrix targetTransform = targetTransAnim->getRelativeTransformAtFrame(targetRelFrame);
    const QPointF targetPivot = target->getPivotRelPos(targetRelFrame);
    const auto targetValues = MatrixDecomposition::decomposePivoted(targetTransform, targetPivot);

    // Get influence values with reasonable bounds to prevent extreme values
    const qreal posXInfl = qBound(-10.0, mPosInfluence->getEffectiveXValue(relFrame), 10.0);
    const qreal posYInfl = qBound(-10.0, mPosInfluence->getEffectiveYValue(relFrame), 10.0);
    const qreal scaleXInfl = qBound(-10.0, mScaleInfluence->getEffectiveXValue(relFrame), 10.0);
    const qreal scaleYInfl = qBound(-10.0, mScaleInfluence->getEffectiveYValue(relFrame), 10.0);
    const qreal rotInfl = qBound(-10.0, mRotInfluence->getEffectiveValue(relFrame), 10.0);

    // Validate influence values for safety
    if (!validateInfluenceValues(posXInfl,
                                 posYInfl,
                                 scaleXInfl,
                                 scaleYInfl,
                                 rotInfl)) { return; }

    // Check for near-zero rotation influence
    const bool zeroRotInfluence = std::abs(rotInfl) < 1e-6;
    
    // Apply influence to transform values using helper method
    TransformValues influencedValues = targetValues;
    applyInfluenceToTransform(influencedValues,
                              targetValues,
                              posXInfl,
                              posYInfl,
                              scaleXInfl,
                              scaleYInfl);
    
    // Handle rotation influence with linear interpolation
    qreal translationRotInfl = 1.0;

    influencedValues.fRotation = targetValues.fRotation * translationRotInfl;

    const qreal desiredRotation = zeroRotInfluence
            ? -targetValues.fRotation
            : targetValues.fRotation * rotInfl;
    const qreal rotDeltaZero = -targetValues.fRotation;
    const qreal rotDeltaFull = desiredRotation - influencedValues.fRotation;

    if (rotInfl >= 0.0 && rotInfl <= 1.0) {
        const qreal t = rotInfl;
        const qreal blendedDelta = rotDeltaZero + t * (rotDeltaFull - rotDeltaZero);
        rot += blendedDelta;
    } else if (zeroRotInfluence) { rot += rotDeltaZero; }
    else { rot += rotDeltaFull; }

    // Calculate final transform matrix
    postTransform = influencedValues.calculate();
}

bool ParentEffect::validateInfluenceValues(const qreal posXInfl,
                                           const qreal posYInfl,
                                           const qreal scaleXInfl,
                                           const qreal scaleYInfl,
                                           const qreal rotInfl) const
{
    return std::isfinite(posXInfl) &&
           std::isfinite(posYInfl) &&
           std::isfinite(scaleXInfl) &&
           std::isfinite(scaleYInfl) &&
           std::isfinite(rotInfl);
}

void ParentEffect::applyInfluenceToTransform(TransformValues& values,
                                             const TransformValues& targetValues,
                                             const qreal posXInfl,
                                             const qreal posYInfl,
                                             const qreal scaleXInfl,
                                             const qreal scaleYInfl) const
{
    values.fMoveX = targetValues.fMoveX * posXInfl;
    values.fMoveY = targetValues.fMoveY * posYInfl;
    
    // Scale influence: interpolate between no scaling (1.0) and target scaling
    values.fScaleX = 1.0 + (targetValues.fScaleX - 1.0) * scaleXInfl;
    values.fScaleY = 1.0 + (targetValues.fScaleY - 1.0) * scaleYInfl;
}
