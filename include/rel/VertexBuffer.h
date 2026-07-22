#pragma once
#include <cstdint>
#include <vector>

#include "rel/rel_fwd.h"

namespace rel {

enum class ElementType : uint8_t { Float, Float2, Float3, Float4, UByte4, UShort, UInt };
enum class BufferUsage : uint8_t { Static, Dynamic, Streaming };

struct VertexAttributeDesc {
    ElementType type;
    uint32_t offset;
    uint32_t stride;
    uint8_t slot;
    bool normalized = false;
};

// Public-facing vertex buffer. Owns a backend::VertexBufferHandle internally
// (see src/engine/VertexBufferImpl.h) — callers never touch the backend.
class VertexBuffer {
public:
    class Builder {
    public:
        Builder& vertexCount(uint32_t count) { mVertexCount = count; return *this; }
        Builder& bufferSizeBytes(uint32_t bytes) { mSizeBytes = bytes; return *this; }
        Builder& usage(BufferUsage usage) { mUsage = usage; return *this; }
        Builder& attribute(VertexAttributeDesc desc) { mAttributes.push_back(desc); return *this; }
        VertexBuffer* build(Engine& engine);

    private:
        uint32_t mVertexCount = 0;
        uint32_t mSizeBytes = 0;
        BufferUsage mUsage = BufferUsage::Static;
        std::vector<VertexAttributeDesc> mAttributes;
        friend class VertexBuffer;
    };

    void setBufferData(Engine& engine, const void* data, uint32_t sizeBytes, uint32_t offset = 0);

private:
    friend class Builder;
    friend class Engine;
    VertexBuffer() = default;
    struct Impl;
    Impl* mImpl = nullptr;
};

} // namespace rel
