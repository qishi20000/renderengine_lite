#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "rel/rel_fwd.h"

namespace rel {

// Material owns compiled-shader-variant state (GLSL ES source + a cache of
// ProgramHandle per #define combination). No .mat file / offline compiler —
// see ARCHITECTURE.md section 5 for why that's the right tradeoff here.
class Material {
public:
    ~Material();

    class Builder {
    public:
        Builder& vertexShader(std::string_view glsl) { mVertexSrc = glsl; return *this; }
        Builder& fragmentShader(std::string_view glsl) { mFragmentSrc = glsl; return *this; }
        Material* build(Engine& engine);

    private:
        std::string mVertexSrc;
        std::string mFragmentSrc;
    };

    // Returns (compiling on first use) the ProgramHandle for a given set of
    // preprocessor defines, e.g. {"N_CAMERA=4", "ENABLE_TONEMAP=1"}.
    MaterialInstance* createInstance(const std::vector<std::string>& defines = {});

    // Internal use only (renderer/): see Camera::Impl comment above.
    struct Impl;
    Impl* getImpl() const { return mImpl; }

private:
    friend class Engine;
    Material() = default;
    Impl* mImpl = nullptr;
};

} // namespace rel
