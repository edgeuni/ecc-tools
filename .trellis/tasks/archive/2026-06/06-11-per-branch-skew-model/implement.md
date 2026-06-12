# Implement: P1-D（新基线校准版）

## Checklist

### 1. 自适应 target（代码）
- [x] Config: `skew_period_fraction` 默认 0.006（getter/setter/reset/键表 21→22/解析/报告行）
- [x] `ResolveClockTargetSkewNs(config, clock)` helper + Optimization.cc per-clock 接入 + Setup 报告 derivation
- [x] default json 增键

### 2. 单测
- [x] helper 4 用例（回退/截断两侧/缺 period/负防御）；config 解析——新目标 `icts_test_flow_optimization` 5 用例 + ConfigTest 4 用例
- [x] 全量 ctest 回归（17/17，原 16 + 新 1）

### 3. vga_lcd 验证（本地）
- [x] 默认 run：target 0.08→0.06、no_op 保持、QoR 与 P1-C 基线逐位一致
- [x] 探针 run（fraction=0.002 → target 0.02）：优化器激活（288 scored edits / 8 batch trials / no_improving_candidate / target_met=false 如实上报）
- [x] R4 扫描：tolerance {0.02, 0.1, 10} 三 run 对比 → **数据定论：保留默认 0.1**（tol=10 skew +34.7% 仅省 WL 0.02%；tol=0.02 双输）
- [x] 写 `research/validation-vga-lcd.md`（含 R1 已成立的 P1-C 证据引用）

### 4. 收尾
- [ ] PRD 验收勾选/勘误（eval 口径标注"待统一重跑"）→ trellis-check → commit → journal → archive

## Review Gates
- Gate-1: helper 单测红→绿 + 默认 run 行为零回归 ✅（17/17 通过；默认 run QoR 逐位一致）
- Gate-2: 三组实验数据齐 + 结论可执行（tolerance 默认调整与否）✅（保留 0.1，数据见 validation）

## Rollback
revert 单提交或 `skew_period_fraction: 0`。
