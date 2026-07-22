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
- **M0（Android 工程骨架 + 旋转三角形 Sample）**：`android/` 下有一个最小
  Gradle 工程，`MainActivity` + `SurfaceView` + `Choreographer` 驱动
  `jni_bridge.cpp` 创建 `rel::Engine`。新增了 `rel::RenderableBuilder`
  （`include/rel/Renderable.h`）+ `Engine::setTransform()` 两个公开 API，
  用来把「几何 + Material」挂到 Entity 上、以及每帧更新 Transform ——
  之前的骨架里这两块是缺失的，`Scene::addEntity()` 只是把 Entity 塞进
  列表，没有任何办法真正让它渲染出东西。`jni_bridge.cpp` 现在用这两个新
  API 建了一个三顶点、按 `u_model`/`u_viewProj` 绕 Z 轴旋转的纯色三角形
  （3 秒一圈），验证 Engine/Scene/Renderer/GLDriver 整条链路。
  **已用真机 NDK 交叉编译（arm64-v8a, NDK r27, `android.toolchain.cmake`）
  完整验证过编译 + 链接**（此前只做过桌面 `-fsyntax-only` 语法检查，从未
  真正过 NDK），过程中额外修掉了两个此前从未被编译验证出来的 bug：
  `PlatformEGLAndroid.cpp` 里 `eglCreateImageKHR`/`eglGetNativeClientBufferANDROID`/
  `eglDestroyImageKHR` 缺少 `EGL_EGLEXT_PROTOTYPES` 声明；`rel::SwapChain`
  声明了析构函数但从未定义（缺 `src/engine/SwapChain.cpp`），导致链接期
  `undefined symbol`。产物 `librel_jni_bridge.so` 已确认 5 个
  `Java_com_rel_avmdemo_NativeEngine_*` 符号全部导出、未定义符号只剩系统库
  （libEGL/libGLESv3/liblog/libandroid/libc++）。**已在真机（Adreno GPU）
  上跑起来**：APK 能正常启动、EGL/GLES3 上下文创建成功、无崩溃，但最初是
  黑屏——排查后发现是个真正的渲染管线 bug：`GLDriver` 的默认渲染目标
  （代表窗口 surface 的 FBO 0）在构造时被硬编码成 `{width:0, height:0}`
  且从未更新，而 `View::setViewport()`（`nativeResize` 里已经在正确调用）
  设置的视口从来没有真正接到 `glViewport()` 上，于是每帧其实都在执行
  `glViewport(0,0,0,0)`——`glClear` 依然把全屏清成黑色，但三角形被光栅化
  到一个 0×0 的视口里，什么都画不出来。修复方式：给
  `backend::RenderPassDescriptor` 加了 `viewport` 字段，`Renderer::render()`
  现在把 `View::getViewport()`（真实宽高，来自 `nativeResize`）显式传给
  `GLDriver::beginRenderPass()`，不再依赖默认渲染目标自己的（不存在的）
  尺寸信息。修复后已在真机上验证三角形正常显示并旋转。
- Android 工程的 Gradle Wrapper（`android/gradlew` + `gradle/wrapper/`）
  和 `android/app/build.gradle.kts` 里的 `ndkVersion` 已补齐/固定为
  `27.1.12297006`（与本文档里用于交叉编译验证的 NDK 版本一致），避免
  AGP 8.5.2 要求的最低 Gradle 版本（8.7）与本机残留/自动生成的旧 wrapper
  冲突。
- **M3（相机接入，CPU NV12 路径）**：视频输入契约明确为 **4 路 NV12 的 CPU
  地址**（`avm::CameraStreamManager` + `NV12FrameDescriptor`），不依赖
  `AHardwareBuffer`/`EGLImage`。每路相机的 Y 平面上传为 `R8` 纹理、交织 UV
  平面上传为 `RG8` 纹理（`GLDriver` 已补全 `PixelFormat`→GL 枚举映射 +
  `GL_UNPACK_ROW_LENGTH` 处理行对齐 padding），NV12→RGB 转换写在
  `surround_view.frag` 里。`CameraStreamManager::bindToMaterialInstance()`
  把纹理绑定为 `u_cameraY[i]`/`u_cameraUV[i]`；Android Demo 的
  `jni_bridge.cpp` 新增了 `nativeOnCameraFrame()` 入口，Kotlin 侧可以直接传
  原生内存地址喂帧。零拷贝 `AHardwareBuffer` 路径（`importExternalTexture`）
  保留为未来可选项，未启用。
- **待补（M4~M6）**：`avm::BowlMeshGenerator`/`FisheyeCalibration`/
  `StitchBlender` 的具体实现（目前只有接口声明，鱼眼 unwarp 网格还没生成）、
  Adreno 专项性能优化。所有非 GLES 依赖的代码已用
  `clang++ -Wall -Wextra -fsyntax-only` 验证过编译通过；GLES 相关代码
  （`GLDriver.cpp`）已用模拟 GLES3 头文件验证过语法正确，尚未在真实
  Android/NDK 环境构建过。

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
