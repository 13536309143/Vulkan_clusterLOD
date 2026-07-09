# LOD 全量数据分析报告

分析时间：2026-07-09

分析范围：本报告只统计具备完整六组 ablation 数据的 5 个模型：

```text
a
b
c
o1778
o3049
```

五个模型合计 153,616 个 mesh。

## 1. 数据覆盖情况

| 模型 | mesh 数 | candidate-only PointCLIP | candidate 占比 | full PointCLIP | fused |
|---|---:|---:|---:|---:|---:|
| a | 24,259 | 12,252 | 50.50% | 24,259 | 24,259 |
| b | 63,063 | 29,115 | 46.17% | 63,063 | 63,063 |
| c | 61,467 | 35,437 | 57.65% | 61,467 | 61,467 |
| o1778 | 1,778 | 920 | 51.74% | 1,778 | 1,778 |
| o3049 | 3,049 | 1,762 | 57.79% | 3,049 | 3,049 |

这五个模型都具备：

```text
PointNeXt 全量分析
candidate-only PointCLIP
full PointCLIP
原 fused LOD
六组 ablation LOD
```

因此它们可以进行严格横向对比。

## 2. 六组策略总体结果

| 策略 | PointCLIP 覆盖 | allow_cull | P1-P3 激进简化 | P8-P10 高保护 |
|---|---:|---:|---:|---:|
| 1 PointNeXt | 0.00% | 63.41% | 73.78% | 23.46% |
| 2 PointCLIP | 100.00% | 87.78% | 87.82% | 0.00% |
| 3 PointNeXt + structure | 0.00% | 44.17% | 45.50% | 21.53% |
| 4 PointCLIP + structure | 100.00% | 4.22% | 8.40% | 0.19% |
| 5 PointNeXt + PointCLIP | 51.74% | 58.83% | 68.54% | 22.01% |
| 6 PointNeXt + PointCLIP + structure | 51.74% | 42.59% | 46.27% | 22.08% |

核心结论：

```text
1. 单独 PointCLIP 不适合作为最终 LOD 策略。
   它把 87.82% 的 mesh 放入 P1-P3，allow_cull 达到 87.78%，几乎不给 P8-P10 高保护。

2. structure 是当前提升最大的因素。
   PointNeXt -> PointNeXt + structure 后，P1-P3 从 73.78% 降到 45.50%，allow_cull 从 63.41% 降到 44.17%。

3. PointCLIP 的合理定位是补充低置信样本。
   PointNeXt -> PointNeXt + PointCLIP 只改变 8.41% 的 mesh，说明它没有推翻 PointNeXt 主结果。

4. 默认策略 6 最平衡。
   它保持 22.08% 的 P8-P10 高保护，同时把 P1-P3 控制在 46.27%，allow_cull 控制在 42.59%。
```

## 3. P1-P10 分布

| 策略 | P1 | P2 | P3 | P4 | P5 | P6 | P7 | P8 | P9 | P10 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 PointNeXt | 24.18% | 39.24% | 10.37% | 0.00% | 0.00% | 0.00% | 2.75% | 11.63% | 0.06% | 11.77% |
| 2 PointCLIP | 85.32% | 2.46% | 0.04% | 8.46% | 0.00% | 0.03% | 3.69% | 0.00% | 0.00% | 0.00% |
| 3 PointNeXt + structure | 5.70% | 38.47% | 1.33% | 16.85% | 1.42% | 11.89% | 2.81% | 9.64% | 0.06% | 11.82% |
| 4 PointCLIP + structure | 3.35% | 0.87% | 4.18% | 49.15% | 5.73% | 33.62% | 2.91% | 0.10% | 0.00% | 0.08% |
| 5 PointNeXt + PointCLIP | 18.41% | 40.43% | 9.70% | 5.94% | 0.00% | 0.06% | 3.46% | 10.46% | 0.06% | 11.49% |
| 6 PointNeXt + PointCLIP + structure | 3.70% | 38.89% | 3.67% | 16.04% | 1.23% | 11.55% | 2.84% | 10.09% | 0.06% | 11.92% |

观察：

```text
1. 策略 6 仍保留大量 P2 repeated_fastener，说明重复小件可以提供简化收益。
2. 策略 6 明显减少 P1，避免把大量不确定件直接归为最低保护。
3. 策略 6 增加 P4/P6，使普通可见件和高细节形状得到中等保护。
4. 策略 6 的 P8/P10 与 PointNeXt 高保护比例接近，没有出现过度保护膨胀。
```

## 4. 策略变化幅度

| 对比 | LOD 改变比例 | 变得更保护 | 变得更激进 | 平均 P 级别变化 |
|---|---:|---:|---:|---:|
| PointNeXt -> PointNeXt + structure | 31.78% | 44,814 | 4,003 | +0.837 |
| PointNeXt -> PointNeXt + PointCLIP | 8.41% | 10,626 | 2,287 | +0.114 |
| PointNeXt + PointCLIP -> 完整策略 | 30.82% | 42,591 | 4,759 | +0.767 |
| PointNeXt -> 完整策略 | 30.35% | 43,628 | 2,995 | +0.882 |
| PointCLIP -> PointCLIP + structure | 86.94% | 130,796 | 2,763 | +3.164 |
| PointCLIP -> 完整策略 | 93.47% | 133,695 | 9,888 | +3.023 |

结论：

```text
1. structure 对策略影响最大，且主要方向是提高保护等级。
2. PointCLIP 对 PointNeXt 的影响较温和，符合“只修正低置信样本”的预期。
3. 完整策略相对 PointNeXt 的平均 P 级别提升为 +0.882，整体更保守、更安全。
4. 单独 PointCLIP 与完整策略差异极大，full PointCLIP 不应直接替代 PointNeXt。
```

## 5. PointNeXt 置信度与 candidate 筛选

| 模型 | candidate 占比 | 平均置信度 | candidate 平均置信度 | 非 candidate 平均置信度 | candidate 平均 margin | 非 candidate 平均 margin |
|---|---:|---:|---:|---:|---:|---:|
| a | 50.50% | 0.622 | 0.460 | 0.787 | 0.329 | 0.758 |
| b | 46.17% | 0.629 | 0.439 | 0.792 | 0.301 | 0.766 |
| c | 57.65% | 0.569 | 0.404 | 0.793 | 0.254 | 0.765 |
| o1778 | 51.74% | 0.596 | 0.411 | 0.794 | 0.271 | 0.766 |
| o3049 | 57.79% | 0.575 | 0.424 | 0.782 | 0.277 | 0.751 |

结论：

```text
candidate 筛选是有效的。

进入 PointCLIP 的样本平均置信度约 0.40-0.46，非 candidate 样本约 0.78-0.79。
进入 PointCLIP 的样本平均 margin 约 0.25-0.33，非 candidate 样本约 0.75-0.77。

这说明 PointCLIP 的输入确实集中在 PointNeXt 不确定区域，而不是随机覆盖全量 mesh。
```

## 6. 默认完整策略下的角色分布

| 模型 | repeated_fastener | ordinary_low_detail | high_detail_shape | structural_control | critical_preserve | allow_cull |
|---|---:|---:|---:|---:|---:|---:|
| a | 7,615 | 2,514 | 2,580 | 7,405 | 1,314 | 33.74% |
| b | 27,775 | 10,554 | 5,312 | 3,584 | 8,464 | 46.81% |
| c | 22,551 | 10,883 | 8,868 | 4,140 | 8,117 | 41.99% |
| o1778 | 534 | 319 | 263 | 201 | 196 | 33.52% |
| o3049 | 1,273 | 367 | 717 | 172 | 227 | 43.49% |

观察：

```text
1. a 的 structural_control 很高，说明它包含大量连接/控制/结构类零件，默认策略保护较强。
2. b 和 c 的 repeated_fastener 数量很大，适合产生较多 P2 简化收益。
3. o3049 的 high_detail_shape 比例偏高，说明它有较多需要形状保护的细节件。
```

## 7. 对 PointCLIP 的判断

PointCLIP full 的角色分布中，`micro_uncertain`、`ordinary_low_detail`、`repeated_fastener` 占比较高，导致单独 PointCLIP 策略明显偏激进：

```text
策略 2：P1 占 85.32%，P1-P3 合计 87.82%，P8-P10 接近 0。
```

这说明当前 PointCLIP 词库和 prompt 对工业零件的开放识别仍不够稳定。它适合做：

```text
低置信补充
开放词库候选提示
辅助 role 判断
```

但不适合做：

```text
全量主分类器
直接决定 P1-P10
覆盖 PointNeXt 高置信结果
```

## 8. 总体结论

当前五个完整模型的数据支持继续使用：

```text
PointNeXt + candidate-only PointCLIP + structure
```

作为默认策略。

理由：

```text
1. PointNeXt 提供稳定主分类，但单独使用时 P1-P3 偏高。
2. candidate-only PointCLIP 的筛选边界合理，确实集中在低置信和低 margin 样本。
3. PointCLIP 单独使用风险较大，full 结果不应进入默认融合流程。
4. structure 显著改善过度简化问题，是 P1-P10 策略合理性的关键来源。
5. 完整策略在保护比例、简化比例和可剔除比例之间最平衡。
```

## 9. 建议后续实验

```text
1. 对策略 6 做可视化抽样检查，重点看 P1/P2 是否误伤关键件。
2. 对策略 4 做人工检查，确认 PointCLIP + structure 是否虽然不激进，但是否过度集中在 P4/P6。
3. 优化 PointCLIP prompt，目标是减少 full 推断中的 micro_uncertain 泛化。
4. 在论文或报告中，重点展示策略 1、5、6 的对比，因为它们最能说明“低置信补充 + 结构校正”的贡献。
```

## 10. 生成的辅助文件

```text
analysis_reports\lod_all_data_analysis.json
analysis_reports\lod_ablation_aggregate_summary.csv
analysis_reports\lod_pairwise_change_summary.csv
analysis_reports\lod_pairwise_change_totals.csv
analysis_reports\lod_pointnext_confidence_candidate_summary.csv
analysis_reports\lod_pointnext_confidence_candidate_summary.json
```
