#pragma once
#include <cstdint>
#include <vector>

#include "backend/Handles.h"

namespace rel { class Engine; }

namespace rel::avm {

// Owns the N camera input streams and their zero-copy import into
// GL_TEXTURE_EXTERNAL_OES textures (via AHardwareBuffer -> EGLImageKHR, see
// ARCHITECTURE.md sections 6 and 7). Runs on/is fed by the camera callback
// thread; the render thread only reads the latest-ready handle per camera
// slot — no per-frame allocation, no CPU-side pixel copies.
class CameraStreamManager {
public:
    explicit CameraStreamManager(Engine& engine, uint32_t cameraCount);
    ~CameraStreamManager();

    // Called from the camera HAL/Camera2 callback thread when a new frame
    // buffer is available for `cameraSlot`. `hardwareBuffer` is an
    // AHardwareBuffer* (opaque here to stay NDK-header-free in this
    // interface file).
    void onFrameAvailable(uint32_t cameraSlot, void* hardwareBuffer);

    // Called from the render thread right before SurroundViewPass executes.
    // Returns the currently-bindable external texture handle for a camera
    // slot (double-buffered internally so the render thread never blocks on
    // the camera thread).
    backend::TextureHandle getCameraTexture(uint32_t cameraSlot) const;

    uint32_t getCameraCount() const { return static_cast<uint32_t>(mCameraCount); }

private:
    Engine& mEngine;
    uint32_t mCameraCount;
    // TODO(M3): per-slot double-buffered {EGLImageKHR, TextureHandle} state
    // + a lightweight mutex or atomic swap for camera-thread/render-thread
    // handoff.
};

} // namespace rel::avm
