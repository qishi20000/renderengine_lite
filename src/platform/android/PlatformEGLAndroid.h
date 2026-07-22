#pragma once
#include "backend/Platform.h"

struct ANativeWindow;
typedef void* EGLDisplay;
typedef void* EGLSurface;
typedef void* EGLContext;
typedef void* EGLConfig;

namespace rel::backend::android {

// EGL + AHardwareBuffer glue for OpenGL ES on Android/Adreno. This is the
// only Platform implementation in this milestone; a hypothetical future
// PlatformVkAndroid would implement the same rel::backend::Platform
// interface for a VulkanDriver.
class PlatformEGLAndroid final : public Platform {
public:
    PlatformEGLAndroid();
    ~PlatformEGLAndroid() override;

    bool createContext(void* nativeWindow) override; // nativeWindow: ANativeWindow*
    void destroyContext() override;
    void makeCurrent() override;
    void swapBuffers() override;

    // nativeBuffer: AHardwareBuffer*. Returns an EGLImageKHR (as void*)
    // suitable for glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, ...).
    void* importExternalImage(void* nativeBuffer) override;
    void releaseExternalImage(void* importedImage) override;

private:
    EGLDisplay mDisplay = nullptr;
    EGLSurface mSurface = nullptr;
    EGLContext mContext = nullptr;
    EGLConfig mConfig = nullptr;
};

} // namespace rel::backend::android
