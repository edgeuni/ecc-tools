# P1-E 验证报告:vga_lcd delay-margin 有界选型(margin 扫描)

> 任务: 06-11-latency-align · 2026-06-13
> 基线: P1-D 后 HEAD(f9df59f2d)margin=0(=旧 Pareto 中位)run,与 P1-D 默认 run 逐位一致
> 运行产物: /tmp/p1e_vga_validation/{run_m000,m005,m007,m008,m010,m020,m050,mdef}.log(临时目录,关键数字已录入本文)
> 级数测量: cts.v buffer 链直接遍历(wb_clk_i → 各 sink,4226 条链全量直方图)

## 总表(margin 扫描,vga_lcd 内部口径)

| 指标 | m000(=旧中位) | **m007(=新默认 0.07)** | m005 | m008 | m010 | m020 | m050 |
|---|---|---|---|---|---|---|---|
| **每路径级数** | 10 | **8** | 8 | 8 | 8 | 8 | 8 |
| 构成 trunk/root/htree/split/cluster | 2/1/5/1/1 | **0**/1/5/1/1 | 0/1/5/1/1 | 同 | 同 | 同 | 同 |
| **internal max arrival** | 0.5564 ns | **0.4486(-19.4%)** | 0.4483(-19.4%) | 0.4715(-15.2%) | 0.4724(-15.1%) | 0.4900(-11.9%) | 0.5621(+1.0%) |
| initial_skew | 0.0294 | **0.0279(改善)** | 0.0279 | 0.0340 | 0.0343 | 0.0356 | 0.0322 |
| buffer 面积 | 17016.7 | **17439.0(+2.48%)** | +5.01% | +2.84% | +2.00% | +0.11% | -0.66% |
| 选中 char power | 1.926 mW | **1.972(+2.4%)** | +4.7% | +2.1% | +1.2% | -0.5% | -1.3% |
| clock WL | 101792.4 | **101792.4(+0.00%)** | +0.00% | +0.29% | +0.29% | +0.29% | +0.25% |
| buffers | 6025 | **6023** | 6023 | 6033 | 6033 | 6033 | 6033 |
| target_met(target 0.06) | true | true | true | true | true | true | true |
| selected depth / htree buffered levels | 9 / 5 | 9 / 5 | 9 / 5 | 9 / 5 | 9 / 5 | 9 / 5 | 9 / 5 |

Gate-1(margin=0 零回归):m000 与 P1-D 基线 skew/buffers/WL/面积逐位一致 ✅;全量 ctest 17/17、既有 HTreeTest tie/Pareto 约定无需改 ✅。
默认路径:无键 run(mdef)与显式 0.07(m007)数值内容完全一致 ✅。

## 默认 margin = 0.07 的依据(拐点分析)

- 0.05→0.07:latency 持平(-19.41% vs -19.37%),面积代价减半(+5.01%→+2.48%),power +4.7%→+2.4%(回到 PRD ≤3% 内)——0.07 支配 0.05;
- 0.07→0.08 出现行为悬崖:选型跳到另一 pattern 族,skew 0.0279→0.0340、latency -19.4%→-15.2%——0.07 是该 front 的甜点;
- 0.5 过松退化为 min-power(maxarr 反超基线 +1%),证实 margin 语义两端行为正确。

## 机理实证(开放问题 O1/O2/O3 回答)

- **O1(trunk 0-buffer 可行性)**:成立。strict_candidates=69670 中 0-buffer 条目幸存硬过滤(driven_cap=整段线+root 输入 cap ≤ 源驱动上限),margin>0 后直接胜出:`policy=delay_bounded, selected_buffer_count=0` ⇒ **trunk 2 级 → 0 级(免 buffer 直驱),级数 10→8 的全部来源**。旧中位策略系统性放弃了这个一直存在的解。
- **O2(htree 还能更少级吗)**:min-delay 端(front_min 0.321)选中 pattern 仍是 5 buffered levels ⇒ depth-9 可行域内 htree 级数已到底,纯选型无法再减(可行域硬墙:顶部 fanout 翻倍 ≤2 连续跳层、底部 split ≤max_fanout²)。
- **O3(depth 10 会否胜出)**:全 margin 扫描全局赢家均 depth 9 ——split 代价虽不在 char delay 内,但 depth 9 的 front 在 delay/power 双维占优,latency 加权不改变 depth 选择。
- **R3(顶层重载段)**:root master 全程 BUFX20(buffer_types 最强,load_cap 0.0219 pF)——"更强驱动"杠杆已饱和;margin 选型后 internal maxarr 0.4486 已低于 Innovus eval 0.472,root 段分段/摆位类结构改造的边际收益待统一 eval 残差定位后再立项。

## 验收对照(PRD,内部口径)

- ✅ **每路径 buffer 级数 ≤8**:10→8(cts.v 实测 4226 条链全部 8)——P1-C 移交的级数条款在此达成。
- ⏸ eval latency_avg ≤0.52 / latency_max ≤0.56:待统一 eval 重跑。内部投影:maxarr -19.4%,按基线 eval/internal 比例(0.6245/0.5564≈1.122)外推 ≈0.503 ns ≤0.52 ✓(标注:外推非实测)。
- ✅ skew 不劣于当期基线:0.0294→0.0279(改善),目标 0.06 下 target_met=true(P1-D 真实判定)。
- ✅ buffer 总数/面积 ≤+5%:6025→6023(-2)/面积 +2.48%;clock power ≤+3%:char power +2.4%(内部代理,eval 口径待重跑)。
- ✅ 单测:SelectionPolicyTest 6/6(中位回退/界内 min-power/两端极限/单条目/空)+ ConfigTest 4 用例;全量 17/17。
- R1 落地两项:✅ (d) 选型目标加入 latency 约束(三处选型点);✅ (a) trunk 冗余级合并的"免 buffer 直驱"形态(经选型自然达成,无结构改动);(c) 跳层偏好由同一机制承载(htree 端数据证明已到可行域下限);(b) 末级/cluster 合并经 O3 数据判定不立项。

## 已知限制/跟进项

1. 默认 0.07 由 vga_lcd front 形状标定;其他设计的拐点可能不同(margin 可配,0=完全回退)。多设计标定挂起待 bench 扫描。
2. split 代价仍不在选型 delay/power 内(P1-C 跟进项保留;本次数据显示不影响 depth 决策)。
3. eval 口径条款(latency/skew/power)统一重跑后回填;内部投影已记录外推假设。
4. trunk 直驱使最长 clock net 434.5→506.7 um(source→root 整段无 buffer)。该段经表征 strict 过滤(cap/slew 合法),但 `max_length=300` 不实施是 P0-A 时已记录的存量缺陷(472um 裸段彼时已合法),非本次引入;若后续实施 max_length,trunk 直驱形态需复核。
5. trellis-check 对 SelectionPolicy.hh 的 2 处 clang-tidy 自修(forwarding ref→const ref、手写 min→std::min)后已用新二进制复跑默认 run:全部确定性指标(skew/buffers/面积/WL/maxarr/trunk=0)逐位一致,实验数据有效。
