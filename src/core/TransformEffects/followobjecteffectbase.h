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

#ifndef FOLLOWOBJECTEFFECTBASE_H
#define FOLLOWOBJECTEFFECTBASE_H

#include "targettransformeffect.h"

#include "Animators/qrealanimator.h"
#include "Animators/qvector3danimator.h"

class QVector3D;

class FollowObjectEffectBase : public TargetTransformEffect {
public:
    FollowObjectEffectBase(const QString& name,
                           const TransformEffectType type);

protected:
    void applyEffectWithTransform(
            const qreal relFrame,
            qreal &pivotX, qreal &pivotY, qreal &pivotZ,
            qreal &posX, qreal &posY, qreal &posZ,
            qreal &rotX, qreal &rotY, qreal &rotZ,
            qreal &scaleX, qreal &scaleY, qreal &scaleZ,
            qreal &shearX, qreal &shearY, qreal &shearZ,
            BoundingBox* const parent,
            const QMatrix& transform);
protected:
    void setRotScaleAfterTargetChange(
                BoundingBox* const oldTarget,
                BoundingBox* const newTarget) override;

    qsptr<QVector3DAnimator> mPosInfluence;
    qsptr<QVector3DAnimator> mScaleInfluence;
    qsptr<QVector3DAnimator> mRotInfluence;
};

#endif // FOLLOWOBJECTEFFECTBASE_H
