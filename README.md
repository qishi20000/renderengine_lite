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

本仓库目前是**架构骨架（scaffold）**：核心抽象接口（`backend::DriverApi` /
`backend::Handles` / ECS-lite 管理器 / RenderPass）已落地并通过语法检查，
`backend::opengl::GLDriver` 已实现 M1 阶段的基础资源管理（纹理/顶点缓冲/
Program/FBO/绘制调用）。相机零拷贝导入、多路拼接融合、车辆模型渲染等在
`ARCHITECTURE.md` 里程碑 M3~M5 阶段逐步补齐（代码中标有 `TODO(M3)` 等标记）。

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
android/             # Gradle 工程占位（JNI wrapper + Demo App）
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
