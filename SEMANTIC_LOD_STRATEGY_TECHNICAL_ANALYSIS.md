# 语义与结构感知 LOD 策略技术分析

本文说明当前项目中语义结构 LOD 的关键技术设计。它不是操作手册，而是解释系统如何把工业子零件的语义、几何结构和尺寸信息转化为可执行的 LOD 约束，以及为什么当前版本采用“保守语义结构代价前置”。

## 1. 问题定义

传统 LOD 简化主要依赖几何误差、屏幕空间误差和三角形数量。对普通模型来说，这通常足够；但工业装配模型有明显不同：

- 螺栓、螺母、垫圈等重复件数量多，可以快速简化。
- 齿轮、轴承、电机、传动件等关键件需要保留结构轮廓。
- 地基、房屋、大型支架等体量大但静态，可以积极降面。
- 管路、接口、孔洞、薄壁和连接面即使面积小，也可能影响结构理解。
- GLB 中很多 mesh 是导出拆分结果，边界不一定等于功能边界。

因此，单纯使用全局误差无法区分“应该保留的小结构”和“可以牺牲的大结构”。本项目的目标是：在不破坏原有高性能 Cluster LOD 管线的前提下，为不同工业子零件生成不同的简化策略。

## 2. 系统结构

完整链路如下：

```text
GLB mesh / node
  -> 点云采样
  -> PointNeXt 闭集分类
  -> PointCLIP V2 开放词表推断
  -> 几何结构特征提取
  -> 语义结构融合
  -> P1-P10 LOD 策略
  -> C++ scene_semantic_lod 读取 fused CSV
  -> meshlod 构建语义结构感知 LOD
  -> runtime semantic lod policy 可视化
```

关键代码位置：

| 模块 | 文件 |
|---|---|
| PointNeXt GLB 分析 | `tools/pointnext_lod/analyze_large_glb_parts_pointnext.py` |
| PointCLIP V2 推断 | `tools/pointclipv2/run_pointclipv2_zeroshot_glb.py` |
| 双分支融合 | `tools/pointclipv2/fuse_pointnext_pointclip_lod.py` |
| 单分支策略生成 | `tools/pointnext_lod/build_lod_constraints_from_analysis.py` |
| C++ 读取语义 CSV | `src/scene/scene_semantic_lod.cpp` |
| LOD 参数注入 | `src/scene/clusterlod.cpp` |
| 特征约束和简化前代价 | `src/meshlod/meshlod_simplify.h` |
| 前端统计显示 | `src/app/lodclusters_ui.cpp` |

## 3. P1-P10 策略空间

P1-P10 不是简单的类别标签，而是 LOD 约束强度空间。

| 策略 | 语义 | 工程目标 |
|---|---|---|
| P1 | micro_uncertain | 微小、低置信、远处可牺牲 |
| P2 | repeated_fastener | 重复标准件，快速降低成本 |
| P3 | large_static_bulk | 大型静态体，优先降面 |
| P4 | ordinary_low_detail | 普通低细节件，偏性能 |
| P5 | balanced_visible | 常规可见件，平衡 |
| P6 | high_detail_shape | 高细节轮廓件，适度保形 |
| P7 | interface_fluid | 孔、接口、管路、阀类 |
| P8 | structural_control | 夹具、把手、连接控制件 |
| P9 | motion_precision | 运动、轴线、精密配合件 |
| P10 | critical_preserve | 齿轮、轴承、电机、核心传动件 |

P 值越高通常越保守，但它不是唯一依据。尺寸、置信度、重复性和结构特征会共同决定最终策略。

## 4. 神经语义证据

### 4.1 PointNeXt

PointNeXt 提供工业零件闭集分类，主要价值是稳定识别训练集内的标准工业件。例如：

- 螺栓、螺母、铆钉、垫圈。
- 齿轮、轴承、弹簧、销。
- 电机、法兰、接头、夹具。

PointNeXt 的输出包括：

```text
predicted_class
confidence
second_class
class probabilities
```

它适合做强先验，但不应单独决定最终 LOD，因为闭集分类对训练集外形态和复杂装配上下文有限。

### 4.2 PointCLIP V2

PointCLIP V2 提供开放词表 zero-shot 语义补充。它通过点云多视角投影，把 3D mesh 转成 CLIP 能处理的多视角图像，再和工业 prompt 对齐。

它的价值是：

- 对训练集外类别提供语义候选。
- 把 PointNeXt 低置信 mesh 从 P1 中拉出来。
- 补充如 structural control、large static bulk、interface 等功能角色。

它的风险是：

- 可能偏向宽泛词。
- 小 mesh 投影视图不稳定。
- 对复杂装配中的局部件可能语义过泛。

因此 PointCLIP V2 在融合中只作为补充证据，不能单独把大量 mesh 推到 P8-P10。

## 5. 结构特征证据

结构特征不是越多越好。本项目只保留能直接影响 LOD 策略的特征：

| 特征 | 用途 |
|---|---|
| 包围盒尺寸 | 判断 micro、large bulk、细长件 |
| 三角形和顶点数量 | 判断复杂度和预算占用 |
| 细长性 | 辅助识别轴、杆、销、管 |
| 扁平性 | 辅助识别板、薄壁、垫片 |
| 紧凑性 | 辅助区分块体和复杂件 |
| 表面积/体积倾向 | 判断薄壁、壳体、外形复杂度 |
| 结构关键词 role | 与神经语义共同决策 |

结构特征的作用是约束神经语义。例如：

- 一个巨大低细节块体不应因为 PointCLIP 给出 structural 而变成 P10。
- 一个小但高置信的齿轮不应被 P1 直接吞掉。
- 一个低置信但明显细长的轴类件，应更接近 P9 而不是 P1。

## 6. 融合策略

融合不是投票，而是分层判断：

```text
PointNeXt role prior
PointCLIP open-vocab role
结构特征分数
尺寸和复杂度门控
置信度门控
  -> P1-P10
```

基本规则：

- 高置信 PointNeXt 不轻易被 PointCLIP 推翻。
- 低置信 PointNeXt 更多依赖 PointCLIP 和结构特征。
- PointCLIP top1 不能直接决定高保护策略。
- P10 必须有关键语义或强结构证据。
- P1 主要用于微小、低置信、低可见价值 mesh。
- P3 用于大型静态体，避免大结构占用过多三角形预算。

## 7. 从策略到 LOD 参数

fused CSV 被 `src/scene/scene_semantic_lod.cpp` 读取后，会转成 `SemanticLodPolicy`。核心参数包括：

```text
simplifyRatio
errorMergeScale
featureWeightScale
featureProtectThreshold
featureCriticalThreshold
featureSoftScale
featureHardLockRatio
semanticStructureWeight
semanticBoundaryWeight
semanticHoleWeight
semanticAxisWeight
semanticThinWallWeight
semanticBulkSuppression
hierarchyDepthDecay
hierarchyMinRatio
partitionSize
lodErrorScale
allowCull
```

这些参数进入两层系统：

```text
离线构建层:
  控制 meshoptimizer 简化比例、feature attribute 权重、锁定候选和层级生成

运行时层:
  控制 LOD error scale、可视化颜色、剔除倾向和策略统计
```

## 8. 为什么改成保守前置

最初版本将语义结构代价较强地前置到简化流程中。它的优点是能更直接保护孔、边界、薄壁、圆柱轴等结构；但在实际 GLB 工业模型中，效果不总是更好。

原因是：

1. GLB 拆分边界很多  
   很多 boundary 是导出或材质拆分造成的，不是功能结构。

2. mesh 级策略可能放大局部高优先级  
   一个 mesh 内如果混入多个 node 策略，合并时可能取更高 priority，从而影响整个 mesh。

3. 强保护会降低简化率  
   如果大量顶点被推到高 importance，meshoptimizer 可折叠空间变小。

4. 神经分类存在不确定性  
   PointCLIP 和 PointNeXt 都可能对小件或复杂件产生偏差，不能让低置信语义直接控制底层代价。

因此当前版本采用：

```text
外层 P1-P10 策略为主
底层语义结构代价为弱辅助
```

这比强前置更适合工程默认使用。

## 9. 当前保守前置实现

核心位置：

```text
src/meshlod/meshlod_simplify.h
```

基本逻辑：

```text
先计算原始结构 importance:
  sharp
  boundary
  cylindrical
  thin_wall
  functional_boundary
  circular_hole
  non_manifold

再计算语义辅助目标:
  semantic_target

通过置信度门控和最大增量限制:
  semantic_importance = original_importance + limited_boost

最后进入 feature attribute:
  meshopt_simplifyWithAttributes
```

当前关键限制：

- `semantic_confidence <= 0` 时不再默认完全可信，而是按中等置信处理。
- `confidence_gate = (confidence - 0.25) / 0.75`。
- P1-P5 的 `semanticStructureWeight` 很低。
- 每个顶点的最大语义 boost 有上限。
- P8-P10 可以更强，但仍然有限制。
- `cylindrical` 和 `thin_wall` 不会被 bulk suppression 误伤。
- functional boundary / thin wall 的额外 hard lock 只在 P7-P10 生效。

## 10. 当前权重设计

当前 `scene_semantic_lod.cpp` 中，语义结构权重被收紧：

```text
semanticStructureWeight:
P1  0.00
P2  0.02
P3  0.03
P4  0.04
P5  0.06
P6  0.10
P7  0.14
P8  0.18
P9  0.22
P10 0.26
```

这说明：

- P1-P4 基本不让语义结构代价干预底层简化。
- P5-P6 只做轻微保形。
- P7-P10 才对接口、孔、轴、薄壁做更明确保护。

同时 clamp 上限也被降低：

```text
semantic_structure_weight <= 0.45
semantic_bulk_suppression <= 0.20
boundary / hole / axis / thin_wall 权重保持在温和范围
```

## 11. 与原方案的区别

原方案：

```text
P1-P10
  -> target ratio / screen error / feature weight
  -> meshoptimizer
```

强前置方案：

```text
P1-P10
  -> 大幅改变每个顶点 importance
  -> meshoptimizer
```

当前方案：

```text
P1-P10
  -> 外层 LOD 策略主导
  -> 小幅修正关键结构 importance
  -> meshoptimizer
```

因此当前方案保留了原方案的稳定性，同时给 P7-P10 的关键结构提供有限保护。

## 12. 质量评估指标

不能只看分类准确率，需要看 LOD 结果。推荐指标：

### 12.1 策略分布

观察 P1-P10 是否合理：

- P1 不应过多，否则说明语义理解不足。
- P10 不应膨胀，否则说明保护过强。
- P3 应覆盖大型静态块体。
- P8/P9 应集中在结构控制和运动件。

### 12.2 简化质量

检查：

- 孔洞是否塌陷。
- 圆柱轴是否变形。
- 薄壁是否破裂。
- 大型静态面是否仍然占大量三角形。
- 小件远处是否被快速降面。

### 12.3 运行统计

前端 `Feature Retention Output` 中关注：

```text
Semantic boosted
Semantic suppressed
Avg semantic delta
```

如果 `Semantic boosted` 占比很高，说明底层语义代价过强。当前保守版的目标是让它保持在较低水平。

## 13. Cache 与可复现性

语义 CSV 内容会参与 cache fingerprint。CSV 改变后，旧 cache 会被识别为 mismatch。

本次底层简化策略改变后，几何 cache 版本已升级：

```text
geoVersion = 13
```

这会强制旧 cache 失效，避免继续显示旧强约束策略的结果。

## 14. 前端可视化

前端支持：

```text
visualization -> semantic lod policy
```

用途：

- 查看每个 mesh 的 P1-P10 颜色。
- 查看策略数量分布。
- 对照模型结构判断策略是否合理。

颜色用于调试策略，不等价于最终渲染材质。

## 15. 调参原则

如果简化效果仍然偏差，按以下顺序调：

1. 先调 fused 策略分布  
   如果 P8/P10 过多，先改融合逻辑，不要直接改底层简化。

2. 再调 P1-P10 外层参数  
   如 `target_ratio_mid`、`screen_error_weight`、`allow_cull`。

3. 最后调语义结构代价  
   如 `semanticByPriority`、`holeByPriority`、`axisByPriority`。

不建议直接整体放大 `feature_attribute_weight` 或 `semanticStructureWeight`，这会让大量 mesh 变得难以简化。

## 16. 当前方案的定位

当前语义结构 LOD 不是一个单纯分类器，也不是完全重写 QEM。它更准确的定位是：

```text
工业语义分类
  + 几何结构证据
  + 可解释 P1-P10 策略
  + 保守底层结构代价
  + 原始高性能 Cluster LOD 管线
```

这种设计的优势是：

- 保持原项目的性能基础。
- 支持按工业零件功能区分 LOD 策略。
- 避免强语义代价导致过度保护。
- 对大型静态件、重复紧固件、运动件和关键件给出不同约束。
- 可通过 CSV、前端颜色和统计指标解释每个决策。

最终目标不是让所有关键结构都完全不变，而是在相同性能预算下，把三角形更多留给真正影响视觉和结构理解的区域。

