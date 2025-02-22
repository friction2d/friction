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

#include "followobjecteffectbase.h"
#include "Boxes/boundingbox.h"
#include "Animators/transformanimator.h"
#include "Animators/qvector3danimator.h"

FollowObjectEffectBase::FollowObjectEffectBase(
        const QString& name, const TransformEffectType type) :
    TargetTransformEffect(name, type) {
    mPosInfluence = enve::make_shared<QVector3DAnimator>(
                        QVector3D{1., 1., 1.}, QVector3D{-10., -10., -10.},
                        QVector3D{10., 10., 10.}, QVector3D{0.01, 0.01, 0.01},
                        "pos influence");
    mScaleInfluence = enve::make_shared<QVector3DAnimator>(
                        QVector3D{1., 1., 1.}, QVector3D{-10., -10., -10.},
                        QVector3D{10., 10., 10.}, QVector3D{0.01, 0.01, 0.01},
                        "scale influence");
    mRotInfluence = enve::make_shared<QVector3DAnimator>(
                        1, -10, 10, 0.01, "rot influence");

    ca_addChild(mPosInfluence);
    ca_addChild(mScaleInfluence);
    ca_addChild(mRotInfluence);
}

void FollowObjectEffectBase::applyEffectWithTransform(
        const qreal relFrame,
        qreal& pivotX, qreal& pivotY, qreal& pivotZ,
        qreal& posX, qreal& posY, qreal& posZ,
        qreal& rotX, qreal& rotY, qreal& rotZ,
        qreal& scaleX, qreal& scaleY, qreal& scaleZ,
        qreal& shearX, qreal& shearY, qreal& shearZ,
        BoundingBox* const parent,
        const QMatrix& transform) {
    Q_UNUSED(pivotX)
    Q_UNUSED(pivotY)
    Q_UNUSED(pivotZ)
    Q_UNUSED(posZ)
    Q_UNUSED(rotZ)
    Q_UNUSED(scaleZ)
    Q_UNUSED(shearX)
    Q_UNUSED(shearY)
    Q_UNUSED(shearZ)

    if(!isVisible()) return;

    if(!parent) return;
    const auto target = targetProperty()->getTarget();
    if(!target) return;
    const auto targetTransformAnimator = target->getTransformAnimator();

    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);

    const auto relPivot = target->getPivotRelPos(targetRelFrame);
    const auto p1 = relPivot*transform;

    const auto rotAnim = targetTransformAnimator->getRotAnimator();
    const qreal targetRot = rotAnim->getEffectiveValue(targetRelFrame);

    const auto scaleAnim = targetTransformAnimator->getScaleAnimator();
    const qreal xScale = scaleAnim->getEffectiveXValue(targetRelFrame);
    const qreal yScale = scaleAnim->getEffectiveYValue(targetRelFrame);

    const qreal posXInfl = mPosInfluence->getEffectiveXValue(relFrame);
    const qreal posYInfl = mPosInfluence->getEffectiveYValue(relFrame);
    const qreal scaleXInfl = mScaleInfluence->getEffectiveXValue(relFrame);
    const qreal scaleYInfl = mScaleInfluence->getEffectiveYValue(relFrame);
    const qreal rotXInfl = mRotInfluence->getEffectiveXValue(relFrame);
    const qreal rotYInfl = mRotInfluence->getEffectiveYValue(relFrame);

    posX += p1.x()*posXInfl;
    posY += p1.y()*posYInfl;

    scaleX *= 1 + (xScale - 1)*scaleXInfl;
    scaleY *= 1 + (yScale - 1)*scaleYInfl;

    rotX += targetRot*rotXInfl;
    rotY += targetRot*rotYInfl;
}

void FollowObjectEffectBase::setRotScaleAfterTargetChange(
        BoundingBox* const oldTarget, BoundingBox* const newTarget) {
    const auto parent = getFirstAncestor<BoundingBox>();
    if(!parent) return;

    const qreal scaleXInfl = mScaleInfluence->getEffectiveXValue();
    const qreal scaleYInfl = mScaleInfluence->getEffectiveYValue();
    const qreal scaleZInfl = mRotInfluence->getEffectiveZValue();
    const qreal rotXInfl = mRotInfluence->getEffectiveXValue();
    const qreal rotYInfl = mRotInfluence->getEffectiveYValue();
    const qreal rotZInfl = mRotInfluence->getEffectiveZValue();

    qreal rotX = 0.;
    qreal rotY = 0.;
    qreal rotZ = 0.;
    qreal scaleX = 1.;
    qreal scaleY = 1.;
    qreal scaleZ = 1.;
    if(oldTarget) {
        const auto trans = oldTarget->getTransformAnimator();
        const auto rotAnim = trans->getRotAnimator();
        const auto scaleAnim = trans->getScaleAnimator();

        rotX += rotAnim->getEffectiveXValue()*rotXInfl;
        rotY += rotAnim->getEffectiveYValue()*rotYInfl;
        rotZ += rotAnim->getEffectiveZValue()*rotZInfl;
        scaleX *= 1 + (scaleAnim->getEffectiveXValue() - 1)*scaleXInfl;
        scaleY *= 1 + (scaleAnim->getEffectiveYValue() - 1)*scaleYInfl;
        scaleZ *= 1 + (scaleAnim->getEffectiveZValue() - 1)*scaleZInfl;
    }

    if(newTarget) {
        const auto trans = newTarget->getTransformAnimator();
        const auto rotAnim = trans->getRotAnimator();
        const auto scaleAnim = trans->getScaleAnimator();

        rotX -= rotAnim->getEffectiveXValue()*rotXInfl;
        rotY -= rotAnim->getEffectiveYValue()*rotYInfl;
        rotZ -= rotAnim->getEffectiveZValue()*rotZInfl;
        const qreal scaleXDiv = 1 + (scaleAnim->getEffectiveXValue() - 1)*scaleXInfl;
        const qreal scaleYDiv = 1 + (scaleAnim->getEffectiveYValue() - 1)*scaleYInfl;
        const qreal scaleZDiv = 1 + (scaleAnim->getEffectiveZValue() - 1)*scaleYInfl;
        if(!isZero4Dec(scaleXDiv)) {
            scaleX /= scaleXDiv;
        }

        if(!isZero4Dec(scaleYDiv)) {
            scaleY /= scaleYDiv;
        }

        if (!isZero4Dec(scaleZDiv)) {
            scaleZ /= scaleZDiv;
        }
    }

    parent->startRotTransform();
    parent->rotateBy(rotX, rotY, rotZ);

    parent->startScaleTransform();
    parent->scale(scaleX, scaleY, scaleZ);
}
