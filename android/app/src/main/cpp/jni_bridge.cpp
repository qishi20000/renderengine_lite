// JNI glue between NativeEngine.kt and rel::Engine. Deliberately thin: all
// real engine/scene/AVM logic lives in the platform-independent
// renderengine_lite library (src/engine, src/renderer, src/avm), not here.
// See ARCHITECTURE.md sections 8 and 11 (M0 milestone).

#include <jni.h>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <cmath>
#include <cstddef>
#include <memory>

#include "avm/CameraStreamManager.h"
#include "rel/Camera.h"
#include "rel/Engine.h"
#include "rel/IndexBuffer.h"
#include "rel/Material.h"
#include "rel/MaterialInstance.h"
#include "rel/Renderable.h"
#include "rel/Renderer.h"
#include "rel/Scene.h"
#include "rel/SwapChain.h"
#include "rel/VertexBuffer.h"
#include "rel/View.h"

namespace {
constexpr uint32_t kCameraCount = 4; // AVM rig: front/rear/left/right, see ARCHITECTURE.md sec.6

// --- "rotating triangle" bring-up sample -----------------------------------
// A throwaway unlit/vertex-color material + single triangle mesh used to
// validate the Engine/Scene/Renderer/GLDriver pipeline end-to-end on a real
// device before any AVM-specific content (bowl mesh, camera textures,
// calibration) exists. Kept local to the JNI sample rather than under
// src/materials/ since it is not meant to become real product content —
// see ARCHITECTURE.md milestones for the M2/M3 replacement plan.
constexpr char kTriangleVert[] = R"GLSL(#version 300 es
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_viewProj;

out vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = u_viewProj * u_model * vec4(a_position, 1.0);
}
)GLSL";

constexpr char kTriangleFrag[] = R"GLSL(#version 300 es
precision mediump float;

in vec4 v_color;
out vec4 fragColor;

void main() {
    fragColor = v_color;
}
)GLSL";

struct TriangleVertex {
    float x, y, z;
    float r, g, b, a;
};

constexpr TriangleVertex kTriangleVertices[3] = {
        {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},   // top: red
        {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f}, // bottom-left: green
        {0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},  // bottom-right: blue
};
constexpr uint32_t kTriangleIndices[3] = {0, 1, 2};

// Full rotation every 3 seconds.
constexpr double kRotationRadiansPerNanosecond = (2.0 * M_PI) / 3.0e9;

} // namespace

#define REL_TAG "RelJniBridge"

namespace {

// Bundles the handful of REL objects this demo needs so a single opaque
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

    // "Rotating triangle" bring-up sample state, see comment block above.
    rel::Entity triangleEntity{};
    jlong firstFrameTimeNanos = 0;
    float aspect = 1.0f;
};

void updateTriangleCameraProjection(NativeEngineHolder& holder) {
    // Symmetric ortho volume adjusted for aspect ratio so the triangle
    // isn't stretched — see rel::Camera::setProjectionOrtho.
    const float a = holder.aspect;
    if (a >= 1.0f) {
        holder.camera->setProjectionOrtho(-a, a, -1.0f, 1.0f, -1.0f, 10.0f);
    } else {
        holder.camera->setProjectionOrtho(-1.0f, 1.0f, -1.0f / a, 1.0f / a, -1.0f, 10.0f);
    }
}

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

    // Scene bring-up: a Scene + camera looking straight on at the origin,
    // plus a single rotating triangle (see comment block above) so there is
    // something visible on screen to confirm the pipeline actually works
    // end-to-end. Real AVM content (bowl mesh, camera textures) replaces
    // this once avm::BowlMeshGenerator / CameraStreamManager land (M2/M3).
    holder->scene = holder->engine->createScene();
    holder->view = holder->engine->createView();
    rel::Entity cameraEntity = holder->engine->createEntity();
    holder->camera = holder->engine->createCamera(cameraEntity);
    updateTriangleCameraProjection(*holder);
    holder->camera->lookAt(0, 0, 3, /*at*/ 0, 0, 0, /*up*/ 0, 1, 0);
    holder->view->setScene(holder->scene);
    holder->view->setCamera(holder->camera);

    rel::VertexBuffer* triangleVb = rel::VertexBuffer::Builder()
            .vertexCount(3)
            .bufferSizeBytes(sizeof(kTriangleVertices))
            .usage(rel::BufferUsage::Static)
            .build(*holder->engine);
    triangleVb->setBufferData(*holder->engine, kTriangleVertices, sizeof(kTriangleVertices));

    rel::IndexBuffer* triangleIb = rel::IndexBuffer::Builder()
            .indexCount(3)
            .usage(rel::BufferUsage::Static)
            .build(*holder->engine);
    triangleIb->setBuffer(*holder->engine, kTriangleIndices, sizeof(kTriangleIndices));

    rel::Material* triangleMaterial = rel::Material::Builder()
            .vertexShader(kTriangleVert)
            .fragmentShader(kTriangleFrag)
            .build(*holder->engine);
    rel::MaterialInstance* triangleMaterialInstance = triangleMaterial->createInstance();

    rel::VertexAttributeDesc positionAttr{};
    positionAttr.type = rel::ElementType::Float3;
    positionAttr.offset = static_cast<uint32_t>(offsetof(TriangleVertex, x));
    positionAttr.stride = static_cast<uint32_t>(sizeof(TriangleVertex));
    positionAttr.slot = 0;

    rel::VertexAttributeDesc colorAttr{};
    colorAttr.type = rel::ElementType::Float4;
    colorAttr.offset = static_cast<uint32_t>(offsetof(TriangleVertex, r));
    colorAttr.stride = static_cast<uint32_t>(sizeof(TriangleVertex));
    colorAttr.slot = 1;

    holder->triangleEntity = holder->engine->createEntity();
    rel::RenderableBuilder()
            .geometry(triangleVb, triangleIb, rel::PrimitiveType::Triangles, 3)
            .attribute(positionAttr)
            .attribute(colorAttr)
            .material(triangleMaterialInstance)
            .build(*holder->engine, holder->triangleEntity);

    static constexpr float kIdentity[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
    };
    holder->engine->setTransform(holder->triangleEntity, kIdentity);
    holder->scene->addEntity(holder->triangleEntity);

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

    if (width > 0 && height > 0) {
        holder->aspect = static_cast<float>(width) / static_cast<float>(height);
        updateTriangleCameraProjection(*holder);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_rel_avmdemo_NativeEngine_nativeOnFrame(JNIEnv*, jobject, jlong handle, jlong frameTimeNanos) {
    auto* holder = reinterpret_cast<NativeEngineHolder*>(handle);
    if (!holder || !holder->view) return;

    // Advance the bring-up triangle's rotation (see comment block near the
    // top of this file). Uses the vsync-driven frameTimeNanos Choreographer
    // hands in, rather than wall-clock time, so animation stays in lockstep
    // with what actually gets presented.
    if (holder->firstFrameTimeNanos == 0) {
        holder->firstFrameTimeNanos = frameTimeNanos;
    }
    const double elapsedNanos = static_cast<double>(frameTimeNanos - holder->firstFrameTimeNanos);
    const double angle = elapsedNanos * kRotationRadiansPerNanosecond;
    const float c = static_cast<float>(std::cos(angle));
    const float s = static_cast<float>(std::sin(angle));
    const float rotationZ[16] = {
            c, s, 0, 0,
            -s, c, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
    };
    holder->engine->setTransform(holder->triangleEntity, rotationZ);

    rel::Renderer* renderer = holder->engine->getRenderer();
    renderer->beginFrame(holder->engine->getSwapChain());
    renderer->render(*holder->view);
    renderer->endFrame();
}

extern "C" JNIEXPORT void JNICALL
Java_com_rel_avmdemo_NativeEngine_nativeDestroy(JNIEnv*, jobject, jlong handle) {
    delete reinterpret_cast<NativeEngineHolder*>(handle);
}
