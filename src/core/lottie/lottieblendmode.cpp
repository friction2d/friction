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
*/

#include "lottie/lottieblendmode.h"

int LottieBlendMode::value(const SkBlendMode mode)
{
    switch (mode) {
    case SkBlendMode::kMultiply: return 1;
    case SkBlendMode::kScreen: return 2;
    case SkBlendMode::kOverlay: return 3;
    case SkBlendMode::kDarken: return 4;
    case SkBlendMode::kLighten: return 5;
    case SkBlendMode::kColorDodge: return 6;
    case SkBlendMode::kColorBurn: return 7;
    case SkBlendMode::kHardLight: return 8;
    case SkBlendMode::kSoftLight: return 9;
    case SkBlendMode::kDifference: return 10;
    case SkBlendMode::kExclusion: return 11;
    case SkBlendMode::kHue: return 12;
    case SkBlendMode::kSaturation: return 13;
    case SkBlendMode::kColor: return 14;
    case SkBlendMode::kLuminosity: return 15;
    case SkBlendMode::kPlus: return 16;
    default: return 0;
    }
}
