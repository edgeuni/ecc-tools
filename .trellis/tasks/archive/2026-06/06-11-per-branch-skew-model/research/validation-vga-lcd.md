# P1-D 验证报告:vga_lcd 自适应 skew target + 优化器通路 + tolerance 扫描

> 任务: 06-11-per-branch-skew-model · 2026-06-13
> 基线: P1-C 后 HEAD(7eb9908ce)默认 run(depth 9,initial_skew 0.0294,buffer 6025,WL 101792.4)
> 二进制: 含 P1-D 改动的同 HEAD 构建;workspace 克隆自 /tmp/p0a_vga_validation/ws_p1c(SDC: wb_clk_i period 10ns)
> 运行产物: /tmp/p1d_vga_validation/{run_default,run_probe,run_tol002,run_tol10}.log(临时目录,关键数字已录入本文)

## R1 per-branch FastSTA(文档性确认,无代码)

P1-C 验证已实证:优化阶段 FastSTA 注入真实嵌入 route tree(`OptimizationPreparation.cc` route-tree injection,本次 log 亦可见 `injected_nets=6026, rc_nodes=39031`),initial_skew 0.0294 即逐分支真实几何的产物。P0-A 修复后 R 项生效,长短分支延迟可分辨(探针 run 中 late/early pure buffer 分类非空:104/56,即模型能区分早到/晚到分支)。R1 成立。

## R3 自适应 target:默认 run(fraction=0.006 默认)

| 项 | P1-C 基线 | P1-D 默认 run | 判定 |
|---|---|---|---|
| target_skew | 0.0800 ns(固定) | **0.0600 ns**(`period_derived: 0.006 × 10ns`) | 对标 Innovus 量级(0.061)✅ |
| initial_skew | 0.0294 ns | 0.0294 ns | 零回归 ✅ |
| selected_depth / buffers / WL | 9 / 6025 / 101792.444 um | 9 / 6025 / 101792.444 um | 逐位一致,零回归 ✅ |
| target_met | true(0.0294<0.08,但 target 拍脑袋) | **true**(0.0294<0.06,target 有依据) | R5:真实达标判定 ✅ |
| 优化器 | no_op | no_op(预期内) | 行为对齐而非 QoR 变化 |

报告可观测性:Setup 表新增 `skew_bound / skew_period_fraction / target_skew_rule` 三行;新增 per-clock `CTS Optimization Clock Target` 表(clock/clock_period/target_skew/derivation)。period 来源 SDC(`clock_source: sdc`)。

## R2/R5 优化器通路探针(fraction=0.002 → target 0.02 < internal 0.0294)

通路健康实证(非 no_op 误判路径):

- 候选生成:`late_pure_buffers=104, early_pure_buffers=56, mixed_buffers=4, scored_edits=288`(模型可分辨分支早晚,R1 旁证);
- 求解行为:scored_batches=10,batch_trial_count=8,rejected_candidate_count=8,solver runtime 3.48s(默认 run 0.90s);
- 结果:accepted_edits=0,optimized_skew 维持 0.0294,**stop_reason=no_improving_candidate,target_met=false**(如实上报未达标,无模型误判达标);
- cap/slew 全程合法(cap_rejected=0, slew_rejected=0)。

**Sizing-only 手段边界结论**:depth-9 + split 后路径级数完全均一(10/10)、同层同 master,剩余 29.4ps skew 来自 per-branch 线长离散,**纯 sizing 无法再压**(8 个 batch 试验全被 FastSTA 评估拒绝,无一改善)。要继续压 internal skew 需要插入延迟/蛇形类手段;但 internal 0.0294 已低于 Innovus eval 0.069,在 eval 残差(模型-实测 gap)收敛前继续压榨内部指标收益存疑——维持设计决策:不在本任务新增插入/蛇形,等统一 eval 后定向。

## R4 balanceTopology tolerance 扫描(htree_topology_tolerance ∈ {0.02, 0.1, 10})

| 指标 | tol=0.02(紧) | tol=0.1(默认) | tol=10(≈关闭挪点) |
|---|---|---|---|
| initial_skew | 0.0299 ns(+1.7%) | **0.0294 ns** | 0.0396 ns(**+34.7%**) |
| total clock WL | 102239.617 um(+0.44%) | 101792.444 um | 101773.398 um(-0.02%) |
| final buffers | 6035(+10) | 6025 | 6025 |
| selected_depth / split_bufs | 9 / 1114 | 9 / 1114 | 9 / 1114 |
| feasible_pareto_refs | 444 | 649 | 649 |

**结论:默认 0.1 保持不变。** 设计预期"depth-9 下挪点作用减弱、关闭可能省 WL"被数据否定:关闭挪点(tol=10)skew 劣化 34.7% 而 WL 仅省 0.02%——几何拉平在浅深度下仍是 skew 均一性的有效来源;收紧(0.02)则双输(skew/WL 均劣化,可行 Pareto 收窄 649→444)。`balanceTopology` 的挪点逻辑判定为有正收益,保留且维持现默认。

## 验收对照(PRD,新基线口径)

- ✅ R1 per-branch FastSTA 成立(P1-C 证据 + 本次探针 late/early 分类旁证)
- ✅ R2 修复手段通路验证:sizing 框架真实求解(候选生成→打分→batch 试验→FastSTA 评估闭环),边界如实暴露
- ✅ R3 自适应 target 落地:0.08→0.06(period 推导),fraction=0 可完全回退旧语义
- ✅ R4 tolerance 扫描完成,数据定论:保留默认 0.1
- ✅ R5 停止条件基于可信模型:default target_met=true(真实达标),probe target_met=false + no_improving_candidate(真实未达标,无误判)
- ⚠️ eval 口径条款(eval skew ≤0.09、内外偏差 ≤20%)按用户决定推迟到全部任务完成后统一重跑 Innovus route+STA 评估,本任务以内部口径交付
- ✅ 单测:icts_test_flow_optimization 5/5(helper 语义)、icts_test_database_config 新增 4 用例;全量 iCTS ctest 17/17

## 已知限制/跟进项

1. 多 clock 设计:per-clock target 已支持,但 Setup 表的 rule 行为全局;per-clock derivation 表逐 clock 打印,设计如此。
2. period 缺失设计(无 SDC period)回退 skew_bound,报告标注 `fallback_skew_bound(no period)`——未在 vga_lcd 实测(其 SDC 有 period),由单测覆盖。
3. 剩余 29.4ps internal skew 的进一步收敛依赖插入/蛇形手段,挂起待统一 eval 重基线后决策(本报告 R2 节)。
