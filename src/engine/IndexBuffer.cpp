#include "rel/IndexBuffer.h"

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"

namespace rel {

IndexBuffer::~IndexBuffer() { delete mImpl; }

IndexBuffer* IndexBuffer::Builder::build(Engine& engine) {
    // Indices are always tightly-packed uint32 in this engine (see
    // backend::DriverApi::draw, which hardcodes GL_UNSIGNED_INT) — simpler
    // than supporting 16/32-bit indices for the small AVM meshes involved.
    uint32_t sizeBytes = mIndexCount * sizeof(uint32_t);

    auto* ib = new IndexBuffer();
    ib->mImpl = new Impl();
    ib->mImpl->indexCount = mIndexCount;
    ib->mImpl->handle = engine.getImpl()->driver->createIndexBuffer(nullptr, sizeBytes, toBackendUsage(mUsage));

    auto owned = std::unique_ptr<IndexBuffer>(ib);
    IndexBuffer* ptr = owned.get();
    engine.getImpl()->indexBuffers.push_back(std::move(owned));
    return ptr;
}

void IndexBuffer::setBuffer(Engine& engine, const void* data, uint32_t sizeBytes, uint32_t offset) {
    engine.getImpl()->driver->updateIndexBuffer(mImpl->handle, data, sizeBytes, offset);
}

} // namespace rel
