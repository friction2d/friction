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

ParentEffect::ParentEffect() :
    FollowObjectEffectBase("parent", TransformEffectType::parent) {}

void ParentEffect::applyEffect(
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
    Q_UNUSED(posX)
    Q_UNUSED(posY)
    Q_UNUSED(posZ)
    Q_UNUSED(rotX)
    Q_UNUSED(rotY)
    Q_UNUSED(rotZ)
    Q_UNUSED(scaleX)
    Q_UNUSED(scaleY)
    Q_UNUSED(scaleZ)
    Q_UNUSED(shearX)
    Q_UNUSED(shearY)
    Q_UNUSED(shearZ)

    if(!isVisible()) return;

    if(!parent) return;
    const auto target = targetProperty()->getTarget();
    if(!target) return;
    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);

    const auto targetTransAnim = target->getTransformAnimator();
    postTransform = targetTransAnim->getRelativeTransformAtFrame(targetRelFrame);
}
