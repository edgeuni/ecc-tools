# Design: P0-B 表征线长覆盖修复（auto 模式直接表征 cap 提升）

> 任务: 06-12-char-wirelength-coverage · 2026-06-12
> 前置: P0-A（已合入 a9e8d7503）。基线 = P0-A 后二进制的 vga_lcd 本地 run（/tmp/p0a_vga_validation/out_fixed）。

## 1. 问题与方案选型

vga_lcd auto 模式：unit=38.98um、required=13，legacy `wirelength_iterations=3` 截断 → 仅 3 个直接 bin，>3× 的层/trunk 长度全靠 SegmentPruning 组合（链深 4~5 级 join），每级 join 有 slew/cap bucket 量化误差（boundary 拒绝 8258 条）且 SourceTrunk 合成耗时 35.5s（总时长 64%）。

**为什么不能全覆盖**：pattern 枚举 = `2^slots × monotonic组合`，`slots = length_idx`（`CharPatternEnumerator.cc:52-66`）。n=13 → ~10⁶ patterns × ~100 STA 采样，不可行。实测吞吐 ~360k chars/s，n=1..8 全表征 ≈ 16.5k patterns ≈ 1.65M chars ≈ ~4.6s（上界，按需更少）。

**为什么不在本任务做 slot 解耦/细 unit**：slot 边界 = unit 整数倍（`CharTopologyPlanner::buildTopologyDesc`），解耦 slots 与 length_idx 需同步改 slot pitch、pattern 位置归一、embedding 插值、join 语义——表征架构级改动，风险与体量超 P0；细 unit 在不解耦时反而放大 slots。转 P1 评估。

**选定方案**：auto 模式 cap 从 3（legacy 模板值）提升为独立可配置 `auto_direct_bins_cap`（设计初值 8）：
- 13× = 8+5 或 7+6 单次 join（链深 4~5 → 1）
- ≤8× 的全部真实 level bin（vga_lcd: 1,2,3,4,8）直接 STA 表征，无 join 误差
- 成本上界 2⁸ 拓扑/length，CharBuilder 增量 ~2-5s

> **实施修订（2026-06-12，cap 扫描实测后）**：单 char STA 成本随 slot 数增长，吞吐估算偏乐观——cap=8 实测 CharBuilder 20.5s。扫描 cap∈{6,7,8} 显示 **QoR 完全一致**（skew 0.0562 全同、buffer ±1），cap=6 为成本拐点（char 2.2s、端到端 37.0s）。**默认值定为 6**（代码 `kDefaultAutoDirectBinsCap=6U`、Config 默认、json 模板同步），需要更长直接 bin/单 join 时配置 cap≥7。本文其余"默认 8"按此修订理解。数据见 `research/validation-vga-lcd.md`。

## 2. 语义定义

| 模式 | 触发 | 直接 bins | 配置生效 |
|---|---|---|---|
| auto-derived | unit 未配置 / 配置 unit collapsed | `min(required_covering_iterations, auto_direct_bins_cap)` | `auto_direct_bins_cap`（默认 8）；legacy `wirelength_iterations` **不参与** |
| runtime_config | 显式 unit 且未 collapsed | 不变（plan 不设置，CharBuilder 直接用配置的 unit+iterations） | `wirelength_unit_um` + `wirelength_iterations` 完全尊重 |

理由：auto 模式下 unit 是推导值，模板里的 `wirelength_iterations: 3` 对推导 unit 没有覆盖语义（正是本 bug 根源）；显式模式保持用户完全控制。存量实验配置（含 "3"）无需修改即受益。

直接索引选择（`ResolveDirectCharacterizationLengthIndices`，行为保持）：
- `required > bins`（capped）→ dense `1..bins`（保证超长 bin 的最优拆分对均直接可用）
- `required ≤ bins` → 按需 sparse unique covering 索引

## 3. 改动点

| 文件 | 改动 |
|---|---|
| `source/flow/synthesis/htree/characterization/wirelength/WirelengthGrid.cc` | `ResolveCharacterizationGridPlan`（CharBuilder::Config 重载）：adapted 分支 `wirelength_iterations = min(required, auto_cap)`，`auto_cap = max(1, config.auto_direct_bins_cap.value_or(8))`；Config 重载传递新键 |
| `source/flow/synthesis/htree/characterization/wirelength/WirelengthGrid.hh` | `CharacterizationGridPlan` 增加 `auto_direct_bins_cap` 字段（报告用） |
| `source/module/characterization/builder/CharBuilder.hh` | `CharBuilder::Config` 增加 `std::optional<unsigned> auto_direct_bins_cap`（注意：若 Config 参与缓存 key 比较，需一并加入） |
| `source/database/config/Config.{hh,cc}` | 成员/getter/setter/reset 默认 8；`kSupportedConfigKeys` 增 `auto_direct_bins_cap`（数组长度 20→21）；`ApplyUnsignedIfPresent` 解析；config 报告行 |
| `source/flow/synthesis/htree/characterization/library/CharacterizationLibrary.cc:145` 附近 | icts::Config → CharBuilder::Config 映射补传新键 |
| `source/flow/synthesis/htree/characterization/Characterization.cc` | grid plan 报告表增加 `auto_direct_bins_cap` 行（仅 adapted 时） |
| `src/interface/default_config/cts_default_config.json` | 增加 `"auto_direct_bins_cap": "8"`（文档化默认值） |

不动：decision_flags 的 `direct_bins_capped` 判定（13>8 仍如实标记）；unit 公式；ceil covering；SegmentPruning。

## 4. 风险

- 风险 A：CharBuilder 时间增长超预期（pattern 估算偏差）。缓解：vga_lcd 实测；cap 可配置回 3 兜底；验收 ≤5s。
- 风险 B：frontier 输入 pattern 变多 → SegmentFrontier/深度搜索状态空间变化 → QoR 漂移。缓解：A/B 实测记录；QoR 漂移方向需可解释（更精的直接表征替代组合链，预期中性偏好）。
- 风险 C：`enumerateWirelength` 对 n=8 的 2⁸ 枚举内 STA 采样吞吐退化（pattern 内 slot 数多 → 单 char 电路更大）。缓解：实测；若超标降默认 cap 至 7（13=7+6 仍单 join）。
- 风险 D：CharacterizationLibrary 缓存复用 key 漏加新字段 → 跨配置错误复用。缓解：实现时核查 ensure()/reused 的比较逻辑并补字段 + 单测。

## 5. 测试设计

新 `test/flow/synthesis/htree/WirelengthGridTest.cc`（挂入 `icts_test_flow_synthesis_htree` SOURCES，纯函数测试）：
1. AutoModeIgnoresLegacyIterationsAndUsesCap：unit 缺失、legacy=3、required=13 → bins=8
2. AutoModeFullCoverageBelowCap：required=5 → bins=5（无 capped）
3. AutoModeHonorsConfiguredCap：cap=4 → bins=4；cap=0/缺失 → 默认 8
4. RuntimeConfigModeUntouched：显式 unit 未 collapsed → adapted=false、plan.wirelength_iterations 不被设置（钉死现状）
5. CollapsedConfiguredGridAdoptsAutoCap：显式 unit collapsed → adapted=true、cap 生效
6. DenseIndicesWhenCapped / SparseIndicesWhenCovered：索引选择行为

## 6. 集成验证（vga_lcd）

复用 /tmp/p0a_vga_validation 流程：ws_p0b 克隆 + tcl_p0b → 新二进制 run → 对比 out_fixed：
- Grid Plan 表（bins 3→8、新报告行）
- CharBuilder build 时间（基线 0.024s）
- HTree Synthesize segment frontiers 时间/entries（基线 0.445s/36824）
- SourceTrunkSegment frontier 时间（基线 35.3~35.5s）
- boundary slew_bucket_mismatch 拒绝量（基线 8258+6577+6935+5519）
- QoR：initial skew/depth/buffer/area/WL/总时长
结果写 `research/validation-vga-lcd.md`。

## 7. 回滚

单 commit；配置 `auto_direct_bins_cap` 可设 3 行为回退（语义上等价 legacy，除 legacy iterations 不再参与 auto 模式）。
