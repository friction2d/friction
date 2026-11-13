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
*/

#include "lottie/lottieparenting.h"

#include "Animators/qrealanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/boundingbox.h"
#include "Properties/boxtargetproperty.h"
#include "TransformEffects/parenteffect.h"
#include "TransformEffects/transformeffectcollection.h"
#include "lottie/lottieanimatedproperty.h"
#include "matrixdecomposition.h"
#include "simplemath.h"

#include <QtMath>

BoundingBox* LottieParenting::target(const BoundingBox* const box)
{
    if (!box) { return nullptr; }
    const auto transform = box->getBoxTransformAnimator();
    if (!transform) { return nullptr; }

    // Keep Parent Effect discovery local to the exporter instead of exposing
    // Lottie-specific accessors on BoundingBox or ParentEffect.
    BoundingBox* result = nullptr;
    for (int i = 0; i < transform->ca_getNumberOfChildren(); i++) {
        const auto child = transform->ca_getChildAt<Property>(i);
        const auto effects = dynamic_cast<TransformEffectCollection*>(child);
        if (!effects) { continue; }

        for (int j = 0; j < effects->ca_getNumberOfChildren(); j++) {
            const auto effect = dynamic_cast<ParentEffect*>(effects->getChild(j));
            if (!effect || !effect->isVisible()) { continue; }

            for (int k = 0; k < effect->ca_getNumberOfChildren(); k++) {
                const auto effectChild = effect->ca_getChildAt<Property>(k);
                const auto property = dynamic_cast<BoxTargetProperty*>(effectChild);
                if (property && property->getTarget()) {
                    result = property->getTarget();
                    break;
                }
            }
        }
    }
    return result;
}

QJsonObject LottieParenting::transform(const BoundingBox* const box,
                                       const BoundingBox* const parent,
                                       const FrameRange& frameRange)
{
    QList<QJsonArray> positions;
    QList<QJsonArray> scales;
    QList<qreal> rotations;
    QList<qreal> opacities;
    QList<qreal> skews;
    QList<qreal> skewAxes;
    qreal previousRotation = 0;
    bool firstRotation = true;

    for (int frame = frameRange.fMin; frame <= frameRange.fMax; frame++) {
        QMatrix matrix = box->getRelativeTransformAtFrame(frame);
        if (parent) {
            const qreal absFrame = box->prp_relFrameToAbsFrameF(frame);
            const qreal parentFrame = parent->prp_absFrameToRelFrameF(absFrame);
            bool invertible = false;
            const QMatrix parentInverse =
                    parent->getRelativeTransformAtFrame(parentFrame).inverted(&invertible);
            if (invertible) { matrix *= parentInverse; }
        }

        const TransformValues values = MatrixDecomposition::decompose(matrix);
        qreal rotation = values.fRotation;
        if (!firstRotation) {
            while (rotation - previousRotation > 180) { rotation -= 360; }
            while (rotation - previousRotation < -180) { rotation += 360; }
        }
        firstRotation = false;
        previousRotation = rotation;

        positions << QJsonArray{values.fMoveX, values.fMoveY, 0};
        scales << QJsonArray{values.fScaleX*100, values.fScaleY*100, 100};
        rotations << rotation;
        if (!isZero6Dec(values.fShearX)) {
            skews << qRadiansToDegrees(std::atan(values.fShearX));
            skewAxes << 0;
        } else {
            skews << qRadiansToDegrees(std::atan(values.fShearY));
            skewAxes << 90;
        }

        const auto boxTransform = box->getBoxTransformAnimator();
        opacities << (boxTransform ?
                          boxTransform->getOpacityAnimator()->getEffectiveValue(frame) :
                          100);
    }

    QJsonObject transform;
    transform.insert(QStringLiteral("o"),
                     LottieAnimatedProperty::scalar(opacities, frameRange));
    transform.insert(QStringLiteral("r"),
                     LottieAnimatedProperty::scalar(rotations, frameRange));
    transform.insert(QStringLiteral("p"),
                     LottieAnimatedProperty::point(positions, frameRange));
    transform.insert(QStringLiteral("a"),
                     LottieAnimatedProperty::staticProperty(QJsonArray{0, 0, 0}));
    transform.insert(QStringLiteral("s"),
                     LottieAnimatedProperty::point(scales, frameRange));
    transform.insert(QStringLiteral("sk"),
                     LottieAnimatedProperty::scalar(skews, frameRange));
    transform.insert(QStringLiteral("sa"),
                     LottieAnimatedProperty::scalar(skewAxes, frameRange));
    return transform;
}
