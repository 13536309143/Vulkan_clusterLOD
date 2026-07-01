# PointNeXt + PointCLIP V2 + 结构特征融合 LOD 完整流程

本文档整合 `NEW_MODEL_SEMANTIC_LOD_WORKFLOW.md` 和 `tools/pointclipv2/README_POINTCLIPV2_LOD.md`，说明当导入新的工业三维模型时，如何按正确顺序完成：

```text
工业三维模型
  -> PointNeXt 闭集分类
  -> PointCLIP V2 开放词表推断
  -> 结构特征提取
  -> 融合判断
  -> P1-P10 LOD 约束
  -> Vulkan Cluster LOD 读取并可视化
```

## 1. 推断有没有顺序

有顺序，但不是所有分支都必须串行。

推荐顺序如下：

```text
Step 0  准备模型
Step 1  PointNeXt 分析
Step 2  PointCLIP V2 分析
Step 3  融合 PointNeXt + PointCLIP V2 + 结构特征
Step 4  生成 fused LOD constraints
Step 5  删除旧 .zippp cache
Step 6  启动 Vulkan LOD 项目查看结果
```

其中：

```text
Step 1 和 Step 2 可以并行执行。
Step 3 必须等 Step 1 和 Step 2 都完成。
Step 5 必须在重新打开模型前完成。
```

原因是：

- PointNeXt 输出 `*_pointnext_analysis.csv`，里面包含闭集类别、置信度、几何尺寸、面数、形状等结构特征。
- PointCLIP V2 输出 `*_pointclipv2_zeroshot.csv`，里面包含开放词表 top-k 和语义角色。
- 融合脚本必须同时读取这两个 CSV，才能生成最终 `*_lod_constraints_fused.csv`。
- Vulkan 项目读取的是最终 LOD 约束 CSV，不直接运行 PointNeXt 或 PointCLIP V2。

## 2. 总体文件关系

以 `c.glb` 为例：

```text
_downloaded_resources\c.glb

lod_analysis_outputs\c_pointnext_analysis.csv
lod_analysis_outputs\c_pointclipv2_zeroshot.csv
lod_analysis_outputs\c_pointnext_pointclip_merged.csv
lod_analysis_outputs\c_lod_constraints_fused.csv
lod_analysis_outputs\c_lod_constraints_fused_summary.json

_downloaded_resources\c.glb.zippp
```

关键文件是：

```text
lod_analysis_outputs\<模型名>_lod_constraints_fused.csv
```

如果没有融合 PointCLIP V2，则旧流程使用：

```text
lod_analysis_outputs\<模型名>_lod_constraints.csv
```

建议后续统一使用 fused 文件作为最终策略来源。

## 3. 环境划分

不要把 PointNeXt 和 PointCLIP V2 放进同一个 Python 环境。

推荐两个环境：

```text
PointNeXt 环境
  作用：闭集工业零件分类
  脚本：tools\pointnext_lod\batch_analyze_glb_lod.py
  输出：*_pointnext_analysis.csv

PointCLIPV2_LOD 环境
  作用：开放词表 zero-shot 推断
  脚本：tools\pointclipv2\run_pointclipv2_zeroshot_glb.py
  输出：*_pointclipv2_zeroshot.csv
```

融合脚本只读取 CSV，通常可在任意能运行普通 Python 的环境中执行。

## 4. Step 0：准备模型

把模型放到：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources
```

例如：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources\my_model.glb
```

建议模型文件名只使用英文、数字和下划线，避免空格和特殊符号。

## 5. Step 1：运行 PointNeXt 闭集分类

进入项目目录：

```bat
cd /d E:\vk_lod_clusters1\t1
```

运行单个模型：

```bat
python tools\pointnext_lod\batch_analyze_glb_lod.py ^
  --resources-dir "%cd%\_downloaded_resources" ^
  --output-dir "%cd%\lod_analysis_outputs" ^
  --pointnext-root "%cd%\tools\pointnext_lod\PointNeXt" ^
  --cfg "%cd%\tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml" ^
  --ckpt "%cd%\tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth" ^
  --classes-file "%cd%\tools\pointnext_lod\industrial_part_14classes.txt" ^
  --models my_model.glb ^
  --num-points 8192 ^
  --batch-size 16
```

输出：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
lod_analysis_outputs\my_model_pointnext_analysis.json
lod_analysis_outputs\my_model_pointnext_class_summary.csv
lod_analysis_outputs\my_model_pointnext_review_candidates.csv
lod_analysis_outputs\my_model_lod_constraints.csv
lod_analysis_outputs\my_model_lod_constraints_summary.json
```

其中 `my_model_pointnext_analysis.csv` 是后续融合必须使用的输入文件。

## 6. Step 2：运行 PointCLIP V2 开放词表推断

第一次使用前，创建独立环境：

```bat
cd /d E:\vk_lod_clusters1\t1
conda env create -f tools\pointclipv2\environment_pointclipv2.yml
conda activate PointCLIPV2_LOD
```

安装 Dassl3D：

```bat
cd /d E:\vk_lod_clusters1\t1\tools\pointclipv2\PointCLIP_V2\zeroshot_cls\Dassl3D
python setup.py develop
```

如果 `torch-scatter` 安装失败，按你的 PyTorch/CUDA 版本安装对应 wheel。例如：

```bat
pip install torch-scatter -f https://data.pyg.org/whl/torch-1.13.0+cu117.html
```

回到项目目录：

```bat
cd /d E:\vk_lod_clusters1\t1
```

先做 100 个 mesh 的小测试：

```bat
python tools\pointclipv2\run_pointclipv2_zeroshot_glb.py ^
  --glb _downloaded_resources\my_model.glb ^
  --pointclip-root tools\pointclipv2\PointCLIP_V2 ^
  --prompt-json tools\pointclipv2\industrial_open_vocab_prompts.json ^
  --num-points 8192 ^
  --batch-size 8 ^
  --top-k 5 ^
  --limit 100 ^
  --output-csv lod_analysis_outputs\my_model_pointclipv2_test.csv
```

确认测试正常后，运行完整模型：

```bat
python tools\pointclipv2\run_pointclipv2_zeroshot_glb.py ^
  --glb _downloaded_resources\my_model.glb ^
  --pointclip-root tools\pointclipv2\PointCLIP_V2 ^
  --prompt-json tools\pointclipv2\industrial_open_vocab_prompts.json ^
  --num-points 8192 ^
  --batch-size 8 ^
  --top-k 5 ^
  --output-csv lod_analysis_outputs\my_model_pointclipv2_zeroshot.csv ^
  --output-json lod_analysis_outputs\my_model_pointclipv2_zeroshot.json ^
  --summary-csv lod_analysis_outputs\my_model_pointclipv2_summary.csv
```

PointCLIP V2 输出字段主要包括：

```text
pointclip_top1_id
pointclip_top1_name
pointclip_top1_role
pointclip_top1_score
pointclip_top2_id
pointclip_top2_role
pointclip_top2_score
pointclip_margin
pointclip_topk_json
```

## 7. Step 3：融合 PointNeXt、PointCLIP V2 和结构特征

融合脚本读取：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
lod_analysis_outputs\my_model_pointclipv2_zeroshot.csv
```

执行：

```bat
cd /d E:\vk_lod_clusters1\t1

python tools\pointclipv2\fuse_pointnext_pointclip_lod.py ^
  --pointnext-csv lod_analysis_outputs\my_model_pointnext_analysis.csv ^
  --pointclip-csv lod_analysis_outputs\my_model_pointclipv2_zeroshot.csv ^
  --merged-csv lod_analysis_outputs\my_model_pointnext_pointclip_merged.csv ^
  --output-csv lod_analysis_outputs\my_model_lod_constraints_fused.csv ^
  --summary-json lod_analysis_outputs\my_model_lod_constraints_fused_summary.json
```

输出：

```text
lod_analysis_outputs\my_model_pointnext_pointclip_merged.csv
lod_analysis_outputs\my_model_lod_constraints_fused.csv
lod_analysis_outputs\my_model_lod_constraints_fused_summary.json
```

`my_model_pointnext_pointclip_merged.csv` 是中间文件，用于检查两个分支是否正确对齐。

`my_model_lod_constraints_fused.csv` 是最终 LOD 策略文件。

## 8. 融合判断逻辑

融合不是简单投票，而是分工加权：

```text
PointNeXt：
  闭集工业类别。
  高置信度时作为主要语义证据。

PointCLIP V2：
  开放词表 top-k 候选。
  用于未知类别、低置信度、语义冲突和补充判断。

结构特征：
  尺寸、细长程度、薄壁程度、圆盘/环状倾向、紧凑度、面数密度、大体块倾向。
  用于约束最终是否应该保护或快速简化。
```

推荐理解方式：

```text
PointNeXt 负责“它像训练过的哪类工业零件”
PointCLIP V2 负责“开放词表里它更像什么”
结构特征负责“它对 LOD 到底该保留还是快速简化”
```

典型规则：

```text
PointNeXt 高置信 + PointCLIP V2 同意 + 结构匹配
  -> 高可信语义，按功能角色生成 P1-P10。

PointNeXt 高置信 + PointCLIP V2 不同意 + 结构支持 PointNeXt
  -> 仍以 PointNeXt 为主，但记录冲突原因。

PointNeXt 低置信 + PointCLIP V2 较明确 + 结构匹配
  -> 使用 PointCLIP V2 作为开放词表补充。

PointNeXt 和 PointCLIP V2 都不确定
  -> 不强行语义分类，主要按尺寸和结构特征决定 LOD。

超大型静态体块
  -> 即使语义不确定，也优先进入 P3/P4 快速简化。

运动件、传动件、结构关键件
  -> 只有语义和结构同时支持时，才升到 P8-P10。
```

## 9. P1-P10 LOD 类型

| 类别 | 名称 | 策略含义 |
|---|---|---|
| P1 | micro uncertain | 微小件、低置信度小件、远处可剔除 |
| P2 | repeated fastener | 螺栓、螺母、垫片、铆钉、销、键等重复标准件 |
| P3 | large static bulk | 地基、房屋、壳体、大型静态支撑、墙体、底座 |
| P4 | ordinary low detail | 普通低细节零件、简单块体 |
| P5 | balanced visible | 普通可见结构件 |
| P6 | high detail shape | 高面数或外形复杂零件 |
| P7 | interface fluid | 管接头、阀、喷嘴、法兰、装配接口 |
| P8 | structural control | 夹具、连接件、铰链、控制件、把手 |
| P9 | motion precision | 一般运动件、精密导向件、滑轨、轮类 |
| P10 | critical preserve | 齿轮、轴承、电机、弹簧、关键运动/传动件 |

策略强度大致为：

```text
P1-P3   快速简化或远处剔除
P4-P6   中等保护
P7-P10  重点保护
```

## 10. Step 4：让 Vulkan LOD 项目读取 fused 结果

当前项目通常按模型名查找：

```text
lod_analysis_outputs\<模型名>_lod_constraints.csv
```

如果你要让项目直接使用融合结果，有两种方式。

方式 A：复制 fused 结果覆盖默认约束文件：

```bat
copy /y lod_analysis_outputs\my_model_lod_constraints_fused.csv lod_analysis_outputs\my_model_lod_constraints.csv
copy /y lod_analysis_outputs\my_model_lod_constraints_fused_summary.json lod_analysis_outputs\my_model_lod_constraints_summary.json
```

方式 B：修改项目读取逻辑，让它优先读取：

```text
<模型名>_lod_constraints_fused.csv
```

如果只是实验对比，建议先用方式 A，最简单稳定。

## 11. Step 5：删除旧 scene cache

只要修改了以下任意内容，都应该删除旧 `.zippp`：

```text
模型文件
PointNeXt 输出
PointCLIP V2 输出
LOD constraints CSV
P1-P10 策略代码
```

单个模型删除：

```bat
del /f /q "%cd%\_downloaded_resources\my_model.glb.zippp"
del /f /q "%cd%\_downloaded_resources\my_model.glb.zippp_partial"
```

五个模型统一删除：

```bat
del /f /q "%cd%\_downloaded_resources\a.glb.zippp"
del /f /q "%cd%\_downloaded_resources\b.glb.zippp"
del /f /q "%cd%\_downloaded_resources\c.glb.zippp"
del /f /q "%cd%\_downloaded_resources\o1778.glb.zippp"
del /f /q "%cd%\_downloaded_resources\o3049.glb.zippp"
del /f /q "%cd%\_downloaded_resources\*.glb.zippp_partial"
```

如果不删除旧 cache，前端可能仍显示旧 LOD 结果，或者日志出现 cache stale / mismatch。

## 12. Step 6：启动项目查看结果

编译：

```bat
cd /d E:\vk_lod_clusters1\t1
cmake --build build --config Release --parallel 8
```

启动：

```bat
_bin\Release\t1.exe
```

打开模型后，日志中应看到类似：

```text
Semantic LOD: loaded ... rows, ... mesh policies from ..._lod_constraints.csv
```

如果没有看到，检查：

```text
lod_analysis_outputs\my_model_lod_constraints.csv
```

文件名必须和模型 stem 对应：

```text
my_model.glb
my_model_lod_constraints.csv
```

## 13. 前端查看 P1-P10 数量

在 UI 中打开：

```text
Statistics
  -> Semantic LOD Policy Distribution
```

可以看到：

```text
Policy      P1-P10 类型
Instances   当前场景中该类型实例数量
Geometries  当前场景中该类型 unique geometry 数量
Share       占比
```

如果显示：

```text
No semantic LOD policy is attached to the current scene.
```

说明当前模型没有加载到对应的 LOD constraints CSV。

## 14. 前端查看 P1-P10 颜色

在 UI 中打开：

```text
Settings
  -> Rendering
  -> Visualize
  -> semantic lod policy
```

检查重点：

```text
大型静态件是否集中在 P3/P4
螺栓、垫片、销钉是否集中在 P1/P2
接口件是否进入 P7
结构连接和控制件是否进入 P8
运动和关键传动件是否进入 P9/P10
```

## 15. 五个模型批量流程

如果模型是：

```text
a.glb
b.glb
c.glb
o1778.glb
o3049.glb
```

完整批处理顺序：

### 15.1 PointNeXt 批量分析

```bat
cd /d E:\vk_lod_clusters1\t1

python tools\pointnext_lod\batch_analyze_glb_lod.py ^
  --resources-dir "%cd%\_downloaded_resources" ^
  --output-dir "%cd%\lod_analysis_outputs" ^
  --pointnext-root "%cd%\tools\pointnext_lod\PointNeXt" ^
  --cfg "%cd%\tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml" ^
  --ckpt "%cd%\tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth" ^
  --classes-file "%cd%\tools\pointnext_lod\industrial_part_14classes.txt" ^
  --models a.glb b.glb c.glb o1778.glb o3049.glb ^
  --num-points 8192 ^
  --batch-size 16
```

### 15.2 PointCLIP V2 批量分析

```bat
cd /d E:\vk_lod_clusters1\t1
conda activate PointCLIPV2_LOD
tools\pointclipv2\run_pointclipv2_5_glbs.bat
```

### 15.3 融合生成 LOD constraints

```bat
cd /d E:\vk_lod_clusters1\t1
tools\pointclipv2\run_fuse_pointnext_pointclip_5_glbs.bat
```

### 15.4 覆盖默认 LOD constraints

如果当前 Vulkan 项目只读取 `<模型名>_lod_constraints.csv`，执行：

```bat
copy /y lod_analysis_outputs\a_lod_constraints_fused.csv lod_analysis_outputs\a_lod_constraints.csv
copy /y lod_analysis_outputs\b_lod_constraints_fused.csv lod_analysis_outputs\b_lod_constraints.csv
copy /y lod_analysis_outputs\c_lod_constraints_fused.csv lod_analysis_outputs\c_lod_constraints.csv
copy /y lod_analysis_outputs\o1778_lod_constraints_fused.csv lod_analysis_outputs\o1778_lod_constraints.csv
copy /y lod_analysis_outputs\o3049_lod_constraints_fused.csv lod_analysis_outputs\o3049_lod_constraints.csv
```

### 15.5 删除旧 cache

```bat
del /f /q _downloaded_resources\a.glb.zippp
del /f /q _downloaded_resources\b.glb.zippp
del /f /q _downloaded_resources\c.glb.zippp
del /f /q _downloaded_resources\o1778.glb.zippp
del /f /q _downloaded_resources\o3049.glb.zippp
del /f /q _downloaded_resources\*.glb.zippp_partial
```

### 15.6 启动查看

```bat
_bin\Release\t1.exe
```

## 16. 输出文件说明

### PointNeXt 输出

```text
*_pointnext_analysis.csv
```

包含：

```text
predicted_class
confidence
second_class
second_confidence
vertex_count
face_count
surface_area
bbox_diagonal
obb_size_long
obb_size_mid
obb_size_short
elongation
flatness
compactness
shape_hint
```

作用：

```text
闭集工业分类 + 基础结构特征
```

### PointCLIP V2 输出

```text
*_pointclipv2_zeroshot.csv
```

包含：

```text
pointclip_top1_id
pointclip_top1_name
pointclip_top1_role
pointclip_top1_score
pointclip_top2_role
pointclip_margin
pointclip_topk_json
```

作用：

```text
开放词表语义补充
```

### 融合中间输出

```text
*_pointnext_pointclip_merged.csv
```

作用：

```text
检查 PointNeXt 和 PointCLIP V2 是否按 order/node_index/mesh_index 正确对齐
```

### 最终 LOD 输出

```text
*_lod_constraints_fused.csv
```

包含：

```text
lod_priority
lod_strategy
target_ratio_near
target_ratio_mid
target_ratio_far
allow_cull
screen_error_weight
inferred_role
function_role
semantic_structural_score
neural_role
pointclip_role
structural_match_role
protected_features
inference_reason
```

作用：

```text
Vulkan LOD 项目最终读取的语义结构 LOD 约束
```

## 17. 常见问题

### 17.1 PointCLIP V2 很慢

正常。它需要：

```text
点云采样
10 视角投影
CLIP 图像编码
开放词表文本匹配
```

建议：

```text
batch-size 从 4 或 8 开始
先使用 --limit 100 测试
大模型可以夜间批量跑
```

### 17.2 PointNeXt 和 PointCLIP V2 分类冲突

这是正常现象。融合逻辑不会直接让 PointCLIP V2 覆盖 PointNeXt，而是结合结构特征判断：

```text
高置信 PointNeXt + 结构支持
  -> 以 PointNeXt 为主

低置信 PointNeXt + PointCLIP V2 明确 + 结构支持
  -> 采用 PointCLIP V2 作为补充

二者都不确定
  -> 以结构和尺寸决定 LOD
```

### 17.3 前端没有变化

通常是旧 cache 没删。

检查并删除：

```text
_downloaded_resources\<模型名>.glb.zippp
_downloaded_resources\<模型名>.glb.zippp_partial
```

### 17.4 只想用 PointNeXt，不用 PointCLIP V2

可以继续使用旧流程：

```text
*_pointnext_analysis.csv
  -> build_lod_constraints_from_analysis.py
  -> *_lod_constraints.csv
```

`build_lod_constraints_from_analysis.py` 如果没有发现 PointCLIP V2 字段，会自动退化为 PointNeXt + 结构特征推断。

## 18. 最小完整命令模板

单模型完整流程：

```bat
cd /d E:\vk_lod_clusters1\t1

python tools\pointnext_lod\batch_analyze_glb_lod.py ^
  --resources-dir "%cd%\_downloaded_resources" ^
  --output-dir "%cd%\lod_analysis_outputs" ^
  --pointnext-root "%cd%\tools\pointnext_lod\PointNeXt" ^
  --cfg "%cd%\tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml" ^
  --ckpt "%cd%\tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth" ^
  --classes-file "%cd%\tools\pointnext_lod\industrial_part_14classes.txt" ^
  --models my_model.glb ^
  --num-points 8192 ^
  --batch-size 16

conda activate PointCLIPV2_LOD

python tools\pointclipv2\run_pointclipv2_zeroshot_glb.py ^
  --glb _downloaded_resources\my_model.glb ^
  --pointclip-root tools\pointclipv2\PointCLIP_V2 ^
  --prompt-json tools\pointclipv2\industrial_open_vocab_prompts.json ^
  --num-points 8192 ^
  --batch-size 8 ^
  --top-k 5 ^
  --output-csv lod_analysis_outputs\my_model_pointclipv2_zeroshot.csv

python tools\pointclipv2\fuse_pointnext_pointclip_lod.py ^
  --pointnext-csv lod_analysis_outputs\my_model_pointnext_analysis.csv ^
  --pointclip-csv lod_analysis_outputs\my_model_pointclipv2_zeroshot.csv ^
  --merged-csv lod_analysis_outputs\my_model_pointnext_pointclip_merged.csv ^
  --output-csv lod_analysis_outputs\my_model_lod_constraints_fused.csv ^
  --summary-json lod_analysis_outputs\my_model_lod_constraints_fused_summary.json

copy /y lod_analysis_outputs\my_model_lod_constraints_fused.csv lod_analysis_outputs\my_model_lod_constraints.csv
del /f /q _downloaded_resources\my_model.glb.zippp
del /f /q _downloaded_resources\my_model.glb.zippp_partial

_bin\Release\t1.exe
```

## 19. 推荐检查顺序

每次处理新模型后，按这个顺序检查：

1. 是否生成 `*_pointnext_analysis.csv`。
2. 是否生成 `*_pointclipv2_zeroshot.csv`。
3. 是否生成 `*_pointnext_pointclip_merged.csv`。
4. merged 文件中 `pointclip_matched` 是否大部分为 `1`。
5. 是否生成 `*_lod_constraints_fused.csv`。
6. `lod_priority` 是否为 P1-P10。
7. 是否覆盖或正确加载最终 constraints CSV。
8. 是否删除旧 `.zippp`。
9. Vulkan 日志是否显示加载 semantic LOD CSV。
10. 前端 `Semantic LOD Policy Distribution` 是否有 P1-P10 数量。
11. `semantic lod policy` 可视化颜色是否符合结构直觉。

