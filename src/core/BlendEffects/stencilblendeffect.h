/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#ifndef STENCILBLENDEFFECT_H
#define STENCILBLENDEFFECT_H

#include "blendeffect.h"
#include "Animators/boolanimator.h"

class CORE_EXPORT StencilBlendEffect : public BlendEffect
{
public:
    StencilBlendEffect();

    void blendSetup(ChildRenderData &data,
                    const int index,
                    const qreal relFrame,
                    QList<ChildRenderData> &delayed) const override;

    void detachedBlendUISetup(const qreal relFrame,
                              const int drawId,
                              QList<UIDelayed> &delayed) override;
                              
    void detachedBlendSetup(const BoundingBox* const boxToDraw,
                            const qreal relFrame,
                            SkCanvas * const canvas,
                            const SkFilterQuality filter,
                            const int drawId,
                            QList<Delayed> &delayed) const override;
                            
    void drawBlendSetup(const qreal relFrame,
                        SkCanvas * const canvas) const override;

private:
    qsptr<BoolAnimator> mInvert;
};

#endif // STENCILBLENDEFFECT_H
