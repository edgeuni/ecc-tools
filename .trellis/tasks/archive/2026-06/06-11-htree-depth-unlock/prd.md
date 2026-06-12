# P1-C 拓扑自由度解锁：深度搜索剪枝修复

> 父任务: 06-11-align-commercial-cts · 优先级: P1 · 基准: iwls2005__vga_lcd
> 根因依据: 父任务 `research/vga-lcd-topology-gap-root-cause.md` §5 R3

## Goal

让 H-tree 深度搜索真正具备探索能力：修复 sink-load region 单调剪枝的过度否决，使浅于几何满深度的候选能够被实际电气评估并参与全局选型，为缩短时钟路径级数（9-10 级 → ≤8 级）打开拓扑空间。

## 背景（现状缺陷）

vga_lcd 上深度搜索窗口 {12,11,10,9}，但 run 报告（`work/icts-native/cts.log` "HTree Depth Candidate Summary"）显示 11/10/9 全部 `monotone_pruned_by_bottom_most_buffered_level threshold=10`，feasible=0——只有 depth=12 被真正评估。机制：

1. `SinkLoadRegion.cc:298-323`：任一边界组 loads 超 `max_fanout`（实验=4）或 pin-cap 下界违例 → `monotone_hard_fail`（first-fail 即整体否决该 pattern）；
2. `SinkLoadRegion.cc:383-393` + `DepthPlan.cc:112`：`max_monotone_failed_level` 跨深度共享，bottom-buffered-level ≤ 阈值的所有 pattern 直接拒绝，不做电气评估；
3. `TopologyGen.cc:360-370`：leaf_count=不超过 load 数的最大 2^n，深度上限本身刚性。

## Requirements

- R1: 重新审视单调剪枝的正确性边界——"bottom-buffered-level 更浅 ⇒ 必然不可行"这一单调性假设在何种条件下成立/不成立，给出结论性分析（写入本任务 design.md）。
- R2: 修复过度否决：候选不应因单个边界组 first-fail 被整体丢弃；考虑 per-group 可行域、或对超 fanout 组允许局部加 buffer/拆分的兜底评估路径。
- R3: 浅深度候选的边界层 load 分组应反映"该深度下的真实 leaf 区域聚合"，而非直接复用满深度树节点的 load 集合判定。
- R4: 不得回退既有结果：depth=12 路径在修复后仍可被选中（若它确实最优）。
- R5: 与 max_fanout 约束的语义对齐：约束应作用于"最终物理 fanout"，而非"候选评估期的几何分组"，必要时与父任务评审确认约束口径。

## 依赖（必须先行）

- **P0-A（wire R 1000x 单位 bug 修复，任务待创建）**：未修复前，深度候选间的延迟/可行性比较失真，本任务的收益评估无意义。本任务 start 前 P0-A 必须已合入并重建 vga_lcd 基线。
- 建议 P0-B（表征覆盖修复）先行或同步：浅深度 ⇒ 段更长 ⇒ 更依赖长线长表征精度。

## Acceptance Criteria

- [x] vga_lcd 深度搜索 ≥2 个深度产生非零 feasible entries（run 报告 Depth Candidate Summary 可见）。
  - 实测（2026-06-12）：4/4 深度全部 feasible（基线 1/4），新增 Split Groups/Split Buffers 列与 Monotone Pruning Origin 报告。
- [x] 全局选型在多深度间比较后给出选择，选择理由（delay/power Pareto）在日志可追溯。
  - 实测：选中 depth 9（best delay 0.3751 vs depth-12 的 0.5194）。
- [x] 若选中更浅深度：eval skew 不劣化。
  - 实测（内部口径，eval 按用户决定统一推迟）：initial_skew 0.0562 → **0.0294（-48%）**，内部 max arrival -53ps；buffer +4.9%、WL +1.5% 为代价。
  - 勘误：「每路径 buffer 级数 ≤8」未达成——级数为 10/10（split 层抵消 htree 变浅，但级数完全均一化是 skew 减半的主因）。≤8 级目标移交 P1-E（trunk/root/cluster 冗余级合并）。
- [x] depth=12 仍可被选（feasible 在候选集中，本次按 Pareto 落选，对比表可查）。
- [x] 既有单测全绿（16/16）+ 新增深度搜索剪枝行为单测（SinkLoadRegionSplitTest 6/6：拆分合法/确定性/超界拒绝/保守性）。

> 详细数据：`research/validation-vga-lcd.md`

## 约束

- 验证基准仅 vga_lcd；`~/project/DAC-27-CTS` 只读，新实验跑新 run 目录。
- 复杂任务：start 前需补 `design.md` + `implement.md`。
