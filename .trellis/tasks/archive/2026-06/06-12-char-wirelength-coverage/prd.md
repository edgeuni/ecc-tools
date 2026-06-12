# P0-B 表征线长覆盖修复

> 父任务: 06-11-align-commercial-cts · 优先级: P0 · 基准: iwls2005__vga_lcd
> 根因依据: 父任务 `research/vga-lcd-topology-gap-root-cause.md` §4 R2

## Goal

消除 H-tree 表征线长网格的覆盖截断与量化失真：让所有拓扑 level 请求长度都能被直接表征（vga_lcd 需 13 个覆盖单位，当前仅 3），减少 SegmentPruning DP 组合插入的中间 buffer 和 ceil 虚高带来的选型偏差。

## 问题事实（vga_lcd, ecc-tools.log "HTree Characterization Grid Plan"）

- `wirelength_unit` 未配置 → auto 推导 = max_level_length/13 = 38.98um（`WirelengthGrid.cc:175`）
- `wirelength_iterations = min(配置 3, 需求 13) = 3`（`WirelengthGrid.cc:192`）→ 仅直接表征 38.98/77.96/116.94um，`decision_flags=direct_bins_capped`
- 13 个请求长度 ceil 对齐（`MakeCoveringLengthIndex`，`WirelengthGrid.cc:46`）后塌缩为 6 bin；底层 ~6-15um 段按 38.98um 表征（虚高至 ~6x）
- 超过 116.94um 的层靠 DP 组合（`SegmentPruning.cc`），每次组合插一个中间 buffer

## Requirements

> **设计修订（2026-06-12，进入设计后的架构事实）**：CharBuilder 的 pattern 枚举是 `2^slots × buffer组合`，且 `slots = length_idx`（`CharPatternEnumerator.cc:52`）——直接表征 13× unit 意味着 ~2¹³ 拓扑、百万级 pattern，**"iterations 跟随 required 全覆盖"不可行**，cap 正是防爆炸设计。同时确认 SegmentPruning 组合 join（HashJoin on slew/cap bucket）**不必然插中间 buffer**（kAll 前沿含纯线 pattern），组合的真实代价是 bucket 量化误差逐级累积（vga_lcd 日志 8258 个 slew_bucket_mismatch 拒绝）与组合开销（SourceTrunk 35.5s）。因此本任务目标修订为：**把 auto 模式直接表征上限从 legacy 3 提升到可配置 cap（默认 8），使组合链深从 4~5 级降至 ≤1 级 join**；unit 公式与 ceil 语义不动（更细 unit 需要 slot 解耦的表征架构改动，超出本任务，转 P1 评估）。

- R1: auto-derived 模式（`wirelength_unit_um` 未配置或 collapsed）下，直接表征 bins = `min(required_covering_iterations, auto_direct_bins_cap)`；新增可选配置 `auto_direct_bins_cap`（默认 8，≤2⁸ 拓扑/length 的成本上界）。legacy `wirelength_iterations` 配置仅在显式 unit（runtime_config）模式下生效，auto 模式不再被模板值 3 截断。
- R2: runtime_config 模式（显式 unit 且未 collapsed）行为完全不变（回归保护）。
- R3: `required > cap` 时 dense 1..cap 直接表征（保证组合可选最优拆分对）；`required ≤ cap` 时维持按需 sparse 覆盖索引。
- R4: DP 组合仅对超出 cap 的超长 bin 兜底；组合行为日志/报告维持可见。
- R5: 集成验证（vga_lcd，本地输出，基线 = P0-A 修复后二进制）：对比 direct bins、CharBuilder 时间、SegmentFrontier/SourceTrunk 合成时间、boundary slew_bucket_mismatch 拒绝量、QoR（内部 skew/depth/buffer/WL）、总运行时间。

## 非目标

- 不改深度搜索/剪枝逻辑（P1-C）；不引入 per-branch 表征（P1-D 范畴）。
- 不改 auto unit 推导公式与 ceil covering 语义（slot 解耦前置，转 P1 实验）。

## Acceptance Criteria

- [x] vga_lcd 默认配置下：direct_characterization_bins 3 → 6（dense 1..6），所有 ≤6× 的请求 bin 直接表征。
  - 勘误（2026-06-12 cap 扫描实测）：原"cap 默认 8 + 13× 链深 ≤1 join"修订——QoR 在 cap 6/7/8 完全一致（skew 0.0562 全同、buffer ±1），cap=6 为成本拐点（CharBuilder 2.2s vs cap8 的 20.5s），13× 链深 2 joins 实测无质量损失。默认值定 6，需要单 join 可配置 cap≥7。
- [x] CharBuilder 构建时间增幅 ≤5s：实测 0.024s → 2.23s（+2.2s）。
- [x] SourceTrunk segment frontier 合成时间较 P0-A 基线显著下降：35.3s → 16.1s（-54%）；端到端 67.5s → 37.1s（-45%）。
- [x] runtime_config（显式 unit）路径行为不变（单测 RuntimeConfiguredGridStaysUntouched 钉死）。
- [x] 既有测试全绿（16/16）+ 新增 WirelengthGrid 单测 7 用例。
- [x] 新旧对比记录写入本任务 `research/validation-vga-lcd.md`（含 cap 6/7/8 扫描全表）。

## 依赖

- **前置：06-12-fix-wire-res-unit（P0-A）**——R 修复改变表征表数值与可行域，本任务的所有对比必须基于 P0-A 之后的新基线。
