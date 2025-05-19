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

#ifndef FRICTION_UI_RECTANGLE_DRAWABLE_H
#define FRICTION_UI_RECTANGLE_DRAWABLE_H

#include "drawable.h"

class RectangleDrawable : public Drawable {
public:
    RectangleDrawable() {};
    ~RectangleDrawable() = default;

    void drawToCanvas(SkCanvas * const canvas) override {
        const auto positionX = position().x();
        const auto positionY = position().y();
        const auto width = size().x();
        const auto height = size().y();

        SkRect rect;
        rect.setXYWH(positionX, positionY, width, height);

        // Set stroke Skia Paint to use
        SkPaint paintStroke;
        paintStroke.setStyle(SkPaint::kStroke_Style);
        paintStroke.setStrokeWidth(strokeSize());
        //paintStroke.setColor(strokeColor()); // TODO: convert to SkColor

        // Set fill Skia Paint to use
        SkPaint paintFill;
        paintFill.setStyle(SkPaint::kFill_Style);
        //paintFill.setColor(fillColor()); // TODO: convert to SkColor

        // Draw the rectangles
        canvas->drawRect(rect, paintStroke);
        canvas->drawRect(rect, paintFill);
    };
};

#endif // FRICTION_UI_RECTANGLE_DRAWABLE_H
