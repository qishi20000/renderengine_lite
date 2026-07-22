# android/（Gradle 工程占位）

预留给：

- `app/`：Demo App（Camera2/CameraX 采集 4~6 路鱼眼相机 + `SurfaceView`/
  `TextureView` 承载渲染输出）
- `app/src/main/cpp/`：JNI 胶水层，桥接 `rel::Engine`（生命周期对齐
  `Activity`/`SurfaceHolder` 回调：`surfaceCreated`→`Engine::create`，
  `surfaceDestroyed`→销毁，`Choreographer.FrameCallback`→驱动
  `Renderer::render`）
- `app/src/main/cpp/CMakeLists.txt`：`add_subdirectory` 指向仓库根
  `CMakeLists.txt`，或直接被根 `CMakeLists.txt` 的 externalNativeBuild 引用

尚未搭建具体 Gradle 文件，属于 M0 里程碑范围（见 `ARCHITECTURE.md` 第 11 节）。
