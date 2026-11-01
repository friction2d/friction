// Based on :
// https://www.shadertoy.com/view/MdyfWz
// "Simple Noise Fade" by Krzysztof Kondrak @k_kondrak

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

uniform float seed;
uniform float size;
uniform float sharpness;
uniform float time;

const float PI = 3.14159;

float r(in vec2 p) {
    return fract(cos((p.x + 0.00001*seed)*42.98 + (p.y + 0.00001*seed)*43.23) * 1127.53);
}

float n(in vec2 p) {
    vec2 fn = floor(p);
    vec2 sn = smoothstep(vec2(0), vec2(1), fract(p));
    
    float h1 = mix(r(fn), r(fn + vec2(1,0)), sn.x);
    float h2 = mix(r(fn + vec2(0,1)), r(fn + vec2(1)), sn.x);
    return mix(h1 ,h2, sn.y);
}

float noise(in vec2 p) {
    float s = max(abs(size), 0.001) * 0.001;
    return 0.58 * n(p/(32.*s)) +
           0.2 * n(p/(16.*s)) +
           0.1 * n(p/(8.*s)) +
           0.05 * n(p/(4.*s)) +
           0.02 * n(p/(2.*s)) +
           0.0125 * n(p/s);
}

void main(void) {
    float t = abs(sin(0.5*PI*time));

    float b = 0.25*(0.75 - 0.749*sharpness);
    float c = 1.0 - smoothstep(t - b, t + b, noise(texCoord * .4));
    fragColor = mix(texture(tex, texCoord), vec4(0), c);
}
