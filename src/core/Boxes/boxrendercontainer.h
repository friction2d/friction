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

#ifndef RENDERCONTAINER_H
#define RENDERCONTAINER_H
#include "smartPointers/ememory.h"
#include "skia/skqtconversions.h"
#include "skia/skiaincludes.h"
#include "smartPointers/ememory.h"
#include "framerange.h"

struct BoxRenderData;

class CORE_EXPORT RenderContainer {
public:
    RenderContainer() {}

    void drawSk(SkCanvas * const canvas,
                const SkFilterQuality filter) const;

    void updatePaintTransformGivenNewTotalTransform(
            const QTransform &totalTransform);

    void clear();
    void setSrcRenderData(BoxRenderData * const data);

    BoxRenderData *getSrcRenderData() const {
        return mSrcRenderData.get();
    }

    bool isExpired() const {
        return mExpired;
    }

    void setExpired(const bool expired) {
        mExpired = expired;
    }
protected:
    bool mExpired = false;
    bool mAntiAlias = false;
    qreal mResolutionFraction;
    QRect mGlobalRect;
    QTransform mTransform;
    SkMatrix mPaintTransform;
    stdsptr<BoxRenderData> mSrcRenderData;
    sk_sp<SkImage> mImageSk;
};

#endif // RENDERCONTAINER_H
