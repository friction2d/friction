#version 330 core
out vec4 FragColor;

uniform vec2 u_resolution;
uniform float u_hue;
uniform float u_sat;
uniform float u_val;
uniform float u_dpr;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;

    vec3 color = hsv2rgb(vec3(u_hue, uv.x, uv.y));

    vec2 ind_pos = vec2(u_sat, u_val) * u_resolution;
    float dist_ind = length(gl_FragCoord.xy - ind_pos);

    float outline_radius = 6.0 * u_dpr;
    float core_radius = 4.0 * u_dpr;

    float ind_alpha = smoothstep(outline_radius + 1.0, outline_radius, dist_ind);

    if (ind_alpha > 0.0) {
        vec3 ind_color = (dist_ind < core_radius) ? vec3(1.0) : vec3(0.0);
        color = mix(color, ind_color, ind_alpha);
    }

    FragColor = vec4(color, 1.0);
}
