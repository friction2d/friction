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

#ifndef FRICTION_UI_VIEW_LAYER_UTILITIES
#define FRICTION_UI_VIEW_LAYER_UTILITIES

#include "../../core/skia/skiaincludes.h"
#include "../../core/efiltersettings.h"


void drawTransparencyMesh(SkCanvas* const canvas,
                          const SkRect &drawRect)
{
    SkPaint paint;
    SkBitmap bitmap;
    bitmap.setInfo(SkImageInfo::MakeA8(2, 2), 2);
    uint8_t pixels[4] = { 0, 255, 255, 0 };
    bitmap.setPixels(pixels);

    SkMatrix matr;
    const float scale = canvas->getTotalMatrix().getMinScale();
    const float dim = eSizesUI::widget*0.5f / (scale > 1.f ? 1.f : scale);
    matr.setScale(dim, dim);
    const auto shader = bitmap.makeShader(SkTileMode::kRepeat,
                                              SkTileMode::kRepeat, &matr);
    paint.setShader(shader);
    paint.setColor(SkColorSetARGB(255, 100, 100, 100));
    canvas->drawRect(drawRect, paint);
}

#endif // FRICTION_UI_VIEW_LAYER_UTILITIES
