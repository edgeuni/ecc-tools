# P1-E Latency 对齐：插入延迟缩减

> 父任务: 06-11-align-commercial-cts · 优先级: P1 · 基准: iwls2005__vga_lcd
> 根因依据: 父任务 `research/vga-lcd-topology-gap-root-cause.md` §2.1/§7 R6

## Goal

把 vga_lcd 的时钟插入延迟压向 Innovus 水平：latency_avg 0.6245 ns → ≤0.52 ns，每路径 buffer 级数 9-10 → ≤8。对标 Innovus CCOpt 的 "Insertion Delay Reduction" 能力（其构建后专门将 insertion delay 0.422→0.382，允许 skew 暂时超标再收敛）。

## 背景（现状缺陷）

- 结构性多级：source trunk(1) + root(1) + 二叉 H-tree 跳 buffer 后 ~7 级 + cluster 层(1) = 9-10 级；Innovus 全程 fanout≈4 仅 7 级（log₄(16901)≈7 已达下界）。
- 同一 sink 实测：Innovus 7 级 0.472 ns vs ECC 9 级 0.651 ns（差 ≈2 级 × ~65ps + 顶层重载段 0.105/0.109）。
- 量化截断（R2）使长段被 DP 组合插入中间 buffer，进一步加深。
- 无任何 latency 显式优化阶段：深度/pattern 选型以 delay-power Pareto 中位数为准（`TopologyPruning.cc:507-513`），不显式压 latency；优化器只做 sizing。

## Requirements

- R1: 增加插入延迟缩减能力，候选手段（design.md 中论证取舍，至少落地两项）：
  - root/source-trunk 冗余级合并（当前 source_to_root_top_segment_buf + root_buf 两级首延迟 0.076+0.105，评估合并或免 buffer 直驱）；
  - htree 末级与 cluster 层合并（leaf buffer 被 prune 后 htree 末级直驱 cluster，评估让 cluster buffer 直接作为 htree 叶，去掉一层）；
  - 更激进的层间跳 buffer（在 slew/cap 可行域内偏好更少 buffer 级的 pattern——依赖 P0-B 表征覆盖修复后长段可直接表征）；
  - 选型目标函数加入 latency 项/约束（替代纯 Pareto 中位数策略）。
- R2: latency 缩减不得以 skew 失控为代价：与 P1-D 的 skew 修复手段协同（先减级、后修 skew 的两阶段，或联合目标）。
- R3: 顶层重载段（root 后第一级 stage delay 0.105/0.109 ns）专项：评估更强驱动/分段 buffer/缩短顶层段的方案。

## 依赖（必须先行）

- **P0-A（wire R bug，任务待创建）硬依赖**：级数-延迟权衡的评估完全依赖可信的线延迟模型。
- **P0-B（表征覆盖）强依赖**：少级 ⇒ 单段更长 ⇒ 必须有 >116.94um 的直接表征能力，否则 DP 组合会把省掉的级数加回来。
- P1-C（深度解锁）协同：更浅深度是减级的主要来源之一；本任务的层级合并手段与其正交，可并行开发、统一基线验收。

## Acceptance Criteria

> 勘误（2026-06-13，design.md §1 基线再校准）：eval 口径条款按用户决定推迟到全部任务完成后统一重跑；内部口径与机理证据见 `research/validation-vga-lcd.md`。R1 落地两项为 (d) 选型 latency 约束 + (a) trunk 免 buffer 直驱（经选型自然达成）；(b) 末级/cluster 合并经数据判定不立项（depth-9 全维占优）；(c) 由同一机制承载且 htree 端已到可行域下限。

- [ ] ⏸ vga_lcd eval latency_avg ≤0.52 / latency_max ≤0.56——**待统一 eval 重跑**。内部 max arrival 0.5564→0.4486（-19.4%），按基线 eval/internal 比例外推 ≈0.503 ns ≤0.52（外推非实测）。
- [x] 每路径 buffer 级数（`clock_path_max_buffer`）≤8：**10→8**（cts.v 实测 4226 条链全部 8 级；trunk 2→0 免 buffer 直驱）。
- [x] skew 不劣于当期基线：internal 0.0294→**0.0279**（改善）；eval 口径（≤0.09）待统一重跑。
- [x] buffer 总数/面积不增超 5%：6025→6023（-2），面积 **+2.48%**；clock power 不增超 3%：选中 char power **+2.4%**（内部代理，eval 待重跑）。
- [x] 单测覆盖 latency-aware 选型路径：SelectionPolicyTest 6/6 + ConfigTest 4 用例，全量 iCTS ctest 17/17；层级合并路径未落地（经 O3 数据判定不立项，见勘误）。

## 约束

- 验证基准仅 vga_lcd；DAC-27-CTS 只读。
- 复杂任务：start 前需补 `design.md` + `implement.md`。
