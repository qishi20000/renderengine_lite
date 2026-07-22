# RenderEngineLite (REL)

面向 Android + 高通 Adreno 平台的轻量级、面向场景（scene-oriented）渲染引擎，
设计思想参考 Google Filament，但裁剪掉 FrameGraph、完整 PBR/IBL、完整 ECS 等
对当前场景（车机 AVM 360° 环视）价值不高的重量级能力。

完整架构设计见 [`ARCHITECTURE.md`](./ARCHITECTURE.md)，包含：

- RHI（`backend::DriverApi`）Handle 抽象设计与取舍
- Engine/Scene/View/Renderer 分层
- ECS-lite（EntityManager + Transform/RenderableManager）
- 显式 RenderPass 链（替代 FrameGraph）取舍
- AVM 业务层（相机流/标定/拼接/网格生成）设计
- Adreno/GLES 专项优化清单
- 目录结构与里程碑规划
- 与 Filament 的能力取舍对照表

## 当前状态

- **M1（GLES 基础资源）**：`backend::DriverApi` + `backend::opengl::GLDriver`
  已实现纹理/顶点缓冲/索引缓冲/Program/FBO/绘制调用，通过语法检查。
- **M2（Engine/Scene/Renderer 打通）**：`Engine`/`Scene`/`View`/`Camera`/
  `Material`/`MaterialInstance`/`VertexBuffer`/`IndexBuffer`/`Texture`/
  `Renderer` 均已实现（pimpl 方式，内部通过 `getImpl()` 访问，详见
  `src/engine/EngineImpl.h`、`src/engine/ResourceImpls.h`）。`Renderer::render()`
  用单个 GL render pass 包裹 `SurroundView/CarModel/OverlayUI` 三个逻辑
  `RenderPass`（避免 Adreno 多次 FBO 落地开销，见 `ARCHITECTURE.md` 第 7 节）。
  目前只有 `SurroundViewPass` 真正绘制（`CarModelPass`/`OverlayUIPass` 待
  Scene 支持按层过滤后再启用，代码中标记 `TODO(M5)`）。
- **M0（Android 工程骨架）**：`android/` 下有一个最小 Gradle 工程，
  `MainActivity` + `SurfaceView` + `Choreographer` 驱动 `jni_bridge.cpp`
  创建 `rel::Engine` 并每帧调用 `Renderer::render()`。当前只建了空
  Scene + 俯视 Camera，还没接相机/bowl mesh，跑起来是黑屏/清屏。
- **待补（M3~M6）**：相机 `AHardwareBuffer` 零拷贝导入
  （`GLDriver::importExternalTexture` 当前是 `assert(false)` 占位）、
  `avm::` 业务层的具体实现（目前只有接口声明）、Adreno 专项性能优化。
  所有非 GLES 依赖的代码已用 `clang++ -fsyntax-only` 验证过编译通过；
  GLES 相关代码（`GLDriver.cpp`）已用模拟 GLES3 头文件验证过语法正确，
  尚未在真实 Android/NDK 环境构建过。

## 目录速览

```
include/rel/        # 公开 API（App/JNI 只依赖这里）
src/engine/          # EntityManager / TransformManager / RenderableManager
src/renderer/        # Renderer + 显式 RenderPass 链
src/backend/         # DriverApi/Handles/DriverEnums（API 无关）
src/backend/opengl/  # GLDriver（本期唯一后端实现）
src/backend/vulkan/  # 占位，未实现
src/materials/       # 内置 GLSL ES shader（surround_view/simple_lit/overlay_ui）
src/avm/             # AVM 业务层：相机流/标定/拼接融合/网格生成
src/platform/android/# EGL + AHardwareBuffer 平台适配
samples/desktop_preview/  # 桌面预览（需 ANGLE，见 CMakeLists.txt 说明）
android/             # M0 Demo Gradle 工程（SurfaceView + JNI wrapper，见 android/README.md）
third_party/         # glm 等轻量依赖
```

## 构建

```bash
# Android（推荐用 Android Studio + externalNativeBuild 指向本 CMakeLists.txt）
cmake -B build-android -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
      -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake .
cmake --build build-android

# 桌面（仅编译不含 GLES 后端的核心库，用于快速语法/接口迭代）
cmake -B build .
cmake --build build
```
