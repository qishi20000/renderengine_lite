#include "avm/CameraStreamManager.h"

#include <string>

#include "rel/Engine.h"
#include "rel/MaterialInstance.h"

namespace rel::avm {

CameraStreamManager::CameraStreamManager(Engine& engine, uint32_t cameraCount)
    : mEngine(engine), mCameras(cameraCount) {}

CameraStreamManager::~CameraStreamManager() {
    for (auto& slot : mCameras) {
        if (slot.yTexture) mEngine.destroy(slot.yTexture);
        if (slot.uvTexture) mEngine.destroy(slot.uvTexture);
    }
}

void CameraStreamManager::ensureTextures(CameraSlot& slot, uint32_t width, uint32_t height) {
    if (slot.yTexture && slot.width == width && slot.height == height) {
        return; // already sized correctly, nothing to do
    }

    // Resolution changed (or first frame): (re)create both planes. This is
    // expected to happen at most once per camera in a real deployment
    // (fixed sensor resolution) — not a per-frame cost.
    if (slot.yTexture) mEngine.destroy(slot.yTexture);
    if (slot.uvTexture) mEngine.destroy(slot.uvTexture);

    slot.yTexture = Texture::Builder()
                            .width(width)
                            .height(height)
                            .format(PixelFormat::R8)
                            .build(mEngine);
    // NV12 chroma plane is subsampled 2x2 (4:2:0): half width, half height,
    // two interleaved 8-bit channels (U, V) -> RG8.
    slot.uvTexture = Texture::Builder()
                             .width(width / 2)
                             .height(height / 2)
                             .format(PixelFormat::RG8)
                             .build(mEngine);
    slot.width = width;
    slot.height = height;
}

void CameraStreamManager::onFrameAvailable(uint32_t cameraSlot, const NV12FrameDescriptor& frame) {
    if (cameraSlot >= mCameras.size() || !frame.data || frame.width == 0 || frame.height == 0) {
        return;
    }

    CameraSlot& slot = mCameras[cameraSlot];
    ensureTextures(slot, frame.width, frame.height);

    const uint32_t strideY = frame.strideY != 0 ? frame.strideY : frame.width;
    const uint32_t strideUV = frame.strideUV != 0 ? frame.strideUV : frame.width;

    // Standard NV12 layout: Y plane (height rows of strideY bytes), then
    // immediately the interleaved UV plane (height/2 rows of strideUV
    // bytes) in the same buffer.
    const uint8_t* yPlane = frame.data;
    const uint8_t* uvPlane = frame.data + static_cast<size_t>(strideY) * frame.height;

    slot.yTexture->setImage(mEngine, yPlane, strideY * frame.height, strideY);
    slot.uvTexture->setImage(mEngine, uvPlane, strideUV * (frame.height / 2), strideUV);
}

Texture* CameraStreamManager::getYTexture(uint32_t cameraSlot) const {
    return cameraSlot < mCameras.size() ? mCameras[cameraSlot].yTexture : nullptr;
}

Texture* CameraStreamManager::getUVTexture(uint32_t cameraSlot) const {
    return cameraSlot < mCameras.size() ? mCameras[cameraSlot].uvTexture : nullptr;
}

void CameraStreamManager::bindToMaterialInstance(MaterialInstance& materialInstance) const {
    for (uint32_t i = 0; i < mCameras.size(); ++i) {
        const CameraSlot& slot = mCameras[i];
        if (!slot.yTexture || !slot.uvTexture) continue; // no frame delivered yet for this slot
        materialInstance.setParameter("u_cameraY[" + std::to_string(i) + "]", slot.yTexture);
        materialInstance.setParameter("u_cameraUV[" + std::to_string(i) + "]", slot.uvTexture);
    }
}

} // namespace rel::avm
