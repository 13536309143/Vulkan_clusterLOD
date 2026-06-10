# 更新日志


本次记录两项核心算法与渲染路径更新：

1. 几何指纹去重 + 实例共享增强
2. 装配 bbox 层级剔除
3. 装配级 visibility pass + 装配级 LOD error + 装配模板指纹共享
4. 修复特殊工业模型 LOD 构建容量估计不足导致的闪退

---

## 一、几何指纹去重 + 实例共享增强

### 变动代码

- `src/scene/scene_gltf.cpp`
  - 新增 `GeometryFingerprint` 相关逻辑。
  - 新增基于 glTF mesh primitive 语义数据的几何指纹计算。
  - 替换原先偏弱的 accessor/pointer/string 级几何复用判断。
  - 调整 glTF buffer 加载流程，保证缓存加载和直接加载时都能稳定计算指纹。
  - 修正 meshopt 压缩 buffer view 的释放条件，避免未解码 view 被错误释放。

- `src/scene/scene.hpp`
  - 更新几何缓存版本号，用于让旧缓存按旧去重规则生成的数据失效。

### 变动位置

- glTF 几何导入阶段：
  - 原来每个 glTF mesh/primitive 更依赖 accessor 指针、buffer view 或局部索引关系判断是否复用。
  - 现在在导入 mesh 时计算几何指纹，再用指纹作为去重 key。

- 几何缓存判断阶段：
  - 由于几何去重规则改变，旧 cache 中的 geometry 数量、顺序和实例引用关系可能不再对应当前规则，因此通过 cache version 触发重新预处理。

### 实现原理

工业三维模型里大量零件可能是重复件，例如螺栓、垫圈、支架、阵列孔位零件。它们在 glTF 中可能不是同一个 accessor 或同一个 buffer view，但真实几何数据完全一致。原先按 accessor 或指针关系去判断复用，会漏掉这类重复件。

新逻辑改为计算“几何语义指纹”：

- 读取 mesh primitive 的关键语义数据。
- 结合 attribute/index 内容和相关布局信息生成稳定 hash。
- 相同几何内容得到相同指纹。
- 指纹命中后复用已有 geometry，实例只保存不同 transform/material 等实例属性。

这样可以把“数据来源不同但几何相同”的零件识别为同一个 geometry，从而增强实例共享。

### 影响

- 减少重复 geometry 的处理次数。
- 降低大型工业模型的几何缓存体积和 GPU 几何数据规模。
- 增强重复零件场景下的实例共享效果。
- 对外部 glTF 文件格式没有额外要求，但重复件必须在几何语义上真正一致。

### 建议验证

- 使用包含大量重复零件的工业 glTF/glb 模型加载。
- 对比改动前后的 geometry 数量、缓存生成时间、显存占用。
- 观察是否还有 `geometry mismatches scene cache file`，如果有旧缓存，应删除或等待新 cache 自动生成。
- 确认重复件位置、材质、朝向仍然正确。

---

## 二、装配 bbox 层级剔除

### 变动代码

- `src/scene/scene.hpp`
  - 新增 `assemblyCullingMinInstances` 配置，默认值为 `8`。
  - `Scene::Instance` 新增 `assemblyID`。
  - 新增 `GltfNodeImportResult`，用于记录 glTF node 子树覆盖的实例范围和 bbox。
  - 新增 `m_assemblyNodes`，保存由 glTF node tree 派生出的装配节点。

- `src/scene/scene_gltf.cpp`
  - 修改 `addInstancesFromNodeGLTF`，从单纯递归创建 instance 改为递归返回 node 子树信息。
  - 对每个 glTF node 统计：
    - `firstInstance`
    - `instanceCount`
    - world-space bbox
    - child count
    - tree depth
  - 当子树实例数大于等于 `assemblyCullingMinInstances` 时生成装配节点。
  - 小于阈值的子装配不生成 assembly node，直接展平为普通实例。

- `src/scene/scene.cpp`
  - 缓存命中后保留当前运行时设置的 `assemblyCullingMinInstances`。
  - 在 `computeInstanceBBoxes` 后同步重算 assembly bbox。
  - 当 scene grid 生成多份复制实例时清空 assembly 数据，避免复制后的实例引用原始 glTF 装配 bbox。
  - 增加日志输出：`assembly nodes: N, min instances: X`。

- `shaders/interface/shaderio_scene.h`
  - 新增 `SHADERIO_INVALID_ASSEMBLY`。
  - 新增 GPU 侧 `AssemblyNode` 结构。
  - `RenderInstance` 新增 `assemblyID`。

- `shaders/interface/shaderio_building.h`
  - `SceneBuilding` 新增：
    - `numAssemblyNodes`
    - `assemblyCullingMinInstances`
    - `assemblyNodes` buffer device address

- `src/renderer/renderer.cpp`
  - 上传 `RenderInstance` 时写入 CPU 侧 instance 的 `assemblyID`。

- `src/renderer/renderer_clusters_lod.cpp`
  - 创建并上传 assembly node buffer。
  - 将 assembly node buffer 的 device address 写入 `SceneBuilding`。
  - 在 renderer deinit 时释放 assembly node buffer。

- `shaders/traversal/traversal_init.comp.glsl`
  - 在 geometry bbox 判断和 LOD traversal 之前增加 assembly bbox 可见性测试。
  - assembly bbox 不可见时，直接跳过该实例后续 traversal 初始化。

- `src/app/lodclusters_config.cpp`
  - 注册命令行参数：`assemblymininstances`。

- `src/app/lodclusters_ui.cpp`
  - 在 UI 中加入 `Assembly min instances` 参数。

### 变动位置

- CPU glTF 导入阶段：
  - 保留 glTF node tree 的层级语义。
  - 从 node 子树生成装配 bbox。
  - 用阈值控制哪些 node 作为装配层级，哪些 node 直接展平。

- CPU 到 GPU 数据上传阶段：
  - 每个 render instance 多携带一个 `assemblyID`。
  - 额外上传 assembly node buffer。
  - 不新增 descriptor binding，而是通过已有 `SceneBuilding` 传递 device address。

- GPU traversal 初始化阶段：
  - 原流程：instance -> geometry bbox culling -> LOD root traversal。
  - 新流程：instance -> assembly bbox culling -> geometry bbox culling -> LOD root traversal。

### 实现原理

大型工业模型通常具有天然装配结构，例如整机、模块、子系统、零部件。glTF node tree 往往能保留这种层级关系。如果每帧只按单个零件实例做裁剪，GPU 需要对大量实例分别进行 bbox、Hi-Z 和 LOD 判断。

本次改动将 glTF node tree 转换为结构感知的装配裁剪层：

1. 导入 glTF 时递归遍历 node tree。
2. 每个 node 统计其子树下包含的 render instance 数量。
3. 如果子树实例数达到阈值，说明它是一个足够大的装配节点。
4. 为该装配节点计算 world-space bbox。
5. 将该装配节点写入 `m_assemblyNodes`。
6. 子树下的实例记录 `assemblyID`。
7. traversal init 阶段先测试 assembly bbox。
8. 如果 assembly bbox 不可见，则该实例直接不进入后续 geometry bbox 和 LOD traversal。

阈值 `assemblyCullingMinInstances` 的作用是避免过小装配带来额外判断成本。例如只有 1 到 3 个零件的小 node，如果也生成 assembly node，GPU 会多一次 assembly bbox 判断，但收益很低。因此小装配会被自动展平。

### 影响

- 对具有清晰 glTF node tree 的大型工业模型更有效。
- 当整块装配在视锥外或被 Hi-Z 遮挡时，可以减少后续 instance/geometry/LOD traversal 工作。
- 当模型本身已经完全扁平化，或每个 node 只有极少零件时，assembly nodes 数量会很少，系统自动退回原来的 instance 级路径。
- 每个 render instance 增加一个 `assemblyID` 字段。
- GPU 侧增加一块 assembly node buffer，通常远小于几何/cluster 数据。

### 参数说明

- `assemblymininstances`
  - 默认值：`8`
  - 含义：一个 glTF node 子树下至少包含多少个 render instance，才生成 assembly culling node。
  - 设置为 `0`：关闭装配 bbox 层级剔除。

示例：

```powershell
.\_bin\Release\t1.exe --assemblymininstances 16
```

### 建议验证

- 使用大型工业 glTF/glb 模型加载。
- 查看启动日志：

```text
assembly nodes: N, min instances: X
```

- 如果 `N = 0`：
  - 说明模型 node tree 太扁平；
  - 或阈值设置过高；
  - 可尝试 `--assemblymininstances 4` 或 `8`。

- A/B 对比：

```powershell
.\_bin\Release\t1.exe --assemblymininstances 0
.\_bin\Release\t1.exe --assemblymininstances 8
.\_bin\Release\t1.exe --assemblymininstances 16
```

- 对比指标：
  - FPS
  - traversal task 数量
  - rendered clusters 数量
  - 视角快速移动时是否有错误剔除
  - 大装配移出视野时帧率是否提升

### 构建验证

已完成 Release 构建：

```powershell
cmake --build build --config Release
```

构建结果：

- 成功生成：`_bin\Release\t1.exe`
- 仍存在既有 warning：`streamutils.cpp` 的 C4307 整型常量溢出警告
- 仍存在既有 post-build 提示：`pwsh.exe` 未找到
- MSBuild 返回成功

---

## 三、装配级 visibility pass + 装配级 LOD error + 装配模板指纹共享

### 日期

2026-06-09

### 变动代码

- `src/scene/scene.hpp`
  - 新增 `assemblyLodPixelThreshold`，默认值为 `24.0f`。
  - 新增 `AssemblyTemplate`，用于记录装配模板指纹、首个装配节点、模板复用次数和实例数量。
  - 新增 `m_assemblyTemplates` 和 `m_assemblyTemplateMap`，保存装配模板共享关系。

- `src/scene/scene_gltf.cpp`
  - 新增装配模板指纹计算逻辑。
  - 在 glTF node tree 生成 assembly node 时，同时计算该 node 子树的模板指纹。
  - 对重复出现的相同子装配复用同一个 `templateID`，并记录 `templateInstanceID`。
  - 指纹输入包含子树实例的几何 ID、材质 ID、双面标志、相对变换、子节点数量和实例数量。

- `src/scene/scene.cpp`
  - 缓存加载后保留当前运行配置中的 `assemblyCullingMinInstances` 和 `assemblyLodPixelThreshold`。
  - 启动日志扩展为输出 assembly node 数量、template 数量、最小实例阈值和 LOD 像素阈值。
  - grid copy 复制场景时清空 assembly node、assembly template 和 instance 上的 `assemblyID`，避免复制场景误用原始 glTF 装配层级。

- `shaders/interface/shaderio.h`
  - 新增 `ASSEMBLY_VISIBILITY_WORKGROUP`，用于控制装配级 visibility pass 的 compute dispatch 粒度。

- `shaders/interface/shaderio_scene.h`
  - `AssemblyNode` 新增 `templateID` 和 `templateInstanceID`。
  - 新增 `SHADERIO_ASSEMBLY_VISIBLE_BIT` 和 `SHADERIO_ASSEMBLY_LOD_COARSE_BIT`。
  - 新增 `AssemblyState`，保存装配节点的可见性、屏幕像素大小和 LOD error 估计值。
  - 新增 `AssemblyStates_inout` buffer reference。

- `shaders/interface/shaderio_building.h`
  - `SceneBuilding` 新增 `assemblyLodPixelThreshold`。
  - `SceneBuilding` 新增 `assemblyStates` buffer device address。

- `shaders/traversal/assembly_visibility.comp.glsl`
  - 新增装配级 visibility compute shader。
  - 每个 assembly node 在 traversal 前单独计算一次可见性。
  - 支持 frustum culling、Hi-Z culling、屏幕像素大小估计和装配级粗 LOD 标志写入。

- `shaders/traversal/traversal_init.comp.glsl`
  - traversal init 从 `AssemblyState` 读取装配可见性。
  - assembly 不可见时，跳过该实例后续 geometry bbox culling 和 LOD traversal。
  - assembly 屏幕占用低于阈值时，强制该装配下实例走低细节 cluster。

- `src/renderer/renderer_clusters_lod.cpp`
  - 新增 `computeAssemblyVisibility` shader 编译和 compute pipeline 创建。
  - 新增 assembly state buffer 创建、地址绑定和释放。
  - 在 traversal init 前 dispatch 装配级 visibility pass。
  - 在 visibility pass 写入后插入 shader write 到 shader read 的 barrier，保证 traversal init 读取到最新装配状态。

- `src/app/lodclusters_config.cpp`
  - 新增命令行参数 `assemblylodpixels`。

- `src/app/lodclusters_ui.cpp`
  - UI 新增 `Assembly LOD pixels` 输入项。

### 变动位置

- CPU glTF 导入阶段：
  - 保留 glTF node tree 的装配层级语义。
  - 对满足 `assemblyCullingMinInstances` 的大子装配生成 assembly node。
  - 对大子装配计算模板指纹，用于识别重复装配结构。

- CPU 到 GPU 数据上传阶段：
  - 原先只上传 assembly node bbox。
  - 现在额外上传装配模板 ID、模板实例 ID，并创建每帧可写的 assembly state buffer。

- GPU traversal 前处理阶段：
  - 原先 assembly bbox culling 在 `traversal_init.comp.glsl` 中按 instance 重复执行。
  - 现在新增独立 `assembly_visibility.comp.glsl`，按 assembly node 预先执行一次 visibility pass。
  - traversal init 只读取预计算好的 assembly state，减少重复判断。

- GPU LOD 决策阶段：
  - 原先 LOD 主要由 geometry/cluster traversal metric 决定。
  - 现在增加装配级 coarse LOD 标志，当整个装配在屏幕上足够小时，优先进入低细节路径。

### 实现原理

大型工业三维模型的性能瓶颈通常不只来自三角形数量，也来自大量零件实例带来的 traversal、bbox 测试和 LOD 判断开销。很多工业模型具有清晰的装配结构，例如整机、模块、子系统、重复安装单元。只按单个零件做可见性和 LOD，会漏掉这些高层结构信息。

本次更新把装配结构提升为 traversal 前的一级调度单元：

1. CPU 导入 glTF 时，递归遍历 node tree。
2. 对实例数量达到阈值的大子树生成 assembly node。
3. 对该 assembly node 计算 world-space bbox。
4. 对该 assembly node 的局部子树计算模板指纹。
5. 相同模板指纹复用同一个 `templateID`。
6. GPU 每帧先运行装配级 visibility pass。
7. visibility pass 对 assembly bbox 做 frustum 和 Hi-Z 判断。
8. visibility pass 同时估计装配 bbox 的屏幕像素大小。
9. 当装配屏幕占用小于 `assemblyLodPixelThreshold` 时写入 coarse LOD 标志。
10. traversal init 读取 assembly state，先判断大装配是否整体不可见或应走粗 LOD。

装配级 LOD error 当前采用 bbox 尺寸与视距关系、屏幕像素覆盖范围作为近似误差信号。它的目标不是替代现有 cluster LOD，而是在 cluster LOD 之前增加一个结构感知的早期决策层。这样可以让大型装配在远距离或遮挡状态下更早减少 traversal 工作。

装配模板指纹共享的意义在于：相同结构的子装配可以被识别为同一模板。当前实现已经保存 `templateID/templateInstanceID`，为后续进一步做模板级统计、模板级 LOD 策略、模板级遮挡历史复用和装配代理生成提供基础。

### 影响

- 对大型工业模型更有针对性，尤其适合包含大量重复子装配、模块化结构或清晰 glTF node tree 的模型。
- 每帧新增一个装配级 compute pass，但它按 assembly node 数量执行，通常远少于 instance 或 cluster 数量。
- traversal init 不再重复对同一装配 bbox 做大量 instance 级判断，而是读取预计算好的 assembly state。
- 远距离小装配可更早进入低细节路径，降低 rendered clusters 和 traversal 负载。
- 如果模型 node tree 已完全展平，或 `assemblyCullingMinInstances` 设置过高，则 assembly node 数量会较少，收益也会降低。
- 当前 coarse LOD 使用已有 `geometry.lowDetailClusterID`，还没有生成独立的装配代理 mesh。

### 参数说明

- `assemblymininstances`
  - 含义：一个 glTF node 子树至少包含多少个 render instance，才生成 assembly node。
  - 默认值：`8`
  - 设置为 `0` 可关闭装配层级 culling。

- `assemblylodpixels`
  - 含义：装配 bbox 投影到屏幕后的像素阈值。
  - 默认值：`24.0`
  - 值越大，装配越容易提前进入 coarse LOD。
  - 设置为 `0` 可关闭装配级 coarse LOD 判断，但保留装配级 visibility pass。

示例：

```powershell
.\_bin\Release\t1.exe --assemblymininstances 8 --assemblylodpixels 24
```

### 建议验证

- 使用大型工业 glTF/glb 模型加载，优先选择包含重复子装配和清晰 node tree 的模型。
- 查看启动日志：

```text
assembly nodes: N, templates: M, min instances: X, lod pixels: Y
```

- 如果 `N = 0`：
  - 说明模型 node tree 可能过于扁平；
  - 或 `assemblymininstances` 设置过高；
  - 可尝试 `--assemblymininstances 4` 或 `--assemblymininstances 8`。

- 如果 `M` 明显小于 `N`：
  - 说明装配模板指纹共享命中了重复子装配；
  - 适合继续做模板级 LOD 或模板级遮挡历史复用实验。

- A/B 对比建议：

```powershell
.\_bin\Release\t1.exe --assemblymininstances 0
.\_bin\Release\t1.exe --assemblymininstances 8 --assemblylodpixels 24
.\_bin\Release\t1.exe --assemblymininstances 8 --assemblylodpixels 48
```

- 对比指标：
  - FPS
  - traversal task 数量
  - rendered clusters 数量
  - culled instances 数量
  - 远距离装配的 LOD 切换是否更及时
  - 快速移动视角时是否出现错误剔除或明显跳变

### 构建与 shader 验证

已完成 Release 构建：

```powershell
cmake --build build --config Release
```

验证结果：

- 成功生成：`_bin\Release\t1.exe`
- 新增 `assembly_visibility.comp.glsl` 离线 SPIR-V 编译通过。
- 修改后的 `traversal_init.comp.glsl` 使用 Vulkan 1.3 目标环境离线 SPIR-V 编译通过。
- `git diff --check` 未发现空白错误。
- 仍存在既有 warning：`streamutils.cpp` 的 C4307 整型常量溢出警告。
- 仍存在既有 post-build 提示：`pwsh.exe` 未找到。

---

## 四、右下角运行参数面板与特征保持率指标可视化

### 日期

2026-06-10

### 变动代码

- `src/app/lodclusters_ui.cpp`
  - 新增右下角固定面板 `Runtime / Cache Parameters`。
  - 在面板中显示场景文件、cache 文件、cache 是否存在、是否从 cache 加载、cache 大小、装配节点数、装配模板数等运行信息。
  - 新增 `Scene Config`、`Runtime Renderer`、`Scene Output Stats`、`Cache Generation Output`、`Feature Retention Output`、`Streaming / Grid` 等折叠区。
  - 新增置顶的 `Feature Retention Summary`，默认展开显示关键特征保持率指标，避免完整参数列表过长时用户需要向下滚动才能看到。
  - 当当前 scene/cache 没有特征统计时，在 UI 中明确提示需要删除或重建旧 cache。

- `src/scene/scene.hpp`
  - 新增 `Scene::ProcessingStatsSnapshot`，用于把几何处理阶段输出到终端的统计数据保存为 Scene 可读取的快照。
  - `Scene` 新增成员 `m_processingStats`，供 UI 读取。
  - `CacheFileHeader` 新增 `processingStats` 字段。
  - cache header 中新增 `processingStatsVersion` 和 `processingStatsStructSize`。
  - cache `geoVersion` 从 `8` 升级到 `9`，使旧 cache 自动失效并重新生成，避免旧 cache 中缺少特征指标导致 UI 显示 0。
  - `CacheFileView` 新增 `getProcessingStats(...)` 声明。
  - `endProcessingOnly(...)` 接口增加 `ProcessingInfo&` 参数，用于大模型专用预处理保存路径回写最终统计。

- `src/scene/scene.cpp`
  - 在场景几何处理结束后，把 `ProcessingInfo::stats` 拷贝到 `m_processingStats`。
  - 从 cache 加载 scene 时，通过 `CacheFileView::getProcessingStats(...)` 回读 cache header 中保存的统计快照。

- `src/core/cache.cpp`
  - 实现 `CacheFileView::getProcessingStats(...)`。
  - 普通 `Scene::saveCache()` 保存 cache 时写入 `m_processingStats`。
  - `beginProcessingOnly(...)` 写入 cache header 时预留并写入统计字段。
  - `endProcessingOnly(...)` 在大模型 dedicated preprocess pass 结束时，将最终 `ProcessingInfo::stats` 写回 cache header。

- `src/scene/scene_gltf.cpp`
  - 调整 `endProcessingOnly(...)` 调用，传入 `processingInfo`，确保 preprocess-only cache 能保存最终特征保持率统计。

### 变动位置

- UI 显示层：
  - 文件：`src/app/lodclusters_ui.cpp`
  - 位置：`LodClusters::onUIRender()`
  - 作用：在主视口右下角显示当前运行参数、cache 参数、场景统计、cache 生成统计和特征保持率指标。

- 场景处理统计保存层：
  - 文件：`src/scene/scene.cpp`
  - 位置：`Scene::init(...)` 中几何处理完成后。
  - 作用：把原本只打印到终端的 `ProcessingInfo::stats` 保存到 `Scene::m_processingStats`。

- cache 持久化层：
  - 文件：`src/scene/scene.hpp`、`src/core/cache.cpp`
  - 位置：`CacheFileHeader`、`Scene::saveCache()`、`Scene::beginProcessingOnly(...)`、`Scene::endProcessingOnly(...)`、`CacheFileView::getProcessingStats(...)`
  - 作用：让特征保持率和 cache 生成统计随 `.zippp` cache 一起保存，后续命中 cache 时仍能在 UI 显示。

- 大模型 preprocess 路径：
  - 文件：`src/scene/scene_gltf.cpp`、`src/core/cache.cpp`
  - 位置：`loadGLTF(...)` 调用 `endProcessingOnly(...)` 的路径。
  - 作用：修复大模型走 dedicated preprocess pass 时，cache header 先写入、最终统计后产生，导致统计无法进入 cache 的问题。

### 实现原理

此前 `Feature Retention Stats`、`Group Data Stats` 等数据主要通过 `ProcessingInfo::stats` 在几何处理结束时打印到终端。UI 想显示这些数据时，只能读取当前 Scene 对象中的运行时状态；如果模型直接命中旧 cache，几何处理不会重新执行，`ProcessingInfo::stats` 就没有有效数据，因此 UI 中的特征保持率会显示为 0 或看不到有效指标。

本次更新把统计数据从“临时终端输出”升级为“Scene 运行时快照 + cache header 持久化字段”：

1. 几何处理阶段继续累积 `ProcessingInfo::stats`。
2. `Scene::init(...)` 在处理完成后把统计值复制到 `m_processingStats`。
3. UI 从 `m_processingStats` 读取并显示，不再依赖终端日志。
4. 保存 `.zippp` cache 时，把 `m_processingStats` 写入 `CacheFileHeader`。
5. 后续加载 cache 时，从 `CacheFileHeader` 回读 `processingStats`。
6. 对大模型 dedicated preprocess pass，在 `endProcessingOnly(...)` 结束阶段回写最终统计，保证大模型 cache 也能保存完整指标。
7. cache `geoVersion` 升级到 `9`，旧 cache 会自动失效并重建，避免旧 header 缺字段导致错误读取。

UI 侧额外增加 `Feature Retention Summary`，把最关键的指标放在面板顶部，包括：

- feature vertices
- protected vertices
- critical vertices
- circular hole vertices
- thin-wall vertices
- cylindrical vertices
- average importance
- max importance

完整指标仍保留在 `Feature Retention Output` 中，便于继续查看 boundary、non-manifold、sharp ring、hole loop 等详细统计。

### 影响

- 右下角 UI 可以直接看到运行参数、cache 参数、生成统计和特征保持率指标，不需要回看终端输出。
- 特征保持率指标在新 cache 中可持久保存，后续命中 cache 加载时仍然可见。
- 旧 `.zippp` cache 会因为 cache 版本升级自动失效并重新生成一次。
- cache header 增加一份统计快照，体积增量很小，相比几何数据可以忽略。
- 不改变 cluster 数据主体、LOD 构建算法和 shader traversal 逻辑。
- 大模型 preprocess-only 保存路径也能保存最终统计，避免大型工业模型加载后 UI 缺少特征指标。

### 建议验证方式

1. 删除或让旧 cache 自动失效后加载模型，例如：

```powershell
.\_bin\Release\t1.exe --scene E:\vk_lod_clusters1\1\_downloaded_resources\c.glb
```

2. 首次加载时观察终端应重新生成 cache，并输出 `Feature Retention Stats`。
3. 进入 UI 右下角 `Runtime / Cache Parameters`，确认顶部 `Feature Retention Summary` 显示非 0 数据。
4. 关闭程序后再次加载同一模型，确认命中 cache 时 `Feature Retention Summary` 仍然显示非 0 数据。
5. 对大模型使用 dedicated preprocess pass 验证，例如 `a.glb` 或其他超大工业模型，确认 `Scene::endProcessOnlySave completed successfully` 后再次加载 cache 仍能显示特征指标。
6. 对比终端输出与 UI 中 `Cache Generation Output`、`Feature Retention Output` 的数值是否一致。

### 构建验证

已完成 Release 构建：

```powershell
cmake --build build --config Release
```

结果：

- 成功生成：`_bin\Release\t1.exe`
- `git diff --check -- src\app\lodclusters_ui.cpp src\scene\scene.hpp src\scene\scene.cpp src\scene\scene_gltf.cpp src\core\cache.cpp` 未发现空白错误。
- 仍存在既有 warning：`src\streaming\streamutils.cpp` 的 C4307 整型常量溢出警告。
- 仍存在既有 post-build 提示：`pwsh.exe` 未找到。
- MSBuild 返回成功。

---

## 五、修复特殊工业模型 LOD 构建容量估计不足导致的闪退

### 日期

2026-06-10

### 问题现象

加载 `12.glb` 时程序会闪退。Release 版本直接异常退出，Debug 版本能够触发明确断言：

```text
Assertion failed: pending_index < context.pending.size()
file: src/meshlod/meshlod_build.h
line: 183
```

Release 下对应的退出码为：

```text
0xC0000005
```

这是典型的访问违规，说明不是普通加载失败，而是内部数组写入越界。

### 模型特征

`12.glb` 本身不是坏文件。检查 glTF 结构后可见：

```text
nodes: 116430
mesh nodes: 57667
meshes: 22528
raw triangles: 35206706
```

该模型具有大量 node、较多 mesh 实例复用以及复杂工业拓扑。它能够通过 glTF JSON 结构检查，也不存在缺失 `indices`、缺失 `POSITION`、空 mesh、非法 child 引用或 node tree 循环等常见导入错误。

### 变动代码

- `src/meshlod/meshlod_build.h`
  - 新增 `estimateSplitClusterCapacity(...)`。
  - 在每轮 LOD 构建进入并行 `clodBuild_iterationTask` 前，根据当前 groups 的真实三角形数量估算本轮可能产生的 split cluster 容量。
  - 将原来的容量预留：

```cpp
context.clusters.resize(context.clusters.size() + context.pending.size() + context.groups.size());
context.pending.resize(context.pending.size() + context.groups.size());
```

  - 改为：

```cpp
const size_t splitCapacity = estimateSplitClusterCapacity(config, context.clusters, context.groups);

context.clusters.resize(context.clusters.size() + splitCapacity);
context.pending.resize(splitCapacity);
```

### 变动位置

- LOD 构建阶段：
  - 文件：`src/meshlod/meshlod_build.h`
  - 函数：`clodBuild(...)`
  - 位置：每轮 `partition(...)` 之后、并行执行 `clodBuild_iterationTask(...)` 之前。

- 并行 worker 写入阶段：
  - 文件：`src/meshlod/meshlod_build.h`
  - 函数：`clodBuild_iterationTask(...)`
  - 原问题发生在 worker 通过 `context.next_pending.fetch_add(split.size())` 获取写入区间后，写入 `context.pending[pending_index++]` 时越界。

### 实现原理

原算法每轮 LOD 构建时，会先把当前 pending clusters 分组，然后对每个 group 做简化，再把简化后的结果重新 clusterize。旧代码假设本轮新产生的 split cluster 数量不会超过：

```text
pending.size() + groups.size()
```

这个估计对多数模型成立，但不是严格上界。对于 `12.glb` 这类复杂工业模型，某些 group 简化后仍然保持较高复杂度，重新 `clusterize` 后可能分裂出比预估更多的 cluster。

旧流程中：

1. 主线程按较小容量 resize `context.pending` 和 `context.clusters`。
2. 多个 worker 并行处理不同 group。
3. 每个 worker 用 atomic 获取一段写入区间。
4. 某些 group 的 `split.size()` 超过旧估计。
5. `pending_index` 超出 `context.pending.size()`。
6. Debug 触发断言，Release 发生越界写并闪退。

新逻辑改为在每轮 worker 启动前做保守容量估计：

1. 遍历本轮所有 group。
2. 统计每个 group 当前包含的三角形数量。
3. 用 `config.min_triangles` 估计该 group 最多可能拆成多少 cluster。
4. 汇总得到本轮 split cluster 的预留容量。
5. 再启动并行 worker。

这样不需要在 worker 中动态扩容，避免了并行写 vector 时的数据竞争；同时容量估计比旧逻辑更接近真实上界，能够覆盖 `12.glb` 这类特殊拓扑。

### 影响

- 修复 `12.glb` 在 LOD 构建阶段的闪退。
- 对普通模型的输出逻辑没有语义改变，只是本轮临时容量预留更稳健。
- 对部分复杂模型会略微增加 LOD 构建阶段的临时内存预留，但避免了越界写入。
- 修复位置在 CPU 预处理阶段，不影响 shader、GPU traversal 或运行时渲染路径。
- 与装配级 visibility pass、装配级 LOD、几何指纹去重逻辑无直接耦合。

### 验证结果

已使用 `12.glb` 验证：

```powershell
.\_bin\Debug\t1.exe --scene E:\vk_lod_clusters1\1\_downloaded_resources\12.glb --processingonly 1 --autoloadcache 0 --autosavecache 0 --assemblymininstances 0
```

结果：

```text
Scene::endProcessOnlySave completed successfully
exit=0
```

已使用 Release 验证：

```powershell
.\_bin\Release\t1.exe --scene E:\vk_lod_clusters1\1\_downloaded_resources\12.glb --processingonly 1 --assemblymininstances 0
```

结果：

```text
geometries: 10884
Groups: 75248
Clusters: 615278
Vertices: 66478647
Scene::endProcessOnlySave completed successfully
saved: E:\vk_lod_clusters1\1\_downloaded_resources\12.glb.zippp
exit=0
```

构建验证：

```powershell
cmake --build build --config Debug
cmake --build build --config Release
```

结果：

- Debug 构建成功。
- Release 构建成功。
- `git diff --check -- src\meshlod\meshlod_build.h` 未发现空白错误。
- 仍存在既有 post-build 提示：`pwsh.exe` 未找到。

---

## 六、工业结构化特征识别与误差约束

### 日期

2026-06-10

### 变动代码

- `src/meshlod/meshlod_types.h`
  - 新增 `clodFeatureMetrics`，用于记录结构化特征识别结果。
  - `clodMesh` 新增 `feature_metrics` 指针，使简化阶段能够把特征统计回传到场景处理层。
  - 指标包括：
    - 输入特征顶点数、输入特征三角形数
    - boundary 顶点数
    - non-manifold 顶点数
    - sharp edge 顶点数
    - boundary loop 组件数
    - sharp ring 组件数
    - circular hole loop 数
    - circular hole 顶点数
    - functional boundary 顶点数
    - cylindrical patch 顶点数
    - thin-wall 顶点数
    - protected / critical 顶点数
    - 平均和最大 feature importance

- `src/meshlod/meshlod_simplify.h`
  - 将原来偏“曲率/锐边启发式”的重要性估计，升级为多通道结构化特征识别。
  - 新增 `detectFunctionalBoundaryLoops(...)`，用于从边界段和锐边段中提取 loop / ring 结构。
  - 新增孔洞识别逻辑：
    - 统计 loop 长度、闭合程度、中心、半径一致性和平面一致性。
    - 对近圆形 boundary loop 和 sharp ring 赋予 `circular_hole` 权重。
  - 新增装配功能区识别逻辑：
    - 根据 boundary loop、sharp ring、局部尺度、法线变化和边界复杂度生成 `functional_boundary` 权重。
    - 把孔边、装配接触边、结构开口边、功能轮廓边纳入保护。
  - 新增圆柱/轴孔类区域识别逻辑：
    - 根据法线切向分布、局部曲率、孔洞 loop 关系和环状锐边关系生成 `cylindrical_patch` 权重。
  - 新增薄壁结构识别逻辑：
    - 根据局部三角形尺度、细长三角形、边长各向异性和小尺度结构生成 `thin_wall` 权重。
  - 新增结构化 `importance` 计算：
    - `circular_hole`
    - `functional_boundary`
    - `cylindrical_patch`
    - `thin_wall`
    - `sharp edge`
    - `boundary`
    - `non-manifold`
  - 对孔洞和高功能边界增加更强的 hard protection。
  - 在边 collapse / 误差评估中引入语义特征强度，使涉及孔洞、圆柱、功能边界和薄壁区域的简化代价更高。
  - 生成 `protected` 和 `critical` 两级特征顶点集合，支持后续分析特征保护强度是否过高。

- `src/scene/clusterlod.cpp`
  - 在每个 geometry 调用 LOD 简化时创建 `clodFeatureMetrics`。
  - 将 `inputMesh.feature_metrics` 指向该统计结构。
  - 简化完成后，把每个 geometry 的特征指标累加到 `ProcessingInfo::stats`。

- `src/scene/scene.hpp`
  - `ProcessingInfo::Stats` 新增结构化特征统计字段。
  - 新增 `featureImportanceSumPpm` 和 `featureImportanceMaxPpm`，避免并行累积浮点数时产生非确定性。

- `src/scene/scene.cpp`
  - `ProcessingInfo::logEnd()` 新增 `Feature Retention Stats` 输出。
  - 输出孔洞、圆柱、薄壁、功能边界、protected、critical 和 importance 等指标。

### 变动位置

- 核心算法位置：
  - 文件：`src/meshlod/meshlod_simplify.h`
  - 阶段：mesh LOD simplify 前的特征分析、collapse 权重构造、语义误差约束。

- 数据结构位置：
  - 文件：`src/meshlod/meshlod_types.h`
  - 阶段：`clodMesh` 输入结构和简化统计回传接口。

- 场景统计汇总位置：
  - 文件：`src/scene/clusterlod.cpp`
  - 阶段：每个 geometry 完成 LOD 生成后，把局部特征统计累加到全局处理统计。

- 终端输出位置：
  - 文件：`src/scene/scene.cpp`
  - 阶段：geometry load & processing 结束时输出 `Feature Retention Stats`。

### 实现原理

原来的特征保护主要依赖曲率、锐边和边界这类低层几何启发式。它能够保留一部分高曲率边和明显锐边，但对工业模型中真正重要的结构语义不够敏感。例如：

- 小孔和轴孔在低 LOD 下可能被错误合并。
- 薄壁、板件和壳体边缘可能被简化成不合理形状。
- 圆柱、管孔、螺栓孔等功能面容易被普通曲率误差低估。
- 装配接触边、开口边和功能边界无法和普通倒角边区分。

本次更新把特征识别从单一启发式升级为多通道结构化判断：

1. 构建边邻接和顶点邻接关系。
2. 从 boundary edge 和 sharp edge 中提取连续段。
3. 对连续段进行 loop / ring 分析。
4. 对 loop 估计中心、半径、闭合程度、半径方差和平面一致性。
5. 如果 loop 近似圆形，则标记为 circular hole。
6. 对 boundary loop、sharp ring 和孔洞周边赋予 functional boundary 权重。
7. 结合法线变化、局部尺度和环状结构估计 cylindrical patch。
8. 根据局部三角形尺度、细长性、边长各向异性和小尺度连接关系估计 thin-wall。
9. 将多通道特征合成为 feature importance。
10. 根据 importance 生成 protected / critical 顶点集合。
11. 在 collapse 误差中提高涉及结构化特征的边的代价，降低孔洞、薄壁和功能边界被过度合并的概率。

这样做的目标不是让所有锐边都绝对不变，而是让算法更接近工业模型的实际需求：

- 孔洞和轴孔属于功能结构，应优先保护。
- 薄壁结构代表真实制造形态，应防止低 LOD 过度坍缩。
- 圆柱和管孔面应保持截面语义。
- 装配功能区边界比普通装饰性锐边更重要。

### 指标含义

- `Circular hole loops`
  - 识别出的圆形或近圆形孔洞 loop 数。
  - 数值越高，说明模型中孔洞、轴孔、螺栓孔或环状功能边越多。

- `Circular hole vertices`
  - 被孔洞 loop 影响的顶点数。
  - 该比例可以衡量孔洞结构在模型特征中的占比。

- `Functional boundaries`
  - 被认为具有装配、开口、接触、孔边或功能轮廓意义的边界顶点数。
  - 高比例通常出现在机械板件、装配面、壳体开口和复杂支架中。

- `Cylindrical vertices`
  - 被识别为圆柱、轴孔、管面或近似圆柱 patch 的顶点数。
  - 如果孔洞很多但该值为 0，通常说明模型只暴露孔边 loop，缺少连续圆柱侧壁，或者圆柱检测阈值偏严格。

- `Thin-wall vertices`
  - 薄壁、板件、壳体或小尺度细长结构相关顶点数。
  - 该值过高时，说明模型简化空间较小，需要谨慎降低保护强度。

- `Protected vertices`
  - 当前策略认为应强保护的顶点。
  - 如果达到 100%，说明保护过强，LOD 简化空间会明显下降。

- `Critical vertices`
  - 比 protected 更关键的高重要度顶点。
  - 适合用于判断孔洞、功能区和高风险结构是否被过度保护或保护不足。

- `Avg importance` / `Max importance`
  - 特征重要性平均值和最大值。
  - 如果平均值接近 1，说明模型中大部分 feature 都被判为高优先级，后续应考虑保护分级。

### 影响

- 对大型工业模型的结构保持更有针对性，尤其是孔洞、薄壁、轴孔、功能边界和装配区域。
- 能解释为什么某些模型 LOD 效果不明显：如果 `Protected vertices` 或 `Avg importance` 过高，说明简化器被结构约束强烈限制。
- 能解释为什么某些模型孔洞仍会消失：如果 `Circular hole vertices` 有值但低 LOD 仍丢孔，问题更可能在 collapse 约束强度或 LOD 选择策略，而不是识别阶段。
- 会增加 CPU 预处理阶段的特征分析成本，但运行时 GPU traversal 不直接增加负担。
- 对没有明显工业结构的普通模型，特征权重会更接近普通曲率/锐边逻辑。
- 对拓扑干净、孔洞清晰的机械零件，特征识别更稳定。
- 对保护比例极高的模型，可能降低简化率，需要后续引入保护分级和上限约束。

### 建议验证方式

1. 使用同一模型删除旧 cache 后重新生成，观察终端输出：

```text
Feature Retention Stats
Circular Hole Loops
Circular Hole Vertices
Functional Boundaries
Cylindrical Vertices
Thin-Wall Vertices
Protected Vertices
Critical Vertices
Avg Feature Importance
Max Feature Importance
```

2. 使用右下角 UI 的 `Feature Retention Summary` 和 `Feature Retention Output` 对比不同模型：
   - 小机械零件
   - 多孔板件
   - 大型装配模型
   - 薄壁壳体模型
   - 管线/圆柱结构模型

3. 对同一模型做 A/B 测试：

```powershell
.\_bin\Release\t1.exe --scene E:\vk_lod_clusters1\1\_downloaded_resources\c.glb
```

对比指标：

- `Circular hole loops`
- `Circular hole vertices`
- `Functional boundaries`
- `Thin-wall vertices`
- `Protected vertices`
- `Critical vertices`
- `Avg importance`
- 低 LOD 下孔洞是否消失
- 远距离模型是否仍有明显孔洞/薄壁坍缩
- FPS 和 rendered clusters 是否发生明显变化

4. 判断策略问题：
   - 如果 `Circular hole vertices` 很高，但孔洞仍消失，说明识别有效，约束或 LOD 策略还需加强。
   - 如果 `Protected vertices` 接近 100%，说明保护过强，后续应降低普通 sharp edge 权重，保留孔洞和功能区硬保护。
   - 如果 `Circular hole loops` 很低，但模型肉眼有很多孔，说明孔洞 loop 检测阈值或拓扑输入需要继续优化。

### 后续优化方向

- 引入保护分级：
  - 孔洞 loop：硬保护。
  - 装配功能边界：强保护。
  - 薄壁边界：强保护或厚度误差约束。
  - 圆柱 patch：半径/轴向误差约束。
  - 普通锐边：从硬保护降为误差加权。

- 增加简化后保持率：
  - 孔洞 loop 保留率。
  - 圆孔半径误差。
  - 孔中心漂移误差。
  - 锐边长度保持率。
  - 薄壁厚度误差。
  - protected / critical 顶点 surviving ratio。

- 增加保护上限策略：
  - 当 `Protected vertices` 超过一定比例时，自动降低普通 sharp edge 权重。
  - 保持孔洞和装配功能区优先级不变，释放重复倒角、小锐边和低风险边的简化空间。

### 构建验证

该算法更新已随当前 Release 版本完成构建验证：

```powershell
cmake --build build --config Release
```

结果：

- 成功生成：`_bin\Release\t1.exe`
- 仍存在既有 warning：`src\streaming\streamutils.cpp` 的 C4307 整型常量溢出警告。
- 仍存在既有 post-build 提示：`pwsh.exe` 未找到。

主要改 [src/meshlod/meshlod_simplify.h](E:/vk_lod_clusters1/t1/src/meshlod/meshlod_simplify.h:553)。保护过激一般不是一个开关造成的，而是三层叠加：

1. `importance` 算得太高  
2. 高 `importance` 被转成 hard lock  
3. `featureAdaptiveTarget()` 自动把简化目标放宽，导致保留三角形太多

我建议按这个顺序改。

**优先改 1：降低普通锐边/功能边界保护**
现在最容易过激的是这些位置：

```cpp
if (circular_hole[v] > 0.35f)
  importance[v] = std::max(importance[v], clamp01(0.88f + circular_hole[v] * 0.12f));
else if (functional_boundary[v] > 0.55f)
  importance[v] = std::max(importance[v], clamp01(0.68f + functional_boundary[v] * 0.18f));

if (cylindrical_patch[v] > 0.55f)
  importance[v] = std::max(importance[v], clamp01(0.58f + cylindrical_patch[v] * 0.22f));
```

如果你觉得保护太强，先不要动孔洞，把功能边界和圆柱降一点：

```cpp
else if (functional_boundary[v] > 0.65f)
  importance[v] = std::max(importance[v], clamp01(0.55f + functional_boundary[v] * 0.15f));

if (cylindrical_patch[v] > 0.65f)
  importance[v] = std::max(importance[v], clamp01(0.48f + cylindrical_patch[v] * 0.18f));
```

孔洞这段建议先保留，因为你之前主要担心孔洞消失。

**优先改 2：提高 hard lock 阈值**
在 [src/meshlod/meshlod_simplify.h](E:/vk_lod_clusters1/t1/src/meshlod/meshlod_simplify.h:785)：

```cpp
float lock_threshold = 0.9f - 0.25f * clamp01(config.silhouette_preservation);
```

如果 `silhouette_preservation = 1`，阈值会变成 `0.65`，这很激进。建议改成：

```cpp
float lock_threshold = 0.95f - 0.15f * clamp01(config.silhouette_preservation);
```

这样阈值范围从 `0.65-0.90` 变成 `0.80-0.95`，只有真正高重要度点才会 hard lock。

**优先改 3：降低 adaptive target 的保守程度**
在 [src/meshlod/meshlod_simplify.h](E:/vk_lod_clusters1/t1/src/meshlod/meshlod_simplify.h:766)：

```cpp
float pressure = clamp01(avg_feature * 0.45f + max_feature * 0.25f + critical_ratio * 0.22f + protected_ratio * 0.08f);
```

可以改成：

```cpp
float pressure = clamp01(avg_feature * 0.30f + max_feature * 0.15f + critical_ratio * 0.18f + protected_ratio * 0.04f);
```

再把下面这段：

```cpp
size_t critical_floor = target_count + size_t(float(indices.size() - target_count) * clamp01(0.35f + strength * (0.25f + critical_ratio)));
```

改成更温和：

```cpp
size_t critical_floor = target_count + size_t(float(indices.size() - target_count) * clamp01(0.20f + strength * (0.18f + critical_ratio * 0.5f)));
```

这会让高特征区域仍然保守，但不会一看到很多 protected/critical 就大幅减少简化。

**最推荐的策略**
别整体削弱孔洞。应该这样分级：

- 孔洞 loop：继续强保护
- 装配功能边界：中强保护
- 薄壁：中等保护
- 圆柱 patch：中等保护
- 普通 sharp edge：从硬保护降成误差加权
