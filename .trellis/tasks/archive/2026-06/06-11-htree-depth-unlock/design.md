# Design: P1-C 拓扑自由度解锁（深度搜索剪枝修复）

> 任务: 06-11-htree-depth-unlock · 2026-06-12
> 前置: P0-A（a9e8d750）+ P0-B（111ac5d6）已合入；基线 = /tmp/p0a_vga_validation/out_final（cap=6 默认，37.1s，initial_skew 0.0562）

## 1. R1 结论：单调剪枝分析（PRD 要求的正确性边界）

**机制链（已实证）**：深度候选 {12,11,10,9} 共享一个 `SinkLoadRegionLegalityContext`（`DepthPlan.cc:112`）。pattern 的 `bottom_most_buffered_level`（段级索引，跨深度统一语义：截断取前 d 级，索引 i 对应同一树段）决定边界层 = 节点层 i+1；边界组 = 该层节点的子树 loads（`TopologyGen.cc:413` 每节点携带全子树 loads——**R3 所疑"分组未反映该深度真实聚合"经核实不成立**，分组语义正确）。组违例 fanout（`SinkLoadRegion.cc:298`）或 pin-cap 下界（`:315`）→ `monotone_hard_fail` → `max_monotone_failed_level=L`，其后全部 bottom ≤ L 的 pattern 直接拒绝（`:383`）。

**单调性本身成立**：边界层每上移一层，组 loads 翻倍（父组 = 子组并集），fanout/cap-下界违例单调恶化；故"L 层组违例 ⇒ ≤L 层全违例"在**当前区域语义**（最后一级 buffer 直驱组内全部 loads，无局部补救）下是正确剪枝，**不是 bug**。

**真正的过约束在语义层**：vga_lcd 中 `max_leaf_load_count = max_fanout = 4`（`SynthesisState.cc:118`），biPartition 允许 level-11 节点持有最多 8 个 loads（2 叶×4）——**一个稠密区域的组 >4 就把 threshold 钉在 10，深度 11/10/9 的全部 pattern（其 bottom 上限分别为 10/9/8 ≤ 10）连电气评估都不做即死**。而对照系：被选中的 depth-12 解通过保留 120 个多负载叶 buffer（3976 个单负载被 prune）来处理同样的稠密区域——即**架构已接受"叶界局部多一级 buffer"的解，但深度搜索的合法性模型不会用它**。

## 2. 方案：边界组局部拆分（split-as-remediation）

把 fanout 违例从"一票否决"改为"局部拆分补救"，使合法性模型与嵌入实现共享同一确定性规则：

**规则 SPLIT(group)**：组 loads N > max_fanout 且 N ≤ max_fanout²：按几何位置递归二分（复用 `Clustering::biPartition`）至每子组 ≤ max_fanout，得 k = ⌈N/max_fanout⌉..≤max_fanout 个子组；每子组由一个**局部 sub-buffer**（最小表征 buffer，如 BUFX8H7L）在子组质心驱动；上游（末级段 buffer 或 wire-through 入口）驱动 k 个 sub-buffer 输入脚。N > max_fanout² 仍 hard-fail + 单调剪枝（两级以上局部拆分 ≈ 退化回更深的树，无意义）。

- **合法性侧**（`SinkLoadRegion.cc`）：kFanout 违例时执行 SPLIT 试评估——每子组以子组质心为锚做电气评估（cap/routing），全部通过 → 组合法，记 `requires_split`、`split_extra_buffers += k`；上游驱动 cap 需求改按 k×sub-buffer 输入 cap + 锚到质心连线评估（`required_leaf_load_cap_pf` 相应修正）。pin-cap 下界违例同路径（拆分后子组 cap 自然下降）。两类 monotone_hard_fail 仅在 SPLIT 也不可行时设置。
- **嵌入侧**（`Embedding.cc::BuildSegmentObjectsAndGetEntryLoads` 及 wire-through 聚合处）：创建终端 net 前若 terminal_loads > max_fanout → 按同一 SPLIT 规则实例化 k 个 sub-buffer（`CreateBufferInstance` + 子组 net），上游 net 连 k 个输入脚。无 buffer 的 wire-through 路径同样在向上传播 entry loads 前拆分（k 个输入脚向上传播）。
- **一致性**：两侧不传递 verdict，依赖同一规则 + 同一输入（loads、max_fanout、最小 master）推导一致结果；规则函数放 `region/` 共享。

**选型成本偏差（接受的限制）**：候选 char 不含 split 附加 buffer 的 delay/power——split 仅发生于离群组（vga 预计 <3% 组），选型偏差有限；DepthSummary 新增 `split_groups/split_extra_buffers` 列保证可追溯（验收"选择理由可追溯"）；嵌入后 FastSTA 在真实网表上裁决最终质量。若实测发现浅深度被系统性高估，把 split 计数折算进候选 power/delay 是后续小步（记录于 implement.md 跟进项）。

## 3. 观测性修复（实现第一步）

当前 Depth Summary 只显示下游的 `monotone_pruned_...`，**原始 hard-fail 的组信息从未上报**。补充：`SinkLoadRegionLegalitySummary` 已有 failure_reason（含 node/anchor/load_count）→ 把"首个 hard-fail 原因"记入 `DepthSummary`/`HTree Depth Candidate Summary` 表（新列 First Hard Fail），并在设置 `max_monotone_failed_level` 时 LOG 一次。先以此跑 vga_lcd 实证违例类型（预测：fanout，load_count 5~8，boundary level 11），再进入拆分实现。

## 4. 改动点

| 文件 | 改动 |
|---|---|
| `region/SinkLoadRegion.{hh,cc}` | SPLIT 规则函数（共享）；kFanout/kPinCapLB 的拆分试评估路径；Summary 增 `requires_split/split_group_count/split_extra_buffers`；首个 hard-fail 原因透出 |
| `plan/DepthPlan.hh` + `topology_pruning/TopologyPruning.cc` | DepthSummary 增 split 统计与 first_hard_fail 字段，填充链路 |
| `plan/DepthPlanReport.cc` | Depth Candidate Summary 表新增列 |
| `embedding/Embedding.cc` | 终端 net 创建与 wire-through 聚合处的 SPLIT 实例化；sub-buffer 命名（`*_split_buf_*`）与 level 记录 |
| `synthesis_state/SynthesisState.cc` 或 HTree config | 把 max_fanout / 最小 buffer master 传入嵌入上下文（核实现有字段，缺则补） |
| 测试 | SinkLoadRegion 单测（fanout 违例→拆分合法/超 fanout² 仍拒/cap 评估）；HTreeTest 级或 region 级"深度搜索 ≥2 可行深度"用例（合成 5-load 边界组） |

## 5. 风险

- 风险 A：拆分嵌入与合法性推导不一致（不同子组划分）→ 共享同一函数 + 单测双侧一致性。
- 风险 B：浅深度被选但 FastSTA 实测变差 → 验收允许两种结局（含量化解释）；DepthSummary 透明化；不回退 depth-12 可选性（R4：候选集仍含 12）。
- 风险 C：split 改变 boundary cap 需求计算 → `required_leaf_load_cap_covering_idx` 链路（TopologyPruning.cc:542 过滤）按拆分后口径，新单测覆盖。
- 风险 D：wire-through 多级聚合下 split 触发点遗漏 → 以"net 创建点统一拦截"为不变式（所有 >fanout 的 net 都经 SPLIT），嵌入后断言无 net fanout > max_fanout（debug 校验 + 测试）。

## 6. 验证（vga_lcd 本地管线）

1. 观测性版本：确认原始违例类型/组规模（§3）。
2. 完整版本：Depth Candidate Summary ≥2 个深度 feasible>0；选中深度与理由（delay/power 对比）可见；若选浅深度：`clock_path_max_buffer`、initial_skew、buffer 数、WL、运行时间全对比；嵌入后无 net fanout >4（含 split buffer 网）。
3. 回归：全量 ctest + 既有 vga 指标不超预期漂移（depth-12 仍被选时应与基线几乎一致）。

## 7. 回滚

单 commit；SPLIT 路径由 `max_fanout` 既有配置驱动、无新配置键；revert 即回到一票否决行为。
