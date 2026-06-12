# Implement: P0-B 表征线长覆盖修复

> 任务: 06-12-char-wirelength-coverage · 按序执行

## Checklist

### 1. 配置与结构体
- [ ] `Config.hh`：`_auto_direct_bins_cap = 8`、getter/setter、reset 默认
- [ ] `Config.cc`：`kSupportedConfigKeys` 20→21 增 `auto_direct_bins_cap`、`ApplyUnsignedIfPresent` 解析、报告行
- [ ] `CharBuilder.hh::Config`：`std::optional<unsigned> auto_direct_bins_cap`；核查该 struct 是否参与 CharacterizationLibrary 缓存比较（ensure/reused），是则补入比较
- [ ] `src/interface/default_config/cts_default_config.json` 增键

### 2. 核心逻辑
- [ ] `WirelengthGrid.hh`：`CharacterizationGridPlan` 增 `auto_direct_bins_cap` 字段
- [ ] `WirelengthGrid.cc`：adapted 分支 `wirelength_iterations = min(required, max(1, cap.value_or(8)))`；icts::Config 重载传递
- [ ] `CharacterizationLibrary.cc` icts::Config→CharBuilder::Config 映射补传
- [ ] `Characterization.cc` grid plan 报告行（adapted 时）

### 3. 单测
- [ ] 新 `test/flow/synthesis/htree/WirelengthGridTest.cc`（6 用例，design §5）
- [ ] 挂入 `icts_test_flow_synthesis_htree` SOURCES
- [ ] `cmake --build build -j --target icts_test_flow_synthesis_htree && ctest --test-dir build -R icts_test_flow_synthesis_htree --output-on-failure`

### 4. 回归
- [ ] `cmake --build build -j` 全量
- [ ] `ctest --test-dir build -R icts --output-on-failure` 16+ 全绿

### 5. vga_lcd 集成验证（design §6）
- [ ] /tmp/p0a_vga_validation: cp -r ws_base ws_p0b（注意 ws_base 已被 base run 改写 config——改用从 DAC-27-CTS 重新克隆）+ 生成 tcl_p0b（输出 out_p0b）
- [ ] `bin/ecc_bin -script tcl_p0b.tcl > run_p0b.log`
- [ ] 提取对比表（vs out_fixed 基线），写 `research/validation-vga-lcd.md`
- [ ] 验收判定：CharBuilder 增幅 ≤5s；SourceTrunk 不恶化；QoR 漂移可解释

### 6. 收尾
- [ ] trellis-check
- [ ] commit（代码+测试+默认配置）→ journal → archive

## Review Gates
- Gate-1（步骤 3 后）：RuntimeConfigModeUntouched 用例证明显式模式零行为变化
- Gate-2（步骤 5 后）：vga_lcd 对比表评审（成本验收 + QoR 可解释性）

## Rollback
revert 单提交；或配置 `auto_direct_bins_cap: 3`。
