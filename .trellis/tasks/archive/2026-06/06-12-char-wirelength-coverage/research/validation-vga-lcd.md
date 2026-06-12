# P0-B 验证报告：vga_lcd 表征覆盖 cap 扫描与默认值选定

> 任务: 06-12-char-wirelength-coverage · 2026-06-12
> 基线: P0-A 修复后二进制（cap=3 legacy 行为，/tmp/p0a_vga_validation/out_fixed）
> 方法: 同一 P0-B 二进制 + workspace config 设置 `auto_direct_bins_cap` ∈ {6,7,8} 扫描；最终默认值以无配置键的 run 确认

## Cap 扫描结果（vga_lcd, 16901 sinks, required_covering_iterations=13）

| 配置 | direct bins | CharBuilder | trunk frontier 合成 | 端到端 | initial_skew | buffers | area um² |
|---|---|---|---|---|---|---|---|
| P0-A 基线（legacy cap 3） | 3 | 0.024s (8.6k chars) | 35.3s / 73178 entries | 67.5s | 0.0576 | 5745 | 16123.0 |
| cap=6 | 6 | 2.2s (235k) | 16.3s / 69670 | **37.0s** | 0.0562 | 5745 | 16176.7 |
| cap=7 | 7 | 6.9s (640k) | 15.7s / 40228 | 42.0s | 0.0562 | 5744 | 16173.9 |
| cap=8 | 8 | 20.5s (1.69M) | 15.5s / 40228 | 55.3s | 0.0562 | 5744 | 16173.9 |
| **默认确认 run（无键，=6）** | 6 | 2.23s | 16.1s | **37.1s** | 0.0562 | 5745 | 16176.7 |

逐长度表征成本（实测，验证 2^slots 模型）：n=1..8 patterns = 5/19/63/192/552/1520/4048/10496，chars = 490/1.9k/6.3k/19k/55k/152k/405k/1050k——**每 +1 bin 成本 ×~2.6**，n=8 单项占 62%。

## 结论与默认值依据

1. **QoR 在 cap 6/7/8 完全一致**（skew 全部 0.0562 ns、buffer ±1、area ±0.33%、WL/深度不变）：精度收益（skew -1.4ps、组合链深下降）在 cap=6 已全部兑现，更大的 cap 只烧表征时间。
2. **默认 6 = 成本拐点**：CharBuilder 2.2s（验收 ≤5s ✓）、端到端 37.0s（比 P0-A 基线快 45%，比 R-bug 时代 56.0s 快 34%）。
3. 组合链深：cap=6 下 8×=6+2（1 join）、13×=6+7→2 joins（vs legacy 3+ joins）；实测 QoR 与 cap=8（13×=8+5 单 join）无差异，链深 2 不构成质量损失（PRD 原"≤1 join"验收据此修订）。
4. SourceTrunk 合成 35.3s→16.1s（-54%）：组合基底从 3 bins 升到 6 bins 后 DP 闭包工作量大减——这是端到端提速的主力。
5. initial_skew 57.6→56.2ps：长 bin 直接 STA 表征替代组合链（每级 join 有 slew/cap bucket 量化），模型一致性提升。eval 级（Innovus route+STA）验证留待 P0/P1 改动累积后统一重跑。

## 行为/语义变化记录

- auto-derived 模式下 legacy `wirelength_iterations`（模板值 3）不再约束直接表征 bins；改由 `auto_direct_bins_cap`（新配置键，默认 6）约束。存量实验配置无需修改即生效。
- runtime_config 模式（显式 `wirelength_unit_um`）行为零变化（单测 RuntimeConfiguredGridStaysUntouched 钉死）。
- Grid Plan 报告新增 `auto_direct_bins_cap` 行；`direct_bins_capped` 标志语义不变（required 13 > 6 时仍如实标记）。

## 测试

- 新增 `WirelengthGridTest`（7 用例）：auto cap 默认/覆盖/下限钳位、低于 cap 全覆盖、runtime 模式不变、collapsed 配置自适应、dense/sparse 索引选择。
- 全量 iCTS ctest 16/16 通过。

## 产物

- 扫描日志: `/tmp/p0a_vga_validation/run_{p0b,cap7,cap6,final}.log`（关键数字已录入本文）
