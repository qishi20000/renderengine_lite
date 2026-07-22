#include "rel/VertexBuffer.h"

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"

namespace rel {

VertexBuffer::~VertexBuffer() { delete mImpl; }

VertexBuffer* VertexBuffer::Builder::build(Engine& engine) {
    auto* vb = new VertexBuffer();
    vb->mImpl = new Impl();
    vb->mImpl->sizeBytes = mSizeBytes;
    vb->mImpl->attributes = mAttributes;
    vb->mImpl->handle = engine.getImpl()->driver->createVertexBuffer(nullptr, mSizeBytes, toBackendUsage(mUsage));

    auto owned = std::unique_ptr<VertexBuffer>(vb);
    VertexBuffer* ptr = owned.get();
    engine.getImpl()->vertexBuffers.push_back(std::move(owned));
    return ptr;
}

void VertexBuffer::setBufferData(Engine& engine, const void* data, uint32_t sizeBytes, uint32_t offset) {
    engine.getImpl()->driver->updateVertexBuffer(mImpl->handle, data, sizeBytes, offset);
}

} // namespace rel
