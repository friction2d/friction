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

#include "followobjecteffect.h"

#include "Boxes/boundingbox.h"
#include "Animators/transformanimator.h"

FollowObjectEffect::FollowObjectEffect() :
    FollowObjectEffectBase("follow object",
                           TransformEffectType::followObject) {}

void FollowObjectEffect::applyEffect(const qreal relFrame,
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

    const auto parentTransform = parent->getInheritedTransformAtFrame(relFrame);
    const auto targetTransform = target->getTotalTransformAtFrame(targetRelFrame);
    const auto transform = targetTransform*parentTransform.inverted();

    applyEffectWithTransform(relFrame, pivotX, pivotY, pivotZ,
                             posX, posY, posZ,
                             rotX, rotY, rotZ,
                             scaleX, scaleY, scaleZ,
                             shearX, shearY, shearZ,
                             parent, transform);
}
