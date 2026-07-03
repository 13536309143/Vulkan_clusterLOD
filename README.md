# Vulkan Cluster LOD Renderer

这是一个基于 Vulkan 的 Cluster LOD 渲染与实验项目。项目从 `.glb` / `.gltf` 模型导入几何数据，在 CPU 侧构建 mesh cluster、group 和多级 LOD 层次结构，在 GPU 侧通过 compute shader、mesh shader、Hi-Z、streaming 和 cache 完成实时渲染。

当前版本在原有 meshoptimizer / Cluster LOD 管线基础上，加入了工业模型的语义结构感知 LOD 策略：

- PointNeXt 负责工业零件闭集分类。
- PointCLIP V2 负责开放词表 zero-shot 语义补充。
- 几何结构特征负责尺寸、形状、复杂度和关键结构判断。
- 融合模块输出 P1-P10 语义 LOD 策略。
- C++ LOD 构建阶段读取 fused CSV，并将策略转为保守的语义结构约束。

当前策略不是用语义分类替代原 LOD 算法，而是在保留原始高性能 LOD 管线的前提下，让不同类型的工业子零件采用不同的简化约束。

## 目录结构

```text
src/
  app/                 ImGui 前端、运行参数、可视化模式
  core/                cache、序列化、基础工具
  meshlod/             meshoptimizer 与层级 LOD 构建核心
  renderer/            Vulkan 渲染、Hi-Z、streaming、cluster traversal
  scene/               glTF/GLB 导入、语义 CSV 读取、几何预处理

tools/
  pointnext_lod/       PointNeXt GLB 分析、结构特征提取、LOD 策略生成
  pointclipv2/         PointCLIP V2 zero-shot 推断与融合脚本

lod_analysis_outputs/  推断输出目录
_downloaded_resources/ 待分析和加载的 GLB 模型
```

## 核心能力

- 基于 meshoptimizer 的多级 LOD 构建。
- Cluster / group 层级组织。
- GPU 侧 LOD traversal、剔除、排序和渲染。
- scene cache，避免每次启动重复预处理。
- semantic lod policy 可视化模式，显示 P1-P10 策略分布。
- 优先读取 `lod_analysis_outputs/<模型名>_lod_constraints_fused.csv`。
- 支持语义结构约束进入 LOD 构建，但采用保守前置策略，避免过度保护。

## 语义结构 LOD 流程

对新模型的完整流程是：

```text
GLB 模型
  -> PointNeXt 闭集分类
  -> PointCLIP V2 开放词表推断
  -> 几何结构特征提取
  -> 融合生成 P1-P10 策略
  -> C++ 自动读取 fused CSV
  -> 构建语义结构感知 LOD cache
  -> 前端可视化与渲染
```

如果 `_downloaded_resources` 下有多个 GLB，可以直接运行：

```bat
run_all_glb_pointnext_pointclipv2_fused.bat
```

默认环境：

```bat
PointNeXt:    gpt-pointnext
PointCLIP V2: PointCLIPV2_LOD
```

脚本会扫描：

```text
_downloaded_resources/*.glb
```

并生成：

```text
lod_analysis_outputs/<模型名>_pointnext_analysis.csv
lod_analysis_outputs/<模型名>_pointclipv2_zeroshot.csv
lod_analysis_outputs/<模型名>_pointnext_pointclip_merged.csv
lod_analysis_outputs/<模型名>_lod_constraints_fused.csv
lod_analysis_outputs/<模型名>_lod_constraints_fused_summary.json
```

## P1-P10 策略

| 策略 | 含义 | LOD 倾向 |
|---|---|---|
| P1 | 微小或低置信零件 | 最激进，可远处快速简化或剔除 |
| P2 | 重复紧固件 | 激进简化，保留基本外形 |
| P3 | 大型静态块体 | 快速降低面数，避免大体量占预算 |
| P4 | 普通低细节零件 | 偏性能 |
| P5 | 一般可见件 | 平衡质量和性能 |
| P6 | 高细节外形件 | 适度保护轮廓 |
| P7 | 接口、流体、管路相关件 | 保护孔、接口和边界 |
| P8 | 结构控制件 | 保护连接面和控制轮廓 |
| P9 | 运动精密件 | 保护轴线、孔和关键形状 |
| P10 | 关键保留件 | 最高保护，但仍保守进入底层代价 |

P1-P10 首先影响外层 LOD 参数，例如目标简化比例、LOD error、feature 权重和是否允许远处剔除。当前版本还会把策略以弱约束形式传入底层简化前的 feature importance 计算。

## 当前语义结构约束原则

当前实现采用“外层策略为主，底层代价为辅”：

```text
主控制：P1-P10 target ratio / error scale / feature weight
辅助控制：保守调整边界、孔、圆柱轴、薄壁、功能边界的重要度
```

这样做的原因是 GLB 工业模型通常有大量拆分 mesh 和导出边界。如果语义约束过强，很多非功能性边界会被误保护，导致简化率下降、LOD 效果变差。当前版本对底层代价做了三层限制：

- 低置信度自动降低语义代价影响。
- 每个顶点的语义 importance boost 有上限。
- 只有 P7-P10 会触发更强的功能边界/薄壁锁定候选。

对应核心代码：

```text
src/scene/scene_semantic_lod.cpp
src/meshlod/meshlod_simplify.h
src/scene/clusterlod.cpp
```

## 前端查看

打开程序后，在 visualization 中选择：

```text
semantic lod policy
```

可以看到 P1-P10 颜色分布和数量统计。Runtime / Cache Parameters 中的 Feature Retention Output 还会显示：

```text
Semantic boosted
Semantic suppressed
Avg semantic delta
```

这些指标用于判断底层语义结构代价是否过强。正常情况下，优化后的保守版不应该出现大面积 `Semantic boosted`。

## Cache 注意事项

当前几何 cache 版本为 `13`。语义结构代价策略更新后，旧 cache 会自动失效并重新构建，避免继续使用旧的强约束结果。

如果你手动修改 fused CSV，项目也会通过语义 fingerprint 触发 cache mismatch 并重建。

## 常用文档

- [POINTNEXT_POINTCLIPV2_SEMANTIC_LOD_FULL_WORKFLOW.md](POINTNEXT_POINTCLIPV2_SEMANTIC_LOD_FULL_WORKFLOW.md)：完整推断、融合、导入和查看流程。
- [SEMANTIC_LOD_STRATEGY_TECHNICAL_ANALYSIS.md](SEMANTIC_LOD_STRATEGY_TECHNICAL_ANALYSIS.md)：语义结构 LOD 策略的技术设计和底层约束分析。

