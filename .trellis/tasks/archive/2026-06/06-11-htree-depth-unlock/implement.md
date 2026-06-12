# Implement: P1-C 拓扑自由度解锁

> 任务: 06-11-htree-depth-unlock · 按序执行

## Checklist

### 1. 观测性（先行，独立可验证）
- [ ] `SinkLoadRegionLegalitySummary` 透出首个 hard-fail 详情（violation kind + node + load_count 已在 failure_reason 字符串中）
- [ ] `DepthSummary` 增 `first_hard_fail` 字段；`TopologyPruning.cc` 评估链路填充；`DepthPlanReport.cc` 表新增列
- [ ] 设置 `max_monotone_failed_level` 时 LOG_WARNING 一次（级别/原因）
- [ ] vga_lcd 本地跑：拿到原始违例实证（预测 fanout load_count 5~8 @ boundary 11）

### 2. SPLIT 规则（共享函数）
- [ ] `region/SinkLoadRegion.hh/.cc`：`SplitBoundaryGroupLoads(loads, max_fanout) -> vector<vector<Pin*>>`（递归 biPartition，确定性）
- [ ] 合法性：kFanout / kPinCapLB 时若 N ≤ fanout² 走拆分试评估（per-子组质心锚电气评估 + 上游驱动 cap 按 k×最小 buffer 输入 cap 修正）；通过 → legal + `requires_split` + 统计；不可行才 monotone_hard_fail
- [ ] `required_leaf_load_cap_pf` 按拆分后口径；核查 `TopologyPruning.cc:542` 过滤链路一致

### 3. 嵌入
- [ ] 确认 max_fanout 与最小 buffer master 在嵌入上下文可得（`HTree::Config`/`DiagnosticBuild`，缺则从 synthesis_state 传入）
- [ ] `Embedding.cc`：net 创建点统一拦截——terminal_loads > max_fanout → SPLIT 实例化（sub-buffer 命名 `<prefix>_split_buf_N`，RecordInsertedInstLevel 用所在 level）；wire-through 向上传播前同样拆分
- [ ] debug 校验：嵌入完成后断言 inserted_nets 无 loads > max_fanout

### 4. 单测
- [ ] region 级：5/8/17-load 组 × fanout=4 → 拆分合法（k=2）/拆分合法（k=2）/hard-fail；子组划分确定性；cap 修正断言
- [ ] 嵌入级（HTreeTest 风格合成数据）：含 >fanout 边界组的小树 → 深度搜索 ≥2 feasible；嵌入后无超 fanout net；split buffer 计数与合法性统计一致
- [ ] `ctest -R "icts_test_flow_synthesis_htree|icts_test_flow_synthesis" --output-on-failure`

### 5. 回归 + vga_lcd 验证
- [ ] 全量 `ctest -R icts` 全绿
- [ ] vga_lcd 本地：Depth Candidate Summary ≥2 深度 feasible>0；选择与理由记录；选浅则全指标对比（path buffer 数/initial_skew/buffer/WL/runtime）；无 fanout>4 net
- [ ] 写 `research/validation-vga-lcd.md`

### 6. 收尾
- [ ] trellis-check → commit → journal → archive

## 跟进项（不阻塞本任务）
- 选型成本含 split 附加（若验证发现浅深度被系统性高估）
- N > fanout² 的多级局部拆分（当前明确 hard-fail）

## Review Gates
- Gate-1（步骤 1 后）：违例实证与设计预测一致性检查（不一致则回设计）
- Gate-2（步骤 5 后）：vga_lcd 行为对比评审

## Rollback
revert 单提交；无新配置键。
