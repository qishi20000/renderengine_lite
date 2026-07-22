#pragma once
#include <cstring>
#include <cmath>

// Minimal math types so the engine skeleton is self-contained and compiles
// without external dependencies. Once third_party/glm is vendored in
// (see CMakeLists.txt), it is recommended to replace this header with
// `#include <glm/glm.hpp>` + a couple of `using` aliases (mat4 = glm::mat4,
// vec3 = glm::vec3, ...) rather than hand-rolling vector math long-term.

namespace rel {

struct vec3 { float x = 0, y = 0, z = 0; };
struct vec4 { float x = 0, y = 0, z = 0, w = 0; };

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

} // namespace rel
