# Design: P0-A 修复 wire 电阻 1000x 单位 bug

> 任务: 06-12-fix-wire-res-unit · 2026-06-12

## 1. 单位契约（修复后的唯一事实源）

```
LEF RESISTANCE RPERSQ (Ω/sq)
  └─ lef_read.cpp:642 原值存入 IdbLayerRouting::_resistance        [不动]
       └─ Wrapper::queryWireResistance / queryRequiredWireResistance
          = RPERSQ × length_um / width_um  → 返回 Ω                 [不动]
            └─ 所有消费方直接按 Ω 使用，禁止再除/乘换算              [本次修复]
```

依据：iSTA `TimingIDBAdapter.cc:172` 同样按 Ω 直接使用 `get_resistance()`；本 PDK（N551P6M）LEF UNITS 无 RESISTANCE 缩放语句。电容链路（`queryWireCapacitance` 返回 pF）已正确，不动。

## 2. 改动点（4 处删除除法 + 常量清理）

| 文件 | 行 | 改动 |
|---|---|---|
| `src/operation/iCTS/source/database/io/WrapperRc.cc` | 159 | `... queryWireResistance(...) / kMilliOhmPerOhm` → 去掉 `/ kMilliOhmPerOhm` |
| 同上 | 293 | `queryRequiredWireResistance(...) / kMilliOhmPerOhm` → 去掉除法 |
| 同上 | 47 | 删除 `constexpr double kMilliOhmPerOhm` |
| `src/operation/iCTS/source/database/adapter/fast_sta/segment_char/FastStaChar.cc` | 134 | 去掉 `/ kMilliOhmPerOhm`；删除 52 行常量 |
| `src/operation/iCTS/source/database/adapter/fast_sta/clock_net_parasitic/FastStaParasitics.cc` | 91 | 去掉 `/ 1000.0`（函数 `queryWireResistanceOhm` 名实归一） |

## 3. 波及面与风险

**直接受益（自动纠正）**：`ClockRouteSegmentRc.resistance_per_um_ohm` 的全部下游——CharBuilder 表征 STA 采样、SinkLoadRegion/ClusterConstraintEvaluator(`unit_h_res`)、Router::buildRCTree、BST(BottomUpMergeBalance, 经 TopologyConfig 注入)、AnalyticalCharacterization、FastSTA（timing/优化/功率中的 R 项）。

**预期行为变化（不是回归，是修正）**：
- 线延迟/线上 slew 退化变为真实量级 → 长段（如 472um trunk）将不再可行 → trunk/htree 可能插入更多 buffer、内部 skew 估计上升（趋近 eval 实测 0.118）；
- 表征表中 slew/delay 数值整体变大 → 既有依赖具体数值的测试期望可能需要更新（逐一核对语义后更新，不得为绿而绿）；
- 优化器可能从 no_op 变为实际执行 sizing。

**风险与缓解**：
- 风险 A：某些 pattern 区间在真实 R 下不可行 → flow 失败。缓解：vga_lcd 集成验证必跑；若失败，分析是否为 P0-B（表征覆盖不足）范畴，记录并在 P0-B 解决，不在本任务内打补丁掩盖。
- 风险 B：隐藏的补偿性换算（某处 ×1000 抵消）。缓解：实现时全仓 grep `1000`/`kMilliOhm`/`milli` 在 R 路径上的出现并逐一核对（验收 R4）。
- 风险 C：测试期望大面积绑定旧数值。缓解：先跑全量 ctest 记录失败清单，按"单位修正后的正确期望"批量重算，提交说明逐条列出。

## 4. 单元测试设计

新文件 `src/operation/iCTS/test/database/io/WrapperRcTest.cc`（gtest，注册进 `test/database/CMakeLists.txt` 既有 `icts_add_test_executable` 模式，目标名 `icts_wrapper_rc_test`）：

- Fixture：手工构造 `idb::IdbLayout`（units: 1000 DBU/um；1 个 routing layer：WIDTH 100dbu(0.1um)、RESISTANCE 0.0914、CAPACITANCE 0.0011069、EDGECAPACITANCE 0.0000409）+ `idb::IdbDesign`（units 1000）；`Wrapper::set_idb_layout/set_idb_design` 注入；`Config` 设 `routing_layer=[1]`、wire_width 不配置（走 LEF width）。
- 断言（相对容差 1e-9）：
  1. `queryRequiredWireResistance(1, 1.0)` == 0.914（Ω）
  2. `queryRequiredWireResistance(1, 10.0)` == 9.14（线长比例）
  3. `queryRequiredWireCapacitance(1, 1.0)` == 0.0011069×0.1 + 0.0000409×2×1.1 = 2.00679e-4（pF）
  4. `queryConfiguredClockRouteSegmentRc(config).resistance_per_um_ohm` == 0.914 —— **回归哨兵：> 0.1 断言**，单位 bug 复发时直接红
  5. `capacitance_per_um_pf` == 2.00679e-4
- 具体 iDB setter 名以 `IdbLayer.h`/`IdbLayout.h`/`IdbDesign.h` 为准（实现时核对）。

## 5. 集成验证设计（vga_lcd, 本地目录）

1. 构建：`cmake --build build -j$(nproc) --target ecc_bin icts_wrapper_rc_test`（复用既有 build 缓存）。
2. 在 `.trellis/tasks/06-12-fix-wire-res-unit/validation/`（或 /tmp）克隆 vga_lcd 的 `icts_cts.tcl` + `flow_config.json` + `db_default_config.json`（源：DAC-27-CTS run 目录 work/workspace/config，只读引用），把所有 output 路径改到本地，input/PDK 路径保持指向原只读位置。
3. `bin/ecc_bin -script <本地 tcl>` 跑通，提取并与基线（父任务研究报告 §2/§3）对比：
   - `Runtime Routing / Wire RC` 表：期望 `unit_resistance 914.000 mOhm/um`
   - `CTS Optimization Clock Summary`：initial/optimized skew、accepted edits
   - `HTree Depth Candidate Summary`、trunk 段结构（`max_clock_net_wirelength`）、buffer 总数、运行时间
4. 结果写 `research/validation-vga-lcd.md`。

## 6. 回滚

单 commit 原子改动（4 处 + 测试），revert 即回滚；无数据迁移/接口变更。
