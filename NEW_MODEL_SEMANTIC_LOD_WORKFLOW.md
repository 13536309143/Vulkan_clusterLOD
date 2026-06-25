# 新模型语义结构 LOD 完整操作文档

本文档说明当导入新的 `.glb` / `.gltf` 模型时，如何使用 PointNeXt 推断子零件类型，生成 P1-P10 语义结构 LOD 约束，并在 Vulkan Cluster LOD 项目前端查看每类数量和可视化结果。

## 1. 总流程

完整流程如下：

```text
新模型 .glb / .gltf
  -> PointNeXt 子 mesh 推断
  -> 生成 *_pointnext_analysis.csv
  -> 生成 P1-P10 *_lod_constraints.csv
  -> Vulkan 项目读取 CSV
  -> 重建 .zippp 缓存
  -> 前端查看 Semantic LOD Policy Distribution
  -> 使用 semantic lod policy 可视化检查 P1-P10 颜色
```

核心文件关系：

```text
_downloaded_resources\模型.glb
lod_analysis_outputs\模型_pointnext_analysis.csv
lod_analysis_outputs\模型_lod_constraints.csv
_downloaded_resources\模型.glb.zippp
```

注意：

- Vulkan 项目只读取 `lod_analysis_outputs\<模型名>_lod_constraints.csv`。
- 如果修改了分类脚本、CSV 或 P1-P10 策略，必须删除旧 `.zippp` 或重新预处理。
- 前端统计的是当前场景实例数量，因此开启模型阵列复制后，instance 数会随之变化。

## 2. P1-P10 类型定义

当前语义结构 LOD 分类为 10 类：

| 类别 | 名称 | 含义 |
|---|---|---|
| P1 | micro / uncertain | 微小件、低置信度小件、极薄小片 |
| P2 | repeated fastener | 螺栓、螺母、垫片、铆钉等重复标准件 |
| P3 | large static bulk | 地基、房屋、壳体、大型静态板件 |
| P4 | ordinary low detail | 普通低细节件、简单块体 |
| P5 | balanced visible | 普通可见结构件 |
| P6 | high-detail shape | 高面数复杂外形件 |
| P7 | interface / fluid | 法兰、管接头、阀、喷嘴、装配接口 |
| P8 | structural / control | 夹具、连接件、控制件、把手 |
| P9 | motion / precision | 一般运动件、精密件、滑轨、轮类 |
| P10 | critical preserve | 齿轮、轴承、电机、弹簧、关键运动件 |

LOD 策略强度大致为：

```text
P1-P3：快速简化
P4-P6：中等保护
P7-P10：重点保护
```

## 3. 放置模型

推荐将模型放在：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources
```

例如：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources\my_model.glb
```

建议文件名只使用英文、数字和下划线，避免空格和特殊符号。

## 4. 检查 PointNeXt 环境

进入项目目录：

```bat
cd /d E:\vk_lod_clusters1\t1
```

使用默认 Python 检查：

```bat
python -c "import sys; sys.path.insert(0, r'%cd%\tools\pointnext_lod\PointNeXt'); import torch; print(torch.__version__, torch.cuda.is_available()); from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnet2 ok')"
```

如果你有单独 Python 环境：

```bat
set PYTHON_EXE=D:\path\to\python.exe
%PYTHON_EXE% -c "import sys; sys.path.insert(0, r'%cd%\tools\pointnext_lod\PointNeXt'); import torch; print(torch.__version__, torch.cuda.is_available()); from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnet2 ok')"
```

看到类似输出即可：

```text
True
pointnet2 ok
```

## 5. 单模型推断并生成 P1-P10 约束

假设模型为：

```text
_downloaded_resources\my_model.glb
```

运行：

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
```

如果设置了 `PYTHON_EXE`，把 `python` 替换为：

```bat
%PYTHON_EXE%
```

运行完成后应生成：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
lod_analysis_outputs\my_model_pointnext_analysis.json
lod_analysis_outputs\my_model_pointnext_class_summary.csv
lod_analysis_outputs\my_model_pointnext_review_candidates.csv
lod_analysis_outputs\my_model_lod_constraints.csv
lod_analysis_outputs\my_model_lod_constraints_summary.json
```

其中最关键的是：

```text
lod_analysis_outputs\my_model_lod_constraints.csv
```

Vulkan 项目会读取它。

## 6. 只重新生成 P1-P10 约束

如果 PointNeXt 分析 CSV 已经存在，只是修改了 P1-P10 规则，不需要重新推断模型。直接运行约束生成脚本：

```bat
cd /d E:\vk_lod_clusters1\t1

python tools\pointnext_lod\build_lod_constraints_from_analysis.py ^
  --input-csv "%cd%\lod_analysis_outputs\my_model_pointnext_analysis.csv" ^
  --output-csv "%cd%\lod_analysis_outputs\my_model_lod_constraints.csv" ^
  --summary-json "%cd%\lod_analysis_outputs\my_model_lod_constraints_summary.json"
```

生成后检查 CSV 中的 `lod_priority` 列，应出现类似：

```text
P1_micro_uncertain
P2_repeated_fastener
P3_large_static_bulk
P4_ordinary_low_detail
P5_balanced_visible
P6_high_detail_shape
P7_interface_fluid
P8_structural_control
P9_motion_precision
P10_critical_preserve
```

## 7. 批量处理多个模型

如果 `_downloaded_resources` 中有多个模型，例如：

```text
a.glb
b.glb
c.glb
o1778.glb
o3049.glb
```

可以使用：

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

每个模型都会生成自己的：

```text
<模型名>_pointnext_analysis.csv
<模型名>_lod_constraints.csv
```

## 8. 重建 LOD 缓存

如果已经存在旧缓存：

```text
_downloaded_resources\my_model.glb.zippp
```

修改 CSV 或 P1-P10 策略后，必须删除旧缓存或通过项目菜单删除缓存。命令行删除示例：

```bat
del "%cd%\_downloaded_resources\my_model.glb.zippp"
```

然后启动项目并打开模型，项目会重新预处理并生成新的 `.zippp`。

也可以使用现有批处理脚本统一预处理多个模型：

```bat
run_preprocess_semantic_lod_5_glbs.bat
```

如果这个脚本里的模型列表不是你当前要处理的模型，需要先编辑脚本中的模型名。

## 9. 打开项目查看结果

启动程序：

```bat
cd /d E:\vk_lod_clusters1\t1
_bin\Release\t1.exe
```

打开模型后，确认日志中能看到类似信息：

```text
Semantic LOD: loaded ... rows, ... mesh policies from ..._lod_constraints.csv
```

如果没有看到这类信息，说明 CSV 没有被找到。请检查：

```text
lod_analysis_outputs\my_model_lod_constraints.csv
```

文件名必须和模型 stem 对应：

```text
my_model.glb
my_model_lod_constraints.csv
```

## 10. 前端查看 P1-P10 数量

打开 UI 里的统计面板：

```text
Statistics
  -> Semantic LOD Policy Distribution
```

该表会显示：

```text
Policy      P1-P10 类型名
Instances   当前场景中该类型实例数量
Geometries  当前场景中该类型 unique geometry 数量
Share       该类型实例占比
```

如果显示：

```text
No semantic LOD policy is attached to the current scene.
```

说明当前模型没有加载到语义约束 CSV，或打开的是没有对应 CSV 的模型。

## 11. 查看 P1-P10 颜色

在 Settings 面板中找到：

```text
Rendering
  -> Visualize
```

选择：

```text
semantic lod policy
```

此模式会将 P1-P10 分别染色。用途：

- 检查大型静态件是否多为 P3。
- 检查螺栓、垫片等是否多为 P1/P2。
- 检查接口件是否进入 P7/P8。
- 检查运动件和关键件是否进入 P9/P10。

如果颜色分布明显不合理，优先检查：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
lod_analysis_outputs\my_model_lod_constraints.csv
```

## 12. 常见问题

### 12.1 前端没有 P1-P10 数量

原因通常是 CSV 没有加载。检查：

```text
lod_analysis_outputs\<模型名>_lod_constraints.csv
```

模型名必须完全对应。例如：

```text
c.glb
c_lod_constraints.csv
```

### 12.2 修改规则后前端结果没变

原因通常是旧 `.zippp` 缓存还在。删除：

```text
_downloaded_resources\<模型名>.glb.zippp
```

然后重新打开模型。

### 12.3 CSV 仍是 P1-P5

说明使用了旧脚本或旧输出。重新运行：

```bat
python tools\pointnext_lod\build_lod_constraints_from_analysis.py ^
  --input-csv "%cd%\lod_analysis_outputs\<模型名>_pointnext_analysis.csv" ^
  --output-csv "%cd%\lod_analysis_outputs\<模型名>_lod_constraints.csv" ^
  --summary-json "%cd%\lod_analysis_outputs\<模型名>_lod_constraints_summary.json"
```

### 12.4 某些零件分类不合理

优先查看：

```text
*_pointnext_review_candidates.csv
```

低置信度、小尺寸、高歧义的零件通常需要人工检查。然后根据情况调整：

```text
tools\pointnext_lod\build_lod_constraints_from_analysis.py
```

中的 P1-P10 细分规则。

## 13. 推荐检查顺序

每次导入新模型后，按这个顺序检查：

1. `*_pointnext_analysis.csv` 是否生成。
2. `*_lod_constraints.csv` 是否生成。
3. `lod_priority` 是否为 P1-P10。
4. 删除旧 `.zippp`。
5. 打开模型并确认日志加载 CSV。
6. 查看 `Semantic LOD Policy Distribution`。
7. 切换 `semantic lod policy` 可视化。
8. 检查 P1-P10 分布是否符合模型结构。

## 14. 当前代码对应关系

| 功能 | 文件 |
|---|---|
| PointNeXt 批量分析 | `tools\pointnext_lod\batch_analyze_glb_lod.py` |
| P1-P10 约束生成 | `tools\pointnext_lod\build_lod_constraints_from_analysis.py` |
| CSV 读取和策略映射 | `src\scene\scene_semantic_lod.cpp` |
| LOD 构建应用策略 | `src\scene\clusterlod.cpp` |
| P1-P10 前端统计 | `src\app\lodclusters_ui.cpp` |
| P1-P10 shader 颜色 | `shaders\common\render_shading.glsl` |

## 15. 最小命令示例

新模型只需要记住这几步：

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

del "%cd%\_downloaded_resources\my_model.glb.zippp"

_bin\Release\t1.exe
```

打开程序后查看：

```text
Statistics -> Semantic LOD Policy Distribution
Settings -> Rendering -> Visualize -> semantic lod policy
```
