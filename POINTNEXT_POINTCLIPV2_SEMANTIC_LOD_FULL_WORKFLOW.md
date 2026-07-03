# PointNeXt + PointCLIP V2 语义结构 LOD 完整流程

本文说明如何对新的工业 GLB 模型执行语义推断、策略融合，并让 Vulkan Cluster LOD 项目自动加载结果。

当前版本的核心原则是：

```text
PointNeXt / PointCLIP V2 / 结构特征负责判断 P1-P10
C++ LOD 构建仍以原始 meshoptimizer / Cluster LOD 为主
语义结构代价只做保守辅助，不再强行主导简化
```

## 1. 总流程

```text
输入 GLB
  -> PointNeXt 分支
       输出闭集类别、置信度、尺寸和结构特征

  -> PointCLIP V2 分支
       输出开放词表 top-k 候选和功能 role

  -> 融合脚本
       合并 PointNeXt、PointCLIP V2、结构特征
       输出 P1-P10 fused LOD 策略

  -> Vulkan 项目
       自动优先读取 <模型名>_lod_constraints_fused.csv
       重新构建 scene cache
       使用语义结构感知 LOD
```

PointNeXt 和 PointCLIP V2 可以分开跑。融合步骤必须等两个分支都生成 CSV 后执行。

## 2. 目录约定

模型放在：

```text
_downloaded_resources/
```

输出放在：

```text
lod_analysis_outputs/
```

推荐输出命名：

```text
<模型名>_pointnext_analysis.csv
<模型名>_pointclipv2_zeroshot.csv
<模型名>_pointnext_pointclip_merged.csv
<模型名>_lod_constraints_fused.csv
<模型名>_lod_constraints_fused_summary.json
```

C++ 项目加载模型时，会优先查找：

```text
lod_analysis_outputs/<模型名>_lod_constraints_fused.csv
```

如果没有 fused 文件，才退回普通：

```text
lod_analysis_outputs/<模型名>_lod_constraints.csv
```

## 3. 环境

### 3.1 PointNeXt

使用：

```bat
conda activate gpt-pointnext
```

需要能正常执行：

```bat
python -c "import torch; from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnext ok')"
```

### 3.2 PointCLIP V2

使用：

```bat
conda activate PointCLIPV2_LOD
```

需要能正常执行：

```bat
python -c "import torch, clip, numpy; print(torch.__version__, torch.cuda.is_available())"
```

如果遇到 OpenMP 冲突，可在当前命令行临时设置：

```bat
set KMP_DUPLICATE_LIB_OK=TRUE
```

## 4. 一键处理全部 GLB

根目录提供：

```bat
run_all_glb_pointnext_pointclipv2_fused.bat
```

它会扫描：

```text
_downloaded_resources\*.glb
```

并依次执行：

```text
1. conda activate gpt-pointnext
   执行 PointNeXt 分析

2. conda activate PointCLIPV2_LOD
   执行 PointCLIP V2 zero-shot 推断

3. 执行融合脚本
   生成 fused CSV 和 summary JSON
```

直接运行：

```bat
cd /d E:\vk_lod_clusters1\t1
run_all_glb_pointnext_pointclipv2_fused.bat
```

如果环境名不同，修改 bat 开头：

```bat
set POINTNEXT_CONDA_ENV=gpt-pointnext
set POINTCLIP_CONDA_ENV=PointCLIPV2_LOD
```

## 5. 单模型手动流程

假设模型为：

```text
_downloaded_resources\o1778.glb
```

### 5.1 PointNeXt

```bat
conda activate gpt-pointnext

python tools\pointnext_lod\analyze_large_glb_parts_pointnext.py ^
  --glb _downloaded_resources\o1778.glb ^
  --pointnext-root tools\pointnext_lod\PointNeXt ^
  --cfg tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml ^
  --ckpt tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth ^
  --classes-file tools\pointnext_lod\industrial_part_14classes.txt ^
  --num-points 8192 ^
  --batch-size 16 ^
  --output-csv lod_analysis_outputs\o1778_pointnext_analysis.csv ^
  --output-json lod_analysis_outputs\o1778_pointnext_analysis.json ^
  --summary-csv lod_analysis_outputs\o1778_pointnext_summary.csv ^
  --review-csv lod_analysis_outputs\o1778_pointnext_review.csv
```

### 5.2 PointCLIP V2

```bat
conda activate PointCLIPV2_LOD
set KMP_DUPLICATE_LIB_OK=TRUE

python tools\pointclipv2\run_pointclipv2_zeroshot_glb.py ^
  --glb _downloaded_resources\o1778.glb ^
  --pointclip-root tools\pointclipv2\PointCLIP_V2 ^
  --prompt-json tools\pointclipv2\industrial_open_vocab_prompts.json ^
  --num-points 8192 ^
  --batch-size 8 ^
  --top-k 5 ^
  --output-csv lod_analysis_outputs\o1778_pointclipv2_zeroshot.csv ^
  --output-json lod_analysis_outputs\o1778_pointclipv2_zeroshot.json ^
  --summary-csv lod_analysis_outputs\o1778_pointclipv2_summary.csv
```

### 5.3 融合

```bat
python tools\pointclipv2\fuse_pointnext_pointclip_lod.py ^
  --pointnext-csv lod_analysis_outputs\o1778_pointnext_analysis.csv ^
  --pointclip-csv lod_analysis_outputs\o1778_pointclipv2_zeroshot.csv ^
  --merged-csv lod_analysis_outputs\o1778_pointnext_pointclip_merged.csv ^
  --output-csv lod_analysis_outputs\o1778_lod_constraints_fused.csv ^
  --summary-json lod_analysis_outputs\o1778_lod_constraints_fused_summary.json
```

正常输出中应看到：

```text
Matched PointCLIP rows: N/N
Wrote: lod_analysis_outputs\o1778_lod_constraints_fused.csv
LOD priority counts:
  P1_...
  P2_...
  ...
```

如果 matched 数量不是 `N/N`，说明两个分支分析的 GLB 不一致，或 mesh 顺序不一致。

## 6. 加载模型并查看策略

打开项目程序，加载对应 GLB。C++ 会自动查找同名 fused CSV：

```text
lod_analysis_outputs\o1778_lod_constraints_fused.csv
```

控制台应出现类似：

```text
Semantic LOD: loaded ... rows, ... mesh policies from ..._lod_constraints_fused.csv
```

前端查看：

```text
visualization -> semantic lod policy
```

可以看到 P1-P10 颜色和数量分布。

## 7. P1-P10 含义

| 策略 | 名称 | 用途 | 约束倾向 |
|---|---|---|---|
| P1 | micro_uncertain | 微小或低置信件 | 最激进，可剔除 |
| P2 | repeated_fastener | 重复螺栓、螺母、铆钉 | 快速简化 |
| P3 | large_static_bulk | 地基、房屋、大型静态体 | 快速降面 |
| P4 | ordinary_low_detail | 普通低细节件 | 偏性能 |
| P5 | balanced_visible | 常规可见件 | 平衡 |
| P6 | high_detail_shape | 高细节外形件 | 适度保护轮廓 |
| P7 | interface_fluid | 管、阀、接口件 | 保护孔和接口 |
| P8 | structural_control | 夹具、把手、连接控制件 | 保护连接面 |
| P9 | motion_precision | 轴、滑块、运动件 | 保护轴线和接触区域 |
| P10 | critical_preserve | 齿轮、轴承、电机等关键件 | 最高保护 |

## 8. 当前底层约束方式

当前版本不是强制把 P1-P10 全部转成高强度 QEM 约束，而是采用保守前置：

```text
P1-P10 外层策略:
  simplify_ratio
  lod error scale
  feature weight
  cull permission
  hierarchy decay

保守底层结构代价:
  boundary
  circular hole
  cylindrical axis
  thin wall
  functional boundary
```

底层代价有三类保护：

1. 置信度门控  
   低置信语义不会强行改变简化代价。

2. importance boost 上限  
   每个顶点的语义增强有最大幅度，避免大面积边界被误保护。

3. hard lock 限制  
   功能边界和薄壁的额外锁定只在 P7-P10 生效。

这比上一版强约束更稳定，能避免“简化效果变差、过度保边、LOD 不够轻”的问题。

## 9. 输出字段说明

fused CSV 中常用字段：

| 字段 | 含义 |
|---|---|
| `mesh_index` | GLB mesh 索引 |
| `node_index` | GLB node 索引 |
| `predicted_class` | PointNeXt 闭集分类 |
| `confidence` | PointNeXt top1 置信度 |
| `pointclip_top1_name` | PointCLIP V2 top1 开放词表名称 |
| `pointclip_top1_role` | PointCLIP V2 映射功能角色 |
| `semantic_structural_score` | 融合置信评分 |
| `lod_priority` | 最终 P1-P10 策略 |
| `target_ratio_near` | 近处目标保留比例 |
| `target_ratio_mid` | 中距离目标保留比例 |
| `target_ratio_far` | 远处目标保留比例 |
| `allow_cull` | 是否允许远处剔除 |
| `screen_error_weight` | 屏幕误差权重 |

## 10. 判断结果是否合理

建议观察：

- P1 是否减少但没有吞掉重要小件。
- P10 是否稳定，不能大面积膨胀。
- P3 是否覆盖大型静态体。
- P8/P9 是否集中在结构连接和运动关键件。
- `Semantic boosted` 是否过高。
- 视觉上是否比 old 策略更保形，同时面数和 FPS 没明显变差。

如果发现过度保护，优先调低：

```text
src/scene/scene_semantic_lod.cpp
  semanticByPriority
  boundaryByPriority
  holeByPriority
  axisByPriority
  thinByPriority
```

如果发现关键件仍被破坏，再小幅调高 P8-P10 的对应权重，不建议直接整体放大。

