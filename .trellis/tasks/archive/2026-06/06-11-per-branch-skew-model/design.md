# Design: P1-D per-branch 时序建模与 skew 修复（新基线校准版）

> 任务: 06-11-per-branch-skew-model · 2026-06-12
> 前置已落地: P0-A（R 修复）、P0-B（表征覆盖）、P1-C（深度解锁）。**基线已变**：internal skew 0.0294 ns（P1-C），远低于固定 target 0.08。

## 1. 对原 PRD 各项的基线再校准

| 原需求 | 新基线下的判定 |
|---|---|
| R1 per-branch FastSTA | **已成立并被 P1-C 验证**：FastSTA 注入真实嵌入 route tree（`OptimizationPreparation.cc` route-tree injection），0.0294 即逐分支真实几何的产物。本任务仅补文档性确认，无代码。 |
| R2 skew 修复手段（sizing/插入/蛇形） | sizing 框架存在但从未激活（target 始终高于 internal skew → no_op）。本任务**验证通路**（紧 target 探针实测 sizing 行为），不新增插入/蛇形（internal 0.0294 已低于 Innovus eval 0.069，内部继续压榨收益≪eval 残差，等用户统一 eval 后再定向）。 |
| R3 自适应 skew target | **本任务核心代码项**（见 §2）。 |
| R4 balanceTopology 挪点重评估 | **本任务实验项**：tolerance 扫描（depth-9 新几何下重新标定），按数据定去留。 |
| R5 优化器停止条件基于可信模型 | 随 R3 落地（target 从拍脑袋 0.08 → period 推导）；P0-A 已修复模型可信性。 |

原验收 "eval skew ≤0.09 / 内外偏差 ≤20%" 依赖 eval 重跑，按用户决定推迟到全部任务后统一执行——本任务以内部口径 + 通路证据交付，eval 口径在最终汇报中标注待验。

## 2. 自适应 skew target（R3）

**语义**：每 clock 的有效 target = `skew_bound`（无 period 或 fraction=0 时）否则 `min(skew_bound, skew_period_fraction × clock_period_ns)`。

- 新配置键 `skew_period_fraction`，默认 **0.006**（0.6%；对标 Innovus vga_lcd 自动推导 0.061ns @ 10ns period），0 = 关闭（完全回退旧语义）。
- `skew_bound` 保持"上限"语义不变（存量配置 0.08 仍是天花板），自适应只会收紧不会放松——与 Innovus "小 period 自动收紧"方向一致。
- period 来源：`Clock::get_clock_period_ns()`（SDC 解析）；缺失（=0）时回退 skew_bound 并在报告标注。
- 实现点：`Optimization.cc:184` 改为 per-clock 解析（helper `ResolveClockTargetSkewNs(config, clock)`，放 optimization 内部或 config 工具），Setup 报告行加 derivation 说明。

vga_lcd 预期：target 0.08 → 0.06；internal 0.0294 仍达标 → 优化器仍 no_op（**预期内**，与 Innovus 同一 target 量级下我们内部已达标）；行为对齐而非 QoR 变化。

## 3. 优化器通路探针（R2/R5 验证，不改代码）

本地 run 设 `skew_period_fraction=0.002`（target 0.02 < 0.0294）→ 优化器必须真正求解。记录：iterations/accepted edits/skew 改善/面积代价/运行时间。判定通路健康（接受任何结果，包括"无可改善候选"——那也是 sizing-only 手段边界的真实证据，写入 validation 支撑后续是否上插入/蛇形的决策）。

## 4. balanceTopology tolerance 扫描（R4）

`htree_topology_tolerance` ∈ {0.02, 0.1(默认), 10(≈关闭)} 三个本地 run，对比 initial_skew / WL / depth 选择 / buffer。按数据：默认改/留 + 记录。预期：depth-9 短 level 几何下投影的作用减弱；若 10(off) 不劣化 skew 且省 WL，倾向放宽默认到更大 tolerance（减少人为挪点）。

## 5. 测试

- `ResolveClockTargetSkewNs` 单测：fraction=0 回退、min 截断两侧、period 缺失回退、负值防御。
- Config 解析 `skew_period_fraction`（22 键）。
- 既有全量回归。

## 6. 改动点

| 文件 | 改动 |
|---|---|
| `Config.{hh,cc}` | `_skew_period_fraction=0.006` + getter/setter/reset/键表(21→22)/解析/报告行 |
| `flow/optimization/Optimization.cc` | per-clock target 解析 + Setup 报告行（target 与 derivation）|
| 新 helper（`flow/optimization/` 内部或 OptimizationPreparation） | `ResolveClockTargetSkewNs` |
| `interface/default_config/cts_default_config.json` | 增键 0.006 |
| 测试 | helper 单测（挂入合适既有目标或新建 `icts_test_flow_optimization`）|

## 7. 风险与回滚

- 风险：自适应收紧使某些设计优化器长跑 → fraction 可配 0 关闭；solver 本身有 stop_reason 收敛。
- 回滚：revert 单提交或配置 `skew_period_fraction: 0`。
