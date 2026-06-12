# Design: P1-E Latency 对齐——插入延迟缩减(新基线校准版)

> 任务: 06-11-latency-align · 2026-06-13
> 前置已落地: P0-A/P0-B/P1-C/P1-D。代码事实依据: `research/structure-levels.md`(全部 file:line 引用见该文)。
> 基线(P1-D 默认 run,HEAD f9df59f2d): 每路径 **10 级** = trunk 2 + root 1 + htree edge 5(depth 9 中 4 层跳过)+ split 1 + cluster 1;internal skew 0.0294;buffer 6025 / 面积 17016.7 um²;WL 101792.4;Innovus 同设计 7 级。

## 1. 对 PRD 各项的基线再校准

| 原需求 | 新基线下的判定 |
|---|---|
| R1-a root/trunk 冗余级合并 | **降级为"trunk 级数缩减"并入核心机制**:trunk pattern 候选含 0/1-buffer 形态(表征 2^N 枚举,`CharPatternEnumerator.cc:112-115`;0-buffer 直连分支已存在 `SourceTrunkSegment.cc:273-277`),但中位选型系统性放弃 front 的低延迟端。改选型策略即可让"免 buffer 直驱/单 buffer"在可行时胜出(-1~-2 级)。**root_buf 与 trunk 末 buffer 的结构合并不做**:root_buf 是 domain 准备阶段结构锚点(trunk 终点 + htree root driver + sizing 对象),合并需重排构建顺序,风险/收益比劣于选型路线(research §Q2)。 |
| R1-b htree 末级与 cluster 合并 | **本任务不落地,留 B 阶段**:legality 与 embedding 是同函数镜像契约(`Embedding.cc:344-347`),改动面大;且 depth-10(叶组 ≈4.1)可天然消除 split,留待选型 latency 化后由全局选择用数据回答(research O3)。 |
| R1-c 更激进跳层 | **由核心机制承载**:跳层 pattern 本就在候选集(bits=0 枚举),可行域硬墙在顶部 fanout 翻倍(根下最多 2 层连续无 buffer,`TopologyPatternLibrary.hh:207-216`)与底部 split 界(≤max_fanout²,`SinkLoadRegion.cc:434`)——**不动可行域**,只让选型在可行域内偏好更快(通常=更少级)的 pattern。 |
| R1-d 选型目标加入 latency 项 | **核心代码项**(§2)。 |
| R2 skew 不失控 | P1-D 自适应 target(0.06)+ 优化器通路已验证;级数变化后 internal skew 由实验验收(不劣于 0.0294 的 ±10% 内,超出则调 margin)。 |
| R3 顶层重载段专项 | **评估项**:root master 由 compensation 钉为第一 buffered level 末位 master(`SolutionSelection.cc:46-57`),vga_lcd 已是 BUFX20=buffer_types 最强 ⇒ "更强驱动"杠杆已饱和;分段/摆位是结构项,以实验数据(选型后该 stage 占比)定是否立后续任务。 |

原验收 eval 口径(latency_avg ≤0.52 / latency_max ≤0.56)按用户决定推迟统一重跑;本任务内部口径:**每路径级数 ≤8(cts.v 实测)+ internal max arrival 改善 + skew/面积/功耗代价受控**。

## 2. 核心机制:delay-margin 有界选型(替代 Pareto 中位)

**现状**:三处选型全部是 delay-power Pareto front → 取中位——trunk(`SourceTrunkSegment.cc:235-246`)、per-depth(`TopologyPruning.cc:496-514`)、**最终结构的全局选择**(`TopologyPruningGlobalSelection.cc:177-190`,唯一决定权,research §Q5)。中位 = 无指向性的折中,系统性放弃 front 低延迟端(末位即 min-delay)。

**新策略**(对已构建的 Pareto front,front 按 power 升序 ⇒ delay 严格降序):
```
margin = config.selection_delay_margin (初版默认 0.1;实验拐点定稿 0.07。0 = 关闭,完全回退中位策略)
bound  = min_delay_on_front × (1 + margin)
选择   = power 最小的满足 delay ≤ bound 的条目
       (即从 power 升序的 front 头部向尾部扫,第一个 delay ≤ bound 的条目)
```
- 语义:在"距最优延迟 margin 以内"的解里挑最省功耗的——latency 作硬约束、power 作目标,与 PRD R1-d「目标函数加入 latency 项/约束」一致;margin→∞ 退化为 min-power,margin→0⁺ 为纯 min-delay,0 保留旧中位(回滚开关,沿用 P1-D「0=disable」惯例)。
- 应用点:上述三处共用一个 header-only 模板 helper(`htree/topology_pruning/SelectionPolicy.hh`,SegmentChar/HTreeTopologyChar 同享 CharCore 的 get_delay/get_power);per-depth 处同步采用以保证 DepthSummary 报表与最终策略一致。
- **不改**:Pareto front 构建、join/剪枝、可行域、relaxed 回退结构;test fixture 的独立副本 `CharacterizationRealTechFrontierBuilder.cc:501`(钉表征值,非生产策略)。
- 选型时延已含 root compensation(`HTreeTopologyChar::get_delay`),margin 作用在全路径延迟上。

**预期机理(vga_lcd)**:
- trunk:front 末端(min-delay)若存在 0/1-buffer 条目则胜出 → -1~-2 级(O1:0-buffer 的 driven_cap=整段线 cap+root 输入 cap 是否 ≤ 源驱动上限 0.15pF,静态不可知,实验回答);
- htree:更少 buffered level / 更强 master 的 pattern 在 margin 内胜出 → 可能 5→4 级;
- 级数 ≤8 的可达路径:trunk-2(→8)或 trunk-1 + htree-1(→8);两条路都不通则如实勘误(P1-C 先例)。

## 3. 配置与可观测性

- `Config.{hh,cc}`:`_selection_delay_margin = 0.07`(初版 0.1,margin 扫描拐点定稿 0.07,依据见 `research/validation-vga-lcd.md`),getter/setter(负值 clamp 0)/reset/键表 22→23/解析/报告行(detail 注明 `0 disables; bounds selected delay to (1+margin) x front-min`)。
- `interface/default_config/cts_default_config.json` 增键 `"selection_delay_margin": "0.07"`。
- 报告:全局选择处(`DiscreteSolution.cc:155` 调用点)emit 一张 `HTree Global Selection` 表:policy(median/delay_bounded)、margin、front_size、front_min_delay、selected delay/power、selected buffered_levels(对选中 entry 一次 materialize 统计,`TopologyPatternLibrary::materialize` 现成范式 research §Q5);trunk 选择同理加 `policy/margin/selected_buffer_count` 字段(已有 selection_stage 的 fields 里补)。

## 4. 实验计划(vga_lcd,内部口径)

margin 扫描 {0(=旧中位,对照), 0.05, 0.1, 0.2, 0.5}:
- 级数(cts.v buffer 链直测,P1-D 同法)与构成(trunk/root/htree/split/cluster 各几级);
- internal skew(优化阶段 initial_skew)、max arrival(target window 上界)、WL、buffer 数/面积、CharBuilder/端到端 runtime;
- O1(trunk strict 候选里 0/1-buffer 幸存与否)、O2(htree 选中 pattern 的 buffered levels)、O3(全局赢家落在哪个 depth)从日志/报告直接读出;
- 据数据定默认 margin(约束:skew 不劣于 0.0294±10%、面积 ≤+5%、内部 power 代理(char power+面积)≤+3%),并回答 R3(选型后 root 重载段 stage 占比)。

## 5. 测试

- SelectionPolicy 单测(新文件挂 `icts_test_flow_synthesis_htree`):margin=0 回退中位(与旧实现逐位一致)、margin 内取 min-power、margin 极小取 front 末位(min-delay)、margin 极大取 front 首位(min-power)、单条目 front、空 front。
- Config 解析(23 键)。
- 既有约定测试适配:`HTreeTest.GlobalSelectionPreservesDelayPowerTieMultiplicity` / `PerDepthParetoCompressionKeepsDepthGroupsIndependent`(`HTreeTest.cc:285,313`)钉的是 tie 多重性与 per-depth 独立性(front 构建层,策略层之下)——预期不需改;若断言落在中位结果上,以 margin=0 路径保住旧断言、新增 margin>0 断言。
- 全量回归(real-tech smoke/matrix 若钉中位选型结果需检视:它们运行在默认 config 上,margin 默认 0.1 会改变选型 ⇒ 凡钉具体 master/级数的断言按新默认更新,并在 PR 说明)。

## 6. 改动点汇总

| 文件 | 改动 |
|---|---|
| `database/config/Config.{hh,cc}` | `selection_delay_margin` 全套(默认 0.1,0=off) |
| `htree/topology_pruning/SelectionPolicy.hh`(新) | header-only 模板:`SelectDelayBoundedEntry(front, margin)` + 中位回退 |
| `topology_pruning/TopologyPruningGlobalSelection.cc` | `SelectBestGlobalEntry` 接 margin 策略 + Global Selection 报表 |
| `topology_pruning/TopologyPruning.cc` | `SelectBestHTreeChar` 接 margin 策略(per-depth 一致性) |
| `topology/trunk/SourceTrunkSegment.cc` | `SelectBestSegmentEntry` 接 margin 策略 + selection 字段 |
| `interface/default_config/cts_default_config.json` | 增键 |
| 测试 | SelectionPolicy 单测 + Config 用例 + 受影响断言适配 |

## 7. 风险与回滚

- 风险 1:margin 过小 → 选 front 高功耗端,面积/功耗超 PRD 上限 → 扫描定默认,margin 可调/可关。
- 风险 2:级数下降改变 skew 分布 → P1-D 优化器(target 0.06)兜底,实验验收 skew;劣化超界则提高 margin 或回退。
- 风险 3:real-tech 测试钉死中位选型产物 → margin=0 路径保留旧行为,断言按新默认显式更新(不可静默)。
- 回滚:`selection_delay_margin: 0`(行为级)或 revert 单提交(代码级)。
