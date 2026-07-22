# android/（M0 Demo 工程）

最小可跑通的 Gradle 工程骨架（`ARCHITECTURE.md` 里程碑 M0）：`MainActivity`
持有一个全屏 `SurfaceView`，`surfaceCreated` 时把 `Surface` 传给 JNI 层创建
`rel::Engine`，用 `Choreographer.FrameCallback` 驱动每帧渲染。

```
android/
├── settings.gradle.kts / build.gradle.kts / gradle.properties
└── app/
    ├── build.gradle.kts          # externalNativeBuild -> src/main/cpp/CMakeLists.txt
    ├── src/main/AndroidManifest.xml
    ├── src/main/res/values/themes.xml
    ├── src/main/java/com/rel/avmdemo/
    │   ├── MainActivity.kt        # SurfaceView + Choreographer 驱动渲染
    │   └── NativeEngine.kt        # JNI 外部方法声明
    └── src/main/cpp/
        ├── CMakeLists.txt         # add_subdirectory(仓库根 CMakeLists.txt)
        └── jni_bridge.cpp         # Surface -> rel::Engine::create(...) 等胶水代码
```

## 用 Android Studio 打开

直接用 Android Studio 打开 `android/` 目录（不是仓库根目录）。
`app/src/main/cpp/CMakeLists.txt` 会通过 `add_subdirectory` 引用仓库根的
`CMakeLists.txt`，所以 `renderengine_lite` 库代码的唯一真源仍然是仓库根，
`android/` 里不重复维护源码列表。

## 当前状态 / 已知限制

- `jni_bridge.cpp` 只创建了一个空 `Scene` + 俯视 `Camera`，还没有接入
  `avm::CameraStreamManager`/`BowlMeshGenerator`（见 M2/M3 里程碑），所以
  当前跑起来是黑屏/清屏，不是完整 AVM 画面。
- `minSdk = 26`：`AHardwareBuffer` + `EGL_ANDROID_get_native_client_buffer`
  零拷贝相机导入路径要求的最低版本（见 `ARCHITECTURE.md` 第 7 节）。
- `abiFilters` 只保留 `arm64-v8a`：车机场景不需要兼容 32 位/x86 模拟器，
  符合"轻量化"目标；本地模拟器调试可临时加回 `x86_64`。
- 渲染当前跑在 UI 线程（`Choreographer` 回调），真机接入相机流后应评估是否
  需要挪到独立渲染线程（`ARCHITECTURE.md` 第 8 节线程模型）。
