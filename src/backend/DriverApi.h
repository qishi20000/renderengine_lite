#pragma once
#include <cstdint>
#include <string_view>

#include "backend/DriverEnums.h"
#include "backend/Handles.h"

// DriverApi is the single seam between the API-agnostic engine (engine/,
// renderer/, materials/) and a concrete graphics backend. Today the only
// implementation is opengl/GLDriver; a future VulkanDriver implements the
// exact same interface so upper layers require zero changes.
//
// Threading model (see ARCHITECTURE.md section 2.3): unlike Filament's
// two-thread CommandStream/Driver split, REL calls DriverApi directly and
// synchronously from the render thread to minimize latency, which matters
// for an AVM pipeline (camera -> display in as few frames as possible).
// The interface is still designed so a CommandStream could be inserted
// later without changing call sites.

namespace rel::backend {

struct TextureDescriptor {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t levels = 1;
    PixelFormat format = PixelFormat::RGBA8;
    uint8_t usage = static_cast<uint8_t>(TextureUsage::Sampleable);
};

struct VertexAttribute {
    ElementType type;
    uint32_t offset;
    uint32_t stride;
    uint8_t bufferSlot;
    bool normalized = false;
};

struct ProgramDescriptor {
    std::string_view vertexSource;
    std::string_view fragmentSource;
    // Preprocessor defines applied to both stages, e.g. {"N_CAMERA", "4"}.
    // Used to build shader variants (see materials/ variant cache).
};

struct RenderPassDescriptor {
    RenderTargetHandle target;
    float clearColor[4] = {0, 0, 0, 1};
    float clearDepth = 1.0f;
    bool clearColorBuffer = true;
    bool clearDepthBuffer = true;
    bool discardDepthOnEnd = true; // maps to glInvalidateFramebuffer on GLES
};

class DriverApi {
public:
    virtual ~DriverApi() = default;

    // --- lifecycle -----------------------------------------------------
    virtual void makeCurrent() = 0;
    virtual void commit() = 0; // swap buffers / present

    // --- resource creation ----------------------------------------------
    virtual VertexBufferHandle createVertexBuffer(
            const void* data, uint32_t sizeBytes, BufferUsage usage) = 0;
    virtual IndexBufferHandle createIndexBuffer(
            const void* data, uint32_t sizeBytes, BufferUsage usage) = 0;
    virtual TextureHandle createTexture(const TextureDescriptor& desc) = 0;
    // Reserved for a *future* zero-copy path (AHardwareBuffer -> EGLImage
    // -> GL_TEXTURE_EXTERNAL_OES), not currently used: the actual camera
    // input contract today is plain CPU-memory NV12 frames, uploaded via
    // createTexture()+updateTexture() as R8/RG8 plane pairs — see
    // avm::CameraStreamManager and ARCHITECTURE.md section 6. Kept in the
    // interface so a platform that *can* hand REL a GPU-importable buffer
    // has a path to avoid the CPU upload without an API change.
    virtual TextureHandle importExternalTexture(void* nativeImage) = 0;
    virtual ProgramHandle createProgram(const ProgramDescriptor& desc) = 0;
    virtual RenderTargetHandle createRenderTarget(
            TextureHandle color, TextureHandle depth,
            uint32_t width, uint32_t height) = 0;
    virtual RenderTargetHandle getDefaultRenderTarget() = 0; // the window surface

    // --- resource update -------------------------------------------------
    virtual void updateVertexBuffer(
            VertexBufferHandle h, const void* data, uint32_t sizeBytes, uint32_t offset) = 0;
    virtual void updateIndexBuffer(
            IndexBufferHandle h, const void* data, uint32_t sizeBytes, uint32_t offset) = 0;
    // `rowStrideBytes` == 0 means the source data is tightly packed
    // (rowStride == width * bytesPerPixel(format)). Pass a non-zero value
    // when uploading directly from a camera buffer whose row stride is
    // larger than its logical width (very common for NV12 camera frames,
    // which are often row-aligned to 16/32/64 bytes) — see
    // avm::CameraStreamManager and ARCHITECTURE.md section 6.
    virtual void updateTexture(
            TextureHandle h, const void* data, uint32_t sizeBytes, uint32_t rowStrideBytes) = 0;

    // --- destruction (may be deferred internally until the GPU is done) --
    virtual void destroyVertexBuffer(VertexBufferHandle) = 0;
    virtual void destroyIndexBuffer(IndexBufferHandle) = 0;
    virtual void destroyTexture(TextureHandle) = 0;
    virtual void destroyProgram(ProgramHandle) = 0;
    virtual void destroyRenderTarget(RenderTargetHandle) = 0;

    // --- drawing -----------------------------------------------------------
    virtual void beginRenderPass(const RenderPassDescriptor& desc) = 0;
    virtual void endRenderPass() = 0;
    virtual void bindProgram(ProgramHandle) = 0;
    virtual void bindTexture(uint32_t unit, TextureHandle, const SamplerParams&) = 0;
    virtual void setUniformMat4(std::string_view name, const float* mat4x4) = 0;
    virtual void setUniformFloat4(std::string_view name, const float* v4) = 0;
    // Needed to point a `sampler2D`/array-of-samplers uniform at the texture
    // unit a texture was bound to via bindTexture() — GLSL ES does not
    // implicitly associate sampler uniforms with texture units by name.
    virtual void setUniformInt(std::string_view name, int value) = 0;
    virtual void setRasterState(const RasterState&) = 0;
    virtual void draw(PrimitiveType type,
                       VertexBufferHandle vb,
                       IndexBufferHandle ib,
                       uint32_t indexCount,
                       const VertexAttribute* attributes,
                       uint32_t attributeCount) = 0;
};

} // namespace rel::backend
