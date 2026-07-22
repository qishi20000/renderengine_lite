#pragma once
#include <unordered_map>
#include <memory>
#include <vector>

#include "backend/DriverApi.h"
#include "backend/Platform.h"

// GLDriver: the (currently) only DriverApi implementation, targeting
// OpenGL ES 3.0+ on Qualcomm Adreno. See ARCHITECTURE.md section 7 for the
// Adreno-specific optimization rules this implementation should follow
// (single-pass rendering, glInvalidateFramebuffer, external textures for
// zero-copy camera import, etc).

namespace rel::backend::opengl {

// Backend-internal resource records referenced by Handle<Tag>::index.
struct GLTexture {
    uint32_t glId = 0;
    uint32_t target = 0; // GL_TEXTURE_2D or GL_TEXTURE_EXTERNAL_OES
    TextureDescriptor desc;
};

struct GLBuffer {
    uint32_t glId = 0;
    uint32_t target = 0; // GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
    uint32_t sizeBytes = 0;
};

struct GLProgram {
    uint32_t glId = 0;
};

struct GLRenderTarget {
    uint32_t fboId = 0; // 0 == default framebuffer (window surface)
    uint32_t width = 0;
    uint32_t height = 0;
};

class GLDriver final : public DriverApi {
public:
    explicit GLDriver(Platform& platform);
    ~GLDriver() override;

    void makeCurrent() override;
    void commit() override;

    VertexBufferHandle createVertexBuffer(const void* data, uint32_t sizeBytes, BufferUsage usage) override;
    IndexBufferHandle createIndexBuffer(const void* data, uint32_t sizeBytes, BufferUsage usage) override;
    TextureHandle createTexture(const TextureDescriptor& desc) override;
    TextureHandle importExternalTexture(void* nativeImage) override;
    ProgramHandle createProgram(const ProgramDescriptor& desc) override;
    RenderTargetHandle createRenderTarget(TextureHandle color, TextureHandle depth,
                                           uint32_t width, uint32_t height) override;
    RenderTargetHandle getDefaultRenderTarget() override;

    void updateVertexBuffer(VertexBufferHandle h, const void* data, uint32_t sizeBytes, uint32_t offset) override;
    void updateIndexBuffer(IndexBufferHandle h, const void* data, uint32_t sizeBytes, uint32_t offset) override;
    void updateTexture(TextureHandle h, const void* data, uint32_t sizeBytes) override;

    void destroyVertexBuffer(VertexBufferHandle) override;
    void destroyIndexBuffer(IndexBufferHandle) override;
    void destroyTexture(TextureHandle) override;
    void destroyProgram(ProgramHandle) override;
    void destroyRenderTarget(RenderTargetHandle) override;

    void beginRenderPass(const RenderPassDescriptor& desc) override;
    void endRenderPass() override;
    void bindProgram(ProgramHandle) override;
    void bindTexture(uint32_t unit, TextureHandle, const SamplerParams&) override;
    void setUniformMat4(std::string_view name, const float* mat4x4) override;
    void setUniformFloat4(std::string_view name, const float* v4) override;
    void setRasterState(const RasterState&) override;
    void draw(PrimitiveType type, VertexBufferHandle vb, IndexBufferHandle ib,
              uint32_t indexCount, const VertexAttribute* attributes,
              uint32_t attributeCount) override;

private:
    Platform& mPlatform;

    // Simple pooled-index allocators. Generation counters guard against
    // stale-handle use-after-free bugs from upper layers.
    template<typename Tag, typename Record>
    struct Pool {
        std::vector<Record> records;
        std::vector<uint16_t> generations;
        std::vector<uint32_t> freeList;

        Handle<Tag> allocate(Record record) {
            uint32_t index;
            if (!freeList.empty()) {
                index = freeList.back();
                freeList.pop_back();
                records[index] = std::move(record);
            } else {
                index = static_cast<uint32_t>(records.size());
                records.push_back(std::move(record));
                generations.push_back(0);
            }
            return Handle<Tag>{{index, generations[index]}};
        }

        Record* get(Handle<Tag> h) {
            if (!h.isValid() || h.index >= records.size()) return nullptr;
            if (generations[h.index] != h.generation) return nullptr;
            return &records[h.index];
        }

        void free(Handle<Tag> h) {
            if (!h.isValid() || h.index >= records.size()) return;
            generations[h.index]++;
            freeList.push_back(h.index);
        }
    };

    Pool<VertexBufferTag, GLBuffer> mVertexBuffers;
    Pool<IndexBufferTag, GLBuffer> mIndexBuffers;
    Pool<TextureTag, GLTexture> mTextures;
    Pool<ProgramTag, GLProgram> mPrograms;
    Pool<RenderTargetTag, GLRenderTarget> mRenderTargets;

    RenderTargetHandle mDefaultRenderTarget;
    ProgramHandle mCurrentProgram;
};

} // namespace rel::backend::opengl
