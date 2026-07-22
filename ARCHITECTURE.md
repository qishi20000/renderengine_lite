# RenderEngineLite (REL) 架构设计文档

> 面向 Android + 高通 Adreno 平台的轻量级、面向场景（scene-oriented）渲染引擎，
> 首要落地场景为车机 AVM（Around View Monitor，360° 环视）。
> 设计思想参考 Google Filament 的分层与抽象方式，但大幅裁剪：**不做 FrameGraph、
> 不做完整 PBR/IBL、不做完整 ECS 框架**，仅保留"面向场景对象 + RHI 硬件抽象"这两个
> 对长期可维护性最有价值的核心思想。

---

## 0. 设计目标与约束

| 维度 | 约束/目标 |
|---|---|
| 平台 | Android，GPU 以高通 Adreno（tile-based deferred renderer）为主 |
| 图形 API | **仅实现 OpenGL ES 3.x 后端**（3.0 起步，尽量用到 3.2/AEP 能力），预留 Vulkan 扩展点但不实现 |
| 场景 | 车机 AVM：4~6 路鱼眼相机输入 → 去畸变 → 拼接融合 → 碗状/平面 3D 模型投影 → 车辆模型叠加 → UI overlay |
| 性能 | 30~60fps 稳定、低延迟（相机到显示 < 2 帧）、低功耗（车机长期运行，发热/续航敏感）|
| 体量 | 轻量化：不需要 FrameGraph、完整材质编译器（matc）、完整 ECS、IBL/PBR 全家桶 |
| 语言 | C++17，尽量少的第三方依赖（glm 数学库 + 平台层）|
| 可扩展性 | 图形后端抽象要保持"Filament DriverApi 风格"，未来可插入 VulkanDriver 而不动上层 |

---

## 1. 整体分层

```
┌─────────────────────────────────────────────────────────┐
│  App / JNI 层 (Android)                                  │
│  - CameraX/Camera HAL 回调、Choreographer vsync 驱动      │
├─────────────────────────────────────────────────────────┤
│  AVM 业务层 (avm/)                                        │
│  - CameraStreamManager / FisheyeCalibration               │
│  - BowlMeshGenerator / StitchBlender                      │
├─────────────────────────────────────────────────────────┤
│  Engine / Scene 层 (engine/)     ← "面向场景"思想核心地带   │
│  - Engine / Scene / View / Camera                          │
│  - EntityManager + TransformManager/RenderableManager      │
├─────────────────────────────────────────────────────────┤
│  Renderer 层 (renderer/)                                  │
│  - Renderer：显式 RenderPass 链（FrameGraph 的轻量替代）    │
├─────────────────────────────────────────────────────────┤
│  Material/Shader 层 (materials/)                           │
│  - Material / MaterialInstance / ShaderVariant 缓存        │
├─────────────────────────────────────────────────────────┤
│  RHI 抽象层 (backend/)           ← 面向未来 Vulkan 的隔离带 │
│  - DriverApi（纯虚接口）/ Handle 资源系统 / DriverEnums     │
│  - backend/opengl/ GLDriver（本期唯一实现）                 │
│  - backend/vulkan/ （占位，未实现）                         │
├─────────────────────────────────────────────────────────┤
│  Platform 层 (platform/android/)                            │
│  - EGL 上下文/Surface、AHardwareBuffer↔EGLImage 零拷贝对接  │
└─────────────────────────────────────────────────────────┘
```

**关键分层原则**：Engine/Scene/Renderer/Material 四层完全不感知具体图形 API，只
通过 `backend::DriverApi` 的 Handle 操作 GPU 资源。这是从 Filament 借鉴的最重要
的一条设计——它保证了"暂时只写 GLES，但未来能长出 Vulkan"这个目标能够成立。

---

## 2. RHI 抽象层：Handle-based 资源系统

### 2.1 为什么用 Handle 而不是裸指针/共享指针

- **解耦生命周期**：上层（Scene/Material）拿到的只是一个轻量 `{index, generation}`
  句柄，真正的 GPU 对象（GLuint 纹理/VBO/Program）生命周期由 backend 内部的
  资源池管理，避免跨模块的裸指针悬挂问题。
- **多后端复用**：`TextureHandle`、`VertexBufferHandle` 等类型与 API 无关，
  GLDriver 内部维护 `HandleAllocator<GLTexture>` 之类的池子做 index→对象 的映射；
  未来 VulkanDriver 只需实现同样的 DriverApi 接口、内部换成 VkImage 等对象，
  上层代码零改动。
- **延迟销毁友好**：Handle 方式天然支持"本帧释放、下一帧再真正 destroy"这种
  GPU 资源延迟回收策略（尤其 Vulkan 需要等 fence，GLES 也可以用来规避
  drop-frame 场景下驱动内部同步开销）。

### 2.2 核心类型（`src/backend/`）

```
DriverEnums.h   // PixelFormat / BufferUsage / PrimitiveType / TextureUsage ...
Handles.h       // TextureHandle / VertexBufferHandle / IndexBufferHandle /
                // ProgramHandle / RenderTargetHandle / SamplerGroupHandle ...
DriverApi.h     // 纯虚接口：createTexture/createVertexBuffer/createProgram/
                // beginRenderPass/draw/commit/destroyXxx ...
opengl/
  GLDriver.h/.cpp        // DriverApi 的 GLES 实现
  GLProgram.h/.cpp       // shader 编译、link、variant 缓存
  GLTexture.h/.cpp       // 含 GL_TEXTURE_EXTERNAL_OES（相机纹理零拷贝）支持
  PlatformEGLAndroid.h/.cpp  // EGL context/surface、AHardwareBuffer→EGLImage
vulkan/                  // 占位目录，接口对齐 DriverApi，暂不实现
```

### 2.3 单线程直接调用模型（与 Filament 的差异点）

Filament 为了跨平台通吞吐量，把"记录命令"和"驱动执行"分到两个线程
（CommandStream + driver thread）。但 AVM 场景对**端到端延迟**极敏感，多一次
线程切换/命令排队都是延迟。因此 REL 采用**单线程直接调用**：Engine 在渲染
线程直接同步调用 `DriverApi`（即 `GLDriver` 本身），不引入 CommandBuffer。

接口签名仍然与"未来加 CommandStream"保持兼容（方法都是无阻塞的、参数是
POD/Handle），如果后续发现多线程录制确有收益，可以在 DriverApi 和 GLDriver
之间插入一层 CommandStream 而不改动上层调用方式。

---

## 3. Engine / Scene 层：ECS-lite

不引入 EnTT 等完整 ECS 框架，原因：AVM 场景中的"场景对象"数量是有限且已知的
（几张网格 + 一个车辆模型 + 若干 UI 元素，通常 < 100 个 Renderable），完整 ECS
带来的 cache-friendly 大规模数据遍历收益不明显，反而增加心智负担。

采用 Filament 式的 "EntityManager + 若干独立 Manager" 模式：

```cpp
// include/rel/Entity.h
struct Entity { uint32_t id = 0; }; // 仅是一个 handle，可复用/回收

// src/engine/EntityManager.h
class EntityManager {
public:
    Entity create();
    void destroy(Entity e);
    bool isAlive(Entity e) const;
private:
    std::vector<uint8_t> mGenerations;
    std::vector<uint32_t> mFreeList;
};

// src/engine/TransformManager.h —— 稀疏存储 Entity -> Transform 数据
class TransformManager {
public:
    using Instance = uint32_t;
    Instance create(Entity e, Instance parent, const mat4f& local);
    void setTransform(Instance i, const mat4f& local);
    mat4f getWorldTransform(Instance i) const;
private:
    struct Node { Entity entity; Instance parent; mat4f local, world; };
    std::vector<Node> mNodes;
    std::unordered_map<Entity, Instance> mEntityToInstance;
};

// src/engine/RenderableManager.h —— 稀疏存储 Entity -> 渲染数据
class RenderableManager {
public:
    struct Builder {
        Builder& geometry(VertexBufferHandle vb, IndexBufferHandle ib,
                           PrimitiveType type, uint32_t count);
        Builder& material(MaterialInstance* mi);
        Builder& boundingBox(const Box& aabb);
        Entity build(Engine& engine);
    };
    // ...
};
```

`Scene` 本身只是一个 `std::vector<Entity>`（外加一个可选的简单空间剔除结构，
比如 AABB 数组用于 frustum cull），`View` 持有 `Scene* + Camera + Viewport +
RenderPass 配置`。这套结构足以覆盖 AVM 场景的复杂度，同时给未来"要不要长出更
复杂的场景管理"留了口子（比如后续要支持雷达点云/多个 3D 场景，可以再加
`LightManager`、`BoundingVolumeHierarchy` 等，而不用推翻现有结构）。

---

## 4. Renderer 层：显式 RenderPass 链（FrameGraph 的轻量替代）

明确不做 FrameGraph（自动依赖分析、资源生命周期自动裁剪、自动 barrier 插入），
原因：AVM 管线的 Pass 结构相对固定，运行时几乎不变化，FrameGraph 带来的"自动
剔除无用 Pass / 自动内存别名复用"收益，在这个体量下不如显式手写划算，还省去
了每帧 build graph 的 CPU 开销（这对功耗敏感的车机很重要）。

```cpp
// src/renderer/RenderPass.h
class RenderPass {
public:
    virtual ~RenderPass() = default;
    virtual void setup(Engine&, View&) {}       // 一次性/惰性资源创建
    virtual void execute(Engine&, View&, backend::DriverApi&) = 0;
};

// src/renderer/Renderer.h
class Renderer {
public:
    void render(View& view);   // 依次 execute() 已注册的 RenderPass
    void addPass(std::unique_ptr<RenderPass> pass);
private:
    std::vector<std::unique_ptr<RenderPass>> mPasses;
};
```

AVM 典型 Pass 序列（可配置增减）：

1. `CameraTextureSyncPass` —— 等待/绑定各路相机 `GL_TEXTURE_EXTERNAL_OES`（通常
   零成本，只是 fence 同步，不产生 draw call）
2. `SurroundViewPass` —— **一步式**渲染：顶点着色器完成 bowl/plane 网格到屏幕
   空间的投影，片元着色器按每相机标定 UV 采样 + 重叠区融合，直接输出到屏幕/
   FBO。刻意避免"先 unwarp 每路相机到独立 FBO 再合成"的多次 FBO 方案，
   因为 Adreno 是 tile-based 架构，带宽是瓶颈，多一次 FBO 落地就多一次
   显著的带宽开销。
3. `CarModelPass`（可选） —— 叠加渲染 3D 车辆模型，简单 Blinn-Phong/半色调
   光照，不需要 PBR/IBL。
4. `OverlayUIPass`（可选） —— 泊车辅助线、雷达距离、Logo 等 2D 叠加。
5. `PresentPass` —— `eglSwapBuffers`。

每个 Pass 是可插拔接口，天然是未来"要不要接入 FrameGraph"的迁移单元
（`execute()` 签名不需要变，只是资源创建方式从"手动 setup"变成"声明式
declare"）。

---

## 5. 材质系统（去掉 matc，保留 Filament 的分层思想）

Filament 用 `.mat` 文件 + `matc` 离线编译器生成跨 API 的 shader 包。REL 不需要
这套重量级工具链（毕竟只有 GLES 一个后端），改为：

- `Material`：持有 GLSL ES 源码（内置字符串常量或从 `assets/shaders/*.glsl`
  加载）+ 支持的宏定义空间（比如 `N_CAMERA=4/5/6`、`ENABLE_TONEMAP` 等）。
- `MaterialInstance`：持有一份 uniform 参数（简单 UBO 或散 uniform），可以从
  同一个 `Material` 创建多个 instance（比如车模型多个部件复用一个 lit 材质）。
- **Shader Variant 缓存**：`hash(defines) -> ProgramHandle`，避免重复编译。

内置材质集中在 `src/materials/`：`surround_view.vert/frag`（鱼眼 unwarp + 融合）、
`simple_lit.vert/frag`（车模型）、`overlay_ui.vert/frag`。

---

## 6. AVM 业务层（`src/avm/`）

| 模块 | 职责 |
|---|---|
| `CameraStreamManager` | 管理 N 路相机的 NV12 帧输入。**输入契约是纯 CPU 内存地址**（`NV12FrameDescriptor{data, width, height, strideY, strideUV}`），不依赖 `AHardwareBuffer`/`EGLImage`：每路相机的 Y 平面上传为 `R8` 纹理、交织 UV 平面上传为 `RG8` 纹理，`glTexSubImage2D` + `GL_UNPACK_ROW_LENGTH` 处理行对齐 padding，NV12→RGB 转换放在 `surround_view.frag` 里做，不在 CPU 侧转换 |
| `FisheyeCalibration` | 加载/管理每路相机的内参（畸变系数）+ 外参（相对车身位姿），生成"世界坐标→相机 UV"的映射数据（可以是 LUT 纹理，也可以是 shader 内联的多项式） |
| `BowlMeshGenerator` | 生成碗状/平面拼接网格（顶点数通常几千级别），每个顶点预烘焙好"应该采样哪几路相机 + 融合权重"，减少 fragment shader 分支 |
| `StitchBlender` | 重叠区域融合策略（线性 alpha / 距离加权），可作为 mesh 顶点属性烘焙，也可作为 shader uniform 表 |

> **关于零拷贝**：`backend::DriverApi::importExternalTexture`（`AHardwareBuffer`→
> `EGLImage`→`GL_TEXTURE_EXTERNAL_OES`）接口仍然保留，但当前不在关键路径上——
> 明确的输入契约是"4 路 NV12 的 CPU 地址"，因此 `CameraStreamManager` 走的是
> `createTexture()`+`updateTexture()` 的 CPU 上传路径。如果未来相机源能直接
> 提供 GPU 可导入的 buffer（如某些高通 VPU/ISP 输出路径支持 `AHardwareBuffer`），
> 可以在不改上层接口的前提下切换到零拷贝路径。

这一层是 AVM 场景特有的，不属于通用引擎能力，因此和 `engine/renderer/backend`
解耦，仅通过标准的 `Material/RenderableManager/Texture` API 与引擎交互——
即"AVM 是引擎的一个使用者，不是引擎内置概念"，方便未来引擎复用到其他车机
渲染场景（仪表、行车记录仪特效等）。

---

## 7. Adreno / GLES 专项优化要点

1. **能用一次 Pass 就不用两次**：tile-based GPU 上 FBO 的 load/store 都有真实
   带宽代价，AVM 主 Pass 尽量一步完成 unwarp+stitch+输出。
2. **CPU 端只做最小拷贝，YUV→RGB 放在 shader 里**：当前输入契约是 CPU 内存
   里的 NV12（见第 6 节），因此 CPU→GPU 上传本身无法避免；但应避免在 CPU
   侧再做一次 NV12→RGBA 转换再上传（那样既费 CPU 又多两倍上传带宽）——
   直接把 Y/UV 平面按 `R8`/`RG8` 原样上传，转换交给
   `materials/surround_view.frag`。若未来相机源能提供
   `AHardwareBuffer`，`backend::DriverApi::importExternalTexture` 预留了
   零拷贝路径（`EGL_ANDROID_get_native_client_buffer`/`eglCreateImageKHR`
   导入 `GL_TEXTURE_EXTERNAL_OES`）。
3. **`glInvalidateFramebuffer`**：每个 FBO 用完立即 invalidate 不需要保留的
   attachment（如 depth），减少 tile store 带宽。
4. **扩展探测优先用**：`GL_QCOM_tiled_rendering`、
   `GL_EXT_shader_framebuffer_fetch`（可用于单 Pass 内做融合而不需要额外
   采样）、`GL_OES_EGL_image_external_essl3`。
5. **动态网格更新用双缓冲 VBO + `glBufferSubData`/orphaning**：标定参数在线
   刷新（如泊车时动态调整视角）不应阻塞渲染线程。
6. **纹理/FBO 池化**：avoid 每帧 alloc/free GPU 对象，固定分辨率场景下资源
   常驻。
7. **GPU/CPU 计时打点**：预留 `GL_EXT_disjoint_timer_query` 封装，便于后续
   性能专项优化阶段直接用。

---

## 8. 线程模型

```
[相机线程] CameraHAL/采集线程写入 NV12 CPU buffer → 通知渲染线程
[渲染线程] Choreographer vsync → CameraStreamManager::onFrameAvailable(NV12) → Renderer::render(view) → eglSwapBuffers
[Job 线程池(可选,轻量)] 标定参数计算 / mesh 重建 / 异步资源上传（不参与每帧渲染路径）
```

由于输入契约是 CPU 内存里的 NV12（见第 6 节），渲染线程与相机线程之间需要
一次真实的数据搬运（`onFrameAvailable` 内部的 `glTexSubImage2D` 上传），
而不是单纯的 Handle/Fence 同步；调用方需要保证在渲染线程调用
`onFrameAvailable` 时，传入的 NV12 buffer 在整个调用期间保持有效（例如用双
缓冲/帧队列，避免相机线程在上传过程中覆写同一块内存）。Job 线程池用简单的
固定线程数 thread pool + lock-free/mutex 队列即可，不需要 Filament 那种复杂
的 work-stealing JobSystem。

---

## 9. 未来 Vulkan 扩展预留点（本期不实现）

- `backend/vulkan/` 目录占位，`VulkanDriver` 需实现与 `GLDriver` 相同的
  `DriverApi` 纯虚接口。
- Shader 目前只维护 GLSL ES 源码；引入 Vulkan 时两个可选路径：
  1. 手写等价 GLSL Vulkan / SPIR-V（体量小，AVM 材质种类少，手写成本可控）；
  2. 引入 `glslang` + `SPIRV-Cross` 做 GLSL↔SPIR-V 互转（体量增大，视需要再引入）。
- `DriverEnums.h` 中的枚举值命名/语义已经按"API 无关"设计（参照 Filament
  `backend/DriverEnums.h`），迁移时不需要改上层调用。
- `PlatformEGLAndroid` 与未来 `PlatformVkAndroid` 都实现同一个
  `Platform` 接口（Surface 创建、vsync、AHardwareBuffer 导入）。

---

## 10. 目录结构

```
renderengine_lite/
├── CMakeLists.txt
├── ARCHITECTURE.md
├── include/rel/                 # 公开 API（App/JNI 只依赖这里）
│   ├── rel_fwd.h
│   ├── Entity.h
│   ├── Engine.h
│   ├── Scene.h
│   ├── View.h
│   ├── Camera.h
│   ├── Material.h
│   ├── MaterialInstance.h
│   ├── VertexBuffer.h / IndexBuffer.h
│   ├── Texture.h
│   ├── Renderable.h             # RenderableBuilder：把几何+Material挂到Entity上
│   └── SwapChain.h
├── src/
│   ├── engine/                  # Engine/EntityManager/TransformManager/RenderableManager
│   ├── renderer/                # Renderer/RenderPass 及内置 Pass 实现
│   ├── backend/                 # DriverApi/Handles/DriverEnums
│   │   ├── opengl/              # GLDriver（本期唯一实现）
│   │   └── vulkan/              # 占位
│   ├── materials/                # 内置 GLSL ES shader 源码
│   ├── avm/                      # AVM 业务层
│   ├── utils/                    # 数学(可直接用 glm)/日志/简单 JobSystem/Pool 分配器
│   └── platform/android/         # JNI 胶水层、AHardwareBuffer 封装
├── samples/desktop_preview/      # 桌面 GLFW+GLES(ANGLE或Mesa) 预览工程，加速迭代
├── android/                      # Gradle 工程：JNI wrapper + Demo App
└── third_party/                  # glm 等轻量依赖（建议 submodule 或 CPM）
```

---

## 11. 里程碑建议

| 里程碑 | 内容 | 状态 |
|---|---|---|
| M0 | CMake + Android(Gradle+CMake) 工程打通，EGL 上屏黑屏/清屏 | ✅ 骨架已搭建（`android/`），已用 NDK r27 交叉编译+链接验证通过，未在真机上肉眼确认画面 |
| M1 | `DriverApi` + `GLDriver` 基础资源（Texture/VertexBuffer/Program），跑通一个三角形 | ✅ 代码已实现；新增 `RenderableBuilder`/`Engine::setTransform` 公开 API 后，`jni_bridge.cpp` 里的旋转三角形 Sample 把整条链路（含 GLDriver）串了起来，NDK 交叉编译+链接通过 |
| M2 | `Engine/Scene/View/Renderer` 打通，`RenderableManager` 渲染静态网格 | ✅ 代码已实现（单一 GL render pass 包裹三个逻辑 Pass） |
| M3 | 4 路 NV12(CPU 地址) 接入 `CameraStreamManager`，单路鱼眼 unwarp demo | ✅ `CameraStreamManager`（NV12→R8/RG8 上传）+ shader NV12→RGB 转换已实现；鱼眼 unwarp mesh 待 M4 |
| M4 | 多路（4/6 路）拼接融合 + bowl mesh，完整环视 | ⏳ 接口已声明（`BowlMeshGenerator`/`StitchBlender`），实现待补 |
| M5 | 车辆 3D 模型叠加 + overlay UI | ⏳ `CarModelPass`/`OverlayUIPass` 占位，待 Scene 支持分层过滤 |
| M6 | Adreno 专项性能优化（GPU timing、带宽分析、功耗测试） | ⏳ 未开始 |
| M7（可选） | Vulkan backend 预研/实现 | ⏳ 未开始 |

---

## 12. 与 Filament 的取舍对照表

| 能力 | Filament | RenderEngineLite | 取舍原因 |
|---|---|---|---|
| RHI Handle 抽象 | ✅ | ✅ 保留 | 面向多后端的核心价值，AVM 场景仍受益 |
| Entity/Manager 场景模型 | ✅（完整） | ✅（裁剪版，无 LightManager 等） | 场景对象少，够用即可 |
| FrameGraph | ✅ | ❌ 显式 Pass 链替代 | Pass 结构固定，省 CPU/心智开销 |
| 材质系统 + matc | ✅ | ✅ 裁剪（无离线编译器） | 只有一个后端，不需要跨 API 编译 |
| PBR/IBL | ✅ | ❌ 简单 Blinn-Phong | AVM 无需电影级光照 |
| 多线程 CommandStream | ✅ | ❌ 单线程直接调用 | 极致低延迟优先于吞吐量 |
| JobSystem(work-stealing) | ✅ | 简化线程池 | 并行任务量小 |
| 多后端（GL/Vulkan/Metal） | ✅ | 仅 GLES，接口预留 Vulkan | 当前平台/工期约束 |
