#pragma once
#include <cstdint>
#include <vector>

#include "rel/Texture.h"

namespace rel {
class Engine;
class MaterialInstance;
} // namespace rel

namespace rel::avm {

// Describes one incoming NV12 camera frame living in plain CPU memory:
// a single base pointer to the Y plane, with the interleaved U/V plane
// immediately following it in the same buffer (standard NV12 layout — Y
// plane first, then a half-height/half-width interleaved UV plane, U
// before V). This matches how most Android camera pipelines (and most
// automotive AVM SerDes/ISP chains) hand off frames when zero-copy
// AHardwareBuffer import isn't available or isn't used.
//
// Row strides default to the logical width when 0 (tightly packed); pass
// the real value when the source buffer pads rows to an alignment (common
// with ISP/VPU output, e.g. 16/32/64-byte row alignment).
struct NV12FrameDescriptor {
    const uint8_t* data = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t strideY = 0;  // bytes per row of the Y plane; 0 => width
    uint32_t strideUV = 0; // bytes per row of the UV plane; 0 => width (UV plane row is width/2 pixels * 2 bytes/pixel = width bytes)
};

// Owns the N camera input textures and uploads each incoming NV12 CPU
// frame into a pair of GLES textures per camera: a full-resolution R8
// "Y" texture and a half-resolution RG8 "UV" texture (NV12's interleaved
// U/V plane maps directly onto RG8, one byte per channel). NV12->RGB
// conversion happens in materials/surround_view.frag, not here — this
// class only owns upload/lifetime of the textures.
//
// See ARCHITECTURE.md section 6: input is defined as N NV12 frames at CPU
// addresses (not AHardwareBuffer/EGLImage), so there is a real per-frame
// CPU->GPU upload cost here (2 glTexSubImage2D calls per camera per frame);
// this is unavoidable given the input contract, but is still far cheaper
// than a CPU-side NV12->RGB conversion followed by an RGBA upload.
class CameraStreamManager {
public:
    CameraStreamManager(Engine& engine, uint32_t cameraCount);
    ~CameraStreamManager();

    CameraStreamManager(const CameraStreamManager&) = delete;
    CameraStreamManager& operator=(const CameraStreamManager&) = delete;

    // Uploads a new NV12 frame for `cameraSlot` (0-based, < getCameraCount()).
    // Lazily (re)creates the camera's Y/UV textures on the first call, or
    // whenever the incoming frame's resolution changes. Safe to call once
    // per camera per render frame from the render thread; if the camera
    // source runs on its own thread, synchronize before calling this (see
    // ARCHITECTURE.md section 8 — REL does not do that synchronization for
    // you).
    void onFrameAvailable(uint32_t cameraSlot, const NV12FrameDescriptor& frame);

    // Bound by SurroundViewPass into the surround_view material instance as
    // "u_cameraY[cameraSlot]" / "u_cameraUV[cameraSlot]". Returns nullptr
    // until the first onFrameAvailable() call for that slot.
    Texture* getYTexture(uint32_t cameraSlot) const;
    Texture* getUVTexture(uint32_t cameraSlot) const;

    uint32_t getCameraCount() const { return static_cast<uint32_t>(mCameras.size()); }

    // Convenience: sets "u_cameraY[i]"/"u_cameraUV[i]" (matching
    // materials/surround_view.frag's sampler arrays) on `materialInstance`
    // for every camera slot that currently has a texture. Call once after
    // all cameras have delivered at least one frame (or every frame if
    // textures get recreated on resolution change — cheap either way,
    // it's just staging parameter values, not a GPU call).
    void bindToMaterialInstance(MaterialInstance& materialInstance) const;

private:
    struct CameraSlot {
        Texture* yTexture = nullptr;
        Texture* uvTexture = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    void ensureTextures(CameraSlot& slot, uint32_t width, uint32_t height);

    Engine& mEngine;
    std::vector<CameraSlot> mCameras;
};

} // namespace rel::avm
