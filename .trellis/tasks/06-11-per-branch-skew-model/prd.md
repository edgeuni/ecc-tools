# P1-D per-branch 时序建模与 skew 修复

> 父任务: 06-11-align-commercial-cts · 优先级: P1 · 基准: iwls2005__vga_lcd
> 根因依据: 父任务 `research/vga-lcd-topology-gap-root-cause.md` §2.3/§5 R4

## Goal

消除"per-level 均一长度模型 vs per-branch 物理嵌入"的失配：让合成后的时序评估与修复按真实分支几何逐路径进行，把 vga_lcd 的 eval skew 从 0.118 ns 压到 ≤0.09 ns，并把内部 skew 估计与 eval 实测的偏差收敛到 ≤20%。

## 背景（现状缺陷）

- 表征/选型使用"层平均 Manhattan 距离 → ceil 量化"的统一长度（`Plan.cc:66-91`），同层 cell master 一致；
- 物理嵌入沿真实父子几何放 buffer，无蛇形补偿（`Embedding.cc:373`），同层分支长度离散 std 高达 125um（L3，见父任务 `research/level-spread-data.md`）；
- `balanceTopology`（`TopologyGen.cc:454-502`）为凑均匀把节点挪离负载质心（±10% tolerance），线长与均匀性双输；
- eval 实测：max/min 两条 9 级路径每级 cell 相同但 stage delay 逐级 +8~29ps 同向累积 = 95ps skew；ECC 内部估计 0.053 vs eval 0.118（方差型模型误差），优化器误判达标而 no_op。

## Requirements

- R1: 合成后（嵌入完成、几何确定）以真实 per-branch 几何驱动 FastSTA（route-tree 注入能力已存在，`OptimizationPreparation.cc`），确认其延迟对每条分支可分辨（依赖 P0-A 修复后 R 项生效）。
- R2: skew 修复手段至少实现其一并验证有效：
  - per-branch buffer sizing（解除 per-level 同型限制，现有 sizing 框架按 instance 粒度即可）；
  - buffer 插入/位置微调（早到分支加延迟 / 晚到分支减负载）；
  - wire snaking（早到分支加蛇形线长）。
- R3: skew target 自适应：替换固定 `skew_bound=0.08`，按 clock period 与设计规模推导（参照 Innovus 0.061=f(period) 的行为，具体公式在 design.md 论证）。
- R4: `balanceTopology` 的挪点逻辑重新评估：在 per-branch 建模生效后，它的"几何拉平"是否仍有正收益；无收益则移除或改为可配置。
- R5: 优化器停止条件以"修复后模型"为准；保留 no-op 路径但其判定必须基于可信的 per-branch 时序。

## 依赖（必须先行）

- **P0-A（wire R 1000x bug，任务待创建）硬依赖**：R≈0 时 per-branch 长度差异在模型中无延迟差，本任务所有手段失效。start 前 P0-A 必须合入并重建基线。
- P0-B（表征覆盖）建议先行：sizing 候选评估依赖表征表精度。
- 与 P1-C（深度解锁）无硬依赖、可并行，但最终在同一基线上联合验收。

## Acceptance Criteria

- [ ] vga_lcd eval skew_max：0.118 ns → **≤0.09 ns**（评估管线同父任务 §3）。
- [ ] ECC 内部 skew 估计 vs eval 实测偏差 ≤20%（基线为 0.053 vs 0.118 = +121%）。
- [ ] eval latency_avg 不劣化（≤0.625 ns），clock WL 增幅 ≤3%（蛇形/加 buffer 的代价上限）。
- [ ] 优化阶段不再因模型误判而 no_op：日志可见非零 accepted edits 或有据可查的真实达标判定。
- [ ] 单测覆盖：per-branch delay 可分辨性（长短分支构造用例）、skew 修复收敛性。

## 约束

- 验证基准仅 vga_lcd；DAC-27-CTS 只读。
- 复杂任务：start 前需补 `design.md` + `implement.md`。
