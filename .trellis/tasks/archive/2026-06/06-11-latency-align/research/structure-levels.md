# Research: P1-E 级数结构与选型机制(structure-levels)

- **Query**: 为「Latency 对齐:插入延迟缩减」design.md 收集代码事实(trunk/root/cluster/pattern 跳层/选型/顶层重载段/配置旋钮/测试地标)
- **Scope**: internal(src/operation/iCTS/source,分支 cts_refactor)
- **Date**: 2026-06-13
- 所有路径相对 `src/operation/iCTS/source/`(除特别注明)。行号基于当前 cts_refactor 工作区。

---

## 已确认结构(主会话已验证,不再重查)

vga_lcd(depth-9 选型)每条 root→sink 路径恰好 10 级 buffer:

1. `source_to_root_top_segment_buf_0/1`(BUFX12 ×2,source trunk 段内 2 个)
2. `regular_root_buf`(BUFX20,每 sink-domain 固定插入:`flow/synthesis/realization/ClockTreeRealization.cc:328-363`,resolveMinimumDriveRootBuffer 先选最小驱动、后由 RootDriverCompensation/root-driver sizing 换型为 BUFX20)
3. `htree_edge_buf` ×5(depth 9 中只有 5 层有 buffer,4 层跳过)
4. `htree_split_buf` ×1(P1-C 边界组 split)
5. `cluster_buf` ×1(每 cluster ~4 sinks,共 4226 cluster)

Innovus 同设计 7 级(全程 fanout≈4)。trunk 选型(`SourceTrunkSegment.cc:235-246` SelectBestSegmentEntry)与 htree 全局选型(`TopologyPruning.cc:496-514` SelectBestHTreeChar)都是 BuildDelayPowerParetoFront → 中位数。

补充修正(本次研究确认):htree 的**最终全局赢家**不是 `SelectBestHTreeChar`,而是跨 depth 的 `SelectBestGlobalEntry`(`topology_pruning/TopologyPruningGlobalSelection.cc:177-190`,同样 Pareto-中位),由 `solution/discrete/DiscreteSolution.cc:154-160` 调用;`SelectBestHTreeChar` 只产生 per-depth 的 `evaluation.best_char`(用于 DepthSummary 报表与 success 判定,最终在 `DiscreteSolution.cc:231` 被全局赢家覆盖)。

### 流程总览(级数从哪里来)

| 级 | 创建点 | 时机 |
|---|---|---|
| root_buf | `ClockDistribution::prepare` → `addRootBufferForSinkDomain`(`distribution/ClockDistribution.cc:40-61,95-105`) | synthesis 最早,先于一切树构建(`Synthesis.cc:191-214,269-274`) |
| cluster_buf | `PrepareSinkTreeLoads` → `buildClusterBufferObjects`(`topology/sink/SinkLoadClustering.cc:203-237,160-199`) | htree 之前;htree 的 loads = cluster_buf 输入 pin(`topology/sink/SinkBranch.cc:191-220`) |
| htree_edge_buf / htree_split_buf | `BuildEmbedding`(`htree/embedding/Embedding.cc:509-607`),命名 `EmbeddingState.hh:48-58` | htree 选型后嵌入 |
| top_segment_buf | `SourceTrunkSegment::build` → `BuildSourceTrunkSegmentObjects`(`topology/trunk/SourceTrunkSegment.cc:268-308,399-592`) | 所有 sink-domain 构建完成后(`topology/Topology.cc:171-181,268-368`) |

---

## Q1 Trunk 段结构

**候选里含 0/1-buffer 变体吗?——含。**

- 表征枚举层面:`module/characterization/pattern/CharPatternEnumerator.cc:94-104` 对每个长度的 N 个 slot 枚举**全部 2^N 个 bitmask**,bits=0(纯导线、0 buffer)在 `enumerateTopology` 中以空 master 列表表征(`CharPatternEnumerator.cc:112-115`);≥1 buffer 时按 sorted_buffers 的单调组合枚举 master(`:123-135`)。`BufferingPattern::isWirePattern()`(`database/characterization/BufferingPattern.hh:115`)即 0-buffer 形态。
- 构建层面:`BuildSourceTrunkSegmentObjects` 有 buffer_count==0 直连分支(`SourceTrunkSegment.cc:273-277`),`distance==0` 时也直连(`:417-423`)。
- 但 0-buffer 样本要进入 frontier,必须在 FastSTA 采样时 slew/cap 不溢出格点:输出 slew 超 `max_slew` 或 driven cap 超 `max_cap` 的样本被丢弃(`module/characterization/sampling/CharStaSampler.cc:206-211,139-142`)。

**2-buffer 是 Pareto 中位结果还是可行性过滤后的最少?——机制上是「先硬过滤、后中位」,两者都参与:**

1. 过滤(`FilterSegmentEntries`,`SourceTrunkSegment.cc:248-266`):
   - `entry.load_cap_idx >= required_load_cap_idx` — 段末端可驱动 root_buf 输入 cap;`required_load_cap_pf = wrapper.queryPinCapacitance(root_input)`(`topology/trunk/SourceTrunk.cc:126`);
   - `entry.driven_cap_idx <= source_drive_cap_idx` — 段呈现给时钟源的 cap ≤ 源驱动上限;`source_drive_cap_pf = queryClockSourceDriveCapLimit`(`SourceTrunk.cc:58-63,127`;实现 `database/io/WrapperLiberty.cc:498-541`:源 inst 的 lib 输出 cap limit → 表轴 max → 配置 max_cap(IO port 走配置));
   - `entry.input_slew_idx >= min_input_slew_idx`(软边界,来自 config `root_input_slew`,`SourceTrunk.cc:83-88`;不满足时整体放宽重试,`SourceTrunkSegment.cc:540-549`)。
2. 选择(`SelectBestSegmentEntry`,`SourceTrunkSegment.cc:235-246`):对过滤后条目建 delay-power Pareto front(`:211-233`),按 `PreferSegmentEntry`(power 升序优先,`:188-209`)排序后取 `(size-1)/2` 中位。

因此 2-buffer **不是被迫的最少可行级数**(若 0/1-buffer 被过滤淘汰,中位自然落在更多 buffer 上;若它们幸存,中位策略也不会选它们——它们在 front 的低功耗端)。静态无法区分这两种情形,需跑一次并查 `strict_candidate_count` 与 frontier 内容(开放问题 O1)。

**frontier 里 delay 最小的 pattern 是几 buffer?——机制层回答:** Pareto front 按 power 升序排序后,非支配性保证 delay 严格降序,所以 **front 末位 = 最小 delay、最大 power** 的条目;其 buffer 数取决于数据(长 RC 线上通常是 buffer 较多/较强的 pattern)。中位策略系统性放弃了 front 后半段的低 delay 解。

**0/1-buffer 可行性如何被两个 cap 边界钳制:**
- 0-buffer:driven_cap = 整段线 cap + root_buf 输入 cap,需 ≤ 源驱动上限,且段输出 slew(纯线衰减)样本不溢出;长 trunk 通常死在 driven_cap 或输出 slew 溢出。
- 1-buffer:driven_cap = buffer 之前线段 cap + buffer 输入 cap;buffer 越靠近源,driven_cap 越小但末端线越长(load_cap/输出 slew 压力越大)。
- 边界索引解析:`CoveringBoundaryIndex`(`htree/constraint/Constraint.cc:33-42`,超 lattice 上限返回 steps+1 ⇒ 必不可行);trunk 入口处还有 length/cap 边界硬校验(`SourceTrunkSegment.cc:475-493`)。
- trunk 的长度直接表征:trunk 长度作为 `additional_characterization_lengths_um` 并入 htree 共享表征(`Topology.cc:127-149,174,237`;`htree/characterization/Characterization.cc:97-98`);若库未 ready 则单独建库且 requested_lengths 只有 trunk 长度本身(`SourceTrunkSegment.cc:380-395,445-467`)。

## Q2 Root buffer 是否可省/可合

**结构性依赖清单(去掉/合并 root_buf 会破的点):**

1. `ClockDistribution::prepare` 把 root_buf 插入视为硬步骤,失败即 domain 失败(`ClockDistribution.cc:95-105`);root_buf 位置 = sink 质心(`ClockTreeRealization.cc:180-205` resolveSinkDomainLocation)。
2. root_buf 的 `root_input` pin 是 trunk 的终点:`collectRootInputs`(`Topology.cc:115-125`)→ `BuildSourceTrunkTree` 要求非空(`SourceTrunk.cc:210-221` "empty_root_inputs");1 个 root_input 走 segment、多个走 top htree(`SourceTrunk.cc:235-258`)。
3. root_buf 的 `root_output` 驱动 `downstream_net`,它是 sink-htree 的 root_net:HTree 要求 root_net 有 driver pin(`htree/HTree.cc:86-90` "missing_root_driver_pin");`root_inst` 从 driver pin 反查(`htree/synthesis_state/SynthesisState.cc:54-63`)。
4. RootDriverCompensation 默认 master = root_inst 当前 master(`SynthesisState.cc:196`);compensation 输入构造也依赖 root_inst 存在。
5. Root driver sizing(BUFX20 换型)直接改写 root_inst:`ValidateRootDriverSizing`/`ApplyRootDriverSizing`(`htree/embedding/Embedding.cc:446-507`)要求 root driver inst 暴露完整 buffer pin 对并重命名 in/out pin、`set_cell_master`(`:498`)。**若 trunk 末 buffer 兼任 root**:该 inst 是 CTS 在 trunk 阶段后插入的(trunk 在 sink-htree 之后构建,`Topology.cc:175-180` 先 domain 后 trunk),时序上 sizing 发生在 sink-htree finalize 时(`solution/finalization/SolutionFinalizer.cc:125-149`),那时 trunk buffer 尚不存在 ⇒ 顺序矛盾,需要重排或把 sizing 后移。
6. 选型侧:`ResolveSelectedRootDriverCellMaster` 取「第一个有 buffer 的 level 的末位 master」(`solution/selection/SolutionSelection.cc:46-57`);compensation 侧同逻辑 `ResolveRootDriverCellMaster`(`htree/compensation/RootDriverCompensation.cc:71-84`)。root master 的自由度被绑定到第一 buffered level 的选择,不是独立搜索。
7. `enable_root_driver_sizing`:sink-htree=true(`topology/sink/SinkBranch.cc:128`)、top-trunk-htree=false(`SourceTrunk.cc:172`);它同时驱动 strict_root_boundary_closure(`SynthesisState.cc:160`)→ compensation 严格边界过滤(`RootDriverCompensation.cc:364-412`)以及 pattern search 阶段丢弃 top_input_slew 约束(`Constraint.cc:64-72`)。
8. 记账类:`ClockNetwork::recordDomainRoot` 存 root_buffer 指针(`database/design/ClockNetwork.cc:67-70`,当前 source 内无其他调用方);ClockLayout phase 名 "root_buffer"(`database/design/ClockLayout.cc:145`);测试 fixture 直接构造 root buffer(`test/flow/FlowDesignFixture.hh:256,283`)。
9. DEF 写回无 root_buf 特例:root_buf 经 `design.makeInst` 直接落在 Design(`ClockTreeRealization.cc:228`),htree/trunk 对象走通用 `commitInsertedObjects`(`ClockTreeRealization.cc:443-547`);写回链路按 Design 通用遍历,无名称特判。

**ResolveRootDriverCellMaster 依赖的输入**(`RootDriverCompensation.cc:71-84,175-203`):topology pattern_id → materialize 出逐层 segment pattern → 第一个非空 cell_masters 的层取 `cell_masters.back()`;载荷估计 `QueryRootClosureLoadEstimate`(见 Q6);查价 `wrapper.queryRootDriverCostDirect(cell_master, input_slew, load_cap, period)`(`RootDriverCompensation.cc:141-142`);input_slew = config `root_input_slew` 或 max_slew/2(`:227-233`)。compensation 仅把 root cell 的 delay/power 加到候选上(`HTreeTopologyChar::set_root_driver_compensation`,`database/characterization/HTreeTopologyChar.hh:65-69`),不改变结构。

## Q3 Cluster 层与 htree 叶

**cluster_buf 创建**:`topology/sink/SinkLoadClustering.cc`
- 聚类:`Clustering::defaultFastClustering(root_loads, config)`(`:221`);config 来自 `FastClustering::buildElectricalBaseConfig(max_fanout, max_cap)`(`module/topology/fast_clustering/FastClustering.cc:128-135`:max_fanout=config.max_fanout,max_diameter=INT_MAX)+ 每 pin 实测 cap(`SinkLoadClustering.cc:138-158`)。
- 成员数策略:递归空间二分 `BuildSpatialRecursiveClusters` + `PolishSmallClusters` + `FinalizeClusters`(`FastClustering.cc:142-192`),直到 fanout≤max_fanout 且 cap 合法 ⇒ vga_lcd 的 ~4 sinks/cluster 直接由 `max_fanout`(vga_lcd 配置 4;Config 默认 32,`database/config/Config.hh:61,184`)决定。
- buffer master:`resolveMinLegalClusterBufferCell` 取 buffer_types 中**最小驱动**的合法 buffer(`SinkLoadClustering.cc:102-136`)。
- 对象:`cluster_buf_<i>` 在 cluster 质心,`cluster_sink_net_<i>` 连 sinks;htree_sinks = cluster_buf 输入 pin(`SinkLoadClustering.cc:160-199`)。

**htree 叶 → cluster_buf 的连接**:cluster_buf 输入 pin 作为 root_net loads 进入 `HTree::build`(`SinkBranch.cc:212-229`,LoadRole::kLocalBuffer `SinkBranch.cc:115`);TopologyGen 按二分把这些 pin 分到叶组(`SynthesisState.cc:116-132`,max_leaf_load_count=max_fanout)。嵌入时:
- 每层每边按 segment pattern 建 buffer 链,末位 buffer 的 net 接 `MaterializeSplitSubBuffers` 处理后的 terminal loads(`Embedding.cc:378-424`,split 注入点 `:421`;root 组超扇出时 `:603` 也会 split)。
- 0-buffer 层直接把 child entry loads 向上传(`Embedding.cc:396-399`)。
- `PruneLeafSingleLoadBuffers`(`Embedding.cc:283-342`):输出 net 仅 1 个**外部**负载(如单个 cluster_buf)的 inserted buffer 被绕接删除——这就是 PRD 所说"leaf buffer 被 prune 后 htree 末级直驱 cluster"。

**"叶组 fanout 约束"评估点(SinkLoadRegion)**:`htree/region/SinkLoadRegion.cc`
- 边界层 = bottom-most buffered level + 1(`ResolveBottomMostBufferedLevel:127-140`,`CollectSinkLoadRegionBoundaryGroups:197-273`,锚点=末 buffer 位置插值 `:240-262`)。
- 组内 loads > max_fanout ⇒ `SplitSinkLoadRegionGroup`(`:431-519`):仅当 `loads ≤ max_fanout²` 可行(`:434`),子组数 ≤ max_fanout(`:500-503`);split 后上游 cap = 子 buffer 输入 cap ×子组数 + 锚点到子组质心的线 cap(`:397-414`)。split buffer = 表征 buffer 列表中最小驱动单元(`SynthesisState.cc:214-220`)。
- fanout/cap 硬违例触发单调剪枝(bottom_most_buffered_level 阈值,`:521-553`)。
- P1-C 现状成因:4226 cluster / 512 叶(depth 9)≈ 8.25 > 4 ⇒ 每叶组 split 成 2-3 子组(+1 级);depth 10(1024 叶,≈4.1)在候选窗口内(见 Q7 depth window),split 几乎消失但 htree 多一层——它输掉的是全局 Pareto 中位选择,不是可行性。

**若让 cluster_buf 直接当 htree 叶 buffer,需要动的点**:
- `SinkBranch.cc:191-229`:cluster_buf 预创建与 htree 解耦——要么取消预创建、把 sinks 直接交给 htree 并让叶层 segment pattern 的末 buffer 落在 cluster 质心(嵌入侧 `Embedding.cc:407-408` 的位置插值逻辑需改为"叶组质心");要么保留 cluster_buf 但禁止叶层再插 buffer + split(等价于把叶组 fanout 约束从「htree net 扇出」转移到「cluster_buf 即子组 buffer」)。
- 合法性侧要同步:`SinkLoadRegion` 的边界组评估假设边界 loads 是"外部负载 pin"(cluster_buf 输入),split 决策与嵌入端 `MaterializeSplitSubBuffers` 用同一确定性函数镜像(注释 `Embedding.cc:344-347`)——两处必须一起改。
- `PruneLeafSingleLoadBuffers` 的"外部负载"判定(`Embedding.cc:303-305`,downstream inst 不在 inserted set)会把任何单负载叶 buffer 删掉,合并方案可借用该机制反向工作(让 cluster_buf 成为 inserted 对象的一部分则 prune 不触发)。

## Q4 Pattern 跳层机制

**枚举**:每层独立持有自己长度 bin 的 segment frontier;层方案 = 该长度的全部 2^N slot-bitmask × buffer master 单调组合(`CharPatternEnumerator.cc:79-135`;slot=长度格点,buffer 位置=slot 边界,`CharTopologyPlanner.cc:56-80`)。bits=0 即「该层无 buffer」。htree 自底向上逐层 hash-join 组合(`TopologyPruning.cc:304-402` BuildPatternSearch;join 条件 `module/characterization/pruning/HTreeTraits.hh:42-71`:上游 output_slew == 下游 input_slew 且 `ceil(load_cap/2) == 下游 driven_cap`——二叉半容差换算)。state-key 维度上保留多条 frontier(`TopologyPruning.cc:206-290`)。

**跳层可行域的限制**:
1. **slew 衰减**:无 buffer 层是纯线 segment char,其 in/out slew 必须落在 slew lattice 内;表征时输出 slew 溢出样本直接丢弃(`CharStaSampler.cc:206-211`)⇒ 连续跳层在 join 链上找不到 slew 匹配条目即断。
2. **cap 上界**:无 buffer 层 driven_cap = 线 cap + 2×下游 driven_cap(经 halfCapKey 反推),溢出 cap lattice 的样本被丢(`CharStaSampler.cc:139-142,211`);`CoveringBoundaryIndex` 超界返回 steps+1 必不可行(`Constraint.cc:38-40`)。
3. **顶部 fanout 合法性**:无 buffer 层使 source_exposed_load_count 翻倍;组合时要求 `下游 count×2 ≤ max_fanout`(`htree/segment_pruning/TopologyPatternLibrary.hh:207-216,262-277`),根处再过滤一次(`TopologyPruning.cc:182-204`)。max_fanout=4 ⇒ 根下最多 2 层连续无 buffer(count 1→2→4=越界前一步)。
4. **底部 sink-load region**:bottom-most buffered level 越浅,边界组 loads 指数增大,split 仅救得了 ≤ max_fanout² 的组(`SinkLoadRegion.cc:434`)——这是 depth-9 下底部至少要 buffer 到第 9 层附近的硬原因。
5. **表征长度上限**:auto 网格下 unit = max_level_len/层数,直接表征 bin 数 = min(required_covering, auto_direct_bins_cap=6)(`htree/characterization/wirelength/WirelengthGrid.cc:162-206`;Config 默认 6,`Config.hh:67,180`);超出 cap 的长 bin 由 SegmentPruning 的 DP 闭包合成(`htree/segment_pruning/SegmentPruning.cc:300-393` SolveRequiredLengthState:目标长 = 左右子长组合)——这就是 R2「量化截断→DP 组合插中间 buffer」的机制位置。**注意:跳层不会拉长任何单层 bin(层长由拓扑几何固定,`htree/plan/Plan.cc:56-108`),所以跳层上限主要是电气项 (1)(2)(3)(4),而不是表征覆盖;表征覆盖影响的是单层内 pattern 的可选形态。**

**理论上还能更少吗**:5 buffered levels 是全局中位选择的结果而非可行域边界。可行域内更少 buffer 层的 pattern 若存在,会出现在 frontier 低功耗端(无 buffer⇒少 cell power),但 delay 通常更差,中位策略不偏好任何一端;真正的机制下限由 (1)-(4) 给出,静态无法给数值,需查 depth 候选的 frontier 分布(开放问题 O2)。

## Q5 选型数据结构与影响面

**SegmentChar**(`database/characterization/SegmentChar.hh:40-92`):CharCore{input_slew_idx, output_slew_idx, driven_cap_idx, load_cap_idx, delay, power, pattern_id, source_boundary_net_switch_power} + length_idx。**无 buffer 数字段**。

**HTreeTopologyChar**(`database/characterization/HTreeTopologyChar.hh:43-122`):CharCore + `levels`(组合层数=深度,**不是 buffered 层数**)+ root_driver_delay/power(compensation 注入,get_delay/get_power 已含,raw_* 不含);`leaf_load_cap_idx == load_cap_idx`(`:63`)。

**buffer 级数在选型时可得吗?——可得,需一次 materialize:** `topology_library.materialize(pattern_id)`(`TopologyPatternLibrary.hh:114-143`)→ 逐层 segment pattern_id → `segment_pattern_library.find(id)->get_buffer_positions().empty()` 判 buffered;`ResolveBottomMostBufferedLevel`(`SinkLoadRegion.cc:127-140`)与 `ResolveRootDriverCellMaster`(`RootDriverCompensation.cc:71-84`)已是现成范式。materialize 是 O(levels) 栈展开,选型处批量调用有成本,可学 SinkLoadRegion 的 signature 缓存(`SinkLoadRegion.cc:539-552`)。另:`ApplySelectedPatternToLevelPlans` 在 finalize 时已统计 per-level `selected_buffer_count`(`SolutionSelection.cc:91-116`),但那在选型之后。

**两个选择函数的调用面**:
- `SelectBestSegmentEntry`:仅 `SourceTrunkSegment.cc:539,544`(strict/relaxed 两次)。改它只影响 trunk。
- `SelectBestHTreeChar`:仅 `TopologyPruning.cc:639,641`(per-depth feasible/relaxed)。其结果决定 per-depth `evaluation.success` 与 DepthSummary 报表,但**不决定最终结构**。
- 最终结构由 `SelectBestGlobalEntry`(`TopologyPruningGlobalSelection.cc:177-190`)决定,调用点仅 `DiscreteSolution.cc:155,160`(analytical 引擎走 `solution/analytical/AnalyticalSolution.cc`,默认关闭:Config `enable_analytical_htree=false`)。喂给它的池子先经 `BuildPerDepthDelayPowerParetoRefs`(per-depth 局部 Pareto,`TopologyPruningGlobalSelection.cc:151-175`)与 sink-load coverage 过滤(`DiscreteSolution.cc:115-141`)。
- **测试镜像**:test fixture 有一份独立实现 `realtech_fixture::SelectBestHTreeChar`(`test/module/characterization/fixture/CharacterizationRealTechFrontierBuilder.cc:501`),被 CharacterizationRealTechSmoke/ExactRegression 用于钉值;改生产策略不会自动改它,行为分叉需留意。
- 若把 min-delay/级数约束接入:改 `SelectBestGlobalEntry` 影响 sink-htree 与 top-htree 全部最终选择(单点);改 `SelectBestHTreeChar` 影响 depth summary 与 relaxed 路径;改 `SelectBestSegmentEntry` 影响 trunk。三处 + `PreferPowerMedianOrder`/`PreferSegmentEntry` 的排序键是全部策略面。`HTreeTest.GlobalSelectionPreservesDelayPowerTieMultiplicity` / `PerDepthParetoCompressionKeepsDepthGroupsIndependent`(`test/flow/synthesis/htree/HTreeTest.cc:285,313`)钉住现 Pareto 语义。

## Q6 R3 顶层重载段(root_buf → 第一个 htree_edge_buf)

**长度/负载由什么决定**:
- root_buf 物理位置 = sink 质心(`ClockTreeRealization.cc:180-205`);sink-htree 的拓扑根节点位置由 TopologyGen 二分自定(`fixed_topology_root_location=nullopt`,`SinkBranch.cc:110`;top-htree 才固定到时钟源,`SourceTrunk.cc:154`)。
- 第一级 buffer 位置 = pattern `positions.front()` 在「父节点→子节点」连线上的 Manhattan 插值(`Embedding.cc:407-408`;compensation 侧同式 `RootDriverCompensationLoad.cc:205-220`)。该段实际负载 = FLUTE Steiner(root 位置 → 各第一 buffer 输入)线 cap + buffer 输入 pin cap(`RootDriverCompensationLoad.cc:151-203,295-319`);若第一 buffered level 之上还有无 buffer 层,terminal 集合按层展开直至首个 buffered 层(`:321-398`)。
- 选型时延里,level-1 线延迟在该层 segment char 内(以表征驱动器测得);root cell 延迟由 compensation 以上述物理负载直查(`queryRootDriverCostDirect`,`RootDriverCompensation.cc:139-148`)叠加(`:404`)。0.105ns 实测 stage = root cell delay + level-1 线到首 buffer。

**补偿自由度**:只换 master(且 master 被钉为第一 buffered level 的末位 master,见 Q2-6),输入 slew 可配(`root_input_slew` / max_slew·0.5,`RootDriverCompensation.cc:227-233`)。**没有**:root 重新摆位、顶层段中插 buffer、独立 root master 搜索。strict 模式下还会按物理 cap/slew bucket 闭合性筛掉候选(`RootDriverCompensation.cc:205-223,364-412`)。

## Q7 配置旋钮清单(影响级数的)

| 旋钮 | 位置 | 机制一句话 |
|---|---|---|
| `htree_depth_explore_window`(默认 4) | `Config.hh:75,108,149,189`;消费 `Plan.cc:142-162` | 探索 depth ∈ [max_depth-window+1, max_depth];max_depth=TopologyGen 二分到叶组≤max_fanout 的层数(`SynthesisState.cc:116-133,172-173`) |
| `target_depth` | `HTree.hh` Config(`:89`);`Plan.cc:148-151` | 钉死单一 depth(仅 caller 级,Config JSON 无此项) |
| `force_branch_buffer`(默认 false) | `Config.hh:74,107,148`;消费 `TopologyPruning.cc:138-144`、`SegmentPruning.cc:440-448` | 每层只允许 kTerminalBranchBuffered frontier(段末端有 buffer 的 pattern,`CharTopologyPlanner.cc:39-52`)⇒ 禁止跳层,级数=深度 |
| `enable_sink_clustering`(默认 true) | `Config.hh:78,111,152`;`SinkBranch.cc:173`、`Topology.cc:243-245` | 关掉则 sinks 直接做 htree 负载(少 cluster 一级,但叶组=真实 sinks,fanout/cap 压力转给 htree 叶) |
| `max_fanout`(默认 32;vga_lcd 4) | `Config.hh:61,103,144` | 同时控制:cluster 大小(SinkLoadClustering)、TopologyGen 叶组、组合 fanout 合法(TopologyPatternLibrary)、叶组 split 可行域(≤max_fanout²) |
| `root_input_slew`(默认 0=off) | `Config.hh:56,91,130`;trunk `SourceTrunk.cc:83-88`、htree `min_top_input_slew_ns`(`SinkBranch.cc:68-71`) | 顶部输入 slew 软边界;htree 侧在 root sizing 开启时改由 compensation 闭合(`Constraint.cc:64-72`) |
| `auto_direct_bins_cap`(默认 6) | `Config.hh:67,99,140,180`;`WirelengthGrid.cc:203-204` | auto 网格直接表征 bin 上限(2^bins pattern 枚举界);超出走 DP 组合 |
| `wirelength_unit_um`/`wirelength_iterations` | `Config.hh:65-66` | 显式网格;缺省/塌缩时 auto 派生(`WirelengthGrid.cc:171-192`) |
| `max_cap`/`max_buf_tran`(slew) | `Config.hh:57-60` | 表征格点上限=溢出剪枝阈值,直接决定跳层可行域 |
| `htree_topology_tolerance`(默认 0.1) | `Config.hh:76,109,150`;`SynthesisState.cc:117` | TopologyGen 二分平衡容差,影响层长均匀性 |
| `enable_analytical_htree`(默认 false) | `Config.hh:77,110,151`;`Solution.cc:35-42` | 切换 analytical 选型引擎 |
| `allow_boundary_relaxation` | `HTree.hh` Config(仅 caller,两处均 false:`SourceTrunk.cc:173`、`SinkBranch.cc:129`) | 允许无严格可行解时取放宽解 |

## Q8 测试地标(防盲改)

| 测试 | 文件 | 钉住什么 |
|---|---|---|
| SinkLoadRegionSplitTest(6 例) | `test/flow/synthesis/htree/SinkLoadRegionSplitTest.cc:67-149` | split 子组划分确定性:5 loads→2 组、fanout 内不拆、>max_fanout² 不可行、16 loads 四分、输入序无关、二分可能超子组预算 —— 改叶组/cluster 合并必碰 |
| HTreeRealTechSmokeTest.SynthesizesMaterializedHTreeFromRealClockLoads | `HTreeRealTechSmokeTest.cc:57` | 真实工艺端到端 htree:selected_depth==levels、best_char 存在、单负载叶 buffer 必须被 prune(`TopologyRealTechHTreeAssertions.cc:81-96`) |
| HTreeRealTechBranchBufferRegressionTest(3 例) | `HTreeRealTechBranchBufferRegressionTest.cc:53-144` | force_branch_buffer 下每层必须 terminal-branch pattern;caller 覆盖 config;top 边界 slew 传播 |
| HTreeRealTechMatrixTest(Arm9 矩阵 ×2) | `HTreeRealTechMatrixTest.cc:48,53` | 多场景矩阵 + auto 网格变体回归 |
| HTreeTest(8 例) | `HTreeTest.cc:140-313` | 必需 frontier kinds、空负载/单负载平凡路径、**全局选择的 tie 多重性与 per-depth Pareto 独立性**(285,313)—— 改选型函数必碰 |
| WirelengthGridTest(7 例) | `WirelengthGridTest.cc:47-136` | auto 网格 cap 行为(默认 6、超 cap 转 dense、配置网格不动)—— P0-B 行为锚点 |
| TopologyRealTechSmokeTest(2 例) | `TopologyRealTechSmokeTest.cc:53,139` | 聚类模式建质心 buffer + 非受限 frontier;聚类+force_branch 组合 |
| TopologyTest(SourceTrunk 相关) | `TopologyTest.cc:420,455,489` | trunk 空 roots 失败不污染 source net、同位置直连不插对象、IO 源驱动上限取 runtime max_cap |
| SegmentJoinTest / HTreeJoinTest / PrunerTest | `test/module/characterization/` | SegmentChar/HTreeTopologyChar 组合与 Pareto 剪枝语义 |
| CharacterizationRealTech*(Exact/Compose/Function) | 同上目录 | 表征精度与 DP 组合 gap;fixture 内有 SelectBestHTreeChar 独立副本(`fixture/CharacterizationRealTechFrontierBuilder.cc:501`) |
| FlowTest / FlowDesignFixture | `test/flow/FlowTest.cc:362-425`、`FlowDesignFixture.hh:256,283` | ClockDistribution.prepare / root buffer 显式构造 —— 动 root_buf 结构必碰 |
| TopologyGenDepthTest | `test/module/topology/topology_gen/TopologyGenDepthTest.cc` | 拓扑深度生成(max_depth 来源) |

## P1-E 可行手段映射(PRD R1 候选 → 改动点)

1. **root/source-trunk 冗余级合并**(0.076+0.105 首延迟)
   - 改动点:`ClockDistribution.cc:40-61`(root_buf 可选化/由 trunk 末 buffer 兼任)、`SourceTrunk.cc:126-127`(required_load_cap 改为第一 htree 级输入 cap)、`SolutionFinalizer.cc:125-149` + `Embedding.cc:446-507`(root sizing 的对象与时机)、`SynthesisState.cc:196`(compensation 默认 master)。
   - 风险一句话:root_buf 是 domain 准备阶段的结构锚点(trunk 终点 + htree root driver + sizing 对象),合并需重排「domain 准备→sink 树→trunk」的构建顺序,FlowTest/FlowDesignFixture 与 ClockNetwork 记账全要跟。
2. **htree 末级与 cluster 层合并**
   - 改动点:`SinkBranch.cc:191-229`(cluster 预创建解耦)、`SinkLoadClustering.cc:160-199`(cluster buffer 选型/位置)、`Embedding.cc:344-424`(split/末级嵌入)、`SinkLoadRegion.cc:275-427`(边界组合法性口径)。
   - 风险一句话:legality 与 embedding 是「同函数镜像」契约(`Embedding.cc:344-347` 注释),只改一侧会让选型可行性与落地结构脱钩;SinkLoadRegionSplitTest/TopologyRealTechCluster* 钉死现行为。
3. **更激进跳层(偏好少 buffer 级 pattern)**
   - 改动点:可行域本身在表征(`CharStaSampler.cc` 溢出剪枝)与组合约束(`TopologyPatternLibrary.hh:207-216`、`SinkLoadRegion.cc:434`),策略在 `SelectBestGlobalEntry`/`PreferPowerMedianOrder`;若靠放宽 max_slew/max_cap 或 slew/cap steps 扩格点,改 Config 即可但表征成本与 lattice 精度互斥。
   - 风险一句话:跳层下限大概率卡在底部 sink-load fanout(max_fanout² split 界)与顶部 fanout 翻倍约束,纯选型偏好改不动这两个硬墙。
4. **选型目标加入 latency 项/约束**
   - 改动点:`TopologyPruningGlobalSelection.cc:177-190`(全局)、`TopologyPruning.cc:496-514,639-641`(per-depth)、`SourceTrunkSegment.cc:235-246`(trunk);buffered-level-count 可经 materialize+缓存获得(Q5)。
   - 风险一句话:三处策略 + 测试 fixture 副本(`CharacterizationRealTechFrontierBuilder.cc:501`)与 `HTreeTest.cc:285,313` 的 tie/独立性约定需同步,否则回归红与生产行为分叉。
5. **R3 顶层重载段专项**
   - 改动点:root 摆位(`ClockTreeRealization.cc:180-205`)、compensation 的 master 解析(`RootDriverCompensation.cc:71-84` 解除与第一 buffered level 绑定)、或在 root_net 上引入分段(目前 `Embedding.cc:604` 直接 ConnectNet root_output→第一级 entry loads,无中继概念)。
   - 风险一句话:compensation 的 strict 边界闭合(cap/slew bucket 匹配)以 root master 与 level-1 buffer 输入为锚,独立换 root master 后 `ResolveSelectedRootDriverCellMaster` 与 compensation 两处口径要保持一致,否则选型时延与落地时延脱节。

## 开放问题

- **O1**:vga_lcd trunk frontier 中 0/1-buffer 条目是否在 strict 过滤后幸存(决定「合并/直驱」是选型问题还是可行性问题)——跑一次并查 `Source Trunk Summary` 的 strict_candidate_count 及 frontier 明细即可。
- **O2**:depth 9/10 候选的 feasible frontier 中最少 buffered-level 数与对应 delay/power——决定「更激进跳层」无需改可行域就能拿到多少级。
- **O3**:depth 10 在全局选择中输给 depth 9 的差距量级(global pool 中两 depth 条目的 delay/power 对比)——决定 latency 加权后 depth 10(去 split)是否自然胜出。
- **O4**:`recordDomainRoot`(ClockNetwork)当前在 source 流程中无调用方——确认是否有外部(report/GUI)消费,影响 root_buf 可选化的兼容面。
- **O5**:trunk 与 sink-htree 共享 CharacterizationLibrary 时,trunk 长度 bin 是否始终被直接表征(`additional_characterization_lengths_um` 注入后若 required>cap 转 dense,trunk 长 bin 可能仍靠 DP)——影响手段 1 的 0/1-buffer 可行性评估可信度。
