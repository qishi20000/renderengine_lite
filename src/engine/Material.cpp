#include "rel/Material.h"
#include "rel/MaterialInstance.h"

#include <algorithm>

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"

namespace rel {

namespace {

// Injects `#define NAME VALUE` lines right after the mandatory first-line
// `#version ...` directive so the same GLSL ES source can be recompiled
// into different variants (e.g. N_CAMERA=4 vs N_CAMERA=6). See
// ARCHITECTURE.md section 5.
std::string injectDefines(const std::string& source, const std::vector<std::string>& defines) {
    if (defines.empty()) return source;
    size_t versionEnd = source.find('\n');
    std::string result = source.substr(0, versionEnd + 1);
    for (const auto& define : defines) {
        result += "#define " + define + "\n";
    }
    result += source.substr(versionEnd + 1);
    return result;
}

std::string variantKey(const std::vector<std::string>& defines) {
    std::vector<std::string> sorted = defines;
    std::sort(sorted.begin(), sorted.end());
    std::string key;
    for (const auto& d : sorted) { key += d; key += ';'; }
    return key;
}

} // namespace

Material::~Material() { delete mImpl; }

Material* Material::Builder::build(Engine& engine) {
    auto* mat = new Material();
    mat->mImpl = new Impl();
    mat->mImpl->engine = &engine;
    mat->mImpl->vertexSource = mVertexSrc;
    mat->mImpl->fragmentSource = mFragmentSrc;

    auto owned = std::unique_ptr<Material>(mat);
    Material* ptr = owned.get();
    engine.getImpl()->materials.push_back(std::move(owned));
    return ptr;
}

MaterialInstance* Material::createInstance(const std::vector<std::string>& defines) {
    const std::string key = variantKey(defines);
    backend::ProgramHandle program;

    auto it = mImpl->variantCache.find(key);
    if (it != mImpl->variantCache.end()) {
        program = it->second;
    } else {
        const std::string vs = injectDefines(mImpl->vertexSource, defines);
        const std::string fs = injectDefines(mImpl->fragmentSource, defines);
        backend::ProgramDescriptor desc;
        desc.vertexSource = vs;
        desc.fragmentSource = fs;
        // GLDriver::createProgram compiles synchronously, so it's safe for
        // vs/fs to be locals that don't outlive this call.
        program = mImpl->engine->getImpl()->driver->createProgram(desc);
        mImpl->variantCache.emplace(key, program);
    }

    auto* instance = new MaterialInstance(this);
    instance->mImpl = new MaterialInstance::Impl();
    instance->mImpl->program = program;

    auto owned = std::unique_ptr<MaterialInstance>(instance);
    MaterialInstance* ptr = owned.get();
    mImpl->instances.push_back(std::move(owned));
    return ptr;
}

} // namespace rel
