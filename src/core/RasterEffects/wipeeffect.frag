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

#version 330 core
layout(location = 0) out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D tex;

uniform float sharpness;
uniform float direction;
uniform float time;

uniform float x0;
uniform float x1;

const float PI = 3.1415926535;
const float sqrt2 = 1.41421356237;

void main(void) {
    float x = texCoord.x;
    float y = texCoord.y;

    bool i = mod(direction, PI) > 0.5*PI;
    float dir = direction;
    if(i) {
        x = 1 - x;
        dir = PI - dir;
    }
    bool ii = mod(dir, 2 * PI) > PI;

    float a = max(sqrt(x*x + y*y), 1e-6);
    float b = dir - asin(y / a);
    float c = 0.25*PI - dir;

    float f = a * cos(b) / (cos(c) * sqrt2 + 1e-6);

    if(ii) f = 1 - f;

    f += 0.33333 * sqrt2 * (1 - sharpness);

    float alpha;
    if(f < x0) {
        alpha = 0;
    } else if(f > x1) {
        alpha = 1;
    } else {
        alpha = 1.0 - 0.5*(cos(PI*(f - x0)/max(1.0 - sharpness, 1e-6)) + 1.0);
    }
    fragColor = texture(tex, texCoord) * alpha;
}
