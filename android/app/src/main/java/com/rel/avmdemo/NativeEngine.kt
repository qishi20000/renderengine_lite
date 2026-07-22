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

    external fun nativeDestroy(handle: Long)
}
