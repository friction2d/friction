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

#ifndef FOLLOWPATHEFFECT_H
#define FOLLOWPATHEFFECT_H

#include "targettransformeffect.h"

#include "Properties/boolproperty.h"

class FollowPathEffect : public TargetTransformEffect {
public:
    FollowPathEffect();

    void applyEffect(const qreal relFrame,
                     qreal &pivotX, qreal &pivotY, qreal& pivotZ,
                     qreal &posX, qreal &posY, qreal& posZ,
                     qreal &rotX, qreal& rotY, qreal& rotZ,
                     qreal &scaleX, qreal &scaleY, qreal& scaleZ,
                     qreal &shearX, qreal &shearY, qreal& shearZ,
                     QMatrix& postTransform,
                     BoundingBox* const parent) override;

    QDomElement saveFollowPathSVG(SvgExporter& exp,
                                  const FrameRange& visRange,
                                  QDomElement &childElement,
                                  QDomElement &parentElement) const;

private:
    void setRotScaleAfterTargetChange(
                BoundingBox* const oldTarget,
                BoundingBox* const newTarget) override;

    qsptr<BoolProperty> mRotate;
    qsptr<BoolProperty> mLengthBased;
    qsptr<QrealAnimator> mComplete;
    qsptr<QrealAnimator> mInfluence;
};


#endif // FOLLOWPATHEFFECT_H
