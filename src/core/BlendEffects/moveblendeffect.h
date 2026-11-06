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

#ifndef MOVEBLENDEFFECT_H
#define MOVEBLENDEFFECT_H

#include "blendeffect.h"
#include "Animators/intanimator.h"

class CORE_EXPORT MoveBlendEffect : public BlendEffect {
public:
    MoveBlendEffect();

    void blendSetup(ChildRenderData &data,
                    const int index,
                    const qreal relFrame,
                    QList<ChildRenderData> &delayed) const;

    void detachedBlendUISetup(const qreal relFrame,
                              const int drawId,
                              QList<UIDelayed> &delayed);
    void detachedBlendSetup(const BoundingBox* const boxToDraw,
                        const qreal relFrame,
                        SkCanvas * const canvas,
                        const SkFilterQuality filter,
                        const int drawId,
                        QList<Delayed> &delayed) const;
    void drawBlendSetup(const qreal relFrame,
                        SkCanvas * const canvas) const;

    virtual void saveSVG(SvgExporter& exp,
                         QDomElement& ele,
                         const FrameRange& visRange,
                         DomEleTask* task,
                         const BoundingBox* targetBox) const;

    int zIndex(const qreal relFrame) const;
private:
    int calcDIndex(const qreal relFrame) const;

    qsptr<IntAnimator> mZIndex;
};

#endif // MOVEBLENDEFFECT_H
