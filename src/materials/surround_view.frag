#version 300 es
precision mediump float;

// Single-pass fisheye-unwarp + multi-camera blend, sampling NV12 camera
// frames uploaded from CPU memory (see avm::CameraStreamManager and
// ARCHITECTURE.md section 6: video input is defined as N NV12 frames at
// plain CPU addresses, not zero-copy AHardwareBuffer/EGLImage — so each
// camera is bound as a plain Y (R8) + UV (RG8) sampler2D pair rather than a
// single samplerExternalOES, and NV12->RGB conversion happens right here.
//
// N_CAMERA is injected as a preprocessor define by Material's shader
// variant cache (typically 4 for a 4-camera AVM rig).
#ifndef N_CAMERA
#define N_CAMERA 4
#endif

uniform sampler2D u_cameraY[N_CAMERA];  // full-resolution luma plane, R8
uniform sampler2D u_cameraUV[N_CAMERA]; // half-resolution interleaved U,V plane, RG8 (NV12 order: R=U, G=V)

in vec4 v_camUV0;
in vec4 v_blendWeights;
flat in ivec4 v_camIndex;

out vec4 fragColor;

// BT.601 limited-range NV12 -> linear RGB. AVM cameras are consumer-grade
// automotive sensors piped through a standard ISP, so limited-range BT.601
// is the safe default; swap in BT.709 coefficients here if a given rig's
// ISP outputs full-range or HD-matrix YUV instead.
vec3 nv12ToRgb(float y, float u, float v) {
    y = (y - 16.0 / 255.0) * 1.164;
    u = u - 0.5;
    v = v - 0.5;
    return vec3(
        y + 1.596 * v,
        y - 0.392 * u - 0.813 * v,
        y + 2.017 * u
    );
}

vec3 sampleCameraRgb(int camIndex, vec2 uv) {
    // GLSL ES does not allow indexing sampler arrays with a non-constant
    // expression on all Adreno driver versions in practice; unroll
    // explicitly to stay compatible with GLES 3.0 baseline.
    vec3 rgb = vec3(0.0);
    for (int i = 0; i < N_CAMERA; ++i) {
        if (i == camIndex) {
            float y = texture(u_cameraY[i], uv).r;
            vec2 chroma = texture(u_cameraUV[i], uv).rg;
            rgb = nv12ToRgb(y, chroma.r, chroma.g);
        }
    }
    return rgb;
}

void main() {
    vec3 blended = vec3(0.0);
    float weightSum = 0.0;

    // Up to 2 overlapping cameras per vertex slot in this baseline
    // (extend to 4 slots via a_camUV1 if a rig needs triple-overlap zones).
    if (v_blendWeights.x > 0.0) {
        blended += v_blendWeights.x * sampleCameraRgb(v_camIndex.x, v_camUV0.xy);
        weightSum += v_blendWeights.x;
    }
    if (v_blendWeights.y > 0.0) {
        blended += v_blendWeights.y * sampleCameraRgb(v_camIndex.y, v_camUV0.zw);
        weightSum += v_blendWeights.y;
    }

    fragColor = weightSum > 0.0 ? vec4(blended / weightSum, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}
