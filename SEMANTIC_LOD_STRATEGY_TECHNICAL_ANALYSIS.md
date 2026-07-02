# 语义与结构感知 LOD 策略判断技术分析

本文分析当前项目中“语义 + 结构感知 LOD 约束策略”的关键技术点。它不是使用手册，而是说明系统如何判断一个 mesh 应该进入 P1-P10 哪一类、如何将该分类转化为底层 LOD 构建参数，以及它为什么能服务于工业模型的结构感知简化。

## 1. 问题定义

传统 meshoptimizer 或 cluster LOD 更关注几何误差、屏幕误差、三角形数和局部拓扑，而工业模型的 LOD 还需要考虑功能语义：

```text
螺栓、螺母、垫片等重复标准件
  -> 数量多，远处应快速简化或剔除。

地基、壳体、墙体、大型板件
  -> 尺寸大但功能上多为静态支撑，应快速降面。

结构连接件、夹具、控制件
  -> 对结构理解和视觉辨识重要，应适度保护。

齿轮、轴承、电机、弹簧等关键运动件
  -> 功能显著，应保留更多几何细节。
```

因此，本项目的 LOD 策略判断目标不是“分类名完全正确”，而是：

```text
判断该 mesh 在 LOD 构建中应该采用怎样的简化约束强度。
```

最终输出是 P1-P10 十级策略。

## 2. 总体技术路线

当前策略判断由三类证据融合：

```text
PointNeXt 闭集分类
  -> 训练过的工业零件类别概率。

PointCLIP V2 开放词表
  -> 开放语义候选和 role 补充。

结构特征库
  -> 尺寸、细长、薄壁、圆盘/环状、紧凑度、面数密度、大体块倾向等。
```

流程如下：

```text
GLB mesh node
  -> PointNeXt: predicted_class, confidence, second_confidence
  -> PointCLIP V2: pointclip_top1_role, score, margin
  -> Geometry: bbox, OBB, face_count, surface_area, shape_hint
  -> semantic + structural role inference
  -> inferred_role
  -> P1-P10 policy
  -> target_ratio / screen_error_weight / allow_cull
  -> C++ clodConfig
  -> LOD hierarchy construction
  -> shader/UI visualization
```

关键实现文件：

| 功能 | 文件 |
|---|---|
| 离线策略推断 | `tools/pointnext_lod/build_lod_constraints_from_analysis.py` |
| PointNeXt + PointCLIP 融合 | `tools/pointclipv2/fuse_pointnext_pointclip_lod.py` |
| PointCLIP V2 GLB 推断 | `tools/pointclipv2/run_pointclipv2_zeroshot_glb.py` |
| C++ 读取 CSV 并派生策略 | `src/scene/scene_semantic_lod.cpp` |
| 应用策略到 LOD 构建 | `src/scene/clusterlod.cpp` |
| cache 失效绑定 | `src/scene/scene_gltf.cpp` |
| GPU 标志和颜色 | `shaders/interface/shaderio_scene.h`, `shaders/common/render_shading.glsl` |
| 前端统计 | `src/app/lodclusters_ui.cpp` |

## 3. P1-P10 策略空间

P1-P10 是语义结构角色的最终离散化结果。它不是简单的“越大越重要”，而是“简化约束越来越保守”。

| 优先级 | role | 策略含义 |
|---|---|---|
| P1 | micro_uncertain | 微小、低置信、不确定件，远处可剔除 |
| P2 | repeated_fastener | 重复紧固件，快速简化 |
| P3 | large_static_bulk | 大型静态体块，快速降面 |
| P4 | ordinary_low_detail | 普通低细节件 |
| P5 | balanced_visible | 普通可见结构 |
| P6 | high_detail_shape | 高细节外形，保留轮廓和特征边 |
| P7 | interface_fluid | 管路、阀、喷嘴、接口件 |
| P8 | structural_control | 夹具、连接件、控制件、把手 |
| P9 | motion_precision | 运动/精密导向件 |
| P10 | critical_preserve | 齿轮、轴承、电机、弹簧等关键件 |

每个 P 级在 Python 侧有一组初始 LOD 参数：

```python
P10_POLICY_TABLE = {
  priority: (
    lod_priority,
    target_ratio_near,
    target_ratio_mid,
    target_ratio_far,
    allow_cull,
    screen_error_weight,
  )
}
```

典型趋势：

```text
P1-P3:
  target_ratio 更低，allow_cull 多为 true，screen_error_weight 较小。

P4-P6:
  target_ratio 中等，保留可见轮廓和复杂形状。

P7-P10:
  target_ratio 更高，allow_cull 为 false，screen_error_weight 更大。
```

## 4. 语义角色原型库

核心思想是先推断“功能角色”，再映射到 P1-P10。当前 role prototype 包含：

```text
micro_uncertain
repeated_fastener
large_static_bulk
ordinary_low_detail
balanced_visible
high_detail_shape
interface_fluid
structural_control
motion_precision
critical_preserve
```

每个原型包含：

```text
priority
function_role
strategy
protected_features
expected_features
```

例如：

```text
repeated_fastener
  priority: P2
  protected_features: placement, head_silhouette, axis_if_visible
  expected_features: micro_score, slender_score, ring_disk_score, detail_score

critical_preserve
  priority: P10
  protected_features: periodic_profile, center_axis, contact_surfaces, silhouette
  expected_features: high_detail_score, ring_disk_score, density_score, compact_score
```

这样做的好处是：

1. 分类名和 LOD 策略解耦。
2. PointNeXt、PointCLIP V2、结构特征都可以转化为同一个 role 空间。
3. 后续增加类别时，不需要直接改底层 LOD 逻辑，只需要扩展 role prior 或 prototype。

## 5. PointNeXt 闭集语义证据

PointNeXt 输出：

```text
predicted_class
confidence
second_class
second_confidence
```

先通过置信度判断可靠性：

```text
C2_high:
  confidence >= high_confidence 且 top1-top2 margin 足够大

C1_medium:
  置信度中等，但没有明显歧义

C0_low_or_ambiguous:
  confidence 过低或 top1-top2 margin 过小
```

然后将 `predicted_class` 映射到语义组：

```text
FASTENER_TYPES                 -> fastener_repeated
MOTION_CRITICAL_TYPES          -> motion_or_precision_part
INTERFACE_TYPES                -> fluid_or_interface_part
STRUCTURAL_KEY_TYPES           -> structural_key_part
BULK_STATIC_TYPES              -> bulk_static_part
CONTROL_TYPES                  -> control_or_handle
```

PointNeXt 的语义证据进入 `CLASS_ROLE_PRIORS`：

```python
"screws_bolts_studs": {"repeated_fastener": 0.95, "motion_precision": 0.15}
"bearings_bushings_guides": {"critical_preserve": 0.85, "motion_precision": 0.75}
"plates_discs_shapes": {"large_static_bulk": 0.55, "ordinary_low_detail": 0.45}
```

它的得分由类别先验、confidence 和 margin 共同决定：

```text
semantic_score = class_prior * evidence_quality
```

如果预测不可靠，则降低 PointNeXt 权重，而不是完全丢弃。

## 6. PointCLIP V2 开放词表证据

PointCLIP V2 的作用不是替代 PointNeXt，而是补充开放语义。

输出关键字段：

```text
pointclip_top1_role
pointclip_top1_score
pointclip_top2_role
pointclip_top2_score
pointclip_margin
pointclip_topk_json
```

由于开放词表概率会被多个 prompt 分散，PointCLIP V2 的绝对 top1 score 往往不高。因此当前融合更重视：

```text
top1 role 是否落在有效 role 空间
top1 与 top2 的 margin
top1 score + margin 的综合质量
```

质量估计：

```text
quality = clamp(0.25 + 1.8 * top1 + 4.0 * margin)
```

融合权重：

```text
pointclip_weight = 0.18 * pointclip_score
```

这意味着 PointCLIP V2 是“弱证据”。它能推动低置信样本，但不应该单独决定高保护策略。

## 7. 结构特征证据

结构特征来自 GLB mesh 的几何统计：

```text
bbox_diagonal
obb_size_long / mid / short
elongation
flatness
compactness
face_count
surface_area
shape_hint
```

首先按模型内分位数建立尺度阈值：

```text
diag_p10, diag_p25, diag_p50, diag_p75, diag_p90, diag_p97
face_p50, face_p75, face_p90, face_p97
area_p50, area_p75, area_p90
```

这样尺寸判断是“模型内相对尺度”，适合不同量纲和不同大小的工业模型。

结构特征被压缩成归一化评分：

| 特征 | 含义 |
|---|---|
| micro_score | 是否微小 |
| size_score | 相对尺寸等级 |
| medium_size_score | 是否中等尺寸 |
| detail_score | 面数/面积/密度综合细节等级 |
| high_detail_score | 高细节倾向 |
| density_score | 单位尺度下面数密度 |
| slender_score | 细长轴状倾向 |
| plate_score | 薄板/薄壁倾向 |
| compact_score | 紧凑块体倾向 |
| ring_disk_score | 圆盘/环状倾向 |
| bulk_score | 大型静态体块倾向 |
| simple_score | 低细节简单形状 |
| uncertainty_score | PointNeXt 歧义程度 |

结构 fallback 类别包括：

```text
geom_ultra_thin_sheet
geom_thin_plate
geom_wire_or_rod
geom_slender_bar
geom_compact_block
geom_ring_or_disk
geom_high_detail_irregular
geom_simple_irregular
```

这些特征是整个系统的稳定锚点。即使神经分类不确定，结构特征仍然可以把大型静态件、薄板、紧固件形状、高细节件分到较合理的 LOD 约束。

## 8. 原型匹配与融合评分

结构特征与每个 role prototype 进行匹配：

```text
prototype_match = weighted_mean(1 - abs(actual_feature - expected_feature))
```

权重由 expected feature 大小决定：

```text
weight = 0.75 + expected_value
```

最终融合得分：

```text
fused_score(role) =
  neural_weight     * PointNeXt_role_score
  + structural_weight * structural_match_score
  + pointclip_weight  * PointCLIP_role_score
```

权重设计：

```text
neural_weight:
  随 PointNeXt confidence 和 margin 增加。
  低置信时降低到 55%。

pointclip_weight:
  由 PointCLIP role score 控制，最大只占辅助权重。

structural_weight:
  至少保留 0.20，保证几何结构不会被神经语义完全覆盖。
```

这套机制避免了两种常见错误：

1. 只看神经网络类别，把大型静态结构误保护。
2. 只看几何形状，把轴承、齿轮、电机等功能件误简化。

## 9. 规则校正层

融合得分之后还有一层轻量规则校正。它不是大规模硬编码，而是对明显工业逻辑做约束：

```text
bulk_score 高且语义为 bulk/static/uncertain
  -> 增强 large_static_bulk

micro_score 高且低置信或紧固件
  -> 增强 micro_uncertain

可靠 fastener 且不是 micro
  -> 增强 repeated_fastener

可靠 motion_or_precision
  -> 增强 motion_precision 或 critical_preserve

可靠 fluid_or_interface
  -> 增强 interface_fluid

可靠 structural_key
  -> 增强 structural_control

high_detail_score 高且不是 bulk
  -> 增强 high_detail_shape
```

这层规则的作用是保证工程常识边界：

- 大型板件不要因为面积大就被过保护。
- 低置信小件不要轻易进入 P8-P10。
- 齿轮、轴承、电机、弹簧等可靠识别时应优先保护。
- 接口件和结构件要区分 P7/P8。

## 10. 输出解释字段

最终 CSV 不只输出 `lod_priority`，还输出可解释字段：

```text
inferred_role
function_role
semantic_structural_score
neural_role
neural_role_score
pointclip_role
pointclip_role_score
structural_match_role
structural_match_score
protected_features
inference_reason
structural_features
```

这些字段用于排查：

```text
为什么某个 mesh 是 P8？
是 PointNeXt 推动的，PointCLIP 推动的，还是结构特征推动的？
它被保护的是轮廓、接触面、轴线、周期轮廓，还是接口边界？
```

这比只输出类别 ID 更适合论文分析和工程调参。

## 11. CSV 到 C++ 策略的转换

C++ 侧在 `Scene::loadSemanticLodPolicies()` 中读取 CSV。

查找顺序：

```text
优先：<model>_lod_constraints_fused.csv
回退：<model>_lod_constraints.csv
```

CSV 行被解析为 `SemanticLodPolicy`：

```text
mesh_index
node_index
lod_priority
allow_cull
confidence
target_ratio_near
target_ratio_mid
target_ratio_far
screen_error_weight
```

同时维护两个层级：

```text
m_semanticNodePolicies:
  node_index + mesh_index 级别策略。

m_semanticMeshPolicies:
  mesh_index 级别策略。
  如果多个 node 复用同一个 mesh，则取更保守的合并策略。
```

合并策略原则：

```text
priority          取最大
allowCull         逻辑 AND
confidence        取最大
targetRatio       取最大
screenErrorWeight 取最大
```

也就是说，如果同一个 mesh 在不同 node 中出现，系统倾向于保守保护，而不是让任一实例过度简化。

## 12. C++ 派生底层 LOD 参数

Python 输出的是高层策略，C++ 会进一步派生底层 `clodConfig` 参数。

核心派生表在 `derivePolicy()`：

```text
simplifyByPriority
mergeByPriority
featureByPriority
protectByPriority
criticalByPriority
softByPriority
lockByPriority
decayByPriority
minByPriority
partitionByPriority
```

主要派生参数：

| 参数 | 作用 |
|---|---|
| simplifyRatio | 控制整体简化目标 |
| errorMergeScale | 控制误差合并尺度 |
| featureWeightScale | 控制特征约束权重 |
| featureProtectThreshold | 保护普通特征的阈值 |
| featureCriticalThreshold | 保护关键特征的阈值 |
| featureSoftScale | 软保护强度 |
| featureHardLockRatio | 硬锁定顶点比例 |
| hierarchyDepthDecay | 层级构建深度衰减 |
| hierarchyMinRatio | 层级最小保留比例 |
| partitionSize | cluster 分区粒度 |
| lodErrorScale | 运行时 LOD 误差尺度 |

策略趋势：

```text
P1-P3:
  simplifyRatio 更低，feature 保护更弱，partition 更粗，允许快速降面。

P4-P6:
  中间约束，平衡可见外形和面数。

P7-P10:
  feature 权重更高，保护阈值更严格，hard lock 比例更高，层级衰减更慢。
```

## 13. 与 cluster LOD 构建的结合

在 `Scene::buildGeometryLod()` 中：

```cpp
clodConfig clodInfo = clodDefaultConfig(...);

if(geometry.hasSemanticLodPolicy)
{
  applySemanticPolicyToConfig(clodInfo, geometry.semanticPolicy);
}
```

语义策略会影响：

```text
simplify_ratio
simplify_error_merge_previous
semantic_priority
semantic_confidence
feature_soft_scale
feature_hard_lock_ratio
hierarchy_depth_decay
hierarchy_min_ratio
partition_size
feature_attribute_weight
feature_protect_threshold
feature_critical_threshold
```

这说明当前策略不是只在前端染色，而是实际进入 LOD 构建算法，影响层级生成和特征保护。

## 14. 特征保护思想

底层 LOD 构建会统计 feature metrics：

```text
boundary_vertices
non_manifold_vertices
sharp_edge_vertices
boundary_components
sharp_ring_components
circular_hole_loops
circular_hole_vertices
functional_boundary_vertices
cylindrical_vertices
thin_wall_vertices
protected_vertices
critical_vertices
feature_importance
```

语义策略通过不同 P 级调整这些特征的保护强度：

```text
P1/P2:
  特征保护弱，主要保留位置和粗轮廓。

P3:
  大型静态件快速简化，但仍保留外轮廓和主要开口。

P6:
  增强高曲率区域、特征边和复杂轮廓保护。

P7/P8:
  保护接口边界、装配面、接触面、控制件轮廓。

P9/P10:
  保护轴线、导向面、周期轮廓、接触面和关键功能区域。
```

这就是“语义与结构引导的特征保护”真正进入 LOD 算法的方式。

## 15. cache 一致性设计

语义策略参与 cache identity：

```cpp
geometry.lodInfo.semanticPolicyHash = semanticPolicyHashForMesh(meshIndex);
```

只要策略 CSV 改变，`semanticPolicyHash` 就改变。旧 `.zippp` cache 会被判定 stale，然后重新构建。

这样避免了一个关键错误：

```text
CSV 已更新，但项目仍然使用旧 cache 中的 LOD 数据。
```

当前代码还将重复的 cache mismatch 日志压缩为一次性提示，避免大模型刷屏。

## 16. GPU 标志与可视化

语义 LOD 标志写入 instance：

```text
SEMANTIC_LOD_VALID_BIT
SEMANTIC_LOD_ALLOW_CULL_BIT
SEMANTIC_LOD_AGGRESSIVE_BIT
SEMANTIC_LOD_PRESERVE_BIT
SEMANTIC_LOD_LOW_CONF_BIT
priority bits
```

shader 通过：

```glsl
semanticLodColor(instanceID)
```

根据 priority 显示 P1-P10 颜色。

前端统计：

```text
Semantic LOD Policy Distribution
```

统计每个 P 级的：

```text
Instances
Geometries
Share
```

这使得策略不只是离线数据，也能在前端直接验证。

## 17. 技术优势

当前方案的优势在于：

1. 多源证据融合  
   不依赖单一神经网络结果，能处理低置信和开放类别。

2. 结构特征兜底  
   即使语义不确定，也能通过尺寸、薄壁、细长、大体块、高细节等特征决定合理策略。

3. 功能角色中间层  
   先推断 role，再映射 P1-P10，减少类别名和 LOD 策略之间的硬耦合。

4. 策略真正进入 LOD 构建  
   不是只做可视化，而是影响 simplify ratio、feature constraints、hierarchy 参数和 partition size。

5. 可解释性强  
   输出 `inference_reason`、`protected_features`、`structural_features`，便于论文分析和工程调参。

6. cache 安全  
   语义策略 hash 进入 cache identity，避免策略更新后误用旧 LOD 数据。

## 18. 当前局限

仍然存在几个局限：

1. PointCLIP V2 是弱证据  
   开放词表分数常偏低，不能单独决定高保护策略。

2. 结构特征仍是统计级别  
   当前可以判断薄壁、细长、圆盘、高细节、大体块，但没有真正解析螺纹、齿数、孔洞拓扑语义。

3. P6/P8 存在过保护风险  
   低置信小件在 PointCLIP 和结构特征共同作用下可能被提升到 P6/P8，需要前端抽查。

4. 尺度是模型内相对尺度  
   对单模型有效，但跨模型统一物理尺度时仍需引入单位和真实尺寸阈值。

5. role prototype 仍需数据驱动校准  
   当前 expected_features 和 boost 规则是工程先验，后续可以用人工标注或 LOD 质量指标自动调参。

## 19. 可继续优化方向

推荐优先级：

1. 增加真实结构检测  
   识别圆孔、螺纹、齿轮齿形、轴孔、装配面、对称轴。

2. 增加尺寸门槛  
   对 P8/P10 加入最小尺寸或最小可见面积约束，减少小件过保护。

3. 增加融合置信校准  
   根据 PointCLIP top1/top2 margin、PointNeXt margin、结构匹配差距建立统一 uncertainty。

4. 增加策略质量评估  
   对比不同策略下三角形减少率、屏幕误差、关键特征保留率。

5. 增加人工审查闭环  
   将低置信、高冲突、高保护的小件输出为 review candidates，用人工反馈更新 role prior。

## 20. 总结

当前项目的 LOD 策略判断已经从传统几何误差驱动，升级为：

```text
闭集监督语义
  + 开放词表语义
  + 结构几何特征
  + 功能角色原型
  + 底层 feature protection
  + cache-aware LOD 构建
```

这使得系统能够针对工业模型中的不同零件类型采用差异化 LOD 策略：

```text
重复标准件快速简化
大型静态体块快速降面
普通可见结构适度保留
接口/结构/运动/关键件重点保护
```

因此，该部分不仅是分类后处理，而是一个完整的“语义与结构引导的 LOD 约束决策层”。

<<<<<<< HEAD
| 颜色 | LOD 类别 | 含义 |
|---|---|---|
| 灰色 | P1 | `micro_uncertain`，微小/不确定件 |
| 棕色 | P2 | `repeated_fastener`，螺栓、螺母、垫片、销钉等重复标准件 |
| 橙黄色 | P3 | `large_static_bulk`，大型静态体块、地基、壳体、板件 |
| 黄色 | P4 | `ordinary_low_detail`，普通低细节件 |
| 绿色 | P5 | `balanced_visible`，普通可见结构件 |
| 青绿色 | P6 | `high_detail_shape`，高细节形状件 |
| 浅蓝色 | P7 | `interface_fluid`，接口、阀、喷嘴、管接头 |
| 蓝色 | P8 | `structural_control`，结构连接件、夹具、控制件、把手 |
| 紫色 | P9 | `motion_precision`，运动/精密导向件 |
| 粉红/玫红色 | P10 | `critical_preserve`，齿轮、轴承、电机、弹簧等关键件 |
| 深灰色 | 无语义 LOD | 没有加载到有效 semantic policy |
=======
>>>>>>> fc11f2f54ca7dd48635dacfcfec54a9c061e8984
