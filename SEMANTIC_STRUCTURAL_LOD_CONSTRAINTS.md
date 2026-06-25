# 语义和结构引导的 Cluster LOD 约束方法

## 摘要

本文提出一种面向工业机械模型的语义和结构引导 Cluster LOD 约束方法。传统 LOD 构建通常依据统一简化比例、距离或屏幕空间误差处理所有网格，但机械装配体中的子零件具有不同的结构功能、运动关系和视觉重要性。仅依赖尺寸或三角形数量，无法判断一个零件应快速简化还是重点保留。

本文方法以 `meshoptimizer` 为底层几何简化和 cluster 构建基础，在其外部加入语义分类、结构分析和局部特征保护。PointNeXt 输出的子零件类别与几何结构特征共同生成 P1-P10 十级语义结构 LOD 策略。不同策略控制软特征权重、硬顶点保护预算、自适应层级简化比例、cluster 分组粒度和运行时屏幕误差缩放。该方法使小型重复标准件和大型静态构件能够快速降面，同时让接口件、结构关键件、运动精密件和视觉关键件保留更多细节。

本文方法的核心不是替换 `meshoptimizer`，而是构建一个更高层的语义结构约束模型，使底层简化算法在不同类别零件上具有不同的约束强度和层级退化速度。

## 关键词

语义 LOD；结构感知 LOD；Cluster LOD；PointNeXt；meshoptimizer；网格简化；特征保护；硬锁预算；机械装配体；Vulkan 渲染。

## 1. 引言

LOD 技术用于在视觉质量和渲染成本之间取得平衡。对于通用场景，使用统一的几何误差和简化比例通常已经足够。但在工业机械模型中，零件的重要性往往由功能语义和结构角色决定，而不是只由尺寸决定。

典型问题包括：

- 螺栓、螺母、垫片等小型标准件数量多、细节密，但远距离可快速简化。
- 房屋、地基、大型底座、壳体和板件尺寸大，但主要需要保留整体轮廓。
- 齿轮、轴承、滑轨、转子、链条等运动件可能不大，但其圆柱面、孔位和轮廓对识别和运动关系很重要。
- 法兰、夹具、管接头、连接件等接口件需要保留装配边界和孔位。
- 把手、控制件和外露结构件需要保持视觉可读性。

因此，机械模型的 LOD 构建需要回答：

```text
这个零件是什么？
它在结构中承担什么作用？
哪些局部特征需要保护？
它应该以多快速度退化到低精度 LOD？
```

本文将这些问题转化为 P1-P10 语义结构 LOD 约束。

## 2. 现有管线

当前项目的 Cluster LOD 构建流程为：

1. 加载 `.glb` / `.gltf` 场景。
2. 提取每个 mesh / primitive 的顶点、索引和属性。
3. 使用 `meshoptimizer` 构建 meshlet / cluster。
4. 对 cluster 进行分组。
5. 合并 group 后调用简化器生成更粗层级。
6. 递归构建 cluster LOD hierarchy。
7. 写入 `.zippp` 缓存。
8. 运行时根据屏幕空间误差选择 LOD。

主要实现文件：

```text
src/scene/scene_gltf.cpp
src/scene/clusterlod.cpp
src/scene/scene_semantic_lod.cpp
src/meshlod/meshlod_types.h
src/meshlod/meshlod_build.h
src/meshlod/meshlod_simplify.h
src/meshlod/meshlod_clustering.h
```

底层依赖的 `meshoptimizer` 能力包括：

```text
meshopt_buildMeshletsSpatial
meshopt_buildMeshletsFlex
meshopt_partitionClusters
meshopt_simplifyWithAttributes
meshopt_simplifySloppy
```

本文方法保留这些稳定的底层能力，只改变其外部约束输入。

## 3. P1-P10 语义结构分类

P1-P10 不是简单的数字等级，而是对应明确的零件角色和 LOD 约束强度。

| 类别 | 名称 | 典型对象 | LOD 倾向 |
|---|---|---|---|
| P1 | micro / uncertain | 微小件、低置信度小件、极薄小片 | 最激进，可远距离剔除 |
| P2 | repeated fastener | 螺栓、螺母、垫片、铆钉、键 | 激进简化，少量保护 |
| P3 | large static bulk | 地基、房屋、壳体、大型静态板件 | 保轮廓，快速降面 |
| P4 | ordinary low detail | 普通低细节件、简单块体 | 较快简化 |
| P5 | balanced visible | 普通可见结构件 | 平衡简化 |
| P6 | high-detail shape | 高面数复杂外形件 | 较强保护 |
| P7 | interface / fluid | 法兰、管接头、阀、喷嘴、装配接口 | 保护孔位和接口边界 |
| P8 | structural / control | 夹具、连接件、控制件、把手 | 保护结构和视觉轮廓 |
| P9 | motion / precision | 滑轨、轮、一般运动件、精密件 | 保守简化 |
| P10 | critical preserve | 齿轮、轴承、电机、弹簧、关键运动件 | 最强保护 |

该分类由以下信息共同决定：

```text
PointNeXt 预测类别
预测置信度和 top1-top2 margin
包围盒尺寸和相对大小分位
面数和表面积
细长度、扁平度、紧凑度
形状 fallback 判断
```

核心原则是：

```text
尺寸不等于重要性，功能和结构角色决定约束强度。
```

## 4. 约束变量

语义策略最终转化为 `clodConfig` 中的算法变量：

```cpp
int semantic_priority;
float semantic_confidence;
float feature_soft_scale;
float feature_hard_lock_ratio;
float hierarchy_depth_decay;
float hierarchy_min_ratio;
```

这些变量控制三类行为：

1. 特征保护：影响软特征权重和硬锁预算。
2. 层级构建：影响每一层的简化目标比例。
3. 运行时选择：影响每个实例的 LOD 切换误差。

当前 P1-P10 还会映射到：

```text
simplify_ratio
feature_attribute_weight
feature_protect_threshold
feature_critical_threshold
partition_size
lodErrorScale
```

## 5. 特征保护模型

### 5.1 局部特征检测

系统检测以下局部结构：

```text
boundary vertices
non-manifold vertices
sharp edge vertices
circular hole vertices
functional boundary vertices
cylindrical vertices
thin-wall vertices
```

这些特征集中描述机械模型中最关键的孔、边界、圆柱、薄壁和拓扑异常。

实现位置：

```text
src/meshlod/meshlod_simplify.h
```

### 5.2 软特征保护

每个顶点先计算几何重要性：

```text
I_geo(v) = max(
    sharp,
    cylindrical,
    boundary,
    thin_wall,
    functional_boundary,
    circular_hole,
    non_manifold
)
```

然后根据 P1-P10 的语义权重缩放：

```text
I_sem(v) = clamp(I_geo(v) * feature_soft_scale, 0, 1)
```

`I_sem` 会作为额外属性输入 `meshopt_simplifyWithAttributes`，使简化器在删除高重要性特征时产生更高误差。这样保留了算法自由度，不会像硬锁一样直接阻止简化。

软特征权重大致随 P1-P10 递增：

```text
P1 最弱
P5 平衡
P10 最强
```

### 5.3 硬顶点保护

硬保护通过：

```cpp
meshopt_SimplifyVertex_Protect
```

实现。硬保护适合关键孔位、非流形点和高价值功能边界，但过度硬锁会导致简化器无法达到目标三角形数。

因此本文引入 hard-lock budget：

```text
hard_lock_budget = vertex_count * feature_hard_lock_ratio
```

除非是非流形点，其它候选顶点必须根据分数排序后进入预算：

```text
Score(v) = I_sem(v)
         + circular_hole_bonus
         + functional_boundary_bonus
         + thin_wall_bonus
         + cylindrical_bonus
```

P1/P2 仅允许极少硬锁；P9/P10 允许更多硬锁。这样可以防止小标准件因孔和锐边过多而无法简化。

## 6. 自适应层级构建

原始策略近似为固定比例简化：

```text
target_triangles = current_triangles * simplify_ratio
```

P1-P10 策略改为随层级深度变化：

```text
R_depth = clamp(
    R_base - (depth - 1) * hierarchy_depth_decay,
    hierarchy_min_ratio,
    R_base
)

target_triangles = current_triangles * R_depth
```

效果：

- P1-P3 快速进入粗 LOD。
- P4-P6 保持中等退化速度。
- P7-P10 更慢退化，保留更多层级细节。

实现位置：

```text
src/meshlod/meshlod_build.h
```

## 7. Cluster 分组粒度

`partition_size` 控制 cluster group 粒度：

- 大 group：节点更少，层级更粗，适合 P1-P3。
- 小 group：控制更细，LOD 切换更精确，适合 P7-P10。

当前策略中：

```text
P1/P2: partition_size 约 24
P5:    partition_size 约 16
P10:   partition_size 约 8
```

实现位置：

```text
src/scene/scene_semantic_lod.cpp
src/scene/clusterlod.cpp
```

最终值仍受全局 `clusterGroupSize` 约束。

## 8. 运行时 LOD 选择

每个实例带有：

```text
lodErrorScale
lodPolicyFlags
```

运行时根据 `lodErrorScale` 调整屏幕误差：

```text
低优先级 -> 更早切换到低精度 LOD
高优先级 -> 更晚切换到低精度 LOD
```

这使语义策略同时作用于离线构建和实时渲染。

## 9. 前端展示

前端新增 `Semantic LOD Policy Distribution` 面板，用于显示当前场景中 P1-P10 的数量分布：

```text
Policy
Instances
Geometries
Share
```

该面板直接统计当前场景实例的 `lodPolicy`，因此会反映模型阵列复制后的实际实例数量。若没有加载语义 CSV，则面板会提示当前场景没有语义 LOD 策略。

同时 `semantic lod policy` 可视化模式将 P1-P10 映射为 10 种颜色，用于检查分类是否符合预期。

## 10. 完整算法流程

### 10.1 离线分析

```text
GLB / GLTF
  -> 子 mesh 提取
  -> 点云采样
  -> PointNeXt 分类
  -> 几何结构分析
  -> P1-P10 策略生成
  -> *_lod_constraints.csv
```

### 10.2 LOD 构建

```text
读取 *_lod_constraints.csv
  -> 解析 P1-P10
  -> 生成 clodConfig
  -> 检测局部几何特征
  -> 计算 soft feature importance
  -> 按 hard-lock budget 选择保护顶点
  -> cluster 构建和分组
  -> 自适应层级简化
  -> 写入 .zippp 缓存
```

### 10.3 运行时渲染

```text
加载 .zippp
  -> 上传实例 lodPolicyFlags / lodErrorScale
  -> 根据屏幕误差选择 LOD
  -> semantic lod policy 模式显示 P1-P10 颜色
```

## 11. 实现位置

| 功能 | 文件 |
|---|---|
| P1-P10 CSV 生成 | `tools/pointnext_lod/build_lod_constraints_from_analysis.py` |
| 语义策略解析和映射 | `src/scene/scene_semantic_lod.cpp` |
| 语义字段和策略结构 | `src/scene/scene.hpp` |
| 语义策略应用到构建配置 | `src/scene/clusterlod.cpp` |
| 特征保护和硬锁预算 | `src/meshlod/meshlod_simplify.h` |
| 自适应层级简化 | `src/meshlod/meshlod_build.h` |
| priority GPU 编码 | `shaders/interface/shaderio_scene.h` |
| P1-P10 可视化颜色 | `shaders/common/render_shading.glsl` |
| 前端数量展示 | `src/app/lodclusters_ui.cpp` |

## 12. 评估指标

建议从以下维度评估：

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
构建时间
运行时 FPS
GPU 显存占用
P1-P10 instance / geometry 数量
P1-P10 可视化截图
```

关键不是简单降低总面数，而是确认：

```text
该快速简化的零件确实被快速简化；
该保护的零件确实被保留；
前端 P1-P10 数量分布符合模型结构。
```

## 13. 消融实验

建议设置以下对照：

| 实验组 | 语义分类 | 软特征权重 | 硬锁预算 | 自适应层级 | 运行时误差缩放 |
|---|---|---|---|---|---|
| Baseline | 否 | 否 | 否 | 否 | 否 |
| Ratio Only | 是 | 否 | 否 | 否 | 是 |
| Soft Feature | 是 | 是 | 否 | 否 | 是 |
| Hard-Lock Budget | 是 | 是 | 是 | 否 | 是 |
| Full P1-P10 | 是 | 是 | 是 | 是 | 是 |

该设计可证明效果不是来自单一比例调节，而是来自语义、结构、特征和层级约束的组合。

## 14. 局限性

1. 该方法依赖 PointNeXt 分类质量。分类错误会导致策略错误。
2. 当前语义策略以 mesh / node 为单位，如果一个 mesh 内包含多个不同功能零件，应在预处理阶段拆分或合并。
3. 特征检测基于三角网格，不直接读取 CAD 参数曲面。
4. P1-P10 参数仍需要结合具体数据集和视觉目标继续调参。

## 15. 结论

本文方法将：

```text
语义分类 -> 结构分析 -> P1-P10 策略 -> 特征保护 -> 层级构建 -> 运行时 LOD
```

连接为完整管线。相比统一简化策略，它能够让不同机械零件获得不同的 LOD 约束：重复件和大型静态件快速简化，接口件、结构件、运动件和关键件得到更强保护。该方法保留 `meshoptimizer` 的稳定性，同时通过语义和结构约束提高工业机械模型的 LOD 构建质量。
