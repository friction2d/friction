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
#include "Animators/qrealanimator.h"

FollowObjectEffectBase::FollowObjectEffectBase(
        const QString& name, const TransformEffectType type) :
    TargetTransformEffect(name, type) {
    mPosInfluence = enve::make_shared<QPointFAnimator>(
                        QPointF{1., 1.}, QPointF{-10., -10.},
                        QPointF{10., 10.}, QPointF{0.01, 0.01},
                        "pos influence");
    mScaleInfluence = enve::make_shared<QPointFAnimator>(
                        QPointF{1., 1.}, QPointF{-10., -10.},
                        QPointF{10., 10.}, QPointF{0.01, 0.01},
                        "scale influence");
    mRotInfluence = enve::make_shared<QrealAnimator>(
                        1, -10, 10, 0.01, "rot influence");

    ca_addChild(mPosInfluence);
    ca_addChild(mScaleInfluence);
    ca_addChild(mRotInfluence);
}

void FollowObjectEffectBase::applyEffectWithTransform(
        const qreal relFrame,
        qreal& pivotX, qreal& pivotY,
        qreal& posX, qreal& posY, qreal& rot,
        qreal& scaleX, qreal& scaleY,
        qreal& shearX, qreal& shearY,
        BoundingBox* const parent,
        const QTransform& transform) {
    Q_UNUSED(pivotX);
    Q_UNUSED(pivotY);
    Q_UNUSED(shearX);
    Q_UNUSED(shearY);

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
    const qreal rotInfl = mRotInfluence->getEffectiveValue(relFrame);

    posX += p1.x()*posXInfl;
    posY += p1.y()*posYInfl;

    scaleX *= 1 + (xScale - 1)*scaleXInfl;
    scaleY *= 1 + (yScale - 1)*scaleYInfl;

    rot += targetRot*rotInfl;
}

void FollowObjectEffectBase::setRotScaleAfterTargetChange(
        BoundingBox* const oldTarget, BoundingBox* const newTarget) {
    const auto parent = getFirstAncestor<BoundingBox>();
    if(!parent) return;

    const qreal scaleXInfl = mScaleInfluence->getEffectiveXValue();
    const qreal scaleYInfl = mScaleInfluence->getEffectiveYValue();
    const qreal rotInfl = mRotInfluence->getEffectiveValue();

    qreal rot = 0.;
    qreal scaleX = 1.;
    qreal scaleY = 1.;
    if(oldTarget) {
        const auto trans = oldTarget->getTransformAnimator();
        const auto rotAnim = trans->getRotAnimator();
        const auto scaleAnim = trans->getScaleAnimator();

        rot += rotAnim->getEffectiveValue()*rotInfl;
        scaleX *= 1 + (scaleAnim->getEffectiveXValue() - 1)*scaleXInfl;
        scaleY *= 1 + (scaleAnim->getEffectiveYValue() - 1)*scaleYInfl;
    }

    if(newTarget) {
        const auto trans = newTarget->getTransformAnimator();
        const auto rotAnim = trans->getRotAnimator();
        const auto scaleAnim = trans->getScaleAnimator();

        rot -= rotAnim->getEffectiveValue()*rotInfl;
        const qreal scaleXDiv = 1 + (scaleAnim->getEffectiveXValue() - 1)*scaleXInfl;
        const qreal scaleYDiv = 1 + (scaleAnim->getEffectiveYValue() - 1)*scaleYInfl;
        if(!isZero4Dec(scaleXDiv)) {
            scaleX /= scaleXDiv;
        }

        if(!isZero4Dec(scaleYDiv)) {
            scaleY /= scaleYDiv;
        }
    }

    parent->startRotTransform();
    parent->rotateBy(rot);

    parent->startScaleTransform();
    parent->scale(scaleX, scaleY);
}
