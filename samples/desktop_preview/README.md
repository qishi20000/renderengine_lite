# 桌面预览（可选）

用于在不装 APK / 不接真实车机相机的情况下快速迭代 `SurroundViewPass` 等渲染
逻辑：用离线录制的相机帧（或棋盘格/合成测试图）替代真实 `AHardwareBuffer`，
在桌面上跑通 GLES 后端。

由于 REL 的 GLES 后端直接使用 `GLES3/gl3.h` + Android 特有的
`GL_OES_EGL_image_external`/`AHardwareBuffer` 扩展，桌面上需要一套 GLES
模拟层，推荐用 [ANGLE](https://chromium.googlesource.com/angle/angle)：

```bash
cmake -B build-desktop -DREL_GLES_DESKTOP_ANGLE_DIR=/path/to/angle/out/Release .
cmake --build build-desktop
```

未指定 `REL_GLES_DESKTOP_ANGLE_DIR` 时，顶层 `CMakeLists.txt` 不会编译
`src/backend/opengl/`，桌面构建仅用于对 `engine/`、`renderer/`、`avm/` 等
API-无关代码做语法/接口层面的快速迭代与单元测试。

TODO（未纳入本次骨架搭建范围）：
- `main.cpp`：创建离屏窗口 + `PlatformEGLDesktop`（实现 `backend::Platform`,
  参考 `src/platform/android/PlatformEGLAndroid.*` 的结构）
- 用静态图片模拟相机输入（跳过 `AHardwareBuffer`，直接 `glTexSubImage2D`
  上传，走 `SurroundViewPass` 里 `u_cameraTex` 采样的是普通 `sampler2D` 而非
  `samplerExternalOES` 的桌面 shader 变体）
