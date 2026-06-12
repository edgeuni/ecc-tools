# PRD: 对齐商业工具 CTS 性能（vga_lcd 基准）

> 任务: 06-11-align-commercial-cts
> 创建: 2026-06-11 · 负责人: dawnli
> 类型: 父任务（research → 派生子任务）

## 1. 背景

ECC iCTS（本仓库 `src/operation/iCTS`，分支 `cts_refactor`）与 Innovus CCOpt 在相同输入/约束下的评估对比（数据根: `~/project/DAC-27-CTS`，只读）显示系统性 QoR 差距。全设计几何平均：skew 1.72x、latency 1.38x、线长 1.18x 差于 Innovus。

本任务以 **iwls2005__vga_lcd**（16901 sinks，最大设计之一）为唯一分析与验证基准，定位差距根因并派生改进子任务。

vga_lcd 关键指标（ECC vs Innovus, eval=同一 Innovus 21.16 后端 refinePlace→globalDetailRoute→extractRC→STA）：

| 指标 | ECC | Innovus | 比 |
|---|---|---|---|
| skew_max | 0.118 ns | 0.069 ns | 1.71x |
| latency_avg | 0.6245 ns | 0.4635 ns | 1.35x |
| clock WL | 109342 um | 97159 um | 1.13x |
| buffer 数 | 5744 | 5770 | ~1.0x |
| 树深（buffer 级） | 9–10 | 7 | +2~3 级 |
| WNS (reg2reg) | 0.216 ns | 0.480 ns | -264 ps |

## 2. 阶段 1 目标（本轮，已完成）

定位 ECC H-tree 拓扑与 Innovus 结果拓扑差异的根因，重点验证两个假设：

- H1: H-tree 构建使用的 wirelength unit 失真严重 → 拓扑差异
- H2: NDR 机制缺失 → timing 表征误差 → 表征与 H-tree 构建被污染

产出：代码级证据链 + 后续子任务方向（见 `research/vga-lcd-topology-gap-root-cause.md`）。

## 3. 总体验收标准（父任务级）

1. 根因结论均有「日志/评估数据 + 源码行号」双重证据，可复核。
2. 派生的每个子任务有独立可验证的验收口径（vga_lcd 上的量化指标）。
3. 最终目标（多个子任务完成后）：vga_lcd 上 ECC skew ≤ 0.09 ns（≤1.3x Innovus）、latency_avg ≤ 0.52 ns（≤1.12x）、内部 skew 估计与 eval 实测偏差 ≤ 20%。
4. 验证统一使用现有评估管线（DAC-27-CTS 实验框架），DAC-27-CTS 仓库保持只读，新实验输出写入新 run 目录。

## 4. 约束

- 只关心 vga_lcd；其他设计仅作旁证，不作为验收口径。
- `~/project/DAC-27-CTS` 只读；分析脚本输出至 /tmp 或本任务目录。
- 代码改动在本仓库 `cts_refactor` 分支的后续子任务中进行，本父任务不直接改代码。

## 5. 子任务方向（详见研究报告 §8）

- P0-A 修复 wire 电阻 1000x 单位 bug（模型保真前提）
- P0-B 表征线长覆盖修复（消除 3-bin 截断与 ceil 量化失真）
- P1-C 拓扑自由度解锁（深度搜索剪枝修复 + per-branch 时序建模）
- P1-D Latency 对齐（插入延迟缩减阶段、层级合并）
- P2-E NDR 机制（模型侧 width/spacing/耦合参数 + 物理侧 DEF NDR 输出）
- P2-F 模型-现实闭环标定（internal vs eval 回归指标）
