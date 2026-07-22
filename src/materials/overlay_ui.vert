#version 300 es
// 2D overlay quad (parking guide lines, radar markers, logo). Rendered in
// normalized device coordinates directly, no camera/model matrices needed.

layout(location = 0) in vec2 a_positionNDC;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_positionNDC, 0.0, 1.0);
}
