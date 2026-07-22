package com.rel.avmdemo

import android.view.Surface

/**
 * Thin JNI wrapper around `rel::Engine` (see src/platform/android's future
 * JNI bridge, `android/app/src/main/cpp/jni_bridge.cpp`). Kept as a plain
 * object with opaque native handles rather than one JNI call per REL API
 * method — the Kotlin/Java side of an AVM demo app only ever needs
 * create/resize/frame/destroy, everything else (scene setup, camera wiring)
 * happens natively in jni_bridge.cpp / avm:: code.
 */
object NativeEngine {
    init {
        System.loadLibrary("rel_jni_bridge")
    }

    /** Returns an opaque native engine handle, or 0 on failure. */
    external fun nativeCreate(surface: Surface): Long

    external fun nativeResize(handle: Long, width: Int, height: Int)

    /** Drives one frame; called from a Choreographer.FrameCallback. */
    external fun nativeOnFrame(handle: Long, frameTimeNanos: Long)

    /**
     * Feeds one NV12 camera frame into `avm::CameraStreamManager` from a
     * plain native memory address (see ARCHITECTURE.md section 6: the video
     * input contract is 4 NV12 frames at CPU addresses, not
     * AHardwareBuffer/EGLImage). [dataPtr] must point at Y-plane-then-UV-
     * plane NV12 data and stay valid for the duration of this call - e.g. a
     * direct ByteBuffer obtained from your camera source, whose address you
     * pass via `directByteBuffer`'s native pointer (see
     * android.os.SharedMemory / your camera SDK's native buffer API).
     *
     * [strideY]/[strideUV] are row strides in bytes; pass 0 for tightly
     * packed (stride == width) buffers.
     */
    external fun nativeOnCameraFrame(
        handle: Long,
        cameraSlot: Int,
        dataPtr: Long,
        width: Int,
        height: Int,
        strideY: Int,
        strideUV: Int
    )

    external fun nativeDestroy(handle: Long)
}
