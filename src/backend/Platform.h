#pragma once
#include <cstdint>

// Platform abstracts window-system/context creation so that GLDriver (and,
// later, VulkanDriver) do not contain any EGL/ANativeWindow/AHardwareBuffer
// specific code directly. Concrete implementations live under
// src/platform/<os>/ (e.g. PlatformEGLAndroid for GLES-on-Android).

namespace rel::backend {

class Platform {
public:
    virtual ~Platform() = default;

    virtual bool createContext(void* nativeWindow) = 0;
    virtual void destroyContext() = 0;
    virtual void makeCurrent() = 0;
    virtual void swapBuffers() = 0;

    // Imports a platform-native image (e.g. AHardwareBuffer wrapped as
    // EGLImageKHR) so the backend can bind it as an external texture.
    // Returns an opaque backend-defined handle; ownership stays with the
    // caller (typically avm::CameraStreamManager).
    virtual void* importExternalImage(void* nativeBuffer) = 0;
    virtual void releaseExternalImage(void* importedImage) = 0;
};

} // namespace rel::backend
