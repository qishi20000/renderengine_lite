#version 300 es
// Surround-view (AVM) vertex shader.
//
// The bowl/ground mesh is pre-baked (see avm::BowlMeshGenerator) so that
// each vertex already knows which camera(s) cover it and the corresponding
// per-camera UV + blend weight. This keeps the fragment shader branch-free
// and lets the whole surround view render in a single pass.

layout(location = 0) in vec3 a_position;      // model-space bowl/plane vertex
layout(location = 1) in vec4 a_camUV0;        // (camA.uv, camB.uv) packed
layout(location = 2) in vec4 a_blendWeights;  // per-camera-slot blend weight, up to 4 slots
layout(location = 3) in vec4 a_camIndex;      // which of the N_CAMERA textures each slot samples

uniform mat4 u_viewProj;

out vec4 v_camUV0;
out vec4 v_blendWeights;
flat out ivec4 v_camIndex;

void main() {
    v_camUV0 = a_camUV0;
    v_blendWeights = a_blendWeights;
    v_camIndex = ivec4(a_camIndex);
    gl_Position = u_viewProj * vec4(a_position, 1.0);
}
