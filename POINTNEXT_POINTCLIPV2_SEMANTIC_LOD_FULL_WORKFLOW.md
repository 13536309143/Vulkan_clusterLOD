# PointNeXt + PointCLIP V2 + 结构语义 LOD 完整流程

本文档说明当前项目中“工业模型语义推断 + 结构特征融合 + P1-P10 LOD 约束”的完整使用流程。当前代码已经支持：

- PointNeXt 闭集分类。
- PointCLIP V2 开放词表推断。
- 几何结构特征分析。
- 融合生成 `*_lod_constraints_fused.csv`。
- Vulkan LOD 项目优先自动读取 fused 约束文件。

## 1. 总体顺序

完整流程如下：

```text
工业 GLB 模型
  -> PointNeXt 分支：闭集工业类别 + 几何结构特征
  -> PointCLIP V2 分支：开放词表 Top-k 候选
  -> 融合脚本：PointNeXt + PointCLIP V2 + 结构特征
  -> 生成 *_lod_constraints_fused.csv
  -> 删除旧 .zippp cache
  -> Vulkan 项目自动优先读取 fused CSV
  -> 前端查看 P1-P10 数量和颜色
```

有顺序，但不是所有步骤都必须串行：

- PointNeXt 和 PointCLIP V2 可以并行跑。
- 融合必须等两个分支 CSV 都生成后再跑。
- 删除 `.zippp` 必须在重新打开模型前做。

## 2. 核心文件关系

以 `o1778.glb` 为例：

```text
_downloaded_resources\o1778.glb

lod_analysis_outputs\o1778_pointnext_analysis.csv
lod_analysis_outputs\o1778_pointclipv2_zeroshot.csv
lod_analysis_outputs\o1778_pointnext_pointclip_merged.csv
lod_analysis_outputs\o1778_lod_constraints_fused.csv
lod_analysis_outputs\o1778_lod_constraints_fused_summary.json

_downloaded_resources\o1778.glb.zippp
```

当前 C++ 读取顺序已经改成：

```text
优先：lod_analysis_outputs\<模型名>_lod_constraints_fused.csv
回退：lod_analysis_outputs\<模型名>_lod_constraints.csv
```

因此不再需要手动把 fused 文件复制覆盖成普通 constraints 文件。

## 3. 环境建议

PointNeXt 和 PointCLIP V2 理论上可以分环境运行，但你的机器上已经验证 `gpt-pointnext` 环境里的 PyTorch 可用：

```text
torch 2.2.0+cu121
cuda available = True
```

所以推荐 PointCLIPV2 环境直接克隆 `gpt-pointnext`，避免重新安装 PyTorch 导致 DLL 问题。

```bat
conda deactivate
conda env remove -n PointCLIPV2_LOD -y
conda create -n PointCLIPV2_LOD --clone gpt-pointnext -y
conda activate PointCLIPV2_LOD
```

补充常用依赖：

```bat
conda install -n PointCLIPV2_LOD -c conda-forge ftfy regex yacs scikit-learn scipy pandas pillow tqdm -y
```

验证：

```bat
python -c "import torch; print(torch.__version__, torch.cuda.is_available())"
python -c "import numpy as np; import torch; print(torch.from_numpy(np.arange(3)))"
```

说明：

- 当前 PointCLIP V2 投影代码已支持没有 `torch-scatter` 的情况。
- 不需要执行 `python setup.py develop` 安装 Dassl3D。
- `flash attention` warning 可以忽略。
- OpenMP 冲突已在脚本中设置环境变量规避。

## 4. Step 0：放置模型

把模型放到：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources
```

例如：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources\my_model.glb
```

文件名建议只用英文、数字和下划线。

## 5. Step 1：运行 PointNeXt

进入项目目录：

```bat
cd /d E:\vk_lod_clusters1\t1
```

单模型示例：

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
lod_analysis_outputs\my_model_lod_constraints.csv
lod_analysis_outputs\my_model_lod_constraints_summary.json
```

融合时真正需要的是：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
```

## 6. Step 2：运行 PointCLIP V2

先做小测试：

```bat
conda activate PointCLIPV2_LOD
cd /d E:\vk_lod_clusters1\t1

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

确认测试成功后，跑完整模型：

```bat
python tools\pointclipv2\run_pointclipv2_zeroshot_glb.py ^
  --glb _downloaded_resources\a.glb ^
  --pointclip-root tools\pointclipv2\PointCLIP_V2 ^
  --prompt-json tools\pointclipv2\industrial_open_vocab_prompts.json ^
  --num-points 8192 ^
  --batch-size 8 ^
  --top-k 5 ^
  --output-csv lod_analysis_outputs\a_pointclipv2_zeroshot.csv ^
  --output-json lod_analysis_outputs\a_pointclipv2_zeroshot.json ^
  --summary-csv lod_analysis_outputs\a_pointclipv2_summary.csv
```

PointCLIP V2 输出关键字段：

```text
pointclip_top1_id
pointclip_top1_name
pointclip_top1_role
pointclip_top1_score
pointclip_top2_role
pointclip_margin
pointclip_topk_json
```

## 7. Step 3：融合生成 LOD 约束

融合脚本读取：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
lod_analysis_outputs\my_model_pointclipv2_zeroshot.csv
```

执行：

```bat
python tools\pointclipv2\fuse_pointnext_pointclip_lod.py ^
  --pointnext-csv lod_analysis_outputs\a_pointnext_analysis.csv ^
  --pointclip-csv lod_analysis_outputs\a_pointclipv2_zeroshot.csv ^
  --merged-csv lod_analysis_outputs\a_pointnext_pointclip_merged.csv ^
  --output-csv lod_analysis_outputs\a_lod_constraints_fused.csv ^
  --summary-json lod_analysis_outputs\a_lod_constraints_fused_summary.json
```

看到类似输出说明对齐成功：

```text
Matched PointCLIP rows: 1778/1778
Wrote: lod_analysis_outputs\my_model_lod_constraints_fused.csv
```

如果匹配数量明显小于总数，检查两个 CSV 的 `order`、`node_index`、`mesh_index` 是否一致。

## 8. 融合逻辑

融合不是简单投票，而是分工：

```text
PointNeXt：
  负责训练过的工业闭集类别，例如螺栓、轴承、齿轮、管接头等。

PointCLIP V2：
  负责开放词表补充，特别适合处理低置信、未知类别、语义冲突。

结构特征：
  负责 LOD 约束本身，包括尺寸、细长、薄壁、圆盘/环状、大体块、面数密度等。
```

典型判断：

```text
PointNeXt 高置信 + PointCLIP V2 同意 + 结构匹配
  -> 高可信语义。

PointNeXt 高置信 + PointCLIP V2 不同意 + 结构支持 PointNeXt
  -> 仍以 PointNeXt 为主。

PointNeXt 低置信 + PointCLIP V2 明确 + 结构匹配
  -> 使用 PointCLIP V2 作为补充。

二者都不确定
  -> 主要按结构特征和尺寸决定 LOD。

超大型静态体块
  -> 优先快速简化。

运动、传动、接口、结构关键件
  -> 语义和结构同时支持时才提高保护等级。
```

## 9. P1-P10 策略含义

| 类别 | 名称 | 含义 |
|---|---|---|
| P1 | micro uncertain | 微小件、低置信度小件、远处可剔除 |
| P2 | repeated fastener | 螺栓、螺母、垫片、销、铆钉、键等重复标准件 |
| P3 | large static bulk | 地基、房屋、底座、墙体、大型静态支撑 |
| P4 | ordinary low detail | 普通低细节零件、简单块体 |
| P5 | balanced visible | 普通可见结构件 |
| P6 | high detail shape | 高细节、高面数或外形复杂件 |
| P7 | interface fluid | 管接头、阀、喷嘴、法兰、接口件 |
| P8 | structural control | 夹具、连接件、铰链、控制件、把手 |
| P9 | motion precision | 一般运动件、精密导向件、轮类、滑轨 |
| P10 | critical preserve | 齿轮、轴承、电机、弹簧、关键传动/运动件 |

大致强度：

```text
P1-P3   快速简化或远处剔除
P4-P6   中等保护
P7-P10  重点保护
```

## 10. Step 4：删除旧 cache

只要更新过 LOD CSV 或策略代码，就删除旧 `.zippp`：

```bat
del /f /q _downloaded_resources\my_model.glb.zippp
del /f /q _downloaded_resources\my_model.glb.zippp_partial
```

五个模型：

```bat
del /f /q _downloaded_resources\a.glb.zippp
del /f /q _downloaded_resources\b.glb.zippp
del /f /q _downloaded_resources\c.glb.zippp
del /f /q _downloaded_resources\o1778.glb.zippp
del /f /q _downloaded_resources\o3049.glb.zippp
del /f /q _downloaded_resources\*.glb.zippp_partial
```

不删除 cache 时，可能仍显示旧策略，或者日志提示 stale / mismatch。

## 11. Step 5：启动并查看

编译：

```bat
cmake --build build --config Release --parallel 8
```

启动：

```bat
_bin\Release\t1.exe
```

打开模型后，日志应显示加载 fused 或普通 constraints：

```text
Semantic LOD: loaded ... from ..._lod_constraints_fused.csv
```

前端查看：

```text
Statistics -> Semantic LOD Policy Distribution
Settings -> Rendering -> Visualize -> semantic lod policy
```

重点检查：

- 大型静态件是否集中在 P3/P4。
- 螺栓、垫片、销钉是否集中在 P1/P2。
- 接口件是否进入 P7。
- 结构连接和控制件是否进入 P8。
- 运动和关键传动件是否进入 P9/P10。

## 12. 五个模型批量流程

模型列表：

```text
a.glb
b.glb
c.glb
o1778.glb
o3049.glb
```

### 12.1 PointNeXt 批量分析

```bat
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

### 12.2 PointCLIP V2 批量分析

```bat
conda activate PointCLIPV2_LOD
tools\pointclipv2\run_pointclipv2_5_glbs.bat
```

### 12.3 融合

```bat
tools\pointclipv2\run_fuse_pointnext_pointclip_5_glbs.bat
```

### 12.4 删除 cache

```bat
del /f /q _downloaded_resources\*.glb.zippp
del /f /q _downloaded_resources\*.glb.zippp_partial
```

然后启动程序查看。

## 13. 常见问题

### 13.1 PointCLIP V2 很慢

正常。它会做点云采样、10 视角投影、CLIP 图像编码和开放词表匹配。建议：

```text
先 --limit 100 测试
batch-size 从 4 或 8 开始
大模型批量任务可以夜间跑
```

### 13.2 `Torch was not compiled with flash attention`

可以忽略，不影响推断。

### 13.3 NumPy / OpenMP 报错

当前脚本已经规避 OpenMP 冲突。如果 NumPy 报错，优先确认：

```bat
python -c "import numpy as np; import torch; print(np.__version__); print(torch.from_numpy(np.arange(3)))"
```

如果失败，重装 conda-forge NumPy：

```bat
pip uninstall numpy -y
conda install -c conda-forge numpy=1.26.4 -y
```

### 13.4 前端没变化

通常是旧 cache 没删，或者没有生成 fused 文件。

检查：

```text
lod_analysis_outputs\<模型名>_lod_constraints_fused.csv
_downloaded_resources\<模型名>.glb.zippp
```

### 13.5 只想用 PointNeXt

可以只跑旧流程。若没有 fused 文件，项目会自动回退读取：

```text
<模型名>_lod_constraints.csv
```

## 14. 最小完整命令

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

del /f /q _downloaded_resources\my_model.glb.zippp
del /f /q _downloaded_resources\my_model.glb.zippp_partial

_bin\Release\t1.exe
```

## 15. 最终检查清单

1. 是否生成 `*_pointnext_analysis.csv`。
2. 是否生成 `*_pointclipv2_zeroshot.csv`。
3. 是否生成 `*_pointnext_pointclip_merged.csv`。
4. `pointclip_matched` 是否大部分为 `1`。
5. 是否生成 `*_lod_constraints_fused.csv`。
6. `lod_priority` 是否为 P1-P10。
7. 是否删除旧 `.zippp`。
8. 日志是否显示加载 fused CSV。
9. `Semantic LOD Policy Distribution` 是否有 P1-P10 数量。
10. `semantic lod policy` 颜色是否符合结构直觉。

