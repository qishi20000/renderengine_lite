#pragma once
#include <cstring>
#include <cmath>

// Minimal math types so the engine skeleton is self-contained and compiles
// without external dependencies. Once third_party/glm is vendored in
// (see CMakeLists.txt), it is recommended to replace this header with
// `#include <glm/glm.hpp>` + a couple of `using` aliases (mat4 = glm::mat4,
// vec3 = glm::vec3, ...) rather than hand-rolling vector math long-term.

namespace rel {

struct vec3 {
    float x = 0, y = 0, z = 0;

    vec3 operator-(const vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    vec3 operator+(const vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
};
struct vec4 { float x = 0, y = 0, z = 0, w = 0; };

inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 cross(const vec3& a, const vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline vec3 normalize(const vec3& v) {
    float len = std::sqrt(dot(v, v));
    return len > 1e-8f ? vec3{v.x / len, v.y / len, v.z / len} : v;
}

struct mat4 {
    // Column-major, matches GLSL/GLES conventions.
    float m[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1,
    };

    static mat4 identity() { return mat4{}; }

    mat4 operator*(const mat4& rhs) const {
        mat4 result{};
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[k * 4 + row] * rhs.m[col * 4 + k];
                }
                result.m[col * 4 + row] = sum;
            }
        }
        return result;
    }

    const float* data() const { return m; }
};

struct Box {
    vec3 min{0, 0, 0};
    vec3 max{0, 0, 0};
};

inline mat4 perspective(float fovYRadians, float aspect, float near, float far) {
    mat4 m{};
    for (float& f : m.m) f = 0.f;
    const float f = 1.0f / std::tan(fovYRadians * 0.5f);
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (far + near) / (near - far);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * far * near) / (near - far);
    return m;
}

inline mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
    mat4 m{};
    for (float& f : m.m) f = 0.f;
    m.m[0] = 2.0f / (right - left);
    m.m[5] = 2.0f / (top - bottom);
    m.m[10] = -2.0f / (far - near);
    m.m[12] = -(right + left) / (right - left);
    m.m[13] = -(top + bottom) / (top - bottom);
    m.m[14] = -(far + near) / (far - near);
    m.m[15] = 1.0f;
    return m;
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    const vec3 forward = normalize(center - eye);
    const vec3 right = normalize(cross(forward, up));
    const vec3 realUp = cross(right, forward);

    mat4 m{};
    m.m[0] = right.x;   m.m[4] = right.y;   m.m[8] = right.z;    m.m[12] = -dot(right, eye);
    m.m[1] = realUp.x;  m.m[5] = realUp.y;  m.m[9] = realUp.z;   m.m[13] = -dot(realUp, eye);
    m.m[2] = -forward.x; m.m[6] = -forward.y; m.m[10] = -forward.z; m.m[14] = dot(forward, eye);
    m.m[3] = 0; m.m[7] = 0; m.m[11] = 0; m.m[15] = 1;
    return m;
}

} // namespace rel
