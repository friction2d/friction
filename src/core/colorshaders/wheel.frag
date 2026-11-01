#version 330 core
out vec4 FragColor;

uniform vec2 u_resolution;
uniform float u_innerRadius;
uniform float u_outerRadius;
uniform float u_hue;
uniform float u_dpr;

const float PI = 3.14159265359;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 center = u_resolution * 0.5;
    vec2 p = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y) - center;

    float dist = length(p);
    float alpha = smoothstep(u_outerRadius, u_outerRadius - u_dpr, dist) *
                  smoothstep(u_innerRadius - u_dpr, u_innerRadius, dist);

    if (alpha <= 0.0) discard;

    float angle = atan(p.y, -p.x);
    float h = (angle / (2.0 * PI)) + 0.5;

    vec3 color = hsv2rgb(vec3(h, 1.0, 1.0));

    float ind_angle = (u_hue - 0.5) * 2.0 * PI;
    vec2 ind_pos = vec2(-cos(ind_angle), sin(ind_angle)) * ((u_innerRadius + u_outerRadius) * 0.5);

    float dist_ind = length(p - ind_pos);

    if (dist_ind < 6.0 * u_dpr) color = vec3(0.0);
    if (dist_ind < 4.0 * u_dpr) color = vec3(1.0);

    FragColor = vec4(color, alpha);
}
