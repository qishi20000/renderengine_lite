#pragma once
#include <cstdint>
#include <limits>

// Handle-based GPU resource system.
//
// Rationale (see ARCHITECTURE.md section 2):
//  - Upper layers never see raw GLuint/VkImage pointers, only a small POD
//    {index, generation} handle. This decouples resource lifetime from the
//    scene/material graph and allows the backend to freely pool/recycle
//    objects, defer destruction, or swap the underlying implementation
//    (GLES today, Vulkan tomorrow) without touching a single call site
//    above backend::DriverApi.

namespace rel::backend {

struct HandleBase {
    static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

    uint32_t index = kInvalidIndex;
    uint16_t generation = 0;

    constexpr bool isValid() const noexcept { return index != kInvalidIndex; }
    constexpr bool operator==(const HandleBase& o) const noexcept {
        return index == o.index && generation == o.generation;
    }
};

// Strongly-typed handle wrapper: Handle<GLTexture> vs Handle<GLProgram> are
// distinct types at compile time even though they share the same layout.
template<typename Tag>
struct Handle : HandleBase {};

// Backend-internal tag types (opaque here; each backend defines the real
// object each handle refers to, e.g. opengl/GLTexture.h).
struct VertexBufferTag {};
struct IndexBufferTag {};
struct TextureTag {};
struct ProgramTag {};
struct RenderTargetTag {};
struct SamplerGroupTag {};
struct FenceTag {};

using VertexBufferHandle = Handle<VertexBufferTag>;
using IndexBufferHandle  = Handle<IndexBufferTag>;
using TextureHandle      = Handle<TextureTag>;
using ProgramHandle      = Handle<ProgramTag>;
using RenderTargetHandle = Handle<RenderTargetTag>;
using SamplerGroupHandle = Handle<SamplerGroupTag>;
using FenceHandle        = Handle<FenceTag>;

} // namespace rel::backend
