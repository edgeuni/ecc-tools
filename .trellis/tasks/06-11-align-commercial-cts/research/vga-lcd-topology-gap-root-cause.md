# vga_lcd ECC vs Innovus CTS 拓扑差异根因调研报告

> 任务: 06-11-align-commercial-cts · 阶段 1（深度调研）
> 日期: 2026-06-11
> 数据根: `~/project/DAC-27-CTS`（只读）· 代码: 本仓库 `cts_refactor` 分支
> 核心问题: 定位 ECC H-tree 拓扑与 Innovus 结果拓扑差异的根因；裁决两个假设（H1 wirelength unit 失真 / H2 NDR 缺失）

---

## 0. 结论速览

| # | 根因 | 严重度 | 证据锚点 |
|---|------|--------|----------|
| R1 | **wire 电阻被 1000x 低估**（单位换算 bug）：所有 CTS 时序模型中线电阻 ≈ 0 | 🔴 P0 | `WrapperRc.cc:159,293`、`FastStaChar.cc:134`、`lef_read.cpp:642`、日志 "914 uOhm/um" vs LEF 实际 0.914 Ω/um |
| R2 | **表征线长覆盖截断 + ceil 量化**：13 个 level 长度只有 3 个直接表征 bin（unit 38.98um），超出部分 DP 组合插入额外 buffer | 🔴 P0 | `WirelengthGrid.cc:46,175,192`、日志 `direct_bins_capped` |
| R3 | **拓扑自由度锁死**：leaf=2^n 规则 + 深度搜索被 sink-load 单调剪枝杀死（11/10/9 深度 feasible=0），深度只能=12 | 🟠 P1 | `TopologyGen.cc:360-370`、`SinkLoadRegion.cc:298-305,383-393`、run 报告 `work/icts-native/cts.log` Depth Candidate Summary：三个浅深度失败原因均为 `monotone_pruned_by_bottom_most_buffered_level threshold=10` |
| R4 | **per-level 均一假设 vs per-branch 物理现实**：模型按层平均长度表征/选型，物理嵌入每层分支长度离散 std 高达 125um，每级累积 8~29ps 偏差 → 95ps skew，模型完全不可见 | 🔴 P0(建模) | `Plan.cc:66-91`、`Embedding.cc:373`（无蛇形）、`TopologyGen.cc:454-502`（拉平挪点）、eval 逐级路径对比 |
| R5 | **NDR 能力为零 + C 模型无耦合**：iCTS 不读/不写/不建模 NDR；C=LEF plate+fringe（2.007e-4 pF/um），无耦合项；R/C 只取 `routing_layers.front()` 单层 | 🟠 P1-P2 | iCTS 全仓 grep 无 NDR 代码；`WrapperRc.cc:254-267`、`:91` |
| R6 | **流程能力缺失**：无 Insertion-Delay-Reduction 阶段；优化器被 R1 污染的模型欺骗（内部 skew 0.053<0.08 达标 → no_op）；source trunk 选 472um 裸长段（eval 中产生 -0.051 tran 违例） | 🟠 P1 | 日志 Optimization no_op、Innovus log `Reducing insertion delay` 阶段、CSV drv_max_tran |

**对两个假设的裁决**：
- **H1（wirelength unit 失真）= 成立，且比预想更深**。失真有三层：① 量化层（unit 38.98um、3-bin 截断、ceil 覆盖）；② 统计层（每层取平均 Manhattan 距离，抹掉 std=125um 的分支差异）；③ 单位层（R/1000 bug 让"每 um 线长的时序代价"本身错了 3 个数量级——这是最致命的"unit 失真"）。
- **H2（NDR 缺失 → timing 误差）= 方向成立，但本评估体系下不是当前主要矛盾**。关键事实：两家的 cts.def 都不带 clock 布线（各只有 2 条电源特殊网 ROUTED），eval 由同一 NanoRoute 用**默认规则**对两家从头布线（两边 rt.def 均无 NONDEFAULTRULE）。Innovus 的 NDR（5771 net 全带 extra-space route_type，preferred MET3/MET4）只作用于其**构建期内部模型**。因此 NDR 缺失目前主要通过「模型保真度」起作用，而该通道上 R1（1000x）≫ 耦合 C 缺失（~1.5-2x）≫ NDR 差异。修 R1 之前谈 NDR 收益无法测量。

---

## 1. 评估体系事实（公平性前提）

两家工具拿到相同 placement snapshot（DEF+Verilog+SDC），产出 cts.def 后由**同一 eval 管线**评估（`eval/*/innovus.cmd`）：

```
Innovus 21.16: defIn cts.def → refinePlace → globalDetailRoute(NanoRoute)
  → defOut rt.def → extractRC(postRoute) → report_clock_timing / timeDesign / verify_drc
```

- 两家 cts.def 中 clock net 均未布线 → NanoRoute 以**默认规则**统一布线，eval 公平。
- 推论：eval 差距完全来自**树的结构本身**（拓扑/级数/位置/sizing），而非布线规则差别。
- 推论：ECC 对"交付后真实布线+提取"的预测能力是第二战场（见 §5）。

## 2. vga_lcd 拓扑差异定量

### 2.1 结构对比

| | ECC | Innovus |
|---|---|---|
| 总 buffer | 5744（1516 htree + 4226 cluster + root + trunk buf） | 5770（单一体系） |
| 每路径 buffer 级数 | 9–10 | 7 |
| 结构 | source trunk(1) → root(1) → 二叉 H-tree（4096 leaves, 12 几何级, 跳 buffer 后 ~7 个 buffer 级）→ cluster 层(fanout 4) | 自底向上聚类，全程 fanout≈4：2→5→18→69→274→1086→4316→16901 |
| 级数下界 | log2(4226)+cluster+root+trunk | log4(16901) ≈ 7 ✓ 达到下界 |

ECC 的「cluster 预层 + 2^n 二叉 H-tree + 独立 root/trunk 段」结构性多出 2~3 级。Buffer 总数几乎相同，但 Innovus 把同样的 buffer 摆成了**更宽更浅**的树。

### 2.2 同层分支离散度（rt.def 实测, driver→load Manhattan 距离）

ECC 顶部各层（L3: mean 215 / **std 125 / min 16 / max 467um**；L4: mean 121 / std 79）。Innovus 同样有离散（L1 std 97），但其模型 per-net 跟踪 RC，离散是"看得见的"；ECC 模型按层平均长度，离散完全不可见。

### 2.3 同一 sink 的逐级路径对比（eval timing_paths.rpt, full_clock）

到 `clut_mem/.../ra_reg[2]/CK`：**Innovus 7 级 0.472ns vs ECC 9 级 0.651ns**。
ECC max/min 两条 9 级路径（0.651 vs 0.556）每级 cell master 完全相同（per-level 统一 sizing），但每级 stage delay 系统性偏差 +8~29ps，逐级同向累积 = 95ps —— **skew 不来自级数或 sizing 差异，而是同层分支物理不均匀的逐级累积**。

### 2.4 模型-现实偏差（最关键的诊断指标）

| | 内部估计 | eval 实测 | 偏差 |
|---|---|---|---|
| ECC skew | 0.0533 ns | 0.118 ns | **+121%** |
| ECC latency max | 0.556 ns | 0.675 ns | +119 ps |
| Innovus skew | 0.060 ns | 0.069 ns | +15% |
| Innovus latency max | 0.382 ns | 0.505 ns | +123 ps |

两家 latency 模型误差几乎相同（约 +120ps，来自真实布线绕行/via/耦合——系统性、可抵消）；**skew 误差 Innovus +9ps vs ECC +65ps**：skew 是差值指标，只有「路径间方差型」模型误差才会留下来——ECC 的 per-level 均一假设 + R≈0 模型正是方差型误差的制造机。后果：优化器看到 0.0533 < 0.08 直接 no_op（`stop_reason=no_improving_candidate`），真实 0.118 的 skew 无人修复。

## 3. 根因 R1：wire 电阻 1000x 单位 bug（代码证据链）

1. tech LEF `N551P6M_ecos.lef`：MET4 `RESISTANCE RPERSQ 0.0914`（Ω/sq），`WIDTH 0.1` → 真实 R = 0.0914/0.1 = **0.914 Ω/um**。LEF UNITS 段无 RESISTANCE 缩放语句。
2. `src/database/manager/builder/lef_builder/lef_read.cpp:642`：`set_resistance(lef_layer->resistance())` **原值存入**（Ω/sq）。
3. `src/operation/iCTS/source/database/io/WrapperRc.cc:251`：`queryWireResistance = get_resistance() × length / width` → 1um 返回 0.914（**Ω**）。
4. 但消费方把它当毫欧：`WrapperRc.cc:159,293` 与 `FastStaChar.cc:134` 均 `/ kMilliOhmPerOhm(1000)` → `ClockRouteSegmentRc.resistance_per_um_ohm = 0.000914 Ω/um`。日志佐证：`unit_resistance 914.000 uOhm/um`。
5. 影响面（消费 `ClockRouteSegmentRc` / 同类除法）：CharBuilder 表征 STA 采样、SinkLoadRegion/ClusterConstraintEvaluator（`unit_h_res`）、Router::buildRCTree、FastSTA（优化与内部 skew 估计）、AnalyticalCharacterization —— **全部时序决策的线电阻 ≈ 0**。

直接后果：
- 所有「线长→延迟/坏 slew」的代价项坍缩，模型只剩负载电容差异 → 内部 skew 只看得到 53ps；
- source trunk 敢选 **472um 无 buffer 段**（R≈0 时仅受 cap 上限约束：472um×0.2007fF/um≈0.095pF < 0.15pF，刚好卡线）→ eval 实测 1 条 net tran 违例（-0.051）；另注意 `max_length=300um` 配置对该段未生效，疑似约束旁路，子任务中一并核查；
- H-tree 长/短分支在模型中无延迟差 → per-level 均一假设"看起来成立"。

## 4. 根因 R2：表征线长量化失真（H1 量化层）

vga_lcd 日志（`HTree Characterization Grid Plan`）：
- `resolved_wirelength_unit = 38.98um`（auto_derived = max_level_length/13，`WirelengthGrid.cc:175`）
- `required_covering_iterations = 13`，但 `wirelength_iterations = min(配置3, 13) = 3`（`WirelengthGrid.cc:192`）→ 只直接表征 38.98/77.96/116.94um 三点，`decision_flags = direct_bins_capped`
- 13 个请求长度 ceil 对齐（`MakeCoveringLengthIndex`, `WirelengthGrid.cc:46`）后塌缩为 6 个 bin

后果：(a) >116.94um 的层（顶部多层 + trunk）靠 SegmentPruning DP 组合，**每次组合插一个中间 buffer**；(b) 底层实际 ~6-15um 的段被 ceil 到 38.98um 表征（模型长度最多虚高 ~6x），选型/延迟估计全部基于虚长；(c) 量化粗 → 各层共享同一 bin，长度差异进一步抹平。

## 5. 根因 R3+R4：拓扑自由度锁死 & per-level 均一 vs per-branch 现实

**R3 锁死链**：
- `TopologyGen.cc:360-370`：leaf_count = 不超过 load 数的最大 2 次幂（4226→4096）→ depth=12 固定；
- 深度搜索（`Plan.cc:142-162` 生成 {12,11,10,9} 窗口）逐个评估，但 `SinkLoadRegion.cc:298-323`：边界组 loads 超 max_fanout(=4) 或 pin-cap 下界违例即 `monotone_hard_fail`，并经 `:383-393` 的 `max_monotone_failed_level`（跨深度共享 context, `DepthPlan.cc:112`）**单调剪掉所有 bottom-buffered-level 更浅的候选**。
- **直接证据**（run 报告 `work/icts-native/cts.log` "HTree Depth Candidate Summary"）：

| Depth | Leaves | Status | Frontier | Feasible | Failure |
|---|---|---|---|---|---|
| 12 | 4096 | selected | 185920 | 138543 | none |
| 11 | 2048 | failed | 153949 | 0 | monotone_pruned... threshold=10, candidate_level=9 |
| 10 | 1024 | failed | 158427 | 0 | monotone_pruned... threshold=10, candidate_level=8 |
| 9 | 512 | failed | 131211 | 0 | monotone_pruned... threshold=10, candidate_level=7 |

  即 depth-12 评估期间，bottom-buffered-level=10 的 pattern 触发 hard-fail（4226 cluster 摊到 level-10 边界 ≈ 4.13 loads/组 > max_fanout 4），阈值升至 10 后，**浅深度候选的全部 pattern 连电气评估都未做即被拒**——"评估了 4 个深度"实为只评估了 1 个。该一刀切剪枝（单 group first-fail 否决整个 level 域）是拓扑探索失效的直接原因。
- 结果：`selected_depth=12` 是唯一选择；同时只有"叶子层带 buffer"的 pattern 幸存（bottom=11），与 `pruned_leaf_single_load_buffers=3976`（嵌入期再裁掉单负载叶 buffer）互相印证——先强制插入、再事后裁剪，选型期的延迟/功耗评估均基于被裁剪前的结构。

**R4 均一假设链**：
- `Plan.cc:66-91`：每层 requested_length = 该层所有父子段 Manhattan 距离的**平均值**；
- `TopologyGen.cc:454-502` `balanceTopology`：为靠近均一假设，把超出 `avg×(1±0.1)` 的节点**挪离负载质心**投影到 L1 圆上（线长换均匀，二者皆伤）；
- `Embedding.cc:373`：buffer 沿**真实**父子几何按 pattern 分数位插值放置，无蛇形补偿 → 物理路径 603~708um 离散（TopologyGen Root-To-Leaf Summary），±8% 永久存在；
- 表征/选型/可行性全部基于「层平均再 ceil」的虚拟长度 → 每层 8~29ps 的分支偏差逐级累积（§2.3），模型零感知。

## 6. 根因 R5：NDR 与 RC 模型保真

- **iCTS 全仓无 NDR 代码**（读/写/建模均无；仅 iDB `IdbNet.h` 有未使用的数据字段）。
- C 模型 `WrapperRc.cc:254-267` = LEF plate(CPERSQDIST×width) + fringe(EDGECAP×2)，**无耦合项**；55nm 默认间距下真实耦合占比大（post-route 提取 C 显著更高）。
- R/C 只取 `routing_layers.front()`（层4）单层值（`WrapperRc.cc:91`）；MET5 的 plate cap 约为 MET4 一半（LEF 0.0006259 vs 0.0011069），混层布线时模型再偏。
- Innovus 构建期：5771 条 clock net 全带 NDR（extra space 1, preferred MET3/MET4），且 "PreRoute extraction honoring NDR/Shielding/ExtraSpace"（innovus-phys.log:705,817）→ 其内部模型与最终提取高度一致（skew 模型误差仅 +9ps）。
- 但**本 eval 体系**对两家都用默认规则重布 → NDR 在评估中无物理差异；其当前价值=模型保真与商业流程对齐，物理收益需 eval 管线支持 NDR 传递后才能兑现。

## 7. 根因 R6：流程能力差距

- Innovus 构建后 insertion delay max=0.422 → 专门跑 **Insertion Delay Reduction** 压到 0.382（允许 skew 暂超目标 0.084>0.061，再收敛到 eval 0.069）；其 skew target 自动推导（0.061ns = f(period)）。
- ECC：固定 skew_bound 0.08；无 latency 缩减阶段；优化仅 per-instance buffer sizing 且被 R1 污染的 FastSTA 判定"已达标"→ no_op、0 edits。
- Source trunk 段合成耗时 35.5s/55.4s（总运行时间的 64%，`SourceTrunkSegment frontier 73178 entries`）——独立的性能问题顺带记录。

## 8. 后续任务方向（建议派生子任务）

| ID | 任务 | 内容 | vga_lcd 验收口径 |
|----|------|------|------------------|
| **P0-A** | 修复 wire R 1000x bug | 移除/修正 `WrapperRc.cc:159,293`、`FastStaChar.cc:134`、`FastStaParasitics.cc:91` 四处 `/1000`（统一单位约定，加单元测试钉死 0.914Ω/um）；全链路复核 R 消费方 | 日志 unit_resistance≈914 mOhm/um；单测哨兵；全量测试无回归。**已完成（06-12）**，实测勘误：优化器仍 no_op（0.0576<固定 target 0.08，需 P1-D 自适应 target）、472um trunk 段仍合法（max_length 未实施，独立缺陷转后续）；vga_lcd 拓扑决策几乎不变 → R1 是模型保真前提而非拓扑差距决定因素，详见 `06-12-fix-wire-res-unit/research/validation-vga-lcd.md` |
| **P0-B** | 表征覆盖修复 | auto 模式直接表征上限与 legacy `wirelength_iterations` 解绑，新配置 `auto_direct_bins_cap`（默认 6）；dense/sparse 直接索引；不动 unit 公式与 ceil 语义（slot 解耦转 P1） | **已完成（06-12）**：vga_lcd direct bins 3→6、SourceTrunk 合成 35.3→16.1s（-54%）、端到端 67.5→37.1s（-45%）、initial_skew 57.6→56.2ps；cap 扫描证明 QoR 在 6/7/8 一致、6 为成本拐点；架构事实勘误：pattern 枚举 2^slots（slots=length_idx）使全覆盖不可行、组合 join 不必然插 buffer（真实代价是 bucket 量化误差与组合开销）。详见 `06-12-char-wirelength-coverage/research/validation-vga-lcd.md` |
| **P1-C** | 拓扑自由度解锁 | 边界组 split 补救替代 first-fail 否决（SplitSinkLoadRegionGroup 合法性/嵌入共享，确定性中位二分）；Split 监控列与 Monotone Origin 报告 | **已完成（06-12）**：4/4 深度可行（基线 1/4）、选中 depth 9；内部 skew 0.0562→0.0294（-48%）、max arrival -53ps、级数均一 10/10；buffer +4.9%/WL +1.5%；DEF 实测 net fanout ≤4。勘误：级数 ≤8 移交 P1-E。详见 `06-11-htree-depth-unlock/research/validation-vga-lcd.md` |
| **P1-D** | per-branch 时序建模 + skew 修复 | R 修复后：FastSTA 按真实嵌入几何逐分支算 delay（已有 route tree 注入能力）；增加 buffer relocation/插入式 skew 修复或 wire snaking；skew target 自适应 f(period) | eval skew 0.118 → ≤0.09 ns；内部/外部 skew 偏差 ≤20% |
| **P1-E** | Latency 对齐 | 插入延迟缩减阶段（合并 root/trunk 冗余级、leaf 区域 1-to-N 展平、层级跳 buffer 更激进） | latency_avg 0.6245 → ≤0.52 ns；每路径级数 ≤8 |
| **P2-F** | NDR 机制 | 模型侧：RC 查询支持 width/spacing 参数+耦合估计（系数标定 vs eval extractRC）；物理侧：DEF NONDEFAULTRULE 输出与 eval 管线传递（需与 benchmark 维护方确认 eval 是否允许携带布线/NDR） | 表征 C 与 eval extractRC 偏差 ≤15%；（若管线支持）带 NDR 布线的 skew/DRV 增益 |
| **P2-G** | Runtime: source trunk | SourceTrunkSegment frontier 合成 35.5s 优化（剪枝/缓存） | vga_lcd 总 runtime ≤ 30s |

**依赖关系**：P0-A 是一切时序相关改进的前提（不修则所有评估失真）；P0-B 与 P0-A 并行可做；P1-C/D/E 依赖 P0-A 落地后重新基线；P2-F 依赖 P0-A/B（否则增益淹没在 1000x 误差里）。

## 9. 本次调研使用的可复用资产

- 图解析: `DAC-27-CTS/.trellis/tasks/06-11-def-clock-graph-parser/def_clock_graph.py`（rt.def → JSON 图，/tmp/vga_{ecc,inv}_graph.json 已生成）
- 每层离散度脚本: 见本报告 §2.2（内联于调研过程，可固化到本任务 research/）
- 逐级路径对照: `eval/*/timing_paths.rpt` 的 full_clock 路径表
- 关键日志锚点: ECC `ecc-tools.log`（Grid Plan 表 / Optimization Summary 表）；Innovus `innovus-phys.log:484,681,897`（skew target / DAG stats / Insertion Delay Reduction）
