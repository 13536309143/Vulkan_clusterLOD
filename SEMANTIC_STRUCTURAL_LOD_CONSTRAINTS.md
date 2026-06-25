# 语义和结构引导的 Cluster LOD 约束方法

## 摘要

本文提出一种面向工业机械模型的语义和结构引导 Cluster LOD 约束方法。传统 LOD 构建通常对所有网格采用统一的几何简化规则，但在机械装配体、工业零件和 CAD 转换模型中，不同子零件具有明显不同的结构功能和视觉重要性。仅依赖三角形数量、包围盒大小或屏幕空间误差，难以判断一个零件是否应该被快速简化。

本文方法以 `meshoptimizer` 为底层几何简化和 cluster 构建基础，在其外部引入语义分类、结构分析和局部几何特征保护机制。PointNeXt 输出的语义类别与网格几何特征共同生成 P1-P5 五级 LOD 策略。不同策略分别控制软特征权重、硬顶点保护预算、自适应层级简化比例、cluster 分组粒度和运行时屏幕误差缩放。该方法能够让螺栓、垫片、大型静态壳体等低优先级或可快速退化对象更激进地简化，同时对轴承、齿轮、滑轨、装配接口、运动构件等关键对象保留更多结构细节。

本文方法的核心并不是替换 `meshoptimizer`，而是构建一个更适合机械模型的约束层，使底层简化算法在不同语义和结构条件下表现出不同的简化行为。

## 关键词

语义 LOD；结构感知 LOD；Cluster LOD；网格简化；PointNeXt；meshoptimizer；特征保护；硬锁预算；机械装配体；Vulkan 渲染。

## 1. 引言

LOD 技术的目标是在尽可能保持视觉质量的前提下降低渲染成本。常见 LOD 方法通常依据距离、屏幕空间误差或统一的网格简化比例生成多层模型。然而，对于工业机械场景而言，统一简化策略存在明显不足。

在机械模型中，一个零件的重要性并不完全由尺寸决定。例如：

- 螺栓、螺母、垫片和铆钉可能包含大量圆孔、锐边和螺纹细节，但在远距离渲染时通常可以快速简化。
- 房屋、地基、大型底座、大板件和外壳可能尺寸很大，但如果其功能是静态支撑或背景结构，则主要需要保留整体轮廓。
- 轴承、齿轮、滑轨、传动轴、铰链和滑轮可能尺寸较小，但具有明确的运动或装配功能，需要保留圆柱面、孔位和接口特征。
- 把手、控制件和外露连接件虽然不一定体积大，但对视觉识别和交互理解具有较高价值。

因此，面向机械模型的 LOD 构建不应只回答：

```text
这个网格有多大？
```

还应回答：

```text
这个网格是什么类型？
它在结构中承担什么作用？
哪些局部特征对该类型零件重要？
它应该以多快速度进入低精度 LOD？
```

本文提出的语义和结构引导 LOD 约束方法正是为解决上述问题而设计。

## 2. 问题定义

给定一个由多个子网格组成的工业模型场景，每个子网格包含：

- 顶点和三角形索引；
- 法线、切线、纹理坐标等可选属性；
- 场景层级中的节点或实例信息；
- PointNeXt 输出的语义类别和置信度；
- 由几何分析得到的尺寸、长宽高比例、扁平度、细长度、特征密度等结构信息。

目标是构建一个 Cluster LOD 层级，使其满足以下要求：

1. 对低优先级零件进行更强简化，降低整体三角形数和 cluster 数量。
2. 对运动关键件、接口关键件和视觉关键件保留更多局部结构。
3. 保持与现有 `meshoptimizer` 简化管线兼容，不引入过重的计算复杂度。
4. 在运行时根据语义优先级调整 LOD 切换时机。

因此，本文关注的问题不是设计一个新的网格简化器，而是设计一个更合理的 LOD 约束模型。

## 3. 现有 Cluster LOD 构建流程

当前项目的 LOD 构建流程可概括为：

1. 加载 glTF / GLB 模型。
2. 提取每个 mesh 或 primitive 的几何数据。
3. 使用 `meshoptimizer` 构建 meshlet / cluster。
4. 对 cluster 进行分组。
5. 合并 group 后进行网格简化。
6. 生成更粗层级的 cluster。
7. 递归构建 LOD 层级。
8. 保存 cluster 数据和层级节点。
9. 运行时根据屏幕空间误差选择合适 LOD。

相关实现文件包括：

```text
src/scene/scene_gltf.cpp
src/scene/clusterlod.cpp
src/scene/scene_semantic_lod.cpp
src/meshlod/meshlod_types.h
src/meshlod/meshlod_build.h
src/meshlod/meshlod_simplify.h
src/meshlod/meshlod_clustering.h
```

底层主要依赖以下 `meshoptimizer` 功能：

```text
meshopt_buildMeshletsSpatial
meshopt_buildMeshletsFlex
meshopt_partitionClusters
meshopt_simplifyWithAttributes
meshopt_simplifySloppy
```

本文方法保留这些底层功能，仅改变其外部约束和参数生成方式。

## 4. 语义和结构优先级模型

本文将每个子网格划分为 P1-P5 五个 LOD 优先级：

| 优先级 | 含义 | 简化策略 |
|---|---|---|
| P1 | 小型标准件或低重要细节 | 极激进简化 |
| P2 | 大型静态结构、壳体、地基、板件 | 激进简化，主要保留整体轮廓 |
| P3 | 普通结构件 | 平衡简化 |
| P4 | 结构关键件或装配接口件 | 保守简化 |
| P5 | 运动关键件、传动件或视觉关键件 | 强保护 |

该优先级由以下信息综合得到：

```text
语义类别
分类置信度
模型尺寸
形状比例
局部特征密度
功能类别
```

其中最重要的原则是：

```text
尺寸不等于重要性。
```

大型静态结构可以快速简化，小型运动构件反而可能需要保留更多细节。

## 5. 约束变量设计

语义策略最终会转换为 `clodConfig` 中的算法约束变量：

```cpp
int semantic_priority;
float semantic_confidence;
float feature_soft_scale;
float feature_hard_lock_ratio;
float hierarchy_depth_decay;
float hierarchy_min_ratio;
```

这些变量分别影响：

1. 局部特征保护强度。
2. 硬顶点保护预算。
3. LOD 层级构建中的目标简化比例。
4. cluster 分组粒度。
5. 运行时屏幕空间误差缩放。

这样，语义分类结果不只是一个显示标签，而是进入 LOD 构建过程的实际算法约束。

## 6. 特征保护模型

### 6.1 局部特征检测

当前实现检测以下几类局部特征：

```text
边界点 boundary
非流形点 non-manifold
锐边点 sharp edge
圆孔点 circular hole
功能边界点 functional boundary
圆柱特征点 cylindrical
薄壁特征点 thin wall
```

这些特征集中在机械模型中最容易影响结构和视觉质量的位置。本文没有引入过多复杂特征，因为过度复杂的特征检测会增加计算成本，也会带来更多不稳定判断。

对应实现位于：

```text
src/meshlod/meshlod_simplify.h
```

### 6.2 软特征保护

对每个顶点，首先计算几何特征重要性：

```text
I_geo(v) = max(
    I_sharp(v),
    I_cylindrical(v),
    I_boundary(v),
    I_thin_wall(v),
    I_functional_boundary(v),
    I_circular_hole(v),
    I_non_manifold(v)
)
```

然后根据语义优先级进行缩放：

```text
I_sem(v) = clamp(I_geo(v) * S_priority, 0, 1)
```

其中 `S_priority` 为语义软特征权重。

当前取值如下：

| 优先级 | 软特征权重 |
|---|---:|
| P1 | 0.45 |
| P2 | 0.65 |
| P3 | 1.00 |
| P4 | 1.30 |
| P5 | 1.60 |

软特征重要性会作为额外属性输入 `meshopt_simplifyWithAttributes`。这意味着简化器仍然可以删除顶点，但删除高重要性顶点会产生更大的属性误差。

软保护比直接锁死所有特征点更稳定，也更适合低优先级零件。

### 6.3 硬顶点保护

硬顶点保护通过以下标记实现：

```cpp
meshopt_SimplifyVertex_Protect
```

被硬保护的顶点在简化过程中更难被移除。该机制适合保护关键结构，但如果过度使用，会导致模型难以达到目标简化比例。

因此本文引入硬锁预算：

```text
B = vertex_count * hard_lock_ratio
```

除非是非流形点，否则其它候选点都需要进入排序队列，只有分数最高且预算允许的顶点才会被硬锁。

当前硬锁比例为：

| 优先级 | 硬锁比例 |
|---|---:|
| P1 | 1.5% |
| P2 | 4.5% |
| P3 | 9.0% |
| P4 | 16.0% |
| P5 | 24.0% |

候选点包括：

```text
圆孔点
功能边界点
薄壁点
达到关键阈值的重要点
```

候选分数定义为：

```text
Score(v) = I_sem(v)
         + circular_hole_bonus
         + functional_boundary_bonus
         + thin_wall_bonus
         + cylindrical_bonus
```

然后按分数降序排序，在预算内依次硬锁。

### 6.4 硬锁预算的必要性

工业模型中经常存在大量圆孔、锐边、倒角和小型连接结构。如果简单地将所有圆孔和边界全部硬锁，简化器会受到过强限制，导致：

- 目标三角形数无法达到；
- LOD 层级下降缓慢；
- 小标准件保留过多无意义细节；
- 整体 cluster 数和节点数偏高；
- GPU 遍历和渲染成本下降不明显。

硬锁预算解决了这一问题：

- P1/P2 主要使用软保护，硬锁极少。
- P4/P5 使用更高硬锁预算。
- 非流形点始终强制保护。
- 复杂 CAD 网格仍能完成有效简化。

## 7. 语义引导的层级构建

### 7.1 原始固定比例策略

原始 LOD 构建中，每次 group 合并后的目标简化比例大致为：

```text
target_triangles = current_triangles * simplify_ratio
```

该方法对所有层级使用固定比例，无法表达“低优先级零件应更快退化，高优先级零件应更慢退化”的需求。

### 7.2 自适应层级比例

本文方法改为基于层级深度的自适应比例：

```text
R_depth = clamp(
    R_base - (depth - 1) * D_priority,
    R_min,
    R_base
)
```

其中：

- `R_base` 为基础简化比例；
- `D_priority` 为层级衰减速度；
- `R_min` 为该策略允许的最低比例；
- `depth` 为当前 LOD 构建深度。

目标三角形数为：

```text
target_triangles = current_triangles * R_depth
```

当前策略为：

| 优先级 | 简化倾向 | 深度衰减 | 最小比例 |
|---|---:|---:|---:|
| P1 | 极激进 | 0.070 | 0.24 |
| P2 | 激进 | 0.055 | 0.30 |
| P3 | 平衡 | 0.030 | 0.40 |
| P4 | 保守 | 0.018 | 0.50 |
| P5 | 强保护 | 0.012 | 0.58 |

实现位置：

```text
src/meshlod/meshlod_build.h
```

该设计使 P1/P2 在 LOD 层级加深时快速降低复杂度，而 P4/P5 保留更多中间层级细节。

## 8. 语义引导的 Cluster 分组粒度

cluster 分组粒度会影响 LOD 层级结构：

- 较大的 group 会产生更粗的层级和更少节点；
- 较小的 group 会产生更细的层级和更精确的 LOD 切换。

因此，本文根据语义优先级调整 `partition_size`：

| 优先级 | Partition Size | 目的 |
|---|---:|---|
| P1 | 24 | 更快合并，减少节点 |
| P2 | 20 | 大型静态结构使用较粗层级 |
| P3 | 16 | 默认平衡 |
| P4 | 12 | 接口和结构件获得更细控制 |
| P5 | 10 | 运动和视觉关键件获得最细控制 |

实现位置：

```text
src/scene/scene_semantic_lod.cpp
src/scene/clusterlod.cpp
```

最终值仍会受到全局 `clusterGroupSize` 限制，避免破坏原始构建约束。

## 9. 运行时屏幕误差缩放

语义策略不仅影响离线 LOD 构建，也影响运行时 LOD 选择。

运行时通过 `lodErrorScale` 调整屏幕空间误差：

```text
低优先级零件 -> 更早切换到低精度 LOD
高优先级零件 -> 更晚切换到低精度 LOD
```

这种设计使 LOD 控制分为两部分：

```text
离线构建阶段：决定可用 LOD 的质量和层级结构。
运行时选择阶段：决定何时使用这些 LOD。
```

两者结合后，语义策略才能真正影响最终渲染行为。

## 10. 完整算法流程

### 10.1 预处理阶段

对每个输入 GLB 模型：

1. 提取或识别每个子 mesh。
2. 将 mesh 采样为点云。
3. 使用 PointNeXt 进行零件类型分类。
4. 提取尺寸、包围盒、形状比例和局部结构特征。
5. 结合语义类别、置信度和结构特征生成 P1-P5 优先级。
6. 输出 LOD 约束 CSV 文件。

### 10.2 LOD 构建阶段

对每个 mesh：

```text
读取语义策略
生成 clodConfig 约束参数
检测局部几何特征
计算软特征重要性
构建硬锁候选点队列
按预算选择硬保护顶点
构建 meshlet / cluster
根据语义粒度分组 cluster
按自适应层级比例递归简化
写入 cluster LOD 缓存
```

### 10.3 运行时渲染阶段

运行时执行：

```text
读取 cluster LOD 缓存
读取每个实例的语义 LOD 缩放参数
计算屏幕空间误差
应用 lodErrorScale
选择合适 cluster LOD
渲染被选中的 cluster
```

## 11. 伪代码

### 11.1 语义策略生成

```text
function derivePolicy(semantic_class, confidence, geometry):
    priority = classifyPriority(semantic_class, geometry)

    policy.simplifyRatio = table_simplify_ratio[priority]
    policy.featureSoftScale = table_soft_scale[priority]
    policy.featureHardLockRatio = table_hard_lock_ratio[priority]
    policy.hierarchyDepthDecay = table_depth_decay[priority]
    policy.hierarchyMinRatio = table_min_ratio[priority]
    policy.partitionSize = table_partition_size[priority]

    if confidence is low:
        reduce hard lock ratio
        reduce soft feature scale

    return policy
```

### 11.2 特征保护

```text
function analyzeFeatureConstraints(mesh, policy):
    features = detectBoundarySharpHoleCylinderThinWall(mesh)

    for each vertex v:
        geoImportance = maxFeatureScore(features[v])
        semanticImportance = clamp(geoImportance * policy.featureSoftScale)
        mesh.featureImportance[v] = semanticImportance

        if vertex is non-manifold:
            hardLock(v)
        else if vertex is hard-lock candidate:
            candidates.append(v, score(v))

    budget = vertexCount * policy.featureHardLockRatio
    sort candidates by score descending

    for each candidate in top budget:
        hardLock(candidate.vertex)
```

### 11.3 自适应层级简化

```text
function targetRatio(policy, depth):
    ratio = policy.baseRatio - (depth - 1) * policy.depthDecay
    return clamp(ratio, policy.minRatio, policy.baseRatio)

function buildLodGroup(group, depth):
    ratio = targetRatio(policy, depth)
    targetTriangleCount = currentTriangleCount * ratio
    simplified = meshopt_simplifyWithAttributes(...)
```

## 12. 实现位置总结

本文方法涉及以下文件：

```text
src/meshlod/meshlod_types.h
```

扩展 `clodConfig`，加入语义约束字段。

```text
src/scene/scene.hpp
```

扩展 `SemanticLodPolicy`。

```text
src/scene/scene_semantic_lod.cpp
```

将 P1-P5 语义优先级映射为具体算法约束。

```text
src/scene/clusterlod.cpp
```

在构建 LOD 之前把语义策略应用到 `clodConfig`。

```text
src/meshlod/meshlod_simplify.h
```

实现语义软特征权重和硬锁预算。

```text
src/meshlod/meshlod_build.h
```

实现基于层级深度的自适应简化比例。

## 13. 不同优先级的预期行为

### 13.1 P1：小型标准件

典型对象：

```text
螺栓
螺母
垫片
铆钉
小型重复紧固件
```

预期行为：

- 硬锁预算极低；
- 软特征权重较弱；
- LOD 退化速度快；
- group 粒度较粗；
- 远距离保留很少细节。

### 13.2 P2：大型静态结构

典型对象：

```text
房屋外壳
地基
大型板件
底座
机壳
静态框架
```

预期行为：

- 保留整体轮廓；
- 削减内部小细节；
- 较快进入低精度 LOD；
- 使用较粗 cluster 分组。

### 13.3 P3：普通结构件

预期行为：

- 使用平衡简化策略；
- 接近默认 cluster LOD 行为；
- 中等特征保护。

### 13.4 P4：结构关键件和接口件

典型对象：

```text
夹具
法兰
连接件
管接头
装配接口
```

预期行为：

- 更强边界和孔位保护；
- 更细 group 粒度；
- 更慢 LOD 退化。

### 13.5 P5：运动关键件和视觉关键件

典型对象：

```text
轴承
齿轮
滑轮
链条
车轮
转子
电机
导轨
外露控制件
```

预期行为：

- 最高软特征权重；
- 最高硬锁预算；
- 保守层级退化；
- 更细分组；
- 运行时更晚切换到低精度 LOD。

## 14. 实验评估指标

为了验证方法有效性，应比较原始 LOD 系统和语义结构约束 LOD 系统。

推荐统计指标包括：

```text
输入三角形数
输出三角形数
三角形压缩率
cluster 数量
LOD level 数量
受保护顶点数量
受保护顶点比例
平均特征重要性
最大特征重要性
LOD 构建时间
运行时 FPS
GPU 显存占用
视觉误差截图
```

还应按 P1-P5 分别统计：

```text
P1 三角形压缩率
P2 三角形压缩率
P3 三角形压缩率
P4 三角形压缩率
P5 三角形压缩率
P1-P5 受保护顶点比例
P1-P5 运行时可见 cluster 数量
```

关键不是单纯降低总三角形数，而是验证：

```text
三角形是否减少在正确的位置。
```

期望结果为：

```text
P1/P2 压缩更明显。
P4/P5 保留更多局部细节。
整体 FPS 提升或保持稳定。
运动件和接口件的视觉结构优于基线方法。
```

## 15. 消融实验设计

为了证明各模块作用，可以设置以下消融实验：

| 实验组 | 语义优先级 | 软特征权重 | 硬锁预算 | 自适应层级 | 运行时误差缩放 |
|---|---|---|---|---|---|
| Baseline | 否 | 否 | 否 | 否 | 否 |
| Semantic Ratio Only | 是 | 否 | 否 | 否 | 是 |
| Soft Feature Only | 是 | 是 | 否 | 否 | 是 |
| Hard-Lock Budget | 是 | 是 | 是 | 否 | 是 |
| Full Method | 是 | 是 | 是 | 是 | 是 |

通过该实验可以证明性能和质量提升并非只来自简单调整 `simplify_ratio`，而是来自完整的语义结构约束模型。

## 16. 局限性

本文方法仍依赖语义分类质量。如果 PointNeXt 预测类别错误，则 LOD 优先级可能不合理。

硬锁预算只能保护有限数量的特征点。对于极小但功能关键的零件，必须通过 P4 或 P5 策略提高保护强度。

当前方法基于三角网格检测特征，并不直接读取 CAD 参数曲面。因此，它无法像原始 CAD 系统一样理解精确工程语义。

如果一个 mesh 内部包含多个功能完全不同的子零件，则应在预处理阶段进一步拆分或合并，否则单一语义策略可能无法准确描述其内部结构差异。

## 17. 实际使用注意事项

为了保证兼容性，当没有加载语义策略时，系统保持接近原始行为：

```text
semantic_priority = 0
feature_hard_lock_ratio = 1.0
hierarchy_depth_decay = 0.0
hierarchy_min_ratio = simplify_ratio
```

也就是说，只有当模型加载到了语义 LOD 约束 CSV 后，新的语义结构约束才会生效。

该设计可以避免普通模型在没有语义数据时受到意外影响。

## 18. 结论

本文提出了一种语义和结构引导的 Cluster LOD 约束方法。该方法将 PointNeXt 语义分类、几何结构分析和局部特征检测转化为具体的 LOD 构建约束，使不同类型的零件在简化过程中表现出不同的保护和退化行为。

方法的核心链路为：

```text
语义类别 -> 结构优先级 -> 特征保护 -> 层级构建 -> 运行时 LOD 选择
```

相比只依赖统一简化比例的方法，该方法能够更合理地处理机械装配体中的不同零件。低优先级重复件和大型静态件可以被快速简化，而运动关键件、接口关键件和视觉关键件可以保留更多结构细节。

本文方法不替换 `meshoptimizer`，而是在其外部构建更高层次的语义结构约束模型，因此既保持了底层算法的稳定性，又提高了 LOD 构建对机械场景的适应性。
