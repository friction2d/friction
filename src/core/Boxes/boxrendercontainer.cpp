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

#include "boxrendercontainer.h"
#include "boxrenderdata.h"
#include "skia/skiahelpers.h"

void RenderContainer::drawSk(SkCanvas * const canvas,
                             const SkFilterQuality filter) const {
    if(!mSrcRenderData) return;
    canvas->save();
    canvas->concat(mPaintTransform);
    SkPaint paint;
    paint.setFilterQuality(filter);
    mSrcRenderData->drawOnParentLayer(canvas, paint);
    canvas->restore();
}

void RenderContainer::updatePaintTransformGivenNewTotalTransform(
                                    const QTransform &totalTransform) {
    QTransform paintTransform = mTransform.inverted()*totalTransform;
    const qreal invRes = 1/mResolutionFraction;
    paintTransform.scale(invRes, invRes);
    mPaintTransform = toSkMatrix(paintTransform);
}

void RenderContainer::clear() {
    mImageSk.reset();
    mSrcRenderData.reset();
}

void RenderContainer::setSrcRenderData(BoxRenderData * const data) {
    mTransform = data->fTotalTransform;
    mResolutionFraction = data->fResolution;
    mImageSk = data->fRenderedImage;
    mGlobalRect = data->fGlobalRect;
    mAntiAlias = data->fAntiAlias;
    QTransform paintTransform;
    paintTransform.scale(1/mResolutionFraction, 1/mResolutionFraction);
    mPaintTransform = toSkMatrix(paintTransform);
    mSrcRenderData = data->ref<BoxRenderData>();
}
