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

#include "matrixdecomposition.h"
#include "simplemath.h"

TransformValues MatrixDecomposition::decompose(const QTransform &transform) {
    TransformValues result;
    result.fPivotX = 0;
    result.fPivotY = 0;

    const qreal m11 = transform.m11();
    const qreal m12 = transform.m12();
    const qreal m21 = transform.m21();
    const qreal m22 = transform.m22();

    const qreal delta = m11 * m22 - m12 * m21;

    // Apply the QR-like decomposition.
    if(!isZero4Dec(m11) || !isZero4Dec(m12)) {
        const qreal r = sqrt(m11 * m11 + m12 * m12);
        const qreal rotRad = m12 > 0 ? acos(m11 / r) : -acos(m11 / r);
        result.fRotation = rotRad*180/PI;
        result.fScaleX = r;
        result.fScaleY = delta/r;
        result.fShearX = (m11 * m21 + m12 * m22) / (r * r);
        result.fShearY = 0;
    } else if(!isZero4Dec(m21) || !isZero4Dec(m22)) {
        const qreal s = sqrt(m21 * m21 + m22 * m22);
        const qreal rotRad = m22 > 0 ? acos(-m21 / s) : -acos(-m21 / s);
        result.fRotation = 90 - rotRad*180/PI;
        result.fScaleX = delta/s;
        result.fScaleY = s;
        result.fShearX = 0;
        result.fShearY = (m11 * m21 + m12 * m22) / (s * s);
    } else {
        result.fRotation = 0;
        result.fScaleX = 0;
        result.fScaleY = 0;
        result.fShearX = 0;
        result.fShearY = 0;
    }

    result.fMoveX = transform.dx();
    result.fMoveY = transform.dy();
    return result;
}

TransformValues MatrixDecomposition::decomposePivoted(
        const QTransform &transform, const QPointF &pivot) {
    const TransformValues notPivoted = decompose(transform);
    TransformValues result = setPivotKeepTransform(notPivoted, pivot);
    return result;
}

TransformValues MatrixDecomposition::setPivotKeepTransform(
        const TransformValues &transform, const QPointF &newPivot) {
    QTransform newTransform;
    newTransform.translate(newPivot.x() + transform.fMoveX,
                           newPivot.y() + transform.fMoveY);

    newTransform.rotate(transform.fRotation);
    newTransform.scale(transform.fScaleX, transform.fScaleY);
    newTransform.shear(transform.fShearX, transform.fShearY);

    newTransform.translate(-newPivot.x(), -newPivot.y());

    const auto oldTransform = transform.calculate();
    const qreal posXInc = oldTransform.dx() - newTransform.dx();
    const qreal posYInc = oldTransform.dy() - newTransform.dy();

    TransformValues result = transform;
    result.fMoveX += posXInc;
    result.fMoveY += posYInc;
    result.fPivotX = newPivot.x();
    result.fPivotY = newPivot.y();
    return result;
}
