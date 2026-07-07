# PointNeXt + PointCLIP V2 语义结构 LOD 完整流程

本文说明当前项目中“PointNeXt 闭集分类 + PointCLIP V2 开放词表推断 + 结构特征分析 + P1-P10 LOD 策略融合”的完整执行流程。它面向实际工程使用：当导入新的工业 GLB 模型时，如何先生成语义分析结果，再让 Vulkan LOD 项目自动加载对应策略。

当前项目已经支持：

- PointNeXt 对工业零件进行闭集分类。
- PointCLIP V2 只对 PointNeXt 低置信或歧义 mesh 做开放词表 zero-shot 补充推断。
- 结构特征提取，包括尺寸、面数、包围盒、细长性、扁平性、紧凑性、形状提示等。
- 融合生成 `*_lod_constraints_fused.csv`。
- C++ 运行时优先读取 fused 文件，再回退到普通 `*_lod_constraints.csv`。
- 前端用 semantic policy 可视化 P1-P10 颜色和数量分布。

## 1. 总体顺序

完整流程如下：

```text
工业 GLB 模型
  -> PointNeXt 全量推断
       输出：*_pointnext_analysis.csv
       内容：闭集类别、置信度、几何结构特征

  -> PointCLIP V2 候选补充
       输入：PointNeXt CSV 中未达到高置信或 margin 过低的 mesh
       输出：*_pointclipv2_candidates.csv
       内容：开放词表 Top-k、role、score、margin

  -> 结构特征判断和融合脚本
       输入：PointNeXt CSV + PointCLIP V2 CSV
       输出：*_pointnext_pointclip_merged.csv
       输出：*_lod_constraints_fused.csv

  -> 删除旧 .zippp cache
       让 C++ 重新构建 LOD cache

  -> 打开 Vulkan 项目
       自动优先读取 *_lod_constraints_fused.csv
       构建语义结构约束 LOD
```

PointCLIP V2 依赖 PointNeXt 的置信度筛选，因此推荐在 PointNeXt 之后执行。融合步骤必须等 PointNeXt CSV 和 PointCLIP V2 候选 CSV 都生成后再执行。

## 2. 关键文件关系

以 `o1778.glb` 为例，输入和输出文件关系如下：

```text
_downloaded_resources\o1778.glb

lod_analysis_outputs\o1778_pointnext_analysis.csv
lod_analysis_outputs\o1778_pointnext_analysis.json

lod_analysis_outputs\o1778_pointclipv2_candidates.csv
lod_analysis_outputs\o1778_pointclipv2_candidates.json
lod_analysis_outputs\o1778_pointclipv2_candidates_summary.csv

lod_analysis_outputs\o1778_pointnext_pointclip_merged.csv
lod_analysis_outputs\o1778_lod_constraints_fused.csv
lod_analysis_outputs\o1778_lod_constraints_fused_summary.json

_downloaded_resources\o1778.glb.zippp
```

C++ 读取顺序已经设置为：

```text
优先读取：lod_analysis_outputs\<模型名>_lod_constraints_fused.csv
回退读取：lod_analysis_outputs\<模型名>_lod_constraints.csv
```

所以 fused 生成后，不需要手动复制覆盖成普通 constraints 文件。

## 3. 环境建议

### 3.1 PointNeXt 环境

PointNeXt 需要能正常导入：

```bat
python -c "import torch; print(torch.__version__, torch.cuda.is_available())"
python -c "from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnet2 ok')"
```

当前批处理默认使用当前命令行里的 `python`。如果你要指定 Python，可以先设置：

```bat
set PYTHON_EXE=D:\path\to\python.exe
```

### 3.2 PointCLIP V2 环境

建议创建单独环境 `PointCLIPV2_LOD`。如果直接安装 PyTorch 出现 DLL 问题，可以使用项目里提供的克隆方式：

```bat
tools\pointclipv2\recreate_pointclipv2_env.bat
```

它会执行：

```bat
conda env remove -n PointCLIPV2_LOD -y
conda create -n PointCLIPV2_LOD --clone gpt-pointnext -y
conda run -n PointCLIPV2_LOD python -m pip install ftfy regex tqdm scipy scikit-learn yacs pillow pandas
```

验证：

```bat
conda activate PointCLIPV2_LOD
python -c "import torch; print(torch.__version__, torch.cuda.is_available())"
```

如果出现 OpenMP 冲突，可以在命令行临时设置：

```bat
set KMP_DUPLICATE_LIB_OK=TRUE
set OMP_NUM_THREADS=1
set MKL_NUM_THREADS=1
```

## 4. 一键分析 `_downloaded_resources` 下全部 GLB

当前推荐使用根目录脚本：

```bat
cd /d E:\vk_lod_clusters1\t1
run_all_glb_pointnext_pointclipv2_fused.bat
```

该脚本会自动扫描：

```text
_downloaded_resources\*.glb
```

然后按三个阶段执行。

### 4.1 PointNeXt 阶段

脚本会先执行：

```bat
call conda activate gpt-pointnext
```

然后检查：

```bat
python -c "import torch; print(torch.__version__, torch.cuda.is_available())"
python -c "from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnet2 ok')"
```

随后调用：

```bat
python tools\pointnext_lod\batch_analyze_glb_lod.py ^
  --resources-dir "%cd%\_downloaded_resources" ^
  --output-dir "%cd%\lod_analysis_outputs" ^
  --pointnext-root "%cd%\tools\pointnext_lod\PointNeXt" ^
  --cfg "%cd%\tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml" ^
  --ckpt "%cd%\tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth" ^
  --classes-file "%cd%\tools\pointnext_lod\industrial_part_14classes.txt" ^
  --num-points 8192 ^
  --batch-size 16 ^
  --pointnext-only ^
  --skip-existing
```

`batch_analyze_glb_lod.py` 现在不传 `--models` 时，会自动处理资源目录下全部 `.glb`。

输出示例：

```text
lod_analysis_outputs\<模型名>_pointnext_analysis.csv
lod_analysis_outputs\<模型名>_pointnext_analysis.json
lod_analysis_outputs\<模型名>_pointnext_class_summary.csv
lod_analysis_outputs\<模型名>_pointnext_review_candidates.csv
```

### 4.2 PointCLIP V2 阶段

脚本会切换环境：

```bat
call conda activate PointCLIPV2_LOD
```

然后对每个 `.glb` 调用：

```bat
python tools\pointclipv2\run_pointclipv2_zeroshot_glb.py ^
  --glb _downloaded_resources\<模型名>.glb ^
  --pointclip-root tools\pointclipv2\PointCLIP_V2 ^
  --prompt-json tools\pointclipv2\industrial_open_vocab_prompts.json ^
  --candidate-csv lod_analysis_outputs\<模型名>_pointnext_analysis.csv ^
  --num-points 8192 ^
  --batch-size 8 ^
  --top-k 5 ^
  --output-csv lod_analysis_outputs\<模型名>_pointclipv2_candidates.csv ^
  --output-json lod_analysis_outputs\<模型名>_pointclipv2_candidates.json ^
  --summary-csv lod_analysis_outputs\<模型名>_pointclipv2_candidates_summary.csv
```

输出示例：

```text
lod_analysis_outputs\<模型名>_pointclipv2_candidates.csv
lod_analysis_outputs\<模型名>_pointclipv2_candidates.json
lod_analysis_outputs\<模型名>_pointclipv2_candidates_summary.csv
```

### 4.3 融合阶段

融合阶段仍在 `PointCLIPV2_LOD` 环境中执行，因为融合脚本只读写 CSV，不依赖 PointNeXt CUDA 扩展。

对每个模型调用：

```bat
python tools\pointclipv2\fuse_pointnext_pointclip_lod.py ^
  --pointnext-csv lod_analysis_outputs\<模型名>_pointnext_analysis.csv ^
  --pointclip-csv lod_analysis_outputs\<模型名>_pointclipv2_candidates.csv ^
  --merged-csv lod_analysis_outputs\<模型名>_pointnext_pointclip_merged.csv ^
  --output-csv lod_analysis_outputs\<模型名>_lod_constraints_fused.csv ^
  --summary-json lod_analysis_outputs\<模型名>_lod_constraints_fused_summary.json
```

输出示例：

```text
lod_analysis_outputs\<模型名>_pointnext_pointclip_merged.csv
lod_analysis_outputs\<模型名>_lod_constraints_fused.csv
lod_analysis_outputs\<模型名>_lod_constraints_fused_summary.json
```

融合脚本按下面三个字段对齐：

```text
order
node_index
mesh_index
```

运行时应看到类似：

```text
Matched PointCLIP candidate rows: 920/1778
Wrote: lod_analysis_outputs\o1778_pointnext_pointclip_merged.csv
Wrote: lod_analysis_outputs\o1778_lod_constraints_fused.csv
Wrote: lod_analysis_outputs\o1778_lod_constraints_fused_summary.json
```

门控流程下，匹配数量小于总行数是正常现象，表示只有低置信或歧义 mesh 进入了 PointCLIP。只有当候选 CSV 本身不为空但匹配数量异常为 0 时，才需要检查两个分支是否分析的是同一个 GLB。

### 4.4 环境名覆盖

默认环境名是：

```text
PointNeXt:    gpt-pointnext
PointCLIP V2: PointCLIPV2_LOD
```

如果你的环境名不同，运行前设置：

```bat
set POINTNEXT_CONDA_ENV=你的PointNeXt环境名
set POINTCLIP_CONDA_ENV=你的PointCLIP环境名
run_all_glb_pointnext_pointclipv2_fused.bat
```

### 4.5 跳过已有结果

一键脚本默认会跳过已存在的 PointNeXt 和 PointCLIP V2 候选输出，避免重复跑耗时推断。融合阶段会重新生成 fused CSV，方便你调整策略融合逻辑后快速重建最终约束。

如果要强制重新推断，删除对应的 `*_pointnext_analysis.csv` 或 `*_pointclipv2_candidates.csv` 后重跑脚本。

## 5. 单个模型命令

如果只分析一个模型，例如 `o1778.glb`，可以分别运行：

### 5.1 PointNeXt

```bat
python tools\pointnext_lod\analyze_large_glb_parts_pointnext.py ^
  --glb _downloaded_resources\o1778.glb ^
  --pointnext-root tools\pointnext_lod\PointNeXt ^
  --cfg tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml ^
  --ckpt tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth ^
  --classes-file tools\pointnext_lod\industrial_part_14classes.txt ^
  --num-points 8192 ^
  --batch-size 16 ^
  --output-csv lod_analysis_outputs\o1778_pointnext_analysis.csv ^
  --output-json lod_analysis_outputs\o1778_pointnext_analysis.json
```

### 5.2 PointCLIP V2

```bat
python tools\pointclipv2\run_pointclipv2_zeroshot_glb.py ^
  --glb _downloaded_resources\o1778.glb ^
  --pointclip-root tools\pointclipv2\PointCLIP_V2 ^
  --prompt-json tools\pointclipv2\industrial_open_vocab_prompts.json ^
  --candidate-csv lod_analysis_outputs\o1778_pointnext_analysis.csv ^
  --num-points 8192 ^
  --batch-size 8 ^
  --top-k 5 ^
  --output-csv lod_analysis_outputs\o1778_pointclipv2_candidates.csv ^
  --output-json lod_analysis_outputs\o1778_pointclipv2_candidates.json ^
  --summary-csv lod_analysis_outputs\o1778_pointclipv2_candidates_summary.csv
```

### 5.3 融合

```bat
python tools\pointclipv2\fuse_pointnext_pointclip_lod.py ^
  --pointnext-csv lod_analysis_outputs\o1778_pointnext_analysis.csv ^
  --pointclip-csv lod_analysis_outputs\o1778_pointclipv2_candidates.csv ^
  --merged-csv lod_analysis_outputs\o1778_pointnext_pointclip_merged.csv ^
  --output-csv lod_analysis_outputs\o1778_lod_constraints_fused.csv ^
  --summary-json lod_analysis_outputs\o1778_lod_constraints_fused_summary.json
```

## 6. 让 C++ 项目加载新策略

生成 fused CSV 后，必须删除旧 cache，否则程序可能继续使用旧的 `.zippp`：

```bat
del /f /q _downloaded_resources\o1778.glb.zippp
del /f /q _downloaded_resources\o1778.glb.zippp_partial
```

然后运行程序并打开模型：

```bat
_bin\Release\t1.exe
```

或直接用命令行打开：

```bat
_bin\Release\t1.exe --scene _downloaded_resources\o1778.glb
```

如果加载成功，日志应出现类似信息：

```text
Semantic LOD: loaded ... rows, ... mesh policies from ..._lod_constraints_fused.csv
```

如果没有找到 fused，程序会回退到：

```text
*_lod_constraints.csv
```

## 7. 输出 CSV 关键字段

`*_lod_constraints_fused.csv` 每一行对应一个原始 mesh，主要字段如下：

| 字段 | 含义 |
|---|---|
| `order` | mesh 在分析输出中的顺序 |
| `node_index` | GLB node index |
| `mesh_index` | GLB mesh index |
| `predicted_class` | PointNeXt 闭集类别 |
| `confidence` | PointNeXt top1 置信度 |
| `second_class` | PointNeXt top2 类别 |
| `bbox_diagonal` | mesh 包围盒对角线 |
| `face_count` | 面数 |
| `shape_hint` | 几何形状提示 |
| `pointclip_top1_name` | PointCLIP V2 top1 开放词表名称 |
| `pointclip_top1_role` | PointCLIP V2 映射后的功能角色 |
| `pointclip_top1_score` | PointCLIP V2 top1 分数 |
| `inferred_role` | 融合后的功能角色 |
| `semantic_structural_score` | 融合置信评分 |
| `lod_priority` | P1-P10 最终策略 |
| `lod_strategy` | 策略名称 |
| `target_ratio_near` | 近距离目标保留比例 |
| `target_ratio_mid` | 中距离目标保留比例 |
| `target_ratio_far` | 远距离目标保留比例 |
| `allow_cull` | 是否允许远距离剔除 |
| `screen_error_weight` | 屏幕误差权重 |

## 8. P1-P10 策略含义

| 策略 | 角色 | 简化倾向 |
|---|---|---|
| P1 | micro / uncertain | 微小或低置信零件，远处可强简化或剔除 |
| P2 | repeated fastener | 螺栓、螺母、垫圈、销等重复标准件，快速简化 |
| P3 | large static bulk | 大型静态构件、地基、壳体、板件，快速降面 |
| P4 | ordinary low detail | 普通低细节零件，中等偏激进简化 |
| P5 | balanced visible | 一般可见结构件，平衡质量和性能 |
| P6 | high-detail shape | 复杂外形件，保留轮廓和高曲率区域 |
| P7 | interface / fluid | 管件、阀门、接口件，保护接口边界 |
| P8 | structural / control | 夹具、连接件、控制件，保护接触面和结构轮廓 |
| P9 | motion / precision | 导向、运动、精密配合件，高保护 |
| P10 | critical preserve | 齿轮、轴承、电机等关键件，最高保护 |

## 9. 前端查看方式

打开程序后：

1. 加载对应 GLB。
2. 切换 visualization 到 `semantic lod policy`。
3. 查看右侧或底部面板中的 `Semantic LOD Policy Distribution`。
4. 对照颜色判断 P1-P10 是否合理。

当前颜色大致为：

```text
P1  灰色
P2  棕色
P3  橙黄色
P4  黄色
P5  绿色
P6  青绿色
P7  浅蓝色
P8  蓝色
P9  紫色
P10 粉红 / 红色
```

## 10. 常见问题

### 10.1 为什么 fused 已生成但程序没有变化

优先检查：

```text
是否删除了 _downloaded_resources\<模型>.glb.zippp
fused 文件名是否为 <模型名>_lod_constraints_fused.csv
fused 文件是否位于 lod_analysis_outputs
日志是否显示 loaded ..._lod_constraints_fused.csv
```

### 10.2 为什么 PointCLIP V2 很慢

PointCLIP V2 会对每个 mesh 点云做多视角投影并送入 CLIP，速度比纯几何规则慢。可以先用：

```bat
--limit 100
```

做小测试。

### 10.3 为什么 b、c 的 fused 改动比 a 大

因为 old 文件中大量 mesh 被压到 `P1_micro_uncertain`，fused 借助 PointCLIP V2 和结构特征把它们重新分配到 P2、P3、P6、P8 等更具体策略中。`a` 的 old 本来已经比较稳定，所以 fused 改动较小。

### 10.4 什么时候只用 PointNeXt 结果

如果 PointCLIP V2 环境暂时不可用，可以只用 PointNeXt 分析结果生成普通策略文件。进入 `gpt-pointnext` 环境后运行：

```bat
conda activate gpt-pointnext
python tools\pointnext_lod\batch_analyze_glb_lod.py ^
  --resources-dir "%cd%\_downloaded_resources" ^
  --output-dir "%cd%\lod_analysis_outputs" ^
  --pointnext-root "%cd%\tools\pointnext_lod\PointNeXt" ^
  --cfg "%cd%\tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml" ^
  --ckpt "%cd%\tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth" ^
  --classes-file "%cd%\tools\pointnext_lod\industrial_part_14classes.txt" ^
  --num-points 8192 ^
  --batch-size 16
```

它会为 `_downloaded_resources` 下所有 `.glb` 生成或更新：

```text
*_pointnext_analysis.csv
*_lod_constraints.csv
```

但推荐最终使用 fused，因为它能减少不确定类别堆积，并补充开放词表语义。
