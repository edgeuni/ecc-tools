# P0-A 验证报告：vga_lcd 修复前后 A/B 对比

> 任务: 06-12-fix-wire-res-unit · 2026-06-12
> 方法: 同一 HEAD 代码，仅 3 处 `/1000` 差异，分别构建 `ecc_bin_base`（含 bug）与 `ecc_bin_fixed`（修复），在 /tmp 克隆 workspace 各跑一次 vga_lcd CTS（不触碰 DAC-27-CTS）。
> Sanity: base run 与 DAC-27-CTS 2026-06-06 原始日志逐项一致（initial_skew 0.0533 / buffer 5744 / area 16190.72 / total WL 100291.911 / frontier 185920），证明 HEAD 与实验快照 CTS 行为等价、本地复现链路可信。

## A/B 结果

| 指标 | base（bug） | fixed | 说明 |
|---|---|---|---|
| unit_resistance 日志 | 914.000 **uOhm**/um | 914.000 **mOhm**/um | ✓ 0.914 Ω/um，修复生效 |
| FastSTA initial_skew | 0.0533 ns | 0.0576 ns | 模型开始感知线长差异（+8%） |
| 优化器 | no_op | no_op | 0.0576 < 固定 target 0.08，行为合理（见发现 2） |
| selected depth | 12 | 12 | 深度搜索行为不变（monotone 剪枝依旧，P1-C 范畴） |
| depth-12 frontier/feasible | 185920 / 138543 | 221287 / 159591 | 真实 R 改变表征值→可行域形状变化 |
| depth 表 best delay | 0.5109 ns | 0.5467 ns | 真实 R 下延迟评估 +7% |
| final buffer count / area | 5744 / 16190.7 | 5745 / 16123.0 | 选型微调（-0.4% 面积） |
| max_clock_net_wirelength | 472.05 um | 472.05 um | 同一 trunk 长段仍被选中（见发现 1） |
| total clock WL | 100291.9 um | 100291.9 um | 拓扑几乎不变 |
| 运行时间 | 56.0 s | 67.5 s | frontier 变大→搜索变慢 +20% |

单测：新增 `icts_test_database_io`（WrapperRcTest 4 用例，含 >0.1 Ω/um 回归哨兵）；全量 iCTS ctest 16/16 通过，无既有用例绑定旧错误数值。

## 结论

1. **修复正确且全链路生效**（探针、ClockRouteSegmentRc、FastSTA 同源一致）。
2. **vga_lcd 上拓扑/QoR 决策几乎不变** —— R1 是模型保真的前提（P1-D 的硬依赖），但它**不是** vga_lcd 拓扑差距的决定性因素；差距主导者仍是结构性问题（R3 深度锁死 / R4 per-level 均一 / 级数）。父任务报告 §0 对 R1 "🔴 P0" 的定性保留（前提地位不变），但"修复即改善 QoR"的隐含预期不成立。
3. eval（Innovus route+STA）层面预计变化微小（DEF 差异仅 +1 buffer / sizing 微调），不必为本任务单独跑商业 eval；待 P0-B/P1 改动累积后统一重评。

## 新发现（转后续任务）

1. **max_length=300um 约束未在 source trunk / htree 段实施**：472um 无 buffer 段在两个版本中都合法存在（0.5ns slew 上限下电气可行）。约束实施缺失是独立缺陷 → 建议并入 P1-E（latency/结构）或单开小任务。
2. **优化器 no_op 的真正解法是 skew target 自适应**（固定 0.08 太松，Innovus 同设计自动推导 0.061）→ 已在 P1-D R3 范围内。
3. **内部 skew 0.058 vs eval 0.118 的剩余缺口**（~60ps）来自模型几何/提取差异：FLUTE 平面 vs NanoRoute 实际绕线、**C 无耦合项**、via 电阻。R 修复后该缺口主要归属 C 侧与几何侧 → P2-F（RC 标定）与 P1-D（真实嵌入几何）的权重应上调 C 耦合估计部分。

## 测试与产物

- 单测: `src/operation/iCTS/test/database/io/WrapperRcTest.cc`（目标 `icts_test_database_io`）
- 验证脚本/日志: `/tmp/p0a_vga_validation/{tcl_fixed.tcl,tcl_base.tcl,run_fixed.log,run_base.log}`（临时目录，关键数字已录入本文）
