#pragma once
#include <cstdint>
#include "rel/rel_fwd.h"
#include "rel/VertexBuffer.h" // BufferUsage

namespace rel {

class IndexBuffer {
public:
    class Builder {
    public:
        Builder& indexCount(uint32_t count) { mIndexCount = count; return *this; }
        Builder& usage(BufferUsage usage) { mUsage = usage; return *this; }
        IndexBuffer* build(Engine& engine);

    private:
        uint32_t mIndexCount = 0;
        BufferUsage mUsage = BufferUsage::Static;
    };

    void setBuffer(Engine& engine, const void* data, uint32_t sizeBytes, uint32_t offset = 0);

private:
    friend class Engine;
    IndexBuffer() = default;
    struct Impl;
    Impl* mImpl = nullptr;
};

} // namespace rel
