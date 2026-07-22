#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;

// Single-pass fisheye-unwarp + multi-camera blend. Cameras are bound as
// GL_TEXTURE_EXTERNAL_OES (zero-copy AHardwareBuffer import, see
// ARCHITECTURE.md section 7) so no CPU-side YUV->RGB conversion happens.
//
// N_CAMERA is injected as a preprocessor define by Material's shader
// variant cache (typically 4 or 6 for AVM rigs).
#ifndef N_CAMERA
#define N_CAMERA 4
#endif

uniform samplerExternalOES u_cameraTex[N_CAMERA];

in vec4 v_camUV0;
in vec4 v_blendWeights;
flat in ivec4 v_camIndex;

out vec4 fragColor;

vec4 sampleCamera(int camIndex, vec2 uv) {
    // GLSL ES does not allow indexing sampler arrays with a non-constant
    // expression on all Adreno driver versions in practice; unroll
    // explicitly to stay compatible with GLES 3.0 baseline.
    vec4 c = vec4(0.0);
    for (int i = 0; i < N_CAMERA; ++i) {
        if (i == camIndex) {
            c = texture(u_cameraTex[i], uv);
        }
    }
    return c;
}

void main() {
    vec4 blended = vec4(0.0);
    float weightSum = 0.0;

    // Up to 2 overlapping cameras per vertex slot in this baseline
    // (extend to 4 slots via a_camUV1 if a rig needs triple-overlap zones).
    if (v_blendWeights.x > 0.0) {
        blended += v_blendWeights.x * sampleCamera(v_camIndex.x, v_camUV0.xy);
        weightSum += v_blendWeights.x;
    }
    if (v_blendWeights.y > 0.0) {
        blended += v_blendWeights.y * sampleCamera(v_camIndex.y, v_camUV0.zw);
        weightSum += v_blendWeights.y;
    }

    fragColor = weightSum > 0.0 ? vec4(blended.rgb / weightSum, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}
