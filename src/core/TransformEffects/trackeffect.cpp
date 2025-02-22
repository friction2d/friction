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

#include "trackeffect.h"

#include "Boxes/boundingbox.h"
#include "Animators/qrealanimator.h"
#include "Animators/transformanimator.h"

TrackEffect::TrackEffect() :
    TargetTransformEffect("track", TransformEffectType::track) {
    mInfluence = enve::make_shared<QrealAnimator>(1, -1, 1, 0.01, "influence");

    ca_prependChild(targetProperty(), mInfluence);
}

qreal calculateTrackAngle(const QPointF& parentPos,
                          const QPointF& targetPos) {
    const QLineF baseLine(parentPos, parentPos + QPointF(100., 0.));
    const QLineF trackLine(parentPos, targetPos);
    qreal trackAngle = trackLine.angleTo(baseLine);
    if(trackAngle > 180) trackAngle -= 360;
    return trackAngle;
}

void TrackEffect::setRotScaleAfterTargetChange(
        BoundingBox* const oldTarget, BoundingBox* const newTarget) {
    const auto parent = getFirstAncestor<BoundingBox>();
    if(!parent) return;

    const qreal infl = mInfluence->getEffectiveValue();

    qreal rot = 0.;
    if(oldTarget) {
        const auto targetPos = oldTarget->getPivotAbsPos();
        const auto parentPos = parent->getPivotAbsPos();

        const qreal trackAngle = calculateTrackAngle(parentPos, targetPos);
        rot += trackAngle*infl;
    }

    if(newTarget) {
        const auto targetPos = newTarget->getPivotAbsPos();
        const auto parentPos = parent->getPivotAbsPos();

        const qreal trackAngle = calculateTrackAngle(parentPos, targetPos);
        rot -= trackAngle*infl;
    }

    parent->startRotTransform();
    parent->rotateBy(rot);
}

void TrackEffect::applyEffect(
        const qreal relFrame,
        qreal& pivotX, qreal& pivotY, qreal& pivotZ,
        qreal& posX, qreal& posY, qreal& posZ,
        qreal& rotX, qreal& rotY, qreal& rotZ,
        qreal& scaleX, qreal& scaleY, qreal& scaleZ,
        qreal& shearX, qreal& shearY, qreal& shearZ,
        QMatrix& postTransform,
        BoundingBox* const parent) {
    Q_UNUSED(pivotX)
    Q_UNUSED(pivotY)
    Q_UNUSED(pivotZ)
    Q_UNUSED(rotX)
    Q_UNUSED(rotY)
    Q_UNUSED(rotZ)
    Q_UNUSED(posX)
    Q_UNUSED(posY)
    Q_UNUSED(posZ)
    Q_UNUSED(scaleX)
    Q_UNUSED(scaleY)
    Q_UNUSED(scaleZ)
    Q_UNUSED(shearX)
    Q_UNUSED(shearY)
    Q_UNUSED(shearZ)
    Q_UNUSED(postTransform)

    if(!isVisible()) return;

    if(!parent) return;
    const auto target = targetProperty()->getTarget();
    if(!target) return;
    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);
    const auto targetPos = target->getPivotAbsPos(targetRelFrame);
    const auto parentPos = parent->getPivotAbsPos(relFrame);

    const qreal infl = mInfluence->getEffectiveValue(relFrame);
    const qreal trackAngle = calculateTrackAngle(parentPos, targetPos);
    rotX += trackAngle*infl;
}
