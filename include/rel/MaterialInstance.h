#pragma once
#include <string_view>
#include "rel/rel_fwd.h"

namespace rel {

// A Material with a concrete set of parameter values (uniforms/textures).
// Multiple instances can share one compiled Material/shader variant, e.g.
// several car-model parts reusing the same "simple_lit" material.
class MaterialInstance {
public:
    ~MaterialInstance();

    void setParameter(std::string_view name, float x, float y, float z, float w);
    void setParameter(std::string_view name, const float* mat4x4);
    void setParameter(std::string_view name, Texture* texture);

    Material* getMaterial() const { return mMaterial; }

    // Internal use only (renderer/): see Camera::Impl comment above.
    struct Impl;
    Impl* getImpl() const { return mImpl; }

private:
    friend class Material;
    explicit MaterialInstance(Material* material) : mMaterial(material) {}
    Material* mMaterial;
    Impl* mImpl = nullptr;
};

} // namespace rel
