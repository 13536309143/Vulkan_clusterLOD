# Src 与 Shaders 代码作用说明

本文按目录说明 `src` 与 `shaders` 下每个代码文件的职责。整体上，CPU 侧负责加载 glTF、生成/缓存 Cluster LOD 数据、维护 Vulkan 资源、驱动遍历和渲染；GPU 侧 shader 负责 LOD 遍历、遮挡/视锥剔除、硬件 Mesh Shader 渲染、可选 Compute 软件光栅、Hi-Z 构建和流式加载状态更新。

## src/app

`src/app` 是应用层。它不直接实现 Cluster LOD 算法本身，而是把命令行参数、场景加载、GPU 资源、Renderer、UI 和每帧调度串起来。

### `src/app/main.cpp`

程序入口。主要职责：

- 创建 `nvapp::ApplicationCreateInfo`，设置窗口名、菜单、VSync、headless 参数。
- 配置 Vulkan instance/device 扩展和特性链，包括 mesh shader、shader clock、atomic float、fragment shading rate、barycentric、cluster acceleration structure 等。
- 创建 profiler、camera manipulator、parameter registry/parser、sequencer。
- 创建 `LodClusters` 应用元素，并把命令行参数注册到 parser。
- 支持 `processingOnly` 模式：只处理场景并生成缓存，不创建窗口和 Vulkan swapchain。
- 创建 Vulkan context、初始化 validation settings、选择物理设备、创建逻辑设备。
- 把 Vulkan instance/device/queue 信息交给 `nvapp`。
- 配置 ImGui docking 布局，将 Debug、Settings、Log、Profiler、Streaming memory、Statistics 等窗口固定到默认区域。
- 添加应用元素：sequencer、窗口标题、`LodClusters`、日志、相机控件、profiler UI。

### `src/app/lodclusters.hpp`

`LodClusters` 顶层应用元素的声明。主要内容：

- 定义 renderer 类型、cluster 配置枚举、UI 枚举、截图模式。
- 定义 `Tweak`、`ViewPoint`、`TargetImage`、`Info` 等应用层配置结构。
- 继承 `nvapp::IAppElement`，声明 `onAttach`、`onDetach`、`onUIMenu`、`onUIRender`、`onPreRender`、`onRender`、`onResize`、`onFileDrop` 等生命周期回调。
- 持有全局应用状态：`Resources`、`FrameConfig`、`Scene`、`RenderScene`、`Renderer`、streaming config、scene config、UI registry、profiler、camera 参数等。
- 私有 helper 按职责分组：场景加载/缓存、RenderScene 和 Renderer 生命周期、Framebuffer 到 ImGui 的桥接、运行时配置变更、拾取辅助和 UI 绘制。

### `src/app/lodclusters.cpp`

`LodClusters` 实现索引文件。当前不承载业务逻辑，只说明 `LodClusters` 的实现已经拆到多个职责明确的编译单元：

- `lodclusters_config.cpp`
- `lodclusters_scene.cpp`
- `lodclusters_lifecycle.cpp`
- `lodclusters_runtime.cpp`
- `lodclusters_ui.cpp`

保留此文件可以让旧项目结构仍有一个可读的入口说明。

### `src/app/lodclusters_config.cpp`

`LodClusters` 构造和默认配置。主要职责：

- 定义全局 `g_verbose`。
- 创建 profiler timeline。
- 注册命令行和配置文件参数，例如 `scene`、`renderer`、`supersample`、`streaming`、`clusterconfig`、LOD simplification 参数、streaming 内存限制、culling 开关、SW raster 开关、缓存策略、processing-only 参数等。
- 初始化 `FrameConfig::frameConstants` 的默认值：线框颜色、AO、visualize 模式、facet shading、sun/sky 参数、时间字段、LOD transition speed。
- 初始化 SW raster effective threshold 和 scene loading progress 指针。

### `src/app/lodclusters_scene.cpp`

场景、缓存、RenderScene 和相机相关逻辑。主要职责：

- `initScene()`：异步加载 `.gltf`、`.glb` 或场景配置对应的模型，调用 `Scene::init()` 生成或读取 Cluster LOD 数据。
- 处理加载完成后的 scene config 同步、grid 更新、cluster preset 回写、是否可 preload 的判断。
- `initRenderScene()` / `deinitRenderScene()`：创建或释放 `RenderScene`。预加载失败时自动回退到 streaming。
- `deinitScene()`：释放 RenderScene 和 Scene。
- `postInitNewScene()`：根据 scene bbox 初始化光源、scene size、相机、streaming group 下限和 sky 参数。
- `saveCacheFile()`：将当前 scene 的处理结果保存到缓存文件。
- `onFileDrop()`：处理拖入模型文件或 `.cfg` 配置文件；配置文件会通过 parameter parser 解析，并可能递归加载配置中指定的模型。
- `doProcessingOnly()`：无窗口模式下只处理模型并生成缓存。
- `s_clusterInfos`、`findSceneClusterConfig()`、`setFromClusterConfig()`：在 UI/命令行 cluster preset 和 `SceneConfig` 的三角形/顶点上限之间转换。
- `updatedSceneGrid()`：根据 grid bbox 和模型 bbox 更新相机速度和裁剪面。
- `setSceneCamera()`：使用 glTF 相机或 bbox 自动设置相机，并注册 home camera。
- `decodePickingDepth()`、`isPickingValid()`：从 GPU readback 解码拾取深度。

### `src/app/lodclusters_lifecycle.cpp`

应用生命周期和 renderer 生命周期。主要职责：

- `onResize()`：窗口大小改变时重建 framebuffer，刷新 ImGui viewport texture，并通知 renderer 更新 framebuffer descriptor。
- `updateImguiImage()`：把当前 color image 或 resolved color image 注册为 ImGui texture。
- `initRenderer()` / `deinitRenderer()`：创建、编译、释放当前 renderer。当前 renderer factory 是 `makeRendererRasterClustersLod()`。
- `onAttach()`：应用元素挂载时初始化 UI enum、profiler GPU timer、`Resources`、sampler、初始 framebuffer、默认模型路径和首个 scene load。
- `onDetach()`：等待 GPU idle，释放 renderer、scene、sampler、ImGui texture、Resources 和 profiler GPU timer。
- `parameterSequenceCallback()`：sequencer 每段结束时输出内存和遍历统计，并按配置保存窗口或 viewport 截图。

### `src/app/lodclusters_runtime.cpp`

运行时变更处理和每帧渲染入口。主要职责：

- `resetSwRasterFeedback()` / `updateSwRasterFeedback()`：根据上一帧 readback 中 SW/HW cluster 和 triangle 占比，自动调节 compute raster 路由阈值。
- `onPreRender()`：每帧渲染前更新 SW raster feedback，并推进 profiler timeline。
- `handleChanges()`：集中处理 UI/命令行导致的状态变化：
  - 文件路径变化触发重新加载。
  - 不支持 NV mesh shader 时强制使用 EXT mesh shader。
  - 根据 visualize 模式切换 shading/depth-only。
  - 约束 compute raster 的前置条件。
  - supersample 改变时重建 framebuffer。
  - scene config、grid config、streaming config、renderer config 改变时重建 Scene、RenderScene 或 Renderer。
  - 记录 last config，并按需重置 profiler。
- `applyCameraString()`：把字符串形式的相机参数应用到 camera manipulator。
- `onRender()`：每帧 CPU 渲染入口：
  - `Resources::beginFrame()`。
  - 更新 `FrameConstants`：时间、viewport、clip planes、projection/view matrix、view size、camera/light、Hi-Z 参数。
  - 更新 traversal/culling 矩阵，处理 freeze LoD/culling。
  - 调用 `Renderer::render()` 或空帧绘制。
  - 后处理、结束 frame、提交 timeline semaphore。

### `src/app/lodclusters_ui.cpp`

全部 ImGui UI。主要职责：

- `formatMemorySize()`、`formatMetric()`：格式化内存和计数。
- `uiPlot()`：用 ImPlot 绘制 streaming memory 等历史曲线。
- `UsagePercentages`：计算 renderer/streaming 使用率和 warning 文案。
- `viewportUI()`：在 viewport 中更新鼠标位置到 `FrameConstants`，显示资源限制 warning。
- `onUIRender()`：绘制所有运行时 UI：
  - loading modal。
  - Settings：rendering、traversal、cluster、streaming 等控制项。
  - Streaming memory：流式几何内存历史图。
  - Statistics：scene、traversal、memory、cluster histogram。
  - Misc Settings：camera、lighting、advanced。
  - Debug：shader readback 调试值。
  - Viewport：显示渲染结果 texture。
- `onUIMenu()`：菜单栏和快捷键，支持打开文件、reload、save/delete cache、reload shaders、退出、VSync。

## src/core

`src/core` 放跨模块基础功能，目前主要是缓存序列化。

### `src/core/serialization.hpp`

二进制缓存序列化辅助。主要职责：

- 定义缓存对齐常量 `ALIGNMENT` 和 `ALIGN_MASK`。
- `getCachedSize()`：计算 `std::span<T>` 按 16 字节对齐后的存储大小。
- `storeAndAdvance()`：把 span 的内容写入目标内存，并推进写指针。
- `loadAndAdvance()`：从缓存内存中恢复 span 视图，并推进读指针。

该文件只处理“视图数据如何连续存储/恢复”，不理解 scene 语义。

### `src/core/cache.cpp`

`Scene` 缓存文件实现。主要职责：

- `Scene::storeCached()` / `Scene::loadCached()`：序列化/反序列化单个 `GeometryView`。
- `Scene::CacheFileView`：解析完整缓存文件的 header、geometry 数据段和 offset table。
- `checkCache()`：判断某个 geometry 是否能从 cache 复用。
- `loadCachedGeometry()`：把 cache 中的 geometry view 接到 scene runtime view。
- `openCache()` / `closeCache()`：打开、关闭缓存或 memory mapped cache。
- `saveCache()`：保存完整 scene cache。
- processing-only 相关：
  - `beginProcessingOnly()`
  - `saveProcessingOnly()`
  - `endProcessingOnly()`
  用于大模型离线预处理和断点/部分保存。

## src/scene

`src/scene` 是 CPU 侧场景数据模型、glTF 导入、LOD 构建、压缩和实例化逻辑。

### `src/scene/scene.hpp`

场景核心数据结构声明。主要内容：

- `SceneConfig`：影响生成结果的参数，例如 cluster 大小、group size、LOD error、simplification 权重、压缩开关、属性开关、特征边/曲率/轮廓保留等。
- `SceneLoaderConfig`：影响加载过程的参数，例如线程比例、processing-only、auto cache、memory mapped cache、preprocess 阈值。
- `SceneGridConfig`：用于 benchmark 的 scene grid 复制配置。
- `Scene`：
  - `GroupInfo`、`GroupView`、`GroupStorage`：cluster group 的 runtime view 和 build-time storage。
  - `GeometryBase`、`GeometryView`、`GeometryStorage`：geometry 的只读 runtime 数据和可写处理数据。
  - `Instance`、`Camera`：实例和相机。
  - `Histograms`：cluster/LOD/group 统计。
  - cache、processing、glTF loading、LOD building、compression、bbox 和 histogram helper 的声明。

### `src/scene/scene.cpp`

`Scene` 主流程实现。主要职责：

- `ProcessingInfo`：初始化线程数、并行策略、压缩 glTF buffer view 状态、进度日志和统计。
- `Scene::init()`：场景加载总入口。负责选择 cache、glTF load、preprocess、cache 保存、runtime view 建立等流程。
- `fillGroupRuntimeData()`：把 CPU group view 转换成 shader 需要的 group/cluster runtime 数据布局。
- `updateSceneGrid()`：复制/变换实例，生成 benchmark grid，并维护 active geometry 数量。
- `computeInstanceBBoxes()`：计算实例 bbox。
- `processGeometry()`：对单个 geometry 执行 cache 检查、LOD 构建、压缩和统计。
- `computeLodBboxes_recursive()`：递归计算 LOD tree node bbox。
- `buildGeometryDedupVertices()`：对 geometry 顶点范围进行去重。
- `computeHistogramMaxs()`：更新统计直方图最大值。

### `src/scene/scene_gltf.cpp`

glTF 导入实现。主要职责：

- 集成 cgltf 的文件读取回调和 buffer mapping 管理。
- `Scene::loadGLTF()`：读取 glTF 文件、解析 mesh/material/camera/node，准备 geometry 和 instance。
- `addInstancesFromNodeGLTF()`：递归遍历 glTF node tree，把 mesh node 转成 `Scene::Instance`。
- `loadCompressedViewsGLTF()` / `unloadCompressedViewsGLTF()`：处理 `EXT_meshopt_compression` buffer view。
- `readAttributesGLTF()`：从 accessor 读取 position、normal、tangent、texcoord 等属性，可按配置量化。
- `loadGeometryGLTF()`：读取单个 mesh primitive，生成 triangle index、vertex positions、vertex attributes、material 映射和 bbox。

### `src/scene/clusterlod.cpp`

把本项目的 `Scene::GeometryStorage` 接到 mesh LOD 构建器的实现文件。主要作用：

- 实现 `Scene::buildGeometryLod()` 和 `Scene::buildHierarchy()` 相关的 Cluster LOD 构建逻辑。
- 调用 `meshlod`/`clodBuild` 生成 cluster、group、LOD node、LOD level 等数据。
- 把算法输出整理成 `Scene` 的 group storage、node storage、histogram 和 traversal metric。

### `src/scene/scene_cluster_compression.cpp`

cluster group 压缩/解压实现。主要职责：

- 定义 bitstream、算术压缩/解压工具。
- `Scene::compressGroup()`：将 group 的 vertex/index/bbox/cluster 数据压缩成更小的存储格式。
- `Scene::decompressGroup()`：运行时或上传前把压缩 group 解回 shader 需要的布局。
- 压缩目标是降低 cache、系统内存或 GPU 上传数据量，渲染逻辑仍通过统一的 group runtime layout 消费。

## src/meshlod

`src/meshlod` 是独立的 Cluster LOD 构建算法层，命名空间主要是 `clod`。它不依赖 Vulkan，输入普通 mesh，输出 cluster/group/LOD 结构。

### `src/meshlod/meshlod.h`

公共 C/C++ API。主要内容：

- 包含 `meshlod_types.h`。
- 声明 `clodBuild()`、`clodBuild_iterationTask()`、`clodLocalIndices()`。
- 提供 C++ lambda callback 包装版本，方便调用方直接传输出回调。

### `src/meshlod/lod.h`

较旧或合并式的 header-only LOD 实现/兼容文件。包含类型、构建、局部索引等逻辑的集中版本。当前项目主要通过拆分后的 `meshlod_*.h` 与 `meshlod.cpp` 使用同一套能力。

### `src/meshlod/meshlod.cpp`

`meshlod` 编译单元。主要作用：

- 汇入/实例化拆分 header 中的实现代码。
- 让 `clodBuild()`、`clodBuild_iterationTask()`、`clodLocalIndices()` 这些 API 以正常 `.cpp` 方式参与链接。

### `src/meshlod/meshlod_types.h`

算法公共数据结构。主要内容：

- `clodConfig`：cluster/group/LOD 构建参数。
- `clodMesh`：输入 mesh 顶点、索引、属性和锁定信息。
- `clodBounds`：bbox。
- `clodCluster`：输出 cluster。
- `clodGroup`：输出 group。
- 回调类型：输出 cluster/group 和并行 iteration。

### `src/meshlod/meshlod_config.h`

算法内部配置和常量。用于统一默认参数、限制值和编译期开关，避免构建逻辑中散落 magic number。

### `src/meshlod/meshlod_bounds.h`

bbox 和几何范围辅助。主要用于：

- 从顶点计算 bbox。
- 合并 bbox。
- 为 cluster/group/node 生成空间范围。
- 计算 LOD error 或简化过程需要的空间尺度。

### `src/meshlod/meshlod_clustering.h`

cluster 分组和边界锁定逻辑。主要职责：

- 基于 meshoptimizer clusterize 结果组织 cluster/group。
- `lockBoundary()`：识别和锁定边界，避免简化时破坏需要保留的拓扑边界。
- 为后续 LOD 层级构建提供 cluster adjacency、group 划分等中间数据。

### `src/meshlod/meshlod_build.h`

LOD 构建主算法。主要职责：

- `clodBuild_iterationTask()`：单个并行任务的构建逻辑。
- `clodBuild()`：从输入 mesh 生成多级 cluster LOD 输出。
- 管理每轮 decimation、cluster grouping、输出回调和 LOD level 迭代。

### `src/meshlod/meshlod_impl.h`

算法内部类型和函数声明。主要内容：

- `clod::Cluster`、`SloppyVertex`、`IterationContext` 等内部结构。
- 声明 boundary lock、perceptual error、fallback simplification 等内部 helper。
- 为多个拆分 header 共享内部状态定义。

### `src/meshlod/meshlod_simplify.h`

mesh 简化实现。主要职责：

- 特征边、曲率、感知权重、轮廓保留相关逻辑。
- `perceptualError()`：将几何误差按视觉/规模权重转换为 LOD traversal metric。
- `simplifyFallback()`：在常规 simplification 不满足目标时提供 fallback。
- 与 `SceneConfig` 中的 feature-aware simplification 参数对应。

### `src/meshlod/meshlod_local_indices.h`

cluster 局部索引重排工具。主要职责：

- `clodLocalIndices()`：把全局 index buffer 转换为 cluster 局部顶点表和 8-bit triangle index。
- 降低 group 数据体积，并匹配 shader 侧 `Cluster`/`Group` 的局部索引访问模式。

## src/renderer

`src/renderer` 是 Vulkan 资源、渲染场景上传和 Cluster LOD renderer。

### `src/renderer/resources.hpp`

Vulkan 资源管理声明。主要内容：

- `FrameConfig`：每帧渲染控制，包括窗口大小、LOD error、culling freeze、streaming age、SW raster threshold、visualize 模式和 `FrameConstants`。
- `BufferRanges`：构建大 buffer 时用的偏移分配辅助。
- `QueueState` / `QueueStateManager`：timeline semaphore 和跨 queue 同步状态。
- `Resources`：
  - Vulkan device、allocator、sampler pool、staging uploader、shader compiler。
  - framebuffer images：color、resolved color、depth/stencil、raster atomic、双 Hi-Z。
  - common buffers：frame constants 和 readback。
  - shader 编译、pipeline 销毁、temporary command buffer、rendering begin、image transition、buffer 创建等 helper。
- `BatchedUploader`：批量 staging upload helper。
- `MemoryPool`：临时内存块池。

### `src/renderer/resources.cpp`

`Resources` 实现。主要职责：

- 初始化/释放 allocator、sampler、shader compiler、queue state、common buffers、Hi-Z、radix sorter。
- `initFramebuffer()`：根据窗口和 supersample 设置 render/target size，创建 framebuffer image 和 Hi-Z 资源。
- `updateFramebufferRenderSizeDependent()`：上传 framebuffer size 相关状态并 transition image。
- `postProcessFrame()`：执行 frame 末尾处理，例如 resolved color、Hi-Z build 之后的 image layout。
- `emptyFrame()`：无 renderer 时清空输出。
- `getReadbackData()`：从 GPU readback host buffer 取统计/调试数据。
- `cmdBuildHiz()`：调用 `NVHizVK` 生成 Hi-Z mip 链。
- `compileShader()`：编译 GLSL shader，注入 include path/宏，并可 dump SPIR-V。
- temporary command buffer 和 timeline semaphore 提交。
- `trackMemoryUsage()` / `logMemoryUsage()`：资源内存统计。

### `src/renderer/renderer.hpp`

renderer 抽象声明。主要内容：

- `RenderScene`：封装同一个 `Scene` 的两种 GPU 驻留方式：
  - `ScenePreloaded`
  - `SceneStreaming`
- `RendererConfig`：renderer 开关，例如 culling、sorting、render stats、two-pass culling、EXT mesh shader、compute raster、primitive culling、depth-only 等。
- `Renderer` 虚基类：
  - `init()`、`render()`、`deinit()`、`updatedFrameBuffer()`。
  - shared descriptor/pipeline/helper：fullscreen background、atomic raster resolve、instance bbox、cluster bbox。
  - 资源用量统计和最大 traversal/render cluster 上限。
- `makeRendererRasterClustersLod()` factory。

### `src/renderer/renderer.cpp`

renderer 共享实现。主要职责：

- `RenderScene::init()`：根据 `useStreaming` 初始化 preload 或 streaming。
- `RenderScene::getShaderGeometriesBuffer()`、`getGeometrySize()`、`getOperationsSize()`：统一访问 preload/streaming 的 GPU 数据。
- `Renderer::initBasicShaders()`：编译 fullscreen、background、atomic resolve、bbox debug shader。
- `Renderer::initBasics()`：创建 render instance buffer，上传 instance matrix/material/geometry mapping，按需创建 sorting aux buffer。
- `Renderer::initBasicPipelines()`：创建 debug bbox、background、atomic raster resolve pipeline。
- `updateBasicDescriptors()`：绑定 frame/readback/raster atomic/geometries/render instances/scene building/streaming buffers。
- `renderInstanceBboxes()`、`renderClusterBboxes()`、`writeAtomicRaster()`、`writeBackgroundSky()`：共享绘制 helper。

### `src/renderer/renderer_clusters_lod.cpp`

Cluster LOD 主 renderer。主要职责：

- 定义 `RendererRasterClustersLod`。
- `initShaders()`：根据 renderer config 和 scene 属性编译主 render/traversal/build/SW raster shader，并注入宏：
  - cluster 顶点/三角形上限
  - streaming/sorting/culling/two-pass/separate-groups
  - EXT/NV mesh shader
  - vertex normal/tangent/texcoord availability
  - shading/depth-only/debug visualization
  - compute raster/adaptive routing
- `init()`：
  - 初始化 shared renderer 基础资源。
  - 创建 `SceneBuilding` buffer、traversal buffer、render cluster info buffer、sorting buffer、two-pass visibility buffer 等。
  - 创建 traversal/render descriptor set、pipeline layout、compute pipeline 和 graphics mesh pipeline。
  - 更新 streaming binding。
- `render()`：核心 GPU 命令顺序：
  - 上传 `FrameConstants` 和 `SceneBuilding`。
  - 清空 readback、traversal buffer、可选 raster atomic image。
  - streaming begin/pre-traversal。
  - pass 循环：presort、traversal init、build setup、persistent traversal、separate group traversal、draw setup。
  - 可选 compute SW raster。
  - hardware mesh shader draw。
  - atomic raster fullscreen resolve。
  - debug bbox。
  - Hi-Z build。
  - streaming post/end。
  - 更新资源统计。
- `updatedFrameBuffer()`：framebuffer 重建后更新 raster atomic 和 Hi-Z descriptor。
- `deinit()`：释放 pipeline、descriptor、scene buffers 和 shared basics。

### `src/renderer/preloaded.hpp`

预加载 GPU scene 声明。主要内容：

- `ScenePreloaded`：一次性把全部 scene geometry/group 数据上传到 GPU。
- 提供 `canPreload()`、`init()`、`deinit()`、shader geometry buffer 和资源大小查询。

### `src/renderer/preloaded.cpp`

预加载实现。主要职责：

- `canPreload()`：根据 scene 数据量和 device local heap 判断是否适合一次性上传。
- `init()`：创建 geometry buffer、group data buffer、cluster data buffer 等，填充 shader 可访问的 `Geometry`、group 和 cluster 地址。
- `deinit()`：释放预加载 buffer。

### `src/renderer/hiz.hpp`

Hi-Z 生成器声明。主要内容：

- `NVHizVK` 类，封装 Hi-Z 纹理、descriptor、pipeline 和 update 参数。
- `TextureInfo`：shader 所需的尺寸因子。
- `Update`：一次 Hi-Z update 的 source/destination view、mip 信息和 descriptor。
- `Config`：格式、mip 层数、near/far 输出、reversed Z 等配置。

### `src/renderer/hiz.cpp`

Hi-Z 实现。主要职责：

- 初始化/释放 descriptor set layout、descriptor pool、compute pipeline。
- `setupUpdateInfos()`：根据 framebuffer depth image 设置 Hi-Z update 参数。
- `initUpdateViews()` / `deinitUpdateViews()`：为 Hi-Z mip 层创建 image view。
- `updateDescriptorSet()`：更新 Hi-Z compute shader 的 descriptor。
- `cmdUpdateHiz()`：dispatch `shaders/post/hiz.comp.glsl`，生成用于遮挡剔除的层级深度。

## src/streaming

`src/streaming` 管理 GPU 几何数据按需驻留、加载、卸载、压缩和状态更新。

### `src/streaming/streamutils.hpp`

streaming 基础数据结构声明。主要内容：

- `StreamingConfig`：传输内存、BLAS cache、geometry memory、resident group 上限、每帧加载/卸载请求上限。
- `StreamingStats`：resident groups/clusters、内存、传输、load/unload 统计。
- `GeometryGroup`：geometry/group 组合 key。
- `StreamingRequests`：GPU 产生的 load/unload request buffer。
- `StreamingResident`：resident group/cluster 状态和分配信息。
- `StreamingAllocator`：GPU 侧空闲区间和分配任务。
- `StreamingUpdates`：scene geometry address 更新和 BLAS/cache 相关任务数据。
- `StreamingStorage`：实际 geometry data buffer 和 staging transfer 管理。
- `StreamingTaskQueue`：CPU 侧异步任务队列。

### `src/streaming/streamutils.cpp`

streaming helper 实现。主要职责：

- 初始化/释放 requests、resident、allocator、updates、storage 的 GPU buffer。
- 管理 resident group 增删、统计和 shader 数据同步。
- 维护 allocator task、free gaps、load/unload buffer range。
- 执行 transfer staging append 和 upload command。
- 判断 storage 是否有足够空间、分配/free geometry data。
- 生成 streaming stats 给 UI 和 sequencer。

### `src/streaming/streaming.hpp`

`SceneStreaming` 高层声明。主要内容：

- 管理一个 scene 的 streaming 模式 GPU 数据。
- 暴露 init/deinit/reset、shader binding 更新、frame begin/pre-traversal/post-traversal/end、stats、geometry size 等接口。

### `src/streaming/streaming.cpp`

`SceneStreaming` 主流程实现。主要职责：

- `init()`：创建 streaming buffer、geometry buffer、request/resident/update/storage/allocator 组件，编译 streaming shader pipeline。
- `initGeometries()`：初始化每个 geometry 的 shader 记录和 streaming group address。
- `cmdBeginFrame()`：处理上一帧任务、上传完成数据、准备 request/update 状态。
- `handleCompletedRequest()`：根据 GPU request 执行 CPU/GPU 数据加载、resident 状态更新、storage 分配。
- `cmdPreTraversal()`：运行 streaming setup，保证 traversal 前地址和状态可用。
- `cmdPostTraversal()`：读取 traversal 产生的 request，执行 age filter、load/unload request compaction。
- `cmdEndFrame()`：提交 streaming update、transfer 同步、清理任务状态。
- `initShadersAndPipelines()` / `deinitShadersAndPipelines()`：编译和释放 streaming compute pipeline。
- `reset()`：重置 streaming resident 和 geometry address。

## src/vendor

### `src/vendor/cgltf.cpp`

第三方 cgltf 单文件实现编译单元。主要作用：

- 定义 cgltf implementation，使 glTF 解析函数在项目中有实现。
- 被 `scene_gltf.cpp` 用于解析 `.gltf` / `.glb` 文件、accessor、buffer view、material、node、camera 等。

## shaders/interface

interface shader 文件同时被 C++ 和 GLSL 使用，定义 CPU/GPU 共享布局、binding 编号、宏和常量。

### `shaders/interface/shaderio_core.h`

最底层共享定义。主要内容：

- CPU/GLSL 两侧统一的 buffer reference 宏。
- 16-bit dispatch grid 线性化 helper。
- packed bitfield helper。
- 基础 buffer reference 类型声明，例如 `uint8s_in`、`uint32s_inout`、`vec3s_in`、`uint64s_coh`。
- indirect dispatch/draw 命令结构。

### `shaders/interface/shaderio_scene.h`

scene runtime 数据布局。主要内容：

- cluster attribute bit。
- cluster/group/node/LOD 限制常量。
- `BBox`、`Cluster`、`TraversalMetric`、`Group`、`NodeRange`、`GroupRange`、`Node`、`LodLevel`、`Geometry`、`RenderInstance`。
- buffer reference 声明，保证 shader 能通过 GPU address 访问 group、cluster、node、geometry 数据。

### `shaders/interface/shaderio_streaming.h`

streaming 共享布局。主要内容：

- streaming request、resident、allocator、update、storage 相关结构。
- GPU 与 CPU 之间传递 load/unload 请求、resident 状态、数据地址和任务统计。
- 被 `streaming.glsl` 和 streaming compute shader 使用。

### `shaders/interface/shaderio_building.h`

每帧构建/遍历共享布局。主要内容：

- `SceneBuilding`，保存 traversal queue、render cluster list、indirect dispatch/draw 参数、sorting buffers、two-pass visibility、SW raster list 等地址和计数。
- 是 `renderer_clusters_lod.cpp` 每帧上传到 GPU 的核心状态结构。

### `shaders/interface/shaderio.h`

总入口 interface header。主要内容：

- include core、scene、streaming、building 和 sky 参数。
- 定义 visualize mode、descriptor binding 编号、build setup command ID、stream setup command ID、workgroup size。
- 定义 culling、streaming、shading、two-sided、SW raster、render stats 等默认编译宏。
- 定义 `FrameConstants`：每帧 view/proj、viewport、LOD、culling、sky/light、debug、mouse、SW raster 参数。
- 定义 `Readback`：GPU 写回 CPU 的统计和 debug 数据。

## shaders/common

common shader 文件是多个 pass 复用的函数库。

### `shaders/common/attribute_encoding.h`

顶点属性编码/解码工具。主要用于：

- 压缩 normal/tangent/texcoord/position 等属性。
- CPU 和 GPU 共享压缩格式。
- 渲染 shader 从 cluster vertex payload 中恢复属性。

### `shaders/common/culling_constants.inc`

culling 相关常量和 helper 定义。作为 include 片段被 `culling.glsl` 等文件使用。

### `shaders/common/culling_frustum.inc`

视锥剔除 helper。主要用于：

- 根据 bbox 和 view-projection matrix 判断 cluster/group/node 是否在视锥内。
- traversal init/run 和 separate groups pass 复用。

### `shaders/common/culling_hiz.inc`

Hi-Z 遮挡剔除 helper。主要用于：

- 将 bbox 投影到屏幕空间。
- 查询 Hi-Z mip 层判断对象是否被前一帧/当前帧深度遮挡。
- 支持 single-pass 和 two-pass culling 的 Hi-Z 纹理选择。

### `shaders/common/culling_raster.inc`

光栅相关 culling helper。主要用于：

- primitive/triangle 层面的背面剔除和屏幕空间估计。
- 与 mesh shader 或 compute raster 路径共享 projected triangle/cluster 判断。
- 处理 two-sided/forced two-sided 对剔除逻辑的影响。

### `shaders/common/culling.glsl`

culling 总入口 include。主要作用：

- include frustum、Hi-Z、raster culling 片段。
- 提供 traversal 和 render shader 统一调用的 culling 函数。
- 根据编译宏启用/禁用不同 culling 路径。

### `shaders/common/render_shading.glsl`

渲染着色函数库。主要职责：

- 根据 `VISUALIZE_*` 模式输出 material、grey、cluster、group、LOD、triangle 等颜色。
- 读取 vertex normal/tangent/texcoord 等属性。
- 实现简单直接光、sun/sky 混合、facet shading、debug visualization。
- 被 `clusters.mesh.glsl` 和 `SWclusters.comp.glsl` 复用。

## shaders/build

### `shaders/build/build_setup.comp.glsl`

间接 dispatch/draw 参数准备 shader。主要职责：

- 根据 `SceneBuilding` 中 traversal/render/SW raster 计数，写入 indirect dispatch 或 mesh draw command。
- `BUILD_SETUP_TRAVERSAL_RUN`：为 persistent traversal run 设置 dispatch。
- `BUILD_SETUP_DRAW`：为硬件 mesh shader draw 和可选 SW raster dispatch 设置 indirect 参数。
- 也预留/支持其他 build setup 类型。

## shaders/traversal

traversal shader 负责从实例的 LOD tree 中选择本帧需要渲染的 cluster，并执行视锥/遮挡剔除、streaming request、可选 sorting 和 separate group 处理。

### `shaders/traversal/traversal.glsl`

遍历共享函数库。主要内容：

- LOD error 计算和 node/group metric 判断。
- traversal task 读写、atomic queue 操作。
- group/cluster enqueue 相关 helper。
- streaming request helper。
- 被 traversal init、run、separate groups shader 复用。

### `shaders/traversal/traversal_presort.comp.glsl`

实例预排序 compute shader。主要职责：

- 为每个 instance 计算 sort key，一般基于深度或距离。
- 输出 key/value buffer，随后 CPU 侧调用 Vulkan radix sort。
- 用于改善 traversal/draw 顺序，提高遮挡和 cache 行为。

### `shaders/traversal/traversal_init.comp.glsl`

traversal 初始化 compute shader。主要职责：

- 每个 instance 检查可见性和基础 LOD 状态。
- 将可遍历的 root node/task 写入 traversal queue。
- 在 two-pass culling 中维护 instance visibility。
- 初始化 traversal 统计和第一批任务计数。

### `shaders/traversal/traversal_run.comp.glsl`

主 LOD traversal compute shader。主要职责：

- 使用 persistent threads 消费 traversal queue。
- 对 node/group 执行 LOD error 判断、视锥/Hi-Z culling。
- 当 `USE_SEPARATE_GROUPS=0` 时也可直接处理底层 group/cluster。
- 当 `USE_SEPARATE_GROUPS=1` 时主要负责内部 node traversal，并把 group work 输出到 separate group queue。
- 处理 streaming 缺页 request、render cluster enqueue、统计 readback。

### `shaders/traversal/traversal_run_separate_groups.comp.glsl`

分离 group 处理 shader。主要职责：

- 消费 `traversalGroupInfos` 中的 group 任务。
- 对 group 内 cluster 做最终可见性判断。
- 根据 cluster 大小/密度和 SW raster 配置，把 cluster 分配到 HW mesh shader list 或 SW raster list。
- 支持 streaming group address 检查和缺页 request。

## shaders/render

render shader 负责把 traversal 输出的 cluster list 转成最终 framebuffer。

### `shaders/render/clusters.mesh.glsl`

硬件 mesh shader 渲染路径。主要职责：

- 读取 `SceneBuilding.renderClusterInfos` 中的 cluster 列表。
- 从 preloaded 或 streaming group 数据中读取 cluster vertices/indices/attributes。
- 输出 mesh shader primitives 和 varyings。
- 执行可选 primitive culling、two-sided 处理、debug visualization 数据输出。
- 与 `frag.glsl` 配合完成正常硬件光栅。

### `shaders/render/frag.glsl`

主 fragment shader。主要职责：

- 接收 mesh shader 输出的 interpolants。
- 调用 `render_shading.glsl` 计算最终颜色。
- 根据 visualize/depth-only/shading 宏选择颜色输出或深度相关输出。

### `shaders/render/SWclusters.comp.glsl`

Compute 软件光栅路径。主要职责：

- 消费 traversal 输出的 SW raster cluster list。
- 在 compute shader 中对 cluster triangles 做屏幕空间 raster。
- 使用 `r64ui` atomic image 写入 visibility/depth/color 编码结果。
- 支持 adaptive SW raster routing、render stats、streaming 数据访问和 culling。
- 后续由 fullscreen atomic resolve pass 合并到正常 framebuffer。

## shaders/post

post shader 负责 fullscreen pass 和 Hi-Z。

### `shaders/post/fullscreen.vert.glsl`

通用 fullscreen triangle vertex shader。主要职责：

- 不依赖 vertex buffer，生成覆盖全屏的三角形。
- 被 background pass、atomic raster resolve pass 等 fullscreen fragment shader 复用。

### `shaders/post/fullscreen_background.frag.glsl`

背景/天空 fragment shader。主要职责：

- 根据 `FrameConstants.skyParams` 和 inverse sky projection 计算背景颜色。
- 在主渲染 pass 开始时写入背景。

### `shaders/post/fullscreen_atomic.frag.glsl`

SW raster atomic resolve fragment shader。主要职责：

- 读取 `imgRasterAtomic` 中 compute raster 写入的 packed 结果。
- 解码深度/颜色/visibility 信息。
- 写入真实 framebuffer，使 SW raster 输出与 HW raster 输出合并。

### `shaders/post/hiz.comp.glsl`

Hi-Z compute shader。主要职责：

- 从 depth texture 或上一层 Hi-Z 读取深度。
- 生成 far/near 层级深度 mip。
- 支持多层级一次 dispatch、reversed-Z、MSAA 和 stereo 配置宏。
- 输出结果被 traversal culling shader 采样，用于遮挡剔除。

## shaders/debug

debug shader 用于可视化 bbox，不影响主渲染数据生成。

### `shaders/debug/render_instance_bbox.mesh.glsl`

实例 bbox mesh shader。主要职责：

- 每个 workgroup 处理多个 render instance。
- 根据 instance matrix 和 geometry bbox 生成线框 bbox。
- 支持 NV/EXT mesh shader 路径。

### `shaders/debug/render_instance_bbox.frag.glsl`

实例 bbox fragment shader。主要职责：

- 输出实例 bbox 线框颜色。
- 颜色通常来自 frame constants/debug 配置。

### `shaders/debug/render_cluster_bbox.mesh.glsl`

cluster bbox mesh shader。主要职责：

- 读取当前渲染 cluster list 和 scene building buffer。
- 根据 cluster bbox 生成线框。
- 支持 preloaded/streaming group 数据和 NV/EXT mesh shader。

### `shaders/debug/render_cluster_bbox.frag.glsl`

cluster bbox fragment shader。主要职责：

- 输出 cluster bbox 线框颜色。
- 用于调试 traversal 选中的 cluster 范围。

## shaders/streaming

streaming shader 管理 GPU 侧 request、resident 状态、地址更新和 age filter。

### `shaders/streaming/streaming.glsl`

streaming 共享函数库。主要内容：

- resident group/cluster 查询。
- load/unload request 写入和 compaction helper。
- allocator/free gap helper。
- geometry group address 更新 helper。
- 被 streaming compute pass 和 traversal shader 复用。

### `shaders/streaming/stream_setup.comp.glsl`

streaming setup compute shader。主要职责：

- 根据 push constant command 执行不同 setup/compaction/allocator 阶段。
- 压缩 request/status 列表。
- 构建 allocator free gaps。
- 准备 load/unload 插入任务。
- 为 CPU/GPU streaming 协作提供紧凑任务列表。

### `shaders/streaming/stream_update_scene.comp.glsl`

scene streaming 地址更新 shader。主要职责：

- 把新 resident group 的 GPU data address 写回对应 geometry/group address table。
- 清理卸载 group 的地址。
- 更新 shader 侧 `Geometry` 中 streaming group address，使 traversal/render 访问到最新驻留数据。

### `shaders/streaming/stream_agefilter_groups.comp.glsl`

streaming age filter shader。主要职责：

- 根据每个 resident group 的最近访问 frame，筛选长时间未使用的数据。
- 生成 unload candidate/request。
- 降低 streaming geometry memory 压力。

