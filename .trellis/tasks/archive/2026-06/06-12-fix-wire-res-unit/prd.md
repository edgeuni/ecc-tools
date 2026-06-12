# P0-A 修复 wire 电阻 1000x 单位 bug

> 父任务: 06-11-align-commercial-cts · 优先级: **P0（最高，其余时序改进的前提）** · 基准: iwls2005__vga_lcd
> 根因依据: 父任务 `research/vga-lcd-topology-gap-root-cause.md` §3 R1

## Goal

修正 iCTS 中 wire 电阻被 1000 倍低估的单位换算错误，使表征、拓扑选型、FastSTA、sink-load 可行性、BST 等所有时序决策使用真实线电阻（vga_lcd/ICsprout55 MET4: 0.914 Ω/um，而非 0.000914），并用单元测试钉死单位契约，防止回归。

## 问题事实

- iDB 约定：`lef_read.cpp:642` 将 LEF `RESISTANCE RPERSQ`（Ω/sq）**原值**存入 `IdbLayerRouting::_resistance`；iSTA（`TimingIDBAdapter.cc:172`）直接按 Ω 使用，无除法。
- `Wrapper::queryWireResistance / queryRequiredWireResistance`（`WrapperRc.cc:251/274`）= `get_resistance() × length / width` → 返回值单位已是 **Ω**。
- 但 4 处消费方把返回值当毫欧再 `/1000`：
  1. `WrapperRc.cc:159`（unit RC probe → 日志报告）
  2. `WrapperRc.cc:293`（`queryConfiguredClockRouteSegmentRc` → `ClockRouteSegmentRc` → CharBuilder 表征 / ClusterConstraintEvaluator / Router::buildRCTree / BST / AnalyticalCharacterization）
  3. `FastStaChar.cc:134`（FastSTA segment 表征）
  4. `FastStaParasitics.cc:91`（FastSTA clock net 寄生，函数名 `queryWireResistanceOhm` 却返回除以 1000 的值）
- 实测后果（vga_lcd）：日志 `unit_resistance 914.000 uOhm/um`；FastSTA 内部 skew 0.053 vs eval 0.118；source trunk 选 472um 裸长段且 `max_length=300` 未拦截；优化器误判达标 no_op。

## Requirements

- R1: 移除上述 4 处 `/1000`（`kMilliOhmPerOhm` 常量与字面量 `1000.0`），使 `ClockRouteSegmentRc.resistance_per_um_ohm` 与 FastSTA 内部 R 均为真实 Ω/um。
- R2: 删除不再使用的 `kMilliOhmPerOhm` 常量定义；`FastStaParasitics.cc` 的 `queryWireResistanceOhm` 名实一致。
- R3: 新增单元测试（gtest，挂入 `icts_add_test_executable` 体系）锁定单位契约：合成 IdbLayout（RPERSQ=0.0914, WIDTH=0.1um, CPERSQDIST=0.0011069, EDGECAP=0.0000409, 1000 DBU/um）下：
  - `queryRequiredWireResistance(layer, 1um)` ≈ 0.914 Ω（容差 1e-9 相对）
  - `queryRequiredWireCapacitance(layer, 1um)` ≈ 2.0067e-4 pF
  - `queryConfiguredClockRouteSegmentRc(...)` 的 `resistance_per_um_ohm` ≈ 0.914（**不是** 0.000914）
- R4: 排查并报告所有其他 R 消费路径无补偿性 `×1000`（含 `unit_h_res`、BST `_unit_horizontal/vertical_resistance` 来源），确认链路一致；发现额外问题记录到任务 research/ 并修复或开新任务。
- R5: 集成验证（vga_lcd，本地输出目录，不写 DAC-27-CTS）：用修复后的 `ecc_bin` 重跑 CTS，记录新旧对比——unit_resistance 日志值、内部 skew/优化器行为、source trunk 段长、htree 深度可行性、buffer 数、运行时间。**允许 QoR 指标暂时变差**（真实 R 下约束变紧属预期），但 flow 必须跑通且行为可解释。

## 非目标

- 不在本任务内调表征覆盖（P0-B）、拓扑/优化策略（P1-C/D/E）。
- 不改 iDB 存储语义与 LEF 读取（保持 LEF 原值约定）。

## Acceptance Criteria

- [x] 4 处除法移除，全仓不再有把 `queryWire*Resistance` 返回值除以 1000 的代码。
- [x] 新增 WrapperRc 单元测试通过，并集成进 ctest；既有 iCTS 测试套件全绿（16/16，无用例需要更新期望——既有用例未绑定旧数值）。
- [x] vga_lcd 本地重跑：日志 unit_resistance ≈ 914 mOhm/um（0.914 Ω/um）；run 完成无 crash。
  - 勘误（2026-06-12 实测）：原预期"不再出现 >300um 无 buffer 段"不成立——472um trunk 段在真实 R 下仍电气合法（0.5ns slew 上限宽松），根因是 **max_length 约束未在 trunk/htree 段实施**，属独立缺陷，已记录至 `research/validation-vga-lcd.md` 并转后续任务（P1-E 或单开）。
- [x] 新旧行为对比记录写入本任务 `research/validation-vga-lcd.md`（A/B 同源二进制对比，base 与 6 月 6 日实验日志逐项一致）。
- [x] 不修改 DAC-27-CTS 仓库任何内容。

## 依赖

- 无前置任务（本任务是 P0-B/P1-C/D/E 的前置）。
- 后续：P0-B 在本任务合入后基于新基线开展。
