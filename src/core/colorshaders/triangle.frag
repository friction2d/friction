#version 330 core
out vec4 FragColor;

uniform vec2 u_resolution;
uniform float u_radius;
uniform float u_hue;
uniform float u_sat;
uniform float u_val;
uniform float u_dpr;

const float PI = 3.14159265359;
const float SQRT_3 = 1.73205080757;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 center = u_resolution * 0.5;
    vec2 local_p = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y) - center;

    float hue_rad = (u_hue + 1.0 / 12.0) * 2.0 * PI;
    float target_y = u_val * (u_radius * 1.5);
    float t_row_width = target_y * 2.0 / SQRT_3;
    float t_row_x_0 = u_radius - t_row_width * 0.5;
    float target_x = u_sat * t_row_width + t_row_x_0;

    vec2 target_tr_p = vec2(target_x, target_y) - u_radius;
    float ic = cos(-hue_rad);
    float is = sin(-hue_rad);
    vec2 ind_pos = vec2(target_tr_p.x * ic - target_tr_p.y * is, target_tr_p.x * is + target_tr_p.y * ic);

    float dist_ind = length(local_p - ind_pos);
    float outline_radius = 6.0 * u_dpr;
    float core_radius = 4.0 * u_dpr;
    float ind_alpha = smoothstep(outline_radius + 1.0, outline_radius, dist_ind);

    vec2 tr_p = vec2(local_p.x * cos(hue_rad) - local_p.y * sin(hue_rad),
                     local_p.x * sin(hue_rad) + local_p.y * cos(hue_rad));
    tr_p += u_radius;

    float d_top = tr_p.y;
    float d_bottom = (u_radius * 1.5) - tr_p.y;
    float d_left = (tr_p.x - u_radius + tr_p.y / SQRT_3) * (SQRT_3 * 0.5);
    float d_right = (u_radius + tr_p.y / SQRT_3 - tr_p.x) * (SQRT_3 * 0.5);

    float tri_alpha = smoothstep(-1.0, 1.0, d_top) *
                      smoothstep(-1.0, 1.0, d_bottom) *
                      smoothstep(-1.0, 1.0, d_left) *
                      smoothstep(-1.0, 1.0, d_right);

    float final_alpha = max(tri_alpha, ind_alpha);
    if (final_alpha <= 0.0) discard;

    float val = clamp(tr_p.y / (u_radius * 1.5), 0.0, 1.0);
    float row_width = max(tr_p.y * 2.0 / SQRT_3, 0.0001);
    float row_x_0 = u_radius - row_width * 0.5;
    float sat = clamp((tr_p.x - row_x_0) / row_width, 0.0, 1.0);

    vec3 color = hsv2rgb(vec3(u_hue, sat, val));

    if (ind_alpha > 0.0) {
        vec3 ind_color = (dist_ind < core_radius) ? vec3(1.0) : vec3(0.0);
        color = mix(color, ind_color, ind_alpha);
    }

    FragColor = vec4(color, final_alpha);
}
