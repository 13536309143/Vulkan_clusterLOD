# 新模型 PointNeXt 语义 LOD 完整操作文档

本文档说明当你拿到一个新的 `.glb` / `.gltf` 模型时，如何先用 PointNeXt 推断子零件类型，再生成 LOD 分类约束，最后导入本 Vulkan Cluster LOD 项目查看效果。

## 0. 总流程

完整流程是：

```text
新模型
  -> PointNeXt 子 mesh 推断
  -> 生成 *_pointnext_analysis.csv
  -> 生成 *_lod_constraints.csv
  -> Vulkan 项目读取 CSV
  -> 预处理生成 .zippp 缓存
  -> 打开模型查看 LOD / semantic policy 颜色
```

核心原则：

- PointNeXt 负责识别子零件类型。
- `build_lod_constraints_from_analysis.py` 负责把类型、大小、形状、置信度转成 P1-P5 LOD 策略。
- Vulkan 项目只读取 `lod_analysis_outputs\<模型名>_lod_constraints.csv`。
- 改了 CSV 或策略后，必须重新生成 `.zippp` 缓存。

## 1. 放置模型

推荐把新模型放到：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources
```

例如：

```text
E:\vk_lod_clusters1\t1\_downloaded_resources\my_model.glb
```

后续输出文件名会自动使用模型 stem：

```text
my_model_pointnext_analysis.csv
my_model_lod_constraints.csv
my_model.glb.zippp
```

重要：模型文件名不要包含奇怪符号，推荐只用英文、数字、下划线。

## 2. 检查 PointNeXt 环境

进入项目目录：

```bat
cd /d E:\vk_lod_clusters1\t1
```

如果你用默认 Python：

```bat
python -c "import sys; sys.path.insert(0, r'%cd%\tools\pointnext_lod\PointNeXt'); import torch; print(torch.__version__, torch.cuda.is_available()); from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnet2 ok')"
```

如果你有专门的 Python 环境，先指定：

```bat
set PYTHON_EXE=D:\path\to\python.exe
```

然后用：

```bat
%PYTHON_EXE% -c "import sys; sys.path.insert(0, r'%cd%\tools\pointnext_lod\PointNeXt'); import torch; print(torch.__version__, torch.cuda.is_available()); from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnet2 ok')"
```

看到：

```text
True
pointnet2 ok
```

说明环境可用。

## 3. 单个 GLB 模型：推断 + 生成 LOD 约束

假设模型是：

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

如果你指定了 `PYTHON_EXE`，把第一行的 `python` 改成：

```bat
%PYTHON_EXE%
```

运行完成后应该得到：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
lod_analysis_outputs\my_model_pointnext_analysis.json
lod_analysis_outputs\my_model_pointnext_class_summary.csv
lod_analysis_outputs\my_model_pointnext_review_candidates.csv
lod_analysis_outputs\my_model_lod_constraints.csv
lod_analysis_outputs\my_model_lod_constraints_summary.json
```

最关键的是：

```text
lod_analysis_outputs\my_model_lod_constraints.csv
```

Vulkan 项目会自动读取它。

## 4. 多个 GLB 模型批量推断

假设有：

```text
_downloaded_resources\model_a.glb
_downloaded_resources\model_b.glb
_downloaded_resources\model_c.glb
```

把 `--models` 后面写成多个文件：

```bat
python tools\pointnext_lod\batch_analyze_glb_lod.py ^
  --resources-dir "%cd%\_downloaded_resources" ^
  --output-dir "%cd%\lod_analysis_outputs" ^
  --pointnext-root "%cd%\tools\pointnext_lod\PointNeXt" ^
  --cfg "%cd%\tools\pointnext_lod\PointNeXt\cfgs\industrial_part\pointnext-s-14class-randomrot-8192-fast.yaml" ^
  --ckpt "%cd%\tools\pointnext_lod\PointNeXt\log\industrial_part\industrial_part-train-pointnext-s-14class\checkpoint\industrial_part-train-pointnext-s-14class_ckpt_best.pth" ^
  --classes-file "%cd%\tools\pointnext_lod\industrial_part_14classes.txt" ^
  --models model_a.glb model_b.glb model_c.glb ^
  --num-points 8192 ^
  --batch-size 16
```

## 5. 已经推断过，只想重新生成 LOD 约束

如果已经有：

```text
lod_analysis_outputs\my_model_pointnext_analysis.csv
```

只是修改了 P1-P5 策略，不需要重新跑 PointNeXt。

单个模型运行：

```bat
python tools\pointnext_lod\build_lod_constraints_from_analysis.py ^
  --input-csv "%cd%\lod_analysis_outputs\my_model_pointnext_analysis.csv" ^
  --output-csv "%cd%\lod_analysis_outputs\my_model_lod_constraints.csv" ^
  --summary-json "%cd%\lod_analysis_outputs\my_model_lod_constraints_summary.json"
```

已有五个默认模型可以直接运行：

```bat
tools\pointnext_lod\run_rebuild_lod_constraints_only.bat
```

## 6. 生成 Vulkan LOD 缓存

有了：

```text
lod_analysis_outputs\my_model_lod_constraints.csv
```

后，需要生成 `.zippp` 缓存：

```bat
cd /d E:\vk_lod_clusters1\t1

_bin\Release\t1.exe ^
  --scene _downloaded_resources\my_model.glb ^
  --processingonly 1 ^
  --processingpartial 1 ^
  --processingthreadpct 0.75
```

成功后会看到类似：

```text
Semantic LOD: loaded xxxx rows, xxxx mesh policies from ...\my_model_lod_constraints.csv
Scene::endProcessOnlySave completed successfully
saved: _downloaded_resources\my_model.glb.zippp
```

注意：本项目的 bool 参数要写 `1/0`，例如：

```bat
--processingonly 1
```

不要只写：

```bat
--processingonly
```

## 7. 打开模型查看结果

普通打开：

```bat
_bin\Release\t1.exe --scene _downloaded_resources\my_model.glb
```

查看语义 LOD 分类颜色：

```bat
_bin\Release\t1.exe --scene _downloaded_resources\my_model.glb --visualize 8
```

颜色含义：

```text
红色 P1：最激进，微小件/低置信度小件/远处可剔除
橙色 P2：激进，重复紧固件/普通件/大块静态件
黄色 P3：标准，普通可见件
青色 P4：保守，控制件/接口件/结构连接件
蓝色 P5：最保守，运动件/精密件/关键结构连接件
灰色：没有读取到 semantic policy
```

查看普通 LOD 层级：

```bat
_bin\Release\t1.exe --scene _downloaded_resources\my_model.glb --visualize 5
```

调节全局 LOD 清晰度：

```bat
_bin\Release\t1.exe --scene _downloaded_resources\my_model.glb --loderror 32
```

推荐先试：

```text
24：质量较高
32：推荐起点
48：偏性能
64：大场景快速预览
```

## 8. 新策略下 P1-P5 的设计目标

现在的策略不是“越大越保守”，而是“功能关键才保守”。

快速简化：

```text
房屋、地基、墙体、外壳、盖板、大平板、大块体
螺栓、螺母、垫片、销、铆钉等重复紧固件
低置信度小件、微小碎片
```

保留细节：

```text
轴承、齿轮、链轮、电机、轮、弹簧、旋转机械
铰链、夹具、支架、结构连接器
接口件、控制件、形状关键件
```

关键规则位置：

```text
tools\pointnext_lod\build_lod_constraints_from_analysis.py
```

主要函数：

```text
semantic_group()
geometry_fallback()
lod_policy()
```

## 9. 常见问题

### 9.1 控制台显示 no constraints CSV found

说明没有找到：

```text
lod_analysis_outputs\my_model_lod_constraints.csv
```

检查文件名是否和模型 stem 一致。

例如模型：

```text
my_model.glb
```

必须对应：

```text
my_model_lod_constraints.csv
```

### 9.2 改了 CSV，但画面没变化

需要重新生成 `.zippp` 缓存：

```bat
_bin\Release\t1.exe --scene _downloaded_resources\my_model.glb --processingonly 1 --processingpartial 1 --processingthreadpct 0.75
```

### 9.3 PointNeXt CUDA 报错

先检查：

```bat
python -c "import torch; print(torch.__version__, torch.cuda.is_available())"
```

再检查：

```bat
python -c "import sys; sys.path.insert(0, r'E:\vk_lod_clusters1\t1\tools\pointnext_lod\PointNeXt'); from openpoints.cpp.pointnet2_batch import pointnet2_cuda; print('pointnet2 ok')"
```

如果失败，说明当前 Python 环境不是之前编译 PointNeXt CUDA 扩展的环境。

### 9.4 模型不是 GLB

本项目渲染入口支持 `.glb` / `.gltf`。如果是 `.obj` / `.stl` / `.ply`，建议先转成 `.glb` 或 `.gltf`，再走本文档流程。

### 9.5 大模型分析很慢

可以先降低 batch size：

```bat
--batch-size 8
```

如果显存充足再用：

```bat
--batch-size 16
```

## 10. 最短命令模板

新模型 `my_model.glb` 的最短流程：

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

_bin\Release\t1.exe --scene _downloaded_resources\my_model.glb --processingonly 1 --processingpartial 1 --processingthreadpct 0.75

_bin\Release\t1.exe --scene _downloaded_resources\my_model.glb --visualize 8
```
