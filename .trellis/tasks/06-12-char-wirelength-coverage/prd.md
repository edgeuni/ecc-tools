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

- R1: 直接表征覆盖自适应：`wirelength_iterations` 默认随 `required_covering_iterations` 走（移除或放宽 `std::min` cap），保留配置显式上限的能力；评估并记录表征运行时间代价（vga_lcd 当前 CharBuilder 仅 0.024s，13 点预计仍 <0.2s，需实测确认）。
- R2: 量化失真收敛：评估两个方向并择一落地（design.md 论证）——
  - a) 缩小 unit（如 median level length / N 或固定细 unit），使 ceil 误差受控；
  - b) 放弃 uniform lattice，对 13 个实际请求长度直接逐点表征（每 level 一个精确长度条目）。
- R3: 修复后 `distinct_level_bins` 应 ≥ 不同请求长度的真实档数；`direct_bins_capped` 标志在默认配置下不再出现。
- R4: DP 组合仅在超出直接表征范围时兜底，组合发生时日志可见（保留现有 schema 报告）。
- R5: 集成验证（vga_lcd，本地输出目录）：对比修复前后 htree 中间组合 buffer 数、每路径级数、内部 skew/latency、CharBuilder 运行时间。

## 非目标

- 不改深度搜索/剪枝逻辑（P1-C）；不引入 per-branch 表征（P1-D 范畴）。

## Acceptance Criteria

- [ ] vga_lcd 默认配置下：`required_covering_iterations` 全覆盖（无 capped 标志），13 个 level 长度各自获得直接表征条目（或 ≥13 distinct bins）。
- [ ] SegmentPruning 因长度组合插入的中间 buffer 数较修复前下降（日志/结构统计佐证）。
- [ ] CharBuilder 运行时间增幅 ≤ 5s（vga_lcd）。
- [ ] 既有测试全绿 + 新增 WirelengthGrid 覆盖/量化行为单测（覆盖 auto-derive、cap 解除、ceil 边界）。
- [ ] 新旧对比记录写入本任务 `research/`。

## 依赖

- **前置：06-12-fix-wire-res-unit（P0-A）**——R 修复改变表征表数值与可行域，本任务的所有对比必须基于 P0-A 之后的新基线。
