#include "platform/android/PlatformEGLAndroid.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/hardware_buffer.h>
#include <android/log.h>
#include <android/native_window.h>

#define REL_LOG_TAG "RenderEngineLite"
#define REL_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, REL_LOG_TAG, __VA_ARGS__)

namespace rel::backend::android {

PlatformEGLAndroid::PlatformEGLAndroid() = default;
PlatformEGLAndroid::~PlatformEGLAndroid() { destroyContext(); }

bool PlatformEGLAndroid::createContext(void* nativeWindow) {
    auto* window = static_cast<ANativeWindow*>(nativeWindow);

    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        REL_LOGE("eglGetDisplay failed");
        return false;
    }
    if (!eglInitialize(static_cast<EGLDisplay>(mDisplay), nullptr, nullptr)) {
        REL_LOGE("eglInitialize failed");
        return false;
    }

    // Request GLES3, 8-bit RGBA, no MSAA (AVM output is a full-screen quad-
    // ish surface; prefer spending the bandwidth budget elsewhere — see
    // ARCHITECTURE.md section 7). Add EGL_DEPTH_SIZE 16 only if the car
    // model / overlay passes need depth testing.
    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE,
    };
    EGLint numConfigs = 0;
    if (!eglChooseConfig(static_cast<EGLDisplay>(mDisplay), configAttribs,
                          reinterpret_cast<EGLConfig*>(&mConfig), 1, &numConfigs) || numConfigs == 0) {
        REL_LOGE("eglChooseConfig failed");
        return false;
    }

    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    mContext = eglCreateContext(static_cast<EGLDisplay>(mDisplay),
                                 static_cast<EGLConfig>(mConfig), EGL_NO_CONTEXT, contextAttribs);
    if (mContext == EGL_NO_CONTEXT) {
        REL_LOGE("eglCreateContext failed");
        return false;
    }

    mSurface = eglCreateWindowSurface(static_cast<EGLDisplay>(mDisplay),
                                       static_cast<EGLConfig>(mConfig), window, nullptr);
    if (mSurface == EGL_NO_SURFACE) {
        REL_LOGE("eglCreateWindowSurface failed");
        return false;
    }

    return true;
}

void PlatformEGLAndroid::destroyContext() {
    if (mDisplay) {
        eglMakeCurrent(static_cast<EGLDisplay>(mDisplay), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (mSurface) eglDestroySurface(static_cast<EGLDisplay>(mDisplay), static_cast<EGLSurface>(mSurface));
        if (mContext) eglDestroyContext(static_cast<EGLDisplay>(mDisplay), static_cast<EGLContext>(mContext));
        eglTerminate(static_cast<EGLDisplay>(mDisplay));
    }
    mSurface = nullptr;
    mContext = nullptr;
    mDisplay = nullptr;
}

void PlatformEGLAndroid::makeCurrent() {
    eglMakeCurrent(static_cast<EGLDisplay>(mDisplay), static_cast<EGLSurface>(mSurface),
                    static_cast<EGLSurface>(mSurface), static_cast<EGLContext>(mContext));
}

void PlatformEGLAndroid::swapBuffers() {
    eglSwapBuffers(static_cast<EGLDisplay>(mDisplay), static_cast<EGLSurface>(mSurface));
}

void* PlatformEGLAndroid::importExternalImage(void* nativeBuffer) {
    // Zero-copy camera import path (ARCHITECTURE.md sections 6/7):
    //   AHardwareBuffer -> EGLClientBuffer -> EGLImageKHR
    // The resulting EGLImageKHR is later bound via
    // glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image) in
    // GLDriver::importExternalTexture (TODO(M3)).
    auto* hardwareBuffer = static_cast<AHardwareBuffer*>(nativeBuffer);
    EGLClientBuffer clientBuffer = eglGetNativeClientBufferANDROID(hardwareBuffer);
    if (!clientBuffer) {
        REL_LOGE("eglGetNativeClientBufferANDROID failed");
        return nullptr;
    }
    const EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    EGLImageKHR image = eglCreateImageKHR(static_cast<EGLDisplay>(mDisplay), EGL_NO_CONTEXT,
                                           EGL_NATIVE_BUFFER_ANDROID, clientBuffer, attribs);
    if (image == EGL_NO_IMAGE_KHR) {
        REL_LOGE("eglCreateImageKHR failed");
        return nullptr;
    }
    return image;
}

void PlatformEGLAndroid::releaseExternalImage(void* importedImage) {
    if (importedImage) {
        eglDestroyImageKHR(static_cast<EGLDisplay>(mDisplay), static_cast<EGLImageKHR>(importedImage));
    }
}

} // namespace rel::backend::android
