// JNI glue between NativeEngine.kt and rel::Engine. Deliberately thin: all
// real engine/scene/AVM logic lives in the platform-independent
// renderengine_lite library (src/engine, src/renderer, src/avm), not here.
// See ARCHITECTURE.md sections 8 and 11 (M0 milestone).

#include <jni.h>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <memory>

#include "avm/CameraStreamManager.h"
#include "rel/Camera.h"
#include "rel/Engine.h"
#include "rel/Renderer.h"
#include "rel/Scene.h"
#include "rel/SwapChain.h"
#include "rel/View.h"

namespace {
constexpr uint32_t kCameraCount = 4; // AVM rig: front/rear/left/right, see ARCHITECTURE.md sec.6
}

#define REL_TAG "RelJniBridge"

namespace {

// Bundles the handful of REL objects this M0 demo needs so a single opaque
// jlong handle can be threaded through the Kotlin side. A real AVM app
// would grow this into (or replace it with) avm::CameraStreamManager +
// BowlMeshGenerator-produced state.
struct NativeEngineHolder {
    std::unique_ptr<rel::Engine> engine;
    rel::Scene* scene = nullptr;
    rel::View* view = nullptr;
    rel::Camera* camera = nullptr;
    // Video input contract: 4 NV12 frames at plain CPU addresses (see
    // ARCHITECTURE.md sec.6 and avm::CameraStreamManager) — the Kotlin
    // side hands frames in via nativeOnCameraFrame() below.
    std::unique_ptr<rel::avm::CameraStreamManager> cameraStreamManager;
};

} // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_com_rel_avmdemo_NativeEngine_nativeCreate(JNIEnv* env, jobject /*thiz*/, jobject surface) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        __android_log_print(ANDROID_LOG_ERROR, REL_TAG, "ANativeWindow_fromSurface failed");
        return 0;
    }

    auto holder = std::make_unique<NativeEngineHolder>();
    holder->engine = rel::Engine::create(window);
    // EGL (via PlatformEGLAndroid::createContext) retains its own reference
    // to the window as needed; release our transient reference here.
    ANativeWindow_release(window);

    if (!holder->engine) {
        __android_log_print(ANDROID_LOG_ERROR, REL_TAG, "rel::Engine::create failed");
        return 0;
    }

    // Minimal scene bring-up: empty Scene + a top-down ortho Camera. Real
    // content (bowl mesh, camera textures) is added once
    // avm::BowlMeshGenerator / CameraStreamManager land (M2/M3).
    holder->scene = holder->engine->createScene();
    holder->view = holder->engine->createView();
    rel::Entity cameraEntity = holder->engine->createEntity();
    holder->camera = holder->engine->createCamera(cameraEntity);
    holder->camera->setProjectionOrtho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    holder->camera->lookAt(0, 10, 0, /*at*/ 0, 0, 0, /*up*/ 0, 0, -1);
    holder->view->setScene(holder->scene);
    holder->view->setCamera(holder->camera);

    holder->cameraStreamManager =
            std::make_unique<rel::avm::CameraStreamManager>(*holder->engine, kCameraCount);

    return reinterpret_cast<jlong>(holder.release());
}

// Feeds one NV12 camera frame in from plain CPU memory. `dataPtr` must
// remain valid (not overwritten by the camera source) for the duration of
// this call — see ARCHITECTURE.md section 8's threading-model caveat about
// the CPU-NV12 input contract not being fence/handle-synchronized like a
// zero-copy path would be.
extern "C" JNIEXPORT void JNICALL
Java_com_rel_avmdemo_NativeEngine_nativeOnCameraFrame(
        JNIEnv*, jobject, jlong handle, jint cameraSlot, jlong dataPtr,
        jint width, jint height, jint strideY, jint strideUV) {
    auto* holder = reinterpret_cast<NativeEngineHolder*>(handle);
    if (!holder || !holder->cameraStreamManager || dataPtr == 0) return;

    rel::avm::NV12FrameDescriptor frame;
    frame.data = reinterpret_cast<const uint8_t*>(static_cast<uintptr_t>(dataPtr));
    frame.width = static_cast<uint32_t>(width);
    frame.height = static_cast<uint32_t>(height);
    frame.strideY = static_cast<uint32_t>(strideY);
    frame.strideUV = static_cast<uint32_t>(strideUV);

    holder->cameraStreamManager->onFrameAvailable(static_cast<uint32_t>(cameraSlot), frame);
}

extern "C" JNIEXPORT void JNICALL
Java_com_rel_avmdemo_NativeEngine_nativeResize(JNIEnv*, jobject, jlong handle, jint width, jint height) {
    auto* holder = reinterpret_cast<NativeEngineHolder*>(handle);
    if (!holder || !holder->view) return;
    holder->view->setViewport(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

extern "C" JNIEXPORT void JNICALL
Java_com_rel_avmdemo_NativeEngine_nativeOnFrame(JNIEnv*, jobject, jlong handle, jlong /*frameTimeNanos*/) {
    auto* holder = reinterpret_cast<NativeEngineHolder*>(handle);
    if (!holder || !holder->view) return;

    rel::Renderer* renderer = holder->engine->getRenderer();
    renderer->beginFrame(holder->engine->getSwapChain());
    renderer->render(*holder->view);
    renderer->endFrame();
}

extern "C" JNIEXPORT void JNICALL
Java_com_rel_avmdemo_NativeEngine_nativeDestroy(JNIEnv*, jobject, jlong handle) {
    delete reinterpret_cast<NativeEngineHolder*>(handle);
}
