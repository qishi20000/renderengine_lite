#include "rel/MaterialInstance.h"

#include <algorithm>
#include <cstring>

#include "engine/ResourceImpls.h"

namespace rel {

namespace {
MaterialParam* findOrAddParam(std::vector<MaterialParam>& params, std::string_view name) {
    auto it = std::find_if(params.begin(), params.end(),
                            [&](const MaterialParam& p) { return p.name == name; });
    if (it != params.end()) return &(*it);
    params.push_back(MaterialParam{});
    params.back().name = std::string(name);
    return &params.back();
}
} // namespace

MaterialInstance::~MaterialInstance() { delete mImpl; }

void MaterialInstance::setParameter(std::string_view name, float x, float y, float z, float w) {
    auto* p = findOrAddParam(mImpl->params, name);
    p->kind = MaterialParam::Kind::Float4;
    p->f4[0] = x; p->f4[1] = y; p->f4[2] = z; p->f4[3] = w;
}

void MaterialInstance::setParameter(std::string_view name, const float* mat4x4) {
    auto* p = findOrAddParam(mImpl->params, name);
    p->kind = MaterialParam::Kind::Mat4;
    std::memcpy(p->m4.m, mat4x4, sizeof(float) * 16);
}

void MaterialInstance::setParameter(std::string_view name, Texture* texture) {
    auto* p = findOrAddParam(mImpl->params, name);
    p->kind = MaterialParam::Kind::Texture;
    p->tex = texture;
}

} // namespace rel
