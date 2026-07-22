#include "rel/Texture.h"

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"

namespace rel {

Texture::~Texture() { delete mImpl; }

Texture* Texture::Builder::build(Engine& engine) {
    backend::TextureDescriptor desc;
    desc.width = mWidth;
    desc.height = mHeight;
    desc.levels = mLevels;
    desc.format = toBackendPixelFormat(mFormat);

    auto* tex = new Texture();
    tex->mImpl = new Impl();
    tex->mImpl->width = mWidth;
    tex->mImpl->height = mHeight;
    tex->mImpl->format = mFormat;
    tex->mImpl->handle = engine.getImpl()->driver->createTexture(desc);

    auto owned = std::unique_ptr<Texture>(tex);
    Texture* ptr = owned.get();
    engine.getImpl()->textures.push_back(std::move(owned));
    return ptr;
}

Texture* Texture::createExternal(Engine& engine, void* nativeImage) {
    // See ARCHITECTURE.md sections 6/7: zero-copy camera import. The
    // resulting Texture is read-only (no setImage()) and must be bound as
    // GL_TEXTURE_EXTERNAL_OES by the SurroundViewPass material.
    auto* tex = new Texture();
    tex->mImpl = new Impl();
    tex->mImpl->isExternal = true;
    tex->mImpl->handle = engine.getImpl()->driver->importExternalTexture(nativeImage);

    auto owned = std::unique_ptr<Texture>(tex);
    Texture* ptr = owned.get();
    engine.getImpl()->textures.push_back(std::move(owned));
    return ptr;
}

void Texture::setImage(Engine& engine, const void* data, uint32_t sizeBytes, uint32_t rowStrideBytes) {
    if (mImpl->isExternal) {
        // External (camera-imported) textures are updated by re-importing a
        // new native image, not by CPU upload — see
        // avm::CameraStreamManager::onFrameAvailable.
        return;
    }
    engine.getImpl()->driver->updateTexture(mImpl->handle, data, sizeBytes, rowStrideBytes);
}

} // namespace rel
