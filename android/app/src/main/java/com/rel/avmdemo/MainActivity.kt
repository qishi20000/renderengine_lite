package com.rel.avmdemo

import android.app.Activity
import android.os.Bundle
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * M0-scope demo host: creates a fullscreen SurfaceView, hands its Surface to
 * `rel::Engine` on creation, and drives rendering off Choreographer vsync
 * (see ARCHITECTURE.md section 8 threading model — this IS the render
 * thread's "vsync -> Engine::tick() -> Renderer::render()" entry point,
 * currently just running on the UI thread for simplicity; move to a
 * dedicated thread once real camera input adds contention).
 */
class MainActivity : Activity(), SurfaceHolder.Callback, Choreographer.FrameCallback {

    private var engineHandle: Long = 0
    private var running = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val surfaceView = SurfaceView(this)
        surfaceView.holder.addCallback(this)
        setContentView(surfaceView)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        engineHandle = NativeEngine.nativeCreate(holder.surface)
        if (engineHandle == 0L) {
            // TODO(M0): surface a real error UI; for the scaffold this is
            // reachable if GLDriver/EGL setup fails on-device.
            return
        }
        running = true
        Choreographer.getInstance().postFrameCallback(this)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (engineHandle != 0L) {
            NativeEngine.nativeResize(engineHandle, width, height)
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        running = false
        if (engineHandle != 0L) {
            NativeEngine.nativeDestroy(engineHandle)
            engineHandle = 0
        }
    }

    override fun doFrame(frameTimeNanos: Long) {
        if (!running || engineHandle == 0L) return
        NativeEngine.nativeOnFrame(engineHandle, frameTimeNanos)
        Choreographer.getInstance().postFrameCallback(this)
    }
}
