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
#include "skia/skiahelpers.h"
#include "simplemath.h"
#include <cmath>

namespace {

QPointF mapLinear(const QMatrix& m, const QPointF& p) {
    return {m.m11()*p.x() + m.m21()*p.y(),
            m.m12()*p.x() + m.m22()*p.y()};
}

TransformValues currentBaseValues(BoxTransformAnimator* const transform,
                                  const qreal relFrame) {
    TransformValues values;
    values.fPivotX = transform->getPivotAnimator()->getEffectiveXValue(relFrame);
    values.fPivotY = transform->getPivotAnimator()->getEffectiveYValue(relFrame);
    values.fMoveX = transform->getPosAnimator()->getEffectiveXValue(relFrame);
    values.fMoveY = transform->getPosAnimator()->getEffectiveYValue(relFrame);
    values.fRotation = transform->getRotAnimator()->getEffectiveValue(relFrame);
    values.fScaleX = transform->getScaleAnimator()->getEffectiveXValue(relFrame);
    values.fScaleY = transform->getScaleAnimator()->getEffectiveYValue(relFrame);
    values.fShearX = transform->getShearAnimator()->getEffectiveXValue(relFrame);
    values.fShearY = transform->getShearAnimator()->getEffectiveYValue(relFrame);
    return values;
}

} // namespace

ParentEffect::ParentEffect() :
    FollowObjectEffectBase("parent", TransformEffectType::parent) {
    prp_enabledDrawingOnCanvas();

    auto connectInfluence = [this](QrealAnimator* const animator) {
        connect(animator, &QrealAnimator::effectiveValueChanged,
                this, [this]() { handleInfluenceChanged(); });
    };

    connectInfluence(mPosInfluence->getXAnimator());
    connectInfluence(mPosInfluence->getYAnimator());
    connectInfluence(mScaleInfluence->getXAnimator());
    connectInfluence(mScaleInfluence->getYAnimator());
    connectInfluence(mRotInfluence.get());

    connect(targetProperty(), &BoxTargetProperty::setActionFinished,
            this, [this](BoundingBox* const, BoundingBox* const newTarget) {
        mBindStateValid = false;
        mDeltaAngleStateValid = false;
        mNoFollowStateValid = false;
        if(newTarget) {
            const auto parent = getFirstAncestor<BoundingBox>();
            if(parent) {
                captureBindState(parent->anim_getCurrentRelFrame());
            }
        }
    });
}

void ParentEffect::prp_drawCanvasControls(SkCanvas * const canvas,
                                          const CanvasMode mode,
                                          const float invScale,
                                          const bool ctrlPressed) {
    Q_UNUSED(mode)
    Q_UNUSED(ctrlPressed)

    if(!isVisible()) { return; }

    const auto parent = getFirstAncestor<BoundingBox>();
    const auto target = targetProperty()->getTarget();
    if(!parent || !target) { return; }

    const qreal relFrame = parent->anim_getCurrentRelFrame();
    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);
    const QPointF childPivotAbs = parent->getPivotAbsPos(relFrame);
    const QPointF targetPivotAbs = target->getPivotAbsPos(targetRelFrame);

    SkPath pivotLink;
    pivotLink.moveTo(toSkScalar(childPivotAbs.x()), toSkScalar(childPivotAbs.y()));
    pivotLink.lineTo(toSkScalar(targetPivotAbs.x()), toSkScalar(targetPivotAbs.y()));
    SkiaHelpers::drawOutlineOverlay(canvas, pivotLink, invScale, true, 6.0f, SK_ColorWHITE);
}

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
                               BoundingBox* const parent) {
    Q_UNUSED(parent)

    if (!isVisible()) { return; }

    const auto target = targetProperty()->getTarget();
    if (!target) { return; }

    const qreal posXInfl = qBound(-10.0, mPosInfluence->getEffectiveXValue(relFrame), 10.0);
    const qreal posYInfl = qBound(-10.0, mPosInfluence->getEffectiveYValue(relFrame), 10.0);
    const qreal scaleXInfl = qBound(-10.0, mScaleInfluence->getEffectiveXValue(relFrame), 10.0);
    const qreal scaleYInfl = qBound(-10.0, mScaleInfluence->getEffectiveYValue(relFrame), 10.0);
    const qreal rotInfl = qBound(-10.0, mRotInfluence->getEffectiveValue(relFrame), 10.0);

    if (!validateInfluenceValues(posXInfl,
                                 posYInfl,
                                 scaleXInfl,
                                 scaleYInfl,
                                 rotInfl)) { return; }

    TransformValues baseValues;
    baseValues.fPivotX = pivotX;
    baseValues.fPivotY = pivotY;
    baseValues.fMoveX = posX;
    baseValues.fMoveY = posY;
    baseValues.fRotation = rot;
    baseValues.fScaleX = scaleX;
    baseValues.fScaleY = scaleY;
    baseValues.fShearX = shearX;
    baseValues.fShearY = shearY;

    if(!computeEffectTransform(relFrame,
                               baseValues,
                               posXInfl,
                               posYInfl,
                               scaleXInfl,
                               scaleYInfl,
                               rotInfl,
                               postTransform,
                               true)) { return; }

    if(!mPrevInfluenceValid) {
        mPrevPosInfluence = {posXInfl, posYInfl};
        mPrevScaleInfluence = {scaleXInfl, scaleYInfl};
        mPrevRotInfluence = rotInfl;
        mPrevInfluenceValid = true;
    }
}

bool ParentEffect::computeEffectTransform(const qreal relFrame,
                                          const TransformValues& baseValues,
                                          const qreal posXInfl,
                                          const qreal posYInfl,
                                          const qreal scaleXInfl,
                                          const qreal scaleYInfl,
                                          const qreal rotInfl,
                                          QMatrix& outPostTransform,
                                          const bool updateState) {
    if (!isVisible()) { return false; }

    const auto parent = getFirstAncestor<BoundingBox>();
    if (!parent) { return false; }

    const auto target = targetProperty()->getTarget();
    if (!target) { return false; }

    if (!validateInfluenceValues(posXInfl,
                                 posYInfl,
                                 scaleXInfl,
                                 scaleYInfl,
                                 rotInfl)) { return false; }

    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);

    if(!ensureBindState(relFrame)) { return false; }

    const QMatrix targetRel = target->getRelativeTransformAtFrame(targetRelFrame);
    const QMatrix targetInParentSpace = targetRel*mBindTargetParentToParentSpace;
    const QMatrix targetLinear(targetInParentSpace.m11(), targetInParentSpace.m12(),
                               targetInParentSpace.m21(), targetInParentSpace.m22(),
                               0.0, 0.0);

    const QPointF targetPivotRel = target->getPivotRelPos(targetRelFrame);
    const QPointF targetPivotInParent = targetInParentSpace.map(targetPivotRel);

    bool bindLinearInvertible = false;
    const QMatrix bindLinearInv = mBindTargetLinearInParent.inverted(&bindLinearInvertible);
    if(!bindLinearInvertible) { return false; }
    const QMatrix deltaLinear = targetLinear*bindLinearInv;
    const TransformValues deltaValues = MatrixDecomposition::decompose(deltaLinear);
    const qreal rawDeltaAngle = std::atan2(deltaLinear.m12(), deltaLinear.m11());
    if(!mDeltaAngleStateValid) {
        mAccumDeltaAngleRad = rawDeltaAngle;
        mDeltaAngleStateValid = true;
    } else {
        // Continuous unwrap without random-walk accumulation:
        // choose the raw angle equivalent closest to previous accumulated value.
        const qreal twoPi = 2.0*PI;
        const qreal wraps = std::round((rawDeltaAngle - mAccumDeltaAngleRad)/twoPi);
        mAccumDeltaAngleRad = rawDeltaAngle - wraps*twoPi;
    }

    TransformValues linearValues;
    applyInfluenceToTransform(linearValues,
                              deltaValues,
                              0.0,
                              0.0,
                              scaleXInfl,
                              scaleYInfl);
    linearValues.fRotation = (mAccumDeltaAngleRad*180.0/PI)*rotInfl;
    linearValues.fShearX = deltaValues.fShearX*scaleXInfl;
    linearValues.fShearY = deltaValues.fShearY*scaleYInfl;

    const QMatrix linear = linearValues.calculate();

    const QPointF objectPivotLocal(baseValues.fPivotX, baseValues.fPivotY);
    const QPointF objectPivotInParent = baseValues.calculate().map(objectPivotLocal);
    if(updateState) {
        if(mLastBaseMoveValid) {
            const QPointF baseMoveDelta(baseValues.fMoveX - mLastBaseMove.x(),
                                        baseValues.fMoveY - mLastBaseMove.y());
            const QPointF mappedDelta = mapLinear(linear, baseMoveDelta);
            QPointF bindDelta = mappedDelta;

            // Partial position influence requires compensating how bind offset
            // contributes to the final pivot: final ~= ((1-p)I + p*linear)*bind.
            const qreal a = (1.0 - posXInfl) + posXInfl*linear.m11();
            const qreal b = posXInfl*linear.m21();
            const qreal c = posYInfl*linear.m12();
            const qreal d = (1.0 - posYInfl) + posYInfl*linear.m22();
            const qreal det = a*d - b*c;
            if(std::abs(det) > 1e-6) {
                bindDelta.setX(( d*mappedDelta.x() - b*mappedDelta.y())/det);
                bindDelta.setY((-c*mappedDelta.x() + a*mappedDelta.y())/det);
            }
            if(!isZero6Dec(bindDelta.x()) || !isZero6Dec(bindDelta.y())) {
                // Treat child position edits as a bind-offset adjustment so
                // translation can be edited while linked (same behavior as rotation).
                mBindObjectPivotInParent.rx() += bindDelta.x();
                mBindObjectPivotInParent.ry() += bindDelta.y();
                if(mNoFollowStateValid) {
                    mNoFollowPivotState.rx() += bindDelta.x();
                    mNoFollowPivotState.ry() += bindDelta.y();
                }
            }
        }
        mLastBaseMove = QPointF(baseValues.fMoveX, baseValues.fMoveY);
        mLastBaseMoveValid = true;
    }
    const QPointF bindOffset(mBindObjectPivotInParent.x() - mBindTargetPivotInParent.x(),
                             mBindObjectPivotInParent.y() - mBindTargetPivotInParent.y());
    const QPointF transformedBindOffset = mapLinear(linear, bindOffset);

    // No translation follow is evaluated incrementally:
    // pure target translation should not move the child, but changes in
    // rotation/scale/shear should be applied around the current target pivot.
    QPointF noFollowPivot = mBindObjectPivotInParent;
    if(mNoFollowStateValid) {
        bool prevLinearInvertible = false;
        const QMatrix prevLinearInv = mNoFollowLinearState.inverted(&prevLinearInvertible);
        if(prevLinearInvertible) {
            const QMatrix deltaLinearStep = linear*prevLinearInv;
            const QPointF prevRel(mNoFollowPivotState.x() - targetPivotInParent.x(),
                                  mNoFollowPivotState.y() - targetPivotInParent.y());
            const QPointF nextRel = mapLinear(deltaLinearStep, prevRel);
            noFollowPivot = QPointF(targetPivotInParent.x() + nextRel.x(),
                                    targetPivotInParent.y() + nextRel.y());
        } else {
            noFollowPivot = mNoFollowPivotState;
        }
    }

    // Full translation follow: move with target while keeping rotated/scaled bind offset.
    const QPointF fullFollowPivot(targetPivotInParent.x() + transformedBindOffset.x(),
                                  targetPivotInParent.y() + transformedBindOffset.y());

    const QPointF finalPivot(noFollowPivot.x() +
                             (fullFollowPivot.x() - noFollowPivot.x())*posXInfl,
                             noFollowPivot.y() +
                             (fullFollowPivot.y() - noFollowPivot.y())*posYInfl);

    // Solve affine translation so objectPivotInParent maps to finalPivot.
    const QPointF linearAtObjectPivot = mapLinear(linear, objectPivotInParent);
    const QPointF offset(finalPivot.x() - linearAtObjectPivot.x(),
                         finalPivot.y() - linearAtObjectPivot.y());

    outPostTransform = QMatrix(linear.m11(), linear.m12(),
                               linear.m21(), linear.m22(),
                               offset.x(), offset.y());

    if(updateState) {
        mNoFollowPivotState = noFollowPivot;
        mNoFollowLinearState = linear;
        mNoFollowStateValid = true;
    }
    return true;
}

void ParentEffect::captureBindState(const qreal relFrame) {
    const auto parent = getFirstAncestor<BoundingBox>();
    const auto target = targetProperty()->getTarget();
    if(!parent || !target) {
        mBindStateValid = false;
        return;
    }

    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);

    const QMatrix inherited = parent->getInheritedTransformAtFrame(relFrame);
    bool inheritedInvertible = false;
    const QMatrix inheritedInv = inherited.inverted(&inheritedInvertible);
    if(!inheritedInvertible) {
        mBindStateValid = false;
        return;
    }

    const QMatrix targetInherited = target->getInheritedTransformAtFrame(targetRelFrame);
    mBindTargetParentToParentSpace = targetInherited*inheritedInv;
    const QMatrix targetRel = target->getRelativeTransformAtFrame(targetRelFrame);
    const QMatrix targetInParentSpace = targetRel*mBindTargetParentToParentSpace;
    const QMatrix targetLinear(targetInParentSpace.m11(), targetInParentSpace.m12(),
                               targetInParentSpace.m21(), targetInParentSpace.m22(),
                               0.0, 0.0);
    const QPointF objectPivotAbs = parent->getPivotAbsPos(relFrame);
    const QPointF objectPivotInParent = inheritedInv.map(objectPivotAbs);
    const QPointF targetPivotRel = target->getPivotRelPos(targetRelFrame);
    const QPointF targetPivotInParent = targetInParentSpace.map(targetPivotRel);
    mBindTargetPivotInParent = targetPivotInParent;
    mBindObjectPivotInParent = objectPivotInParent;
    mBindTargetLinearInParent = targetLinear;
    mBindStateValid = true;
    mAccumDeltaAngleRad = 0.0;
    mDeltaAngleStateValid = false;
    mNoFollowPivotState = objectPivotInParent;
    mNoFollowLinearState = QMatrix();
    mNoFollowStateValid = true;
    const auto transform = parent->getBoxTransformAnimator();
    if(transform) {
        const TransformValues baseValues = currentBaseValues(transform, relFrame);
        mLastBaseMove = QPointF(baseValues.fMoveX, baseValues.fMoveY);
        mLastBaseMoveValid = true;
    } else {
        mLastBaseMove = QPointF(0.0, 0.0);
        mLastBaseMoveValid = false;
    }
}

bool ParentEffect::ensureBindState(const qreal relFrame) {
    if(mBindStateValid) { return true; }
    captureBindState(relFrame);
    return mBindStateValid;
}

void ParentEffect::handleInfluenceChanged() {
    const auto parent = getFirstAncestor<BoundingBox>();
    if(!parent) {
        updatePrevInfluences(anim_getCurrentRelFrame());
        return;
    }

    const auto transform = parent->getBoxTransformAnimator();
    if(!transform) {
        updatePrevInfluences(parent->anim_getCurrentRelFrame());
        return;
    }

    const qreal relFrame = parent->anim_getCurrentRelFrame();

    const qreal posXInfl = qBound(-10.0, mPosInfluence->getEffectiveXValue(relFrame), 10.0);
    const qreal posYInfl = qBound(-10.0, mPosInfluence->getEffectiveYValue(relFrame), 10.0);
    const qreal scaleXInfl = qBound(-10.0, mScaleInfluence->getEffectiveXValue(relFrame), 10.0);
    const qreal scaleYInfl = qBound(-10.0, mScaleInfluence->getEffectiveYValue(relFrame), 10.0);
    const qreal rotInfl = qBound(-10.0, mRotInfluence->getEffectiveValue(relFrame), 10.0);

    if(!mPrevInfluenceValid) {
        mPrevPosInfluence = {posXInfl, posYInfl};
        mPrevScaleInfluence = {scaleXInfl, scaleYInfl};
        mPrevRotInfluence = rotInfl;
        mPrevInfluenceValid = true;
        if(!mBindStateValid) {
            captureBindState(relFrame);
        }
        return;
    }

    if(isZero6Dec(posXInfl - mPrevPosInfluence.x()) &&
       isZero6Dec(posYInfl - mPrevPosInfluence.y()) &&
       isZero6Dec(scaleXInfl - mPrevScaleInfluence.x()) &&
       isZero6Dec(scaleYInfl - mPrevScaleInfluence.y()) &&
       isZero6Dec(rotInfl - mPrevRotInfluence)) {
        return;
    }

    const TransformValues baseValues = currentBaseValues(transform, relFrame);

    QMatrix oldPost;
    if(!computeEffectTransform(relFrame,
                               baseValues,
                               mPrevPosInfluence.x(),
                               mPrevPosInfluence.y(),
                               mPrevScaleInfluence.x(),
                               mPrevScaleInfluence.y(),
                               mPrevRotInfluence,
                               oldPost,
                               false)) {
        updatePrevInfluences(relFrame);
        return;
    }

    QMatrix newPost;
    if(!computeEffectTransform(relFrame,
                               baseValues,
                               posXInfl,
                               posYInfl,
                               scaleXInfl,
                               scaleYInfl,
                               rotInfl,
                               newPost,
                               false)) {
        updatePrevInfluences(relFrame);
        return;
    }

    bool invertible = false;
    const QMatrix invNewPost = newPost.inverted(&invertible);
    if(!invertible) {
        updatePrevInfluences(relFrame);
        return;
    }

    const QMatrix baseRel = baseValues.calculate();
    const QMatrix newBaseRel = baseRel*oldPost*invNewPost;

    TransformValues newValues = MatrixDecomposition::decomposePivoted(
                newBaseRel, QPointF(baseValues.fPivotX, baseValues.fPivotY));

    transform->startTransformSkipOpacity();
    transform->setValues(newValues);
    transform->prp_finishTransform();

    updatePrevInfluences(relFrame);
}

void ParentEffect::updatePrevInfluences(const qreal relFrame) {
    mPrevPosInfluence = {qBound(-10.0, mPosInfluence->getEffectiveXValue(relFrame), 10.0),
                         qBound(-10.0, mPosInfluence->getEffectiveYValue(relFrame), 10.0)};
    mPrevScaleInfluence = {qBound(-10.0, mScaleInfluence->getEffectiveXValue(relFrame), 10.0),
                           qBound(-10.0, mScaleInfluence->getEffectiveYValue(relFrame), 10.0)};
    mPrevRotInfluence = qBound(-10.0, mRotInfluence->getEffectiveValue(relFrame), 10.0);
    mPrevInfluenceValid = true;
}

bool ParentEffect::validateInfluenceValues(const qreal posXInfl,
                                           const qreal posYInfl,
                                           const qreal scaleXInfl,
                                           const qreal scaleYInfl,
                                           const qreal rotInfl) const {
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
                                             const qreal scaleYInfl) const {
    values.fMoveX = targetValues.fMoveX * posXInfl;
    values.fMoveY = targetValues.fMoveY * posYInfl;

    // Scale influence interpolates around identity to avoid drift.
    values.fScaleX = 1.0 + (targetValues.fScaleX - 1.0) * scaleXInfl;
    values.fScaleY = 1.0 + (targetValues.fScaleY - 1.0) * scaleYInfl;
}
