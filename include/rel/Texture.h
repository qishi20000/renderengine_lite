#pragma once
#include <cstdint>
#include "rel/rel_fwd.h"

namespace rel {

enum class PixelFormat : uint8_t { RGBA8, RGB8, RGBA16F, R8, RG8, DEPTH24, DEPTH32F };

class Texture {
public:
    ~Texture();

    class Builder {
    public:
        Builder& width(uint32_t w) { mWidth = w; return *this; }
        Builder& height(uint32_t h) { mHeight = h; return *this; }
        Builder& levels(uint32_t l) { mLevels = l; return *this; }
        Builder& format(PixelFormat f) { mFormat = f; return *this; }
        Texture* build(Engine& engine);

    private:
        uint32_t mWidth = 0, mHeight = 0, mLevels = 1;
        PixelFormat mFormat = PixelFormat::RGBA8;
    };

    // Creates a Texture that wraps a platform-native external image (e.g.
    // AHardwareBuffer from a camera frame) with zero copy. See
    // avm::CameraStreamManager and ARCHITECTURE.md section 7.
    static Texture* createExternal(Engine& engine, void* nativeImage);

    // `rowStrideBytes` == 0 means `data` is tightly packed. Pass the real
    // row stride when uploading directly from a camera buffer (e.g. NV12
    // frames with row-aligned padding) — see avm::CameraStreamManager.
    void setImage(Engine& engine, const void* data, uint32_t sizeBytes, uint32_t rowStrideBytes = 0);

    // Internal use only (renderer/, avm/): see Camera::Impl comment above.
    struct Impl;
    Impl* getImpl() const { return mImpl; }

private:
    friend class Engine;
    Texture() = default;
    Impl* mImpl = nullptr;
};

} // namespace rel
