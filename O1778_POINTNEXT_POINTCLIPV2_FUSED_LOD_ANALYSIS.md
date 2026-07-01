# o1778.glb 融合语义 LOD 结果分析

本文对比：

```text
旧结果：lod_analysis_outputs\o1778_lod_constraints_old.csv
新结果：lod_analysis_outputs\o1778_lod_constraints_fused.csv
```

新结果由以下三类证据融合得到：

```text
PointNeXt 闭集分类
PointCLIP V2 开放词表 Top-k
结构特征库：尺寸、细长、薄壁、圆盘/环状、面数密度、大体块倾向等
```

## 1. 总体结论

相较旧版，`o1778_lod_constraints_fused.csv` 有明显改进。

核心改进是：

1. 低置信度或模糊零件不再大量落入 P1。
2. 螺栓、螺母、垫片、销钉等重复标准件更集中进入 P2。
3. 大型静态板件和体块更多进入 P3，符合快速简化目标。
4. 结构连接件从旧版 P7 被纠正到 P8，语义更合理。
5. 高细节但非关键运动件更多进入 P6，避免被过度简化。
6. P10 关键保护类基本保持稳定，没有明显丢失。

需要注意的是：

```text
PointCLIP V2 的 top1 分数整体不高，应该作为弱语义证据使用。
少量低置信小件被提升到 P8/P6，建议在前端 semantic lod policy 颜色模式下抽查。
```

## 2. 数据完整性

两个文件行数一致：

```text
old   : 1778 rows
fused : 1778 rows
```

PointCLIP V2 匹配情况：

```text
Matched PointCLIP rows: 1778 / 1778
```

说明 `order + node_index + mesh_index` 对齐完整，融合不是部分匹配。

## 3. P1-P10 分布对比

| LOD 类别 | old 数量 | fused 数量 | 变化 | 解释 |
|---|---:|---:|---:|---|
| P1_micro_uncertain | 523 | 57 | -466 | 大量低置信小件被重新分配，减少“全丢进不确定类” |
| P2_repeated_fastener | 333 | 551 | +218 | 紧固件识别更集中，适合快速简化 |
| P3_large_static_bulk | 57 | 167 | +110 | 大型板件/静态体块被更积极地简化 |
| P4_ordinary_low_detail | 304 | 228 | -76 | 一部分普通件被更细分到 P3/P6/P8 |
| P5_balanced_visible | 54 | 35 | -19 | 部分普通可见件被重分配到更明确类别 |
| P6_high_detail_shape | 38 | 247 | +209 | 高细节形状得到中高等级保护 |
| P7_interface_fluid | 200 | 33 | -167 | 旧版把大量结构连接件放入接口类，新版纠正 |
| P8_structural_control | 43 | 233 | +190 | 结构连接/控制件保护显著增强 |
| P9_motion_precision | 30 | 30 | 0 | 一般运动精密件稳定 |
| P10_critical_preserve | 196 | 197 | +1 | 关键保护类基本稳定 |

## 4. 策略强度变化

按策略强度分桶：

| 策略桶 | old | fused | 变化 |
|---|---:|---:|---:|
| P1-P3 快速简化 | 913 | 775 | -138 |
| P4-P6 中等保护 | 396 | 510 | +114 |
| P7-P10 重点保护 | 469 | 493 | +24 |

解释：

- 新版不是简单地把所有东西升高保护等级。
- 快速简化总数下降，主要来自 P1 大幅减少。
- 但 P2 和 P3 都增加，说明紧固件和大型静态件仍然保持快速简化。
- 中等保护 P6 明显增加，说明高细节形状不再被粗暴降到 P1/P4。
- 重点保护略增，主要来自结构控制件 P8。

## 5. 主要变化来源

共有：

```text
发生变化：896
保持不变：882
保护等级升高：762
简化等级增强：134
```

主要迁移如下：

| 迁移 | 数量 | 说明 |
|---|---:|---|
| P1 -> P2 | 217 | 低置信紧固件被恢复为重复标准件 |
| P7 -> P8 | 167 | 结构连接件从接口类纠正为结构控制类 |
| P1 -> P6 | 115 | 高细节但低置信零件获得形状保护 |
| P4 -> P6 | 95 | 普通低细节判断被修正为高细节形状 |
| P1 -> P4 | 89 | 低置信小件恢复为普通低细节件 |
| P4 -> P3 | 66 | 大型静态板件/体块转入快速简化 |
| P5 -> P3 | 51 | 普通可见件中一部分被识别为大型静态件 |

这些迁移整体符合预期：

```text
减少 P1 滥用
增强重复标准件归类
把大型静态件快速简化
把结构连接件和高细节件保护起来
```

## 6. PointCLIP V2 贡献分析

PointCLIP V2 的 top1 角色分布：

| PointCLIP top1 role | 数量 |
|---|---:|
| structural_control | 781 |
| repeated_fastener | 722 |
| large_static_bulk | 201 |
| ordinary_low_detail | 73 |
| interface_fluid | 1 |

PointCLIP top1 具体类别：

| top1 id | 数量 |
|---|---:|
| joint_clamp_hinge | 687 |
| pin_rivet_key | 375 |
| nut | 214 |
| washer_spacer_ring | 133 |
| large_foundation_frame | 110 |
| handle_lever_knob | 94 |
| plate_disc_panel | 91 |
| ordinary_irregular_part | 73 |
| pipe_fitting_valve_nozzle | 1 |

PointCLIP 与最终 inferred role 完全一致：

```text
576 / 1778
```

这说明当前融合不是直接照搬 PointCLIP，而是把它作为弱证据，与 PointNeXt 和结构特征共同判断。

PointCLIP top1 平均分数：

| role | 数量 | 平均 top1 score |
|---|---:|---:|
| repeated_fastener | 722 | 0.209 |
| structural_control | 781 | 0.163 |
| ordinary_low_detail | 73 | 0.159 |
| large_static_bulk | 201 | 0.154 |
| interface_fluid | 1 | 0.140 |

结论：

```text
PointCLIP V2 对 o1778 的开放词表结果可用，但置信度不高。
它适合参与融合，不适合作为唯一分类依据。
当前融合方式是合理的。
```

## 7. 关键类别变化

### 7.1 紧固件更合理

旧版 P1 中大量紧固件被新版恢复为 P2：

| 类别 | 迁移 | 数量 |
|---|---|---:|
| nuts | P1 -> P2 | 76 |
| screws_bolts_studs | P1 -> P2 | 54 |
| washers_rings_spacers | P1 -> P2 | 41 |
| pins_rivets_keys | P1 -> P2 | 40 |

这对 LOD 是改进：

- P1 更适合极小、低置信、可远处剔除的零件。
- P2 更适合大量重复标准件，保留基本位置和轮廓但快速简化。

### 7.2 大型静态件更积极简化

`plates_discs_shapes` 的主要迁移：

| 迁移 | 数量 |
|---|---:|
| P4 -> P3 | 66 |
| P5 -> P3 | 51 |

其中大多数几何 fallback 是：

```text
geom_thin_plate
geom_simple_irregular
geom_ultra_thin_sheet
geom_ring_or_disk
```

这符合你的 LOD 目标：类似地基、板件、壳体、静态支撑应更快简化。

### 7.3 结构连接件从 P7 修正到 P8

旧版中大量 `joints_clamps_structural_connectors` 被归到 P7：

```text
joints_clamps_structural_connectors: P7 -> P8 = 167
```

新版将其归到：

```text
P8_structural_control
```

这是语义上的改进。P7 更适合管路、阀、喷嘴、接口件；P8 更适合夹具、关节、连接件、控制结构。

### 7.4 P10 基本稳定

新版 P10 分布：

| predicted_class | 数量 |
|---|---:|
| bearings_bushings_guides | 150 |
| gears_pulleys_chains | 25 |
| motors_gearmotors | 19 |
| springs | 3 |

旧版 P10 为 196，新版为 197，基本稳定。

这说明融合没有破坏关键运动/传动件保护。

## 8. 潜在风险

### 8.1 少量 P1 被提升到 P8

有 14 个 mesh 从：

```text
P1_micro_uncertain -> P8_structural_control
```

这些多数来自：

```text
PointNeXt 低置信
PointCLIP 判断为 joint_clamp_hinge
结构匹配普通或 balanced visible
```

这可能是合理的结构连接件，也可能是 PointCLIP 对小零件的开放词表误判。

建议在前端颜色模式下抽查 P8 中的小尺寸零件。

### 8.2 P6 增加较多

P6 从 38 增加到 247，主要来源：

```text
P1 -> P6
P4 -> P6
```

这表示新版更重视高细节形状。对视觉质量是有利的，但可能增加部分中近距离 LOD 成本。

建议观察：

```text
P6 是否集中在真正高面数或复杂形状区域
是否有大量无意义小件进入 P6
```

### 8.3 PointCLIP 分数偏低

PointCLIP top1 平均分数大多在 0.15-0.21 左右。

这不是异常，因为开放词表 prompt 较多，概率会被分散。但也说明：

```text
不要单独依赖 PointCLIP V2。
继续保持 PointNeXt + 结构特征主导是正确的。
```

## 9. 是否相较 old 有改进

结论：有改进，而且改进方向符合当前 LOD 目标。

主要依据：

1. `PointCLIP` 全量匹配，融合数据完整。
2. `P1_micro_uncertain` 从 523 降到 57，显著减少不确定类滥用。
3. `P2_repeated_fastener` 从 333 增到 551，重复标准件归类更集中。
4. `P3_large_static_bulk` 从 57 增到 167，大型静态件更符合快速简化策略。
5. `P8_structural_control` 从 43 增到 233，结构连接件保护增强。
6. `P10_critical_preserve` 保持稳定，关键件没有被误降级。
7. 旧版 P7 中 167 个结构连接件被修正到 P8，语义边界更清晰。

整体评价：

```text
fused 版本比 old 版本更适合作为 o1778.glb 的语义结构 LOD 输入。
```

## 10. 建议下一步检查

在 Vulkan 项目中打开：

```text
Settings -> Rendering -> Visualize -> semantic lod policy
Statistics -> Semantic LOD Policy Distribution
```

重点抽查：

1. P8 是否确实落在连接件、夹具、关节、控制结构上。
2. P6 是否集中在高面数复杂形状，而不是无意义小件。
3. P3 是否覆盖大型静态板件、底座、壳体。
4. P2 是否覆盖螺栓、螺母、垫片、销钉等重复标准件。
5. P10 是否覆盖轴承、齿轮、电机、弹簧等关键件。

如果 P8/P6 过多，可以后续调低 PointCLIP 对低置信小件的提升权重，或增加尺寸门槛。

