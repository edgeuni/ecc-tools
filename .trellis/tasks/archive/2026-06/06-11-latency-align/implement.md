# Implement: P1-E(delay-margin 有界选型)

## Checklist

### 1. 选型策略(代码)
- [x] Config: `selection_delay_margin` 默认 **0.07**(初版 0.1,扫描定 0.07),0=off(getter/setter 负值 clamp/reset/键表 22→23/解析/报告行)
- [x] `SelectionPolicy.hh`(header-only 模板):margin=0 → 中位(与旧实现逐位一致);margin>0 → delay ≤ front_min×(1+margin) 内取 min-power
- [x] 三处接入:`SelectBestGlobalEntry`(决定权)、`SelectBestHTreeChar`(per-depth 报表一致性)、`SelectBestSegmentEntry`(trunk)
- [x] 可观测性:HTree Global Selection 表(policy/margin/front_min_delay/selected delay/power/buffered_levels/depth)+ trunk selection 字段(policy/margin/selected_buffer_count)
- [x] default json 增键(0.07)

### 2. 单测
- [x] SelectionPolicy 6 用例(回退一致/界内 min-power/极小 margin→min-delay/极大→min-power/单条目/空)
- [x] Config 解析(23 键;默认值断言随 0.07 更新)
- [x] 受影响断言适配:无(内部 Config 默认 0,fixture 不经 icts::Config 装配,零分叉)
- [x] 全量 ctest 回归 17/17

### 3. vga_lcd 实验(margin 扫描)
- [x] 7 run:margin {0, 0.05, 0.07, 0.08, 0.1, 0.2, 0.5} + 无键默认路径 run;级数构成/skew/maxarr/WL/buffer/面积/power 全表见 research/validation-vga-lcd.md
- [x] O1:0-buffer trunk 幸存 strict 过滤且 margin>0 即胜出(级数 10→8 全部来源);O2:htree min-delay 端仍 5 buffered levels(可行域下限);O3:全 margin 全局赢家 depth 9
- [x] R3:root master BUFX20 已饱和;margin 后 internal maxarr 0.4486 < Innovus eval 0.472,结构改造挂起待 eval 残差
- [x] 默认 margin=0.07(拐点:vs 0.05 latency 持平面积减半;vs 0.08 有行为悬崖)→ 无键 run 与 m007 数值一致
- [x] 写 `research/validation-vga-lcd.md`

### 4. 收尾
- [ ] PRD 验收勾选/勘误 → trellis-check → commit → journal → archive

## Review Gates
- Gate-1: margin=0 全量回归零变化 ✅(m000 与基线逐位一致;ctest 17/17)
- Gate-2: 扫描数据齐 + 默认 margin 有据 ✅(0.07,拐点分析见 validation)

## Rollback
`selection_delay_margin: 0` 或 revert 单提交。
