# P1-C 验证报告：vga_lcd 深度搜索解锁

> 任务: 06-11-htree-depth-unlock · 2026-06-12
> 基线: P0-B 后默认配置 run（/tmp/p0a_vga_validation/out_final，depth 12，initial_skew 0.0562）
> 二进制: P1-C 实现（split 补救 + 观测性）

## Gate-1：违例实证（观测性增量先行）

新增的 Monotone Pruning Origin 报告/日志在改动前的行为下捕获：

```
monotone threshold raised to bottom-most buffered level 10 by hard violation:
htree_load_group_node_912 anchor=(48392,535627) fanout_violation load_count=5, max_fanout=4
```

与设计预测完全一致：**单个 5-load 稠密组**（level-11 边界）把阈值钉在 10，连带灭掉 depth 11/10/9 的全部 pattern（其 bottom 上限 ≤10）。

## Gate-2：split 补救后的深度搜索（vga_lcd）

Depth Candidate Summary（新表含 Split 列）：

| Depth | Leaves | Status | Feasible | Split Groups | Split Buffers | Best Delay | Best Power |
|---|---|---|---|---|---|---|---|
| 12 | 4096 | feasible | 186397 | 81 | 162 | 0.5194 ns | 9.309 mW |
| 11 | 2048 | feasible | 153744 | 512 | 1114 | 0.4478 ns | 4.711 mW |
| 10 | 1024 | feasible | 142422 | 512 | 1114 | 0.3929 ns | 2.481 mW |
| **9** | 512 | **selected** | 83710 | 512 | 1114 | **0.3751 ns** | **1.926 mW** |

——基线为 1/4 可行（12 独苗），现 **4/4 可行**，选型理由（delay/power Pareto + split 代价列）完整可追溯。

## 嵌入后真实质量（FastSTA，同一内部口径）

| 指标 | P0-B 基线（depth 12） | P1-C（depth 9 + split） | Δ |
|---|---|---|---|
| **initial_skew** | 0.0562 ns | **0.0294 ns** | **-48%** |
| 内部 latency 窗口 | [0.540, 0.620] | [0.486, 0.566] | **max arrival -53ps（-8.6%）** |
| 路径 buffer 级数 | 9~10（不齐） | **10 / 10（完全均一）** | 级差消除 |
| buffer 总数 | 5744 | 6025 | +281（+4.9%，其中 split 1114、edge 682+1） |
| buffer 面积 | 16176.7 um² | 17016.7 um² | +5.2% |
| total clock WL | 100291.9 um | 101792.4 um | +1.5% |
| max net WL | 472.1 um | 434.5 um | -8% |
| 运行时间 | 37.1 s | 38.6 s | +1.5s |
| pruned 单负载叶 buffer | 3976 | 0 | 结构变化（叶层由 split/cluster 接管） |

**net fanout 不变式**（直接解析 DEF NETS）：P1-C 全部 6026 条 clock net fanout ≤4（hist {1:3, 2:513, 3:143, 4:5367}），split 嵌入正确。

**风险 B（选型对 split 代价盲视）实测结论**：未反噬——浅深度的真实质量更好（skew/latency 双降）。机理：split 层使全部路径级数均一（10/10），消除了基线 9~10 级差这一主要 skew 来源；代价是 buffer +4.9%。选型 surcharge 优化记为跟进项，当前数据不构成必要性。

## 验收对照（PRD）

- ✅ ≥2 深度 feasible（4/4），全局选型跨深度比较、理由可追溯（表 + Monotone Origin 行）
- ✅ depth-12 仍在候选集且可选（feasible）
- ⚠️ 勘误：「选浅深度则级数 ≤8」**未达成**——级数为 10/10（split 层抵消了 htree 变浅），但内部 skew -48%、latency -53ps 远超原条款意图（条款本意是防止"白变浅"）。≤8 级目标移交 P1-E（层级合并：trunk/root/cluster 冗余级才是级数主要来源）。
- ✅ 单测：SinkLoadRegionSplitTest 6/6（含确定性、>fanout² 拒绝、bisection 保守性文档化用例）；全量 iCTS ctest 16/16。
- ✅ 内部 skew 不劣化（大幅改善）；eval 级验证按用户决定推迟到全部修复完成后统一重跑。

## 已知限制/跟进项

1. 选型 delay/power 不含 split buffer 代价（本次数据显示无害，保留跟进）。
2. N > max_fanout² 仍 hard-fail（单级 split 上限），中位二分可能多于 ⌈N/fanout⌉ 个子组（保守，单测文档化）。
3. cap-LB 违例未走 split 补救（实务上 fF 级 pin cap 不会触发；记录于设计）。
