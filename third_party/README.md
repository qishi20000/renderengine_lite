# third_party

建议以 git submodule 方式引入以下轻量依赖（均为 header-only 或极小体积，
符合"轻量化"目标；未引入前 `src/utils/Math.h` 提供了最小可编译的替代实现）：

- `glm`（`https://github.com/g-truc/glm`）：数学库，替换 `src/utils/Math.h` 中的
  手写 `mat4`/`vec3`。引入后建议保留 `rel::mat4 = glm::mat4` 等别名，避免大范围
  改动上层代码。

```bash
git submodule add https://github.com/g-truc/glm third_party/glm
```

不建议引入的重量级依赖（不符合"轻量化"目标，除非未来明确需要）：

- `assimp`：车辆模型加载可以用更轻量的自定义二进制格式或 `tinygltf`
- `entt`：ECS 框架，本项目采用 ECS-lite（见 `ARCHITECTURE.md` 第 3 节），
  除非场景对象数量级发生数量级增长，否则不建议引入
