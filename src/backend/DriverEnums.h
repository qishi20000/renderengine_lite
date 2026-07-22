#pragma once
#include <cstdint>

// API-agnostic enums shared by every backend implementation (GLES today,
// Vulkan in the future). Naming/semantics are kept independent of any single
// graphics API so that upper layers (engine/renderer/materials) never need
// to change when a new backend is added.

namespace rel::backend {

enum class PixelFormat : uint8_t {
    RGBA8,
    RGB8,
    RGBA16F,
    R8,
    RG8,
    DEPTH24,
    DEPTH32F,
    DEPTH24_STENCIL8,
    // External/opaque camera formats (imported via AHardwareBuffer/EGLImage).
    YUV_420_888_EXTERNAL,
    RGBA8_EXTERNAL,
};

enum class TextureUsage : uint8_t {
    Sampleable   = 1 << 0,
    ColorAttachment = 1 << 1,
    DepthAttachment = 1 << 2,
    // Backed by a platform-provided external image (GL_TEXTURE_EXTERNAL_OES
    // on GLES / VkSamplerYcbcrConversion on Vulkan). Cannot be rendered to.
    ExternalImport = 1 << 3,
};

enum class BufferUsage : uint8_t {
    Static,     // uploaded once, rarely/never updated (e.g. bowl mesh)
    Dynamic,    // updated occasionally (e.g. calibration refresh)
    Streaming,  // updated (almost) every frame
};

enum class PrimitiveType : uint8_t {
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
};

enum class ElementType : uint8_t {
    Float,
    Float2,
    Float3,
    Float4,
    UByte4,
    UShort,
    UInt,
};

enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
};

enum class CullMode : uint8_t { None, Front, Back };
enum class DepthFunc : uint8_t { Never, Less, Equal, LessEqual, Greater, Always };

enum class SamplerWrap : uint8_t { Repeat, ClampToEdge, MirroredRepeat };
enum class SamplerFilter : uint8_t { Nearest, Linear };

struct SamplerParams {
    SamplerFilter minFilter = SamplerFilter::Linear;
    SamplerFilter magFilter = SamplerFilter::Linear;
    SamplerWrap wrapS = SamplerWrap::ClampToEdge;
    SamplerWrap wrapT = SamplerWrap::ClampToEdge;
};

struct RasterState {
    CullMode cullMode = CullMode::Back;
    DepthFunc depthFunc = DepthFunc::LessEqual;
    bool depthWrite = true;
    bool blendEnable = false;
};

} // namespace rel::backend
