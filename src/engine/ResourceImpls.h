#pragma once
// Internal-only Impl struct definitions for the pimpl'd public API classes.
// Only .cpp files inside src/ should include this header; include/rel/*.h
// consumers only ever see an incomplete `struct Impl;` forward declaration,
// so this file is the single place backend::Handle types leak into the
// otherwise API-agnostic public surface.

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "rel/Camera.h"
#include "rel/IndexBuffer.h"
#include "rel/Material.h"
#include "rel/MaterialInstance.h"
#include "rel/SwapChain.h"
#include "rel/Texture.h"
#include "rel/VertexBuffer.h"

#include "backend/DriverEnums.h"
#include "backend/Handles.h"
#include "utils/Math.h"

namespace rel {

// Public rel::BufferUsage/ElementType/PixelFormat (include/rel/*.h) are kept
// separate, minimal enums so app code including only include/rel/ never
// needs backend/DriverEnums.h. These converters are the (only) place the
// two enum sets are bridged.
inline backend::BufferUsage toBackendUsage(BufferUsage u) {
    switch (u) {
        case BufferUsage::Static:    return backend::BufferUsage::Static;
        case BufferUsage::Dynamic:   return backend::BufferUsage::Dynamic;
        case BufferUsage::Streaming: return backend::BufferUsage::Streaming;
    }
    return backend::BufferUsage::Static;
}

inline backend::ElementType toBackendElementType(ElementType t) {
    switch (t) {
        case ElementType::Float:  return backend::ElementType::Float;
        case ElementType::Float2: return backend::ElementType::Float2;
        case ElementType::Float3: return backend::ElementType::Float3;
        case ElementType::Float4: return backend::ElementType::Float4;
        case ElementType::UByte4: return backend::ElementType::UByte4;
        case ElementType::UShort: return backend::ElementType::UShort;
        case ElementType::UInt:   return backend::ElementType::UInt;
    }
    return backend::ElementType::Float;
}

inline backend::PixelFormat toBackendPixelFormat(PixelFormat f) {
    switch (f) {
        case PixelFormat::RGBA8:   return backend::PixelFormat::RGBA8;
        case PixelFormat::RGB8:    return backend::PixelFormat::RGB8;
        case PixelFormat::RGBA16F: return backend::PixelFormat::RGBA16F;
        case PixelFormat::R8:      return backend::PixelFormat::R8;
        case PixelFormat::RG8:     return backend::PixelFormat::RG8;
        case PixelFormat::DEPTH24: return backend::PixelFormat::DEPTH24;
        case PixelFormat::DEPTH32F:return backend::PixelFormat::DEPTH32F;
    }
    return backend::PixelFormat::RGBA8;
}

struct VertexBuffer::Impl {
    backend::VertexBufferHandle handle;
    uint32_t sizeBytes = 0;
    std::vector<VertexAttributeDesc> attributes;
};

struct IndexBuffer::Impl {
    backend::IndexBufferHandle handle;
    uint32_t indexCount = 0;
};

struct Texture::Impl {
    backend::TextureHandle handle;
    uint32_t width = 0;
    uint32_t height = 0;
    PixelFormat format = PixelFormat::RGBA8;
    bool isExternal = false; // true for zero-copy camera imports (GL_TEXTURE_EXTERNAL_OES)
};

// A single uniform parameter value staged on a MaterialInstance. Kept as a
// tiny tagged union rather than pulling in a generic std::variant to stay
// dependency-light; add cases here as materials/*.frag need more types.
struct MaterialParam {
    enum class Kind { Float4, Mat4, Texture } kind;
    std::string name;
    float f4[4] = {0, 0, 0, 0};
    mat4 m4;
    Texture* tex = nullptr;
};

struct MaterialInstance::Impl {
    backend::ProgramHandle program; // resolved shader variant for this instance
    std::vector<MaterialParam> params;
};

struct Material::Impl {
    Engine* engine = nullptr;
    std::string vertexSource;
    std::string fragmentSource;
    // hash(sorted defines) -> compiled program, see Material.cpp buildVariant().
    std::unordered_map<std::string, backend::ProgramHandle> variantCache;
    std::vector<std::unique_ptr<MaterialInstance>> instances;
};

struct Camera::Impl {
    mat4 projection = mat4::identity();
    mat4 view = mat4::identity();
    mat4 getViewProj() const { return projection * view; }
};

struct SwapChain::Impl {
    void* nativeWindow = nullptr;
};

} // namespace rel
