#version 300 es
// Minimal Blinn-Phong vertex shader for the 3D car model overlay.
// No skinning/morph targets — the car model is a static rigid mesh.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_model;
uniform mat4 u_viewProj;
uniform mat4 u_normalMatrix; // inverse-transpose(u_model), computed on CPU

out vec3 v_worldNormal;
out vec2 v_uv;

void main() {
    v_worldNormal = mat3(u_normalMatrix) * a_normal;
    v_uv = a_uv;
    gl_Position = u_viewProj * u_model * vec4(a_position, 1.0);
}
