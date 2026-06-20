# CTS Timing/Skew First-Principles Alignment

## Objective

Improve ECC CTS quality by tracing the physical chain:

`topology -> sink/load distribution -> RC/cap/slew -> cell delay + wire delay -> endpoint arrival order -> skew/QoR`

The goal is not to fully match commercial Innovus in one step. A production algorithm change is effective when the current candidate is closer to the commercial Innovus reference than the remote-branch baseline is, and the case-by-case evidence shows a real algorithmic effect rather than a mean-value artifact. Light regression in secondary metrics can be accepted only when it is bounded, explained by first-principles evidence, and outweighed by a strategic Innovus-alignment or raw-QoR gain. Case-specific tuning, fitted scalar correction, behavior config, and magic-number optimization remain out of scope.

## Evidence Root

Generated evidence stays under:

`/nfs/share/home/liweiguo/ecc_cts_innovus_align/evaluations/06-15-cts-next-stage-capability-research`

Trellis keeps compact decisions and links only. Full traces, logs, CTS outputs, evaluation outputs, and CSVs stay under the NFS evidence root.

## Current Baseline

Active comparison baseline:

`origin/cts_refactor@65d67000753a`

Evidence snapshot:

`summary/remote_branch_baseline_65d670007`

Commercial reference:

`/home/liweiguo/project/DAC-27-CTS/experiments/summary/synthesized_cts_eval.commercial_strict/innovus.summary.csv`

The old `candidate_38a_local_split_physical_guard_refresh` numbers are retained only as historical continuity evidence. New keep/reject decisions use commercial Innovus as the target reference: candidate-vs-Innovus error must improve over remote-branch-vs-Innovus error, while the latest retained technical node remains the incremental comparison point.

| Signal | Remote baseline |
| --- | ---: |
| All-23 CTS-only | `23/23` |
| Innovus evaluation | `23/23` |
| Final skew MAE / RMSE / R2 | `19.0435 ps` / `24.7843 ps` / `-1.11395` |
| Final average latency MAE / RMSE / R2 | `59.0435 ps` / `69.9191 ps` / `0.535582` |
| Buffer count MAE / RMSE / R2 | `24.2609` / `44.2051` / `0.998859` |
| Clock wire MAE / RMSE / R2 | `1752.77 um` / `3459.75 um` / `0.975965` |
| Clock total cap MAE / RMSE / R2 | `0.234522 pF` / `0.388983 pF` / `0.999204` |
| Clock power MAE / RMSE / R2 | `0.0843435 mW` / `0.150243 mW` / `0.998365` |
| WNS MAE / RMSE / R2 | `355.043 ps` / `488.689 ps` / `0.9477` |
| DRC markers MAE / RMSE / R2 | `30.4783` / `72.5735` / `0.951025` |
| Raw skew / latency mean | `48.8696 ps` / `236.935 ps` |
| Raw buffer / wire mean | `684.87` / `13516.5 um` |
| Raw cap / clock power mean | `7.05226 pF` / `1.9545 mW` |
| Raw WNS / DRC markers mean | `5863.74 ps` / `298.435` |

## Benchmark Layer Policy

Evidence:

- `remote_branch_benchmark_design_filter_report.md`
- `summary/benchmark_layer_remote_baseline`
- `summary/benchmark_layer_45a_only_vs_remote`
- `summary/benchmark_layer_62c_only_vs_remote`
- `summary/benchmark_layer_45a_62c_vs_remote`
- `summary/remote_baseline_case_suitability_analysis`
- `summary/benchmark_layer_candidate_71a_vs_remote`
- `summary/remote_branch_benchmark_multiclock_deep_check`

All 23 cases must still complete CTS-only and Innovus evaluation. However, the primary single-clock CTS acceptance gate uses `core16`, while the full all-case result is treated as an `extended23` regression guard.

| Layer | Cases | Role |
| --- | ---: | --- |
| `core16` | `16` | Primary benchmark for single-clock CTS algorithm decisions. |
| `extended23` | `23` | Completion and catastrophic-regression guard; not the primary average. |
| `quarantine7` | `7` | Report separately because the cases are multi-clock/unconstrained-clock or dominated by one abnormal outlier. |
| `real_multiclock6` | `6` | Excluded from primary single-clock metric averages. |
| `outlier1` | `1` | Excluded from primary averages, still tracked as stress evidence. |
| `watchlist1` | `1` | Kept in `core16`, but monitored because an extra clock-like port exists without direct CK fanout. |

Cases excluded from the primary benchmark:

| Case | Reason from SDC/netlist/report scan |
| --- | --- |
| `iwls2005__ac97_ctrl` | `default.sdc` constrains only `clk_i`; top `bit_clk_pad_i` directly drives CK pins. |
| `iwls2005__mem_ctrl` | `default.sdc` constrains only `clk_i`; top `mc_clk_i` directly drives CK pins. |
| `iwls2005__pci` | `default.sdc` constrains only `wb_clk_i`; top `pci_clk_i` directly drives CK pins. |
| `iwls2005__usb_funct` | `default.sdc` constrains only `clk_i`; top `phy_clk_pad_i` directly drives CK pins. |
| `openroad__ethmac` | `default.sdc` constrains only `wb_clk_i`; top `mtx_clk_pad_i` and `mrx_clk_pad_i` directly drive CK pins. |
| `openroad__fifo` | `default.sdc` constrains only `wclk`; top `rclk` directly drives CK pins. |
| `iwls2005__vga_lcd` | Dominant outlier with generated/video clock-like behavior; it contributes a disproportionate share of remote-vs-Innovus wire/DRC error. |

`iwls2005__ss_pcm` remains in `core16` as a watchlist case: `pcm_clk_i` is clock-like, but the scanned top module shows no direct CK-pin fanout.

Suitability analysis confirms this partition from both directions:

- SDC/netlist evidence proves the six real multi-clock cases each have one constrained `create_clock` plus an unconstrained clock-like top port directly driving CK pins.
- `iwls2005__vga_lcd` is a dominant generated/video-clock-like outlier and alone contributes `29.6%` of all-23 clock-wire error and `42.4%` of all-23 DRC-marker error.
- Moving from `extended23` to `core16` reduces remote-vs-Innovus MAE by `47.4%` on buffer count, `51.8%` on clock wire, `32.9%` on clock cap, `25.7%` on clock power, and `64.9%` on DRC markers.

Remote branch error after layering:

| Layer | Skew MAE | Latency MAE | Buffer MAE | Wire MAE | Cap MAE | Power MAE | WNS MAE | DRC MAE |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `extended23` | `19.0435 ps` | `59.0435 ps` | `24.2609` | `1752.7747 um` | `0.2345 pF` | `0.0843 mW` | `355.0435 ps` | `30.4783` |
| `core16` | `16.1250 ps` | `56.8438 ps` | `12.7500` | `844.6147 um` | `0.1572 pF` | `0.0627 mW` | `311.3125 ps` | `10.6875` |
| `quarantine7` | `25.7143 ps` | `64.0714 ps` | `50.5714` | `3828.5690 um` | `0.4111 pF` | `0.1338 mW` | `455.0000 ps` | `75.7143` |

Future candidate decisions use this order:

1. Validate `23/23` completion.
2. Decide primary algorithm value from `core16` Innovus-reference error and case-level win/loss.
3. Use `extended23` only to reject catastrophic broad damage.
4. Report `quarantine7` separately; a change that only improves quarantine cases is not enough for a primary single-clock CTS technical node.

## Goal-Iteration Optimization Plan

### North-Star Goal

Use the benchmark evaluation to continuously move ECC CTS behavior toward Innovus-quality CTS metrics. The workflow still runs all 23 cases, but primary single-clock algorithm decisions use `core16`; `extended23` and `quarantine7` are guard and stress views. The long-term target is commercial-tool metric alignment; the near-term deliverable is a sequence of technically justified algorithm nodes that improve part of the benchmark without causing disproportionate damage elsewhere.

### Innovus-Reference Comparison Frame

Each candidate must report three views:

- Remote reference error: `remote-branch CTS result` versus `commercial Innovus result`.
- Candidate reference error: `current candidate CTS result` versus `commercial Innovus result`.
- Incremental delta: current candidate versus the latest retained technical node.

The technical-node acceptance criterion is:

`candidate-vs-Innovus error < remote-branch-vs-Innovus error`

on the `core16` primary benchmark for at least one primary metric, with case-by-case support and no disproportionate guard damage in `extended23`. The incremental view prevents stacking a new change that looks good against the remote branch but regresses the current retained branch without a good reason.

### Primary Metrics

Primary Innovus-alignment metrics:

- final skew;
- average latency;
- buffer count;
- clock wire length;
- clock total capacitance;
- clock power;
- WNS.

Guard metrics:

- max-cap violation nets;
- max-tran violation nets;
- DRC markers;
- all-23 CTS/evaluation completion;
- `extended23` and `quarantine7` catastrophic-regression checks;
- runtime or memory blow-up when materially changed.

Every candidate summary must include MAE, MSE, RMSE, R2 where available, raw mean deltas, case-level win/loss counts, changed-case coverage, and top outlier deltas. Final-stat deltas alone are not enough; analysis must connect them to topology, clustering, RC/cap, buffer placement, or endpoint-order evidence.

### Keep, Defer, or Revert Gate

Retain as a technical node when all conditions hold:

- all `23/23` benchmark cases complete CTS-only and Innovus evaluation;
- the candidate is based on a first-principles hypothesis, not on fitted constants or case-specific tuning;
- at least one `core16` primary metric has lower candidate-vs-Innovus error than remote-branch-vs-Innovus error;
- the improvement is supported by case-by-case analysis, not only by a small number of unrelated outliers or aggregate averaging;
- any regression is bounded, explainable, and strategically worth accepting;
- guard metrics do not show broad or severe degradation;
- the code remains an algorithmic behavior change, not an added external config knob.

If the algorithmic mechanism is reasonable but only affects a small number of cases, do not immediately promote it to a technical node. Perform one bounded follow-up attempt to make the mechanism broader, safer, or more targeted, then decide from the refreshed all-23 benchmark result.

Defer as an experimental node when the evidence is promising but incomplete, such as a partial workflow failure, a strong single-axis improvement with unresolved guard risk, or a trace-only result that needs a production formulation.

Reject and revert when any of these hold:

- the candidate requires magic thresholds, scalar correction, or case-specific behavior;
- the all-23 workflow fails without a clear infrastructure reason;
- the improvement is final-metric-only and lacks native ECC observability;
- regressions are larger or broader than the improvement;
- the result is equivalent to the retained node and adds code complexity.

Rejected candidates keep their NFS evidence and a compact PRD decision row. Production code must be restored before the next iteration.

### Periodic Retrospective

The task must periodically summarize the optimization history instead of only appending candidate rows. Do this after each retained technical node, after each cluster of failed attempts in the same direction, and before moving to a new phase.

Each retrospective must classify:

- successful paths that produced Innovus-reference improvement and why they worked;
- failed paths that looked plausible but regressed benchmark alignment or guards;
- closed directions that should not be retried without new evidence;
- reusable first-principles observations, such as H-tree level planning, clustering distribution, RC/cap/slew mismatch, endpoint-order movement, or buffering side effects;
- the next most promising direction, derived from actual benchmark gaps rather than from speculative tuning.

### Innovus Flow-Log Research Protocol

Every research or exploration round starts from actual Innovus benchmark evidence:

- commercial Innovus summary and timing reports;
- ECC-result Innovus evaluation summary and timing reports;
- available Innovus flow logs from CTS, route, post-route timing, DRC, and optimization stages;
- case-level deltas between remote branch, current candidate, and commercial Innovus.

When logs are available, summarize the Innovus flow into a compact algorithm-flow map before proposing ECC changes. The map should capture observable stages such as clock construction, buffering, routing mode, detail/global route behavior, SI/SPEF/parasitic extraction cues, timing repair, DRC repair, and any NDR or clock-net handling visible in the logs. Then compare that map with ECC's current flow and list concrete missing capabilities or weaker approximations.

Use a structured sequential-reasoning summary for these investigations:

`Innovus observation -> benchmark gap -> ECC missing capability -> first-principles hypothesis -> candidate mechanism -> expected metric movement -> risk/guard -> benchmark decision`

Do not store raw private reasoning. Store the concise evidence chain and the decision-relevant conclusion.

### Iteration Loop

1. Re-anchor: confirm remote baseline hash, current retained technical node, and commercial Innovus reference.
2. Benchmark-gap read: start from actual all-23 Innovus-reference gaps and identify which cases and metrics dominate the error.
3. Innovus-flow read: inspect available Innovus evaluation logs/reports and summarize the observable commercial-tool flow relevant to the gap.
4. Diagnose: choose one root-cause hypothesis from traces, Innovus logs, timing reports, and case-level outliers.
5. Implement: change algorithm behavior only; avoid new user-facing configs and tuned constants.
6. Build: compile the ECC binary used by the benchmark workflow.
7. Evaluate: run all-23 CTS-only and Innovus evaluation under the shared evidence root.
8. Summarize: generate benchmark summary, raw metric deltas, Innovus-reference errors, case win/loss tables, and outlier tables.
9. Decide: retain, defer, retry once with bounded adjustment, or reject using the gate above.
10. Retrospect: summarize what the latest success/failure teaches about the broader optimization path.
11. Record: update PRD and implementation notes with only compact conclusions and evidence paths.
12. Package: stage/commit retained technical nodes only after the phase gate asks for commit.

### Current Layered Remote-Baseline Ablation Reading

Evidence:

`summary/remote_branch_algo_ablation_vs_origin_65d670007`

Layered Innovus-reference evidence:

- `summary/benchmark_layer_45a_only_vs_remote`
- `summary/benchmark_layer_62c_only_vs_remote`
- `summary/benchmark_layer_45a_62c_vs_remote`

| Candidate | `core16` Innovus-reference movement | `extended23`/`quarantine7` reading | Current interpretation |
| --- | --- | --- | --- |
| `45a-only` | Improves skew `-2.9375 ps`, latency `-0.8438 ps`, wire `-73.6845 um`, cap `-0.0364 pF`, power `-0.0119 mW`, WNS `-102.1875 ps`, and DRC `-0.5625`; buffer MAE regresses `+0.7500`. | `extended23` latency regresses `+3.0870 ps` because quarantine cases move differently; `quarantine7` improves wire/cap/power but worsens skew/latency/WNS. | Valid primary single-clock direction. It is not primarily a Core16 latency problem after quarantine removal; the follow-up risk is buffer-count regression and quarantine instability. |
| `62c-only` | Exactly neutral on `core16`: all primary and guard deltas are `0`. | The small all-23 movement comes only from quarantine cases: skew `-0.1739 ps`, wire `-1.5830 um`, power `-0.0005 mW`, but latency/WNS/cap regress slightly. | Not a standalone technical node for primary single-clock benchmark. Keep only as historical stress evidence unless a later mechanism gives it Core16 effect. |
| `45a+62c` | Same as `45a-only` on `core16`; no additional primary effect. | Mirrors `45a-only`, with slightly worse `extended23` latency/WNS because quarantine cases move. | No demonstrated synergy. Package and future iterations should reason from the `45a` mechanism, not from the pair. |

### Phase Roadmap

| Phase | Goal | Exit condition |
| --- | --- | --- |
| `G0` Remote-baseline frame | Treat `origin/cts_refactor@65d67000753a` as active starting point, commercial Innovus as the target reference, and old baseline38 only as history. | PRD, summaries, and future candidate reports use the Innovus-reference gate. |
| `G1` Core16 split improvement | Preserve the `45a` Core16 skew/latency/wire/cap/power/WNS gains while explaining or reducing the buffer-count regression. | Candidate improves Core16 candidate-vs-Innovus error versus remote-branch-vs-Innovus error and avoids broad `extended23` or `quarantine7` damage. |
| `G2` H-tree topology generation | Generate better topology choices instead of selector-only retuning when existing front has no safe alternative. | New topology behavior has native observability and passes all-23 benchmark gate. |
| `G3` Clustering/outlier handling | Target cluster-final and endpoint-order outliers only after H-tree-level evidence is separated. | Case-level improvements do not create broad cap, power, or wire regression. |
| `G4` Timing semantics monitoring | Change timing semantics only if trace proves a deterministic FastSTA-vs-Innovus RC/cell-delay mismatch. | No scalar correction; any timing change is explained at RC/cap/slew/cell model level. |
| `G5` Packaging | Convert retained nodes into small commits with clean code and compact evidence links. | No temporary traces, large assets, or rejected experiments staged. |

## Latest Candidate Decisions

| Candidate | Decision | Evidence | Reason |
| --- | --- | --- | --- |
| `candidate_71a_core16_gate_refresh` | Completed as current cleaned-worktree refresh | `summary/candidate_71a_core16_gate_refresh`, `summary/benchmark_layer_candidate_71a_vs_remote` | Re-ran the current cleaned worktree after dropping later non-core/non-retained experiment behavior. It completes `23/23` CTS and evaluation and matches the layered `45a-only` core16 movement: skew, latency, wire, cap, power, WNS, and DRC improve versus remote; buffer-count alignment remains the main regression to attack next. |
| `candidate_72a_final_cluster_extra_cluster_guard` | Rejected as no-op and production code reverted | `summary/candidate_72a_final_cluster_extra_cluster_guard`, `summary/benchmark_layer_candidate_72a_vs_remote` | Tested a final-cluster selector that compared the retained Pareto split result with the longest-axis fallback and would reject extra clusters unless cap/wire improved. The all-23 result was identical to 71a because every case still selected the retained Pareto path, so the added selector complexity has no production value. |
| `candidate_73a_htree_sink_envelope_guard` | Rejected and production code reverted | `summary/candidate_73a_htree_sink_envelope_guard`, `summary/benchmark_layer_candidate_73a_vs_remote` | Tested a stricter sink-envelope selector that falls back to longest-axis clustering when the Pareto centers expand the H-tree envelope. It slightly improves `core16` buffer and DRC MAE versus 71a by `0.1875`, but regresses skew `+0.5 ps`, latency `+0.875 ps`, wire `+71.13 um`, cap `+0.0431 pF`, power `+0.0159 mW`, and WNS `+12.94 ps`; cap and power also become worse than the remote baseline. The guard is too blunt and discards the main 45a gains on large cases such as `des` and `jpeg`. |
| `candidate_74a_htree_precost_direction_precheck` | Closed as read-only direction evidence | `summary/candidate_74a_htree_precost_direction_precheck` | Re-read 73a through the layered gate. In the 9 `core16` cases where 73a selected the longest-axis fallback, buffer and DRC improve only `-0.3333` on average, while skew regresses `+0.8889 ps`, latency `+1.5556 ps`, wire `+126.4558 um`, cap `+0.0767 pF`, power `+0.0282 mW`, and WNS `+23 ps` versus 71a. Input-cluster envelope fallback is therefore not a production-safe pre-cost. |
| `candidate_74b_htree_branch_spread_selector_precheck` | Closed as read-only no-opportunity evidence | `summary/candidate_74b_htree_branch_spread_selector_precheck` | Re-ran the branch-spread selector precheck on retained 71a traces. Both the physical no-worse rule and the strict timing no-worse rule have `0/23` hit cases, so the current H-tree feasible front has no existing candidate that lowers weighted branch spread without delay, power, physical-depth, or buffer-count cost. Selector-only H-tree retuning remains closed. |
| `candidate_110a_deferred_boundary_polish_flow_contract` | Closed as implementation-contract evidence; no production code changed | `summary/candidate_110a_deferred_boundary_polish_flow_contract` | Rechecked the 109a rollback policy against current trace and route-tree timing observability. All `7/7` `core16` rollback source/target clusters are visible in route-tree timing, so the timing-rank signal is implementable. But all `7/7` rollback moves also change source or target cluster centers, and post-route sink-net-only rewiring is equivalent in `0/7` moves. Therefore the next valid production attempt must create preliminary timing before final cluster-buffer/HTree commit; do not implement a trace-driven or sink-net-only shortcut. |
| `candidate_110b_boundary_polish_timing_guidance_hook` | Completed as dormant production hook; not a benchmark node yet | code-only, covered by `FastClusteringSyntheticTest.BoundaryTimingGuidanceRejectsEarlySourceDepletion` | Added a native `BoundaryPolishTimingGuidance` input to fast clustering and implemented the 109a policy at the existing boundary-move decision point: if a locally improving boundary move would increase predicted arrival spread from the source cluster timing rank, reject it. Default guidance is null, so current production behavior remains unchanged until a preliminary-timing flow supplies ranks. This avoids trace-driven behavior and prepares the bounded 110c flow prototype. |
| `candidate_110c_preliminary_timing_boundary_polish_flow` | Deferred as experimental; code remains the current worktree prototype, not a promoted technical node | `summary/candidate_110c_preliminary_timing_boundary_polish_flow`, `summary/benchmark_layer_candidate_110c_vs_remote` | Implemented the bounded preliminary-timing flow: build a route-tree timing view before final cluster-buffer/HTree commit, derive native source-cluster arrival ranks, and feed them into the 110b boundary-polish guidance hook. It completes all-23 CTS/evaluation and improves every `core16` primary MAE versus the remote branch: skew `-1.375 ps`, latency `-3.46875 ps`, buffer `-2.3125`, wire `-110.005 um`, cap `-0.0535625 pF`, power `-0.0201562 mW`, WNS `-101.125 ps`, and DRC `-1.125`. However versus retained 82b it still gives back latency `+2.96875 ps` and WNS `+25.5625 ps`, with case-level damage on `systemcaes` and `tv80`. Treat the source-rank timing hook as valid evidence, but do not promote 110c until a critical-path/WNS-preserving formulation is proven. |
| `candidate_110d_pairwise_timing_boundary_polish_flow` | Rejected and production code reverted | `summary/candidate_110d_pairwise_timing_boundary_polish_flow`, `summary/benchmark_layer_candidate_110d_vs_remote` | Tested a bounded follow-up that used source-plus-target pairwise arrival-rank spread instead of source depletion alone. It improves `core16` skew further versus 110c and remote, but WNS alignment regresses too broadly: versus retained 82b, `core16` WNS MAE worsens by `+72.0625 ps`; versus 110c it worsens by `+46.5 ps`, with large losses on `wb_conmax`, `tv80`, `systemcaes`, and `systemcdes`. Endpoint-spread-only guidance is therefore insufficient; any next timing-aware boundary-polish candidate must include critical-path/WNS or stage-delay preservation instead of only max-minus-min spread reduction. |
| `candidate_110e_critical_path_boundary_guard_precheck` | Closed as read-only evidence; no production code changed | `summary/candidate_110e_critical_path_boundary_guard_precheck` | Rechecked 82b/110c/110d using the benchmark-layer WNS field (`wns_default_ns`) and joined post-route top timing-path cluster IDs with boundary-move source/target clusters. 110c improves skew/buffer/wire/cap/power/DRC versus 82b but worsens WNS by `+25.5625 ps`; 110d improves skew by `-1.5625 ps` but worsens WNS by `+72.0625 ps`. Four `core16` 110d cases are skew-win/WNS-loss conflicts, and two severe WNS regressions (`wb_conmax`, `tv80`) have no moved boundary source/target cluster on the parsed top timing paths. This means local source/target arrival-rank guards are not a native WNS guard. Do not implement 110e production logic until a stronger read-only join maps Innovus launch/capture data-path endpoints to ECC clock sink clusters and proves a no-fitted-threshold predictor. |
| `candidate_110f_data_path_endpoint_boundary_join` | Closed as read-only evidence; no production code changed | `summary/candidate_110f_data_path_endpoint_boundary_join` | Parsed Innovus top data paths into launch and capture clock endpoint clusters, then joined those clusters against boundary-move source/target IDs for 82b/110c/110d. On 110d moved `core16` cases, endpoint-cluster touch is still not selective: `endpoint_touch_predicate` has `4` WNS-loss true positives, `3` WNS-safe false positives, `2` WNS-loss false negatives, and `1` true negative. The two largest WNS regressions (`wb_conmax` `+511 ps`, `tv80` `+446 ps`) still have no launch or capture endpoint touch. Therefore launch/capture endpoint touch alone is not a production WNS guard. Timing-aware boundary polish stays deferred unless a stronger native signal combines endpoint-cluster touch with path-stage delay or data-path slack sensitivity without fitted thresholds. The next algorithmic iteration should return to retained-82b H-tree/split-edge residual work. |
| `candidate_111a_htree_split_edge_residual_queue_refresh` | Closed as read-only goal-stop analysis; no production code changed | `summary/candidate_111a_htree_split_edge_residual_queue_refresh` | Refreshed the retained-82b residual queue after 110f and inspected commercial Innovus flow logs for the top residual cases. Retained 82b remains closer than remote on every `core16` primary metric, and residual mass is still H-tree/split-edge dominated (`92.2366%`) rather than cluster-final (`7.76333%`). The largest split-materialization bucket (`des/jpeg`, `40.2938%`) remains queued but current alternate-axis, broad principal-axis, and minimal endpoint-transplant/repair shape families are closed by 99a/100a. The more actionable restart point is same-depth edge materialization (`systemcaes/wb_dma/tv80`, `24.0414%`) because 98a localizes the missing states to composition monotonic, source-fanout, and root-fanout pruning with zero sink-load rejection. This goal iteration stops here with no new production C++ change; if work restarts, begin with a read-only monotonic-boundary/source-fanout-safe same-depth edge generator precheck, not another selector or boundary-polish guard. |
| `candidate_74c_htree_buffer_frontier_gap` | Closed as read-only root-cause split for buffer regression | `summary/candidate_74c_htree_buffer_frontier_gap` | Joined 71a `core16` buffer-loss cases to the H-tree feasible front. Of the five buffer-loss cases, four have no lower-buffer candidate in the selected front, and `openroad__dynamic_node` has lower-buffer alternatives only with worse delay, power, and arrival-spread proxies. The retained front is therefore not hiding a safe lower-buffer selector; the next production direction must generate new topology alternatives or change cluster/local materialization with endpoint-order evidence. |
| `candidate_82b_local_recursive_uniform_split_packing` | Retained current technical node | `summary/candidate_82b_local_recursive_uniform_split_packing`, `summary/candidate_82a_local_split_giveback_attribution` | Preserves fanout-derived uniform local split depth while recursively repartitioning each local subset, recovering physical packing without reintroducing fitted parameters. It improves every `core16` primary MAE versus 81a and remains the current production direction. |
| `candidate_85b_local_split_physical_anchor_generation` | Rejected and production code reverted | `summary/candidate_85b_local_split_physical_anchor_generation` | Coordinate-wise median anchoring for local split buffers gives back 82b's endpoint-order and DRC gains. It worsens retained-82b `core16` skew, latency, wire, cap, power, WNS, and DRC; local anchors need endpoint-order/materialized-route invariants before reopening. |
| `candidate_86a_edge_materialization_next_axis_precheck` | Closed as read-only direction evidence | `summary/candidate_86a_edge_materialization_next_axis_precheck` | Edge-dominant residual cases are real, but relaxed lower-edge current-front candidates reduce H-tree depth and move the cost into local split-extra buffers, regressing effective split delay. Selector-only lower-edge recovery is closed. |
| `candidate_86b_same_depth_edge_materialization_generation_precheck` | Closed as read-only no-opportunity evidence | `summary/candidate_86b_same_depth_edge_materialization_generation_precheck` | On `systemcaes`, `wb_dma`, `tv80`, and `spi`, the retained 82b front exposes only the selected same-depth leaf-terminal sequence (`1;1;1 / 0;1;1` or `1;1 / 0;1`). There are no strict lower-edge hits preserving effective split delay, exact segment delay, branch-spread proxy, power, and materialized complexity. A real 86b would require new generation semantics, not another current-front selector. |
| `candidate_87a_compact_local_split_tree` | Rejected and production code reverted | `summary/candidate_87a_compact_local_split_tree` | Mixed-depth minimal-buffer local split generation reduces buffer/wire/cap/power but regresses `core16` skew, latency, WNS, and DRC. The main failure is `des`: collapsing local symmetry makes the global selector choose a shallower H-tree with many local split buffers, increasing effective split delay. Do not minimize local split buffers independently of endpoint-order/depth symmetry. |
| `candidate_88a_effective_split_delay_global_selection` | Rejected and production code reverted; kept as diagnostic evidence | `summary/candidate_88a_effective_split_delay_global_selection`, `summary/benchmark_layer_candidate_88a_vs_remote`, `summary/candidate_88a_effective_split_delay_global_selection/effective_selection_diagnosis` | Replacing global selection delay with effective split delay improves `core16` skew `-2.1875 ps` and latency `-4.375 ps` versus 82b, and improves skew/latency/wire/WNS versus remote. It is not production-safe because it regresses `core16` buffer `+4.0`, wire `+47.0 um`, cap `+0.1465 pF`, and power `+0.1193 mW` versus 82b; cap and power are also worse than remote. Effective split delay is a real signal, but it should be used only with exact-delay, cap/power, branch-spread, and complexity no-worse guards. |
| `candidate_88b_effective_timing_guarded_no_power` | Rejected and production code reverted; kept as guard-ablation evidence | `summary/candidate_88b_effective_delay_guarded_tiebreak_precheck`, `summary/candidate_88b_effective_timing_guarded_no_power`, `summary/benchmark_layer_candidate_88b_vs_remote`, `summary/candidate_88b_effective_timing_guarded_no_power/delta_vs_82b` | Strict guarded effective-delay selection has `0/16` `core16` hits. Removing only the power guard creates `14/16` core opportunities and the all-23 run improves core skew `-1.75 ps` and latency `-3.9375 ps` versus 82b, but it regresses wire `+11.9542 um`, cap `+0.0582 pF`, power `+0.0388 mW`, and WNS abs error `+16.1875 ps` versus 82b; cap and power also become worse than remote. Effective-delay tie-break without a native cap/power invariant is not safe. |
| `candidate_89a_effective_delay_power_source_precheck` | Closed as read-only root-cause evidence | `summary/candidate_89a_effective_delay_power_source_precheck` | The `14/16` no-power effective-delay opportunities preserve physical buffer count and sink-cap spread, but they mostly change segment patterns/cell strengths, often toward stronger `BUFX20H7L` choices. Mean trace topology power rises `+3.0519 uW`, while final `core16` cap and power abs error worsen versus 82b by `+0.0665 pF` and `+0.0444 mW`. The next production path must create a new lower-effective-delay geometry/materialization point; relaxing the power guard or selecting stronger segment cells is closed. |
| `candidate_90a_effective_delay_geometry_substitution_precheck` | Closed as read-only next-axis evidence | `summary/candidate_90a_effective_delay_geometry_substitution_precheck` | Converts the 88b stronger-cell delay gain into equivalent selected-cell geometry shortening. `13/14` changed-hit cases have no changed level with same-or-lower exact segment power, and `7/14` need more than half of the changed-level weighted branch-spread budget; `openroad__dynamic_node` exceeds the available weighted spread. Geometry/materialization is the right first-principles substitute, but the required movement is too strong for another broad level-length or branch-spread selector. |
| `candidate_91a_effective_delay_changed_level_scope` | Closed as read-only scope evidence | `summary/candidate_91a_effective_delay_changed_level_scope` | Classifies the 88b stronger-cell substitutions by H-tree level. Of `19` changed levels across `13` cases, `13` are leaf levels and `12` are terminal-branch-buffered; the dominant substitutions are `BUFX16H7L -> BUFX20H7L` (`13` levels) and `BUFX12H7L -> BUFX20H7L` (`5` levels). The next generator should therefore target leaf-side/near-leaf terminal materialization, not whole-trunk level planning. |
| `candidate_92a_leaf_terminal_materialization_current_front_precheck` | Closed as read-only no-selector evidence | `summary/candidate_92a_leaf_terminal_materialization_current_front_precheck` | Scans the retained 82b `core16` feasible H-tree front for lower-effective leaf/near-leaf terminal geometry substitutes. `14/16` core cases have lower-effective alternatives and `773` lower-effective candidates exist, but every lower-effective candidate fails topology power no-worse; only `1/16` cases has a relaxed leaf-terminal geometry hit and strict hits are `0`. Current-front leaf-terminal selection is therefore closed. The next step must add new localized generation/materialization semantics or first expose full segment-frontier choices with trace. |
| `candidate_93a_segment_frontier_trace_opportunity` | Closed as read-only no-opportunity evidence; default-off trace retained for diagnosis | `summary/candidate_93a_segment_frontier_trace_opportunity`, `summary/candidate_93a_segment_frontier_trace_refresh` | Adds default-off full segment-frontier trace under `ICTS_TIMING_TRACE_DIR`, exposing `all`, `terminal_branch_buffered`, and `terminal_leaf_unbuffered` entries. The trace-only refresh completed core16 coverage and was stopped before finishing quarantined `vga_lcd` to avoid large outlier assets. After matching same electrical boundary bins and same source-boundary switching-power state, `core16` has `0` hidden semantic patterns and `0` lower-delay/power-no-worse leaf-side opportunities. Existing segment-frontier selection/materialization is therefore closed; a future production change needs genuinely new geometry/topology generation or a different residual axis. |
| `candidate_94a_retained_82b_residual_reanchor` | Closed as read-only next-axis reanchor | `summary/candidate_94a_retained_82b_residual_reanchor` | Rejoins retained-82b layered benchmark movement, residual priority, same-tree root direction, and the closed 85b/86a/86b/92a/93a paths. Retained 82b remains closer than remote on every `core16` primary metric. Residual share is still mainly H-tree/split-edge (`92.24%`) rather than cluster-final (`7.76%`). The largest open axes are split-materialization endpoint-order-aware generation (`des`, `jpeg`, `40.29%`), new same-depth edge-materialization generation (`systemcaes`, `wb_dma`, `tv80`, `24.04%`), and smaller targeted endpoint-order cluster diagnosis (`17.34%`). Next production work must create new generation semantics or endpoint-order/path-context evidence; selector-only current-front retuning stays closed. |
| `candidate_95a_endpoint_split_order_precheck` | Closed as read-only feasibility filter | `summary/candidate_95a_endpoint_split_order_precheck` | Reconstructs retained-82b fast-clustering recursive split decisions and joins them with endpoint-order evidence. For the largest split-materialization residual axis (`des`, `jpeg`, `40.29%`), Innovus critical endpoints are separated by selected split decisions, and existing alternate split axes would change endpoint-side assignment in both cases, but those alternates are non-Pareto under native score/cap-spread/diameter/utilization guards. Therefore the next production candidate should not simply promote existing alternate split axes; it needs new split/materialization generation semantics or a new native invariant that preserves endpoint order without relaxing cap, wire, utilization, or diameter safety. |
| `candidate_96a_principal_axis_split_generation_precheck` | Closed as read-only generator feasibility evidence | `summary/candidate_96a_principal_axis_split_generation_precheck` | Simulates a third recursive split-generation axis from the principal direction of each sink subset while applying the retained native Pareto guard. It finds many score/physical Pareto opportunities in the top residual axes (`des/jpeg` have `84` nodes), proving the hypothesis is not empty, but endpoint-changing Pareto hits are `0` across all axes. This makes it a broad physical-cost generator, not endpoint-order-aware split-materialization closure. |
| `candidate_96b_principal_axis_split_generation` | Rejected and production code reverted | `summary/candidate_96b_principal_axis_split_generation`, `summary/benchmark_layer_candidate_96b_vs_remote`, `summary/candidate_96b_principal_axis_split_generation/delta_vs_82b` | Production implementation added principal-axis split generation as a third candidate and accepted it only under the existing no-worse guard. The all-23 workflow completed `23/23`, but relative to retained 82b on `core16` it regressed skew `+1.0625 ps`, latency `+9.46875 ps`, buffer `+3.8125`, cap `+0.0284375 pF`, power `+0.0250956 mW`, WNS abs error `+43.375 ps`, and DRC `+1.25`; only wire improved `-9.5799 um`. Against remote it still improves some metrics, but it is worse than the retained node and the regressions are broad, so the production code was restored to the 82b X/Y split generator. |
| `candidate_97a_same_depth_edge_generation_feasibility_precheck` | Closed as read-only feasibility filter | `summary/candidate_97a_same_depth_edge_generation_feasibility_precheck` | Joins the edge-dominant residual cases with full segment-frontier trace and native pruning contracts. The target residual share is `27.4109%`, but current-front strict-safe case count is `0/4` and strict same-state lower-buffer segment opportunity is also `0/4`. Relaxed lower-buffer rows exist in all target cases, but they change boundary, terminal, slew/cap, source-power, or fanout state; depth-3 all-unbuffered source fanout estimates also reach `64` against the max-fanout `32` guard. Since production H-tree search already uses `SegmentFrontierKind::kAll`, direct same-depth edge generation is under-evidenced. Next evidence should trace topology composition/sink-load rejection or switch to targeted endpoint-order cluster diagnosis. |
| `candidate_98a_topology_composition_rejection_trace_precheck` | Closed as read-only native-pruning evidence | `summary/candidate_98a_topology_composition_rejection_trace_precheck` | Adds default-off H-tree composition/pruning trace on the four 97a edge target cases. It shows selected-depth sink-load rejected entries are `0`, while composition monotonic rejects are `119080/255570` (`46.5939%`), composition fanout rejects are `55050/255570` (`21.5401%`), and root fanout rejects are `4158/21104` (`19.7024%`). Therefore direct edge-axis production remains closed until a new generator can create monotonic-boundary-safe and source/root-fanout-safe states while preserving effective delay, exact delay, cap/power, physical depth, and materialized complexity. The next iteration should first precheck the larger split-materialization endpoint-order axis (`des/jpeg`, `40.29%`) rather than forcing edge generation. |
| `candidate_99a_split_materialization_endpoint_order_generator_precheck` | Closed as read-only generator no-opportunity evidence | `summary/candidate_99a_split_materialization_endpoint_order_generator_precheck` | Rejoins the `des/jpeg` split-materialization residual bucket with 95a endpoint-side movement, 96a principal-axis generation, and retained-82b split-decision guards at the endpoint-changing nodes. The bucket is real (`40.2938%` residual share) and both cases have endpoint-changing existing alternates, but all endpoint-changing alternates are non-Pareto. `des` is blocked by score and total-child-diameter, while `jpeg` is blocked by total-child-diameter; the broad principal-axis generator has `84` physical Pareto nodes and `0` endpoint-changing Pareto nodes. Do not implement an existing alternate-axis selector or broad principal-axis generator; the next viable split/materialization work needs a new materialized-route shape with endpoint-side intent plus cap/wire, diameter, effective-delay, cap/power, and complexity no-worse evidence. |
| `candidate_100a_endpoint_materialized_route_shape_precheck` | Closed as read-only shape no-opportunity evidence | `summary/candidate_100a_endpoint_materialized_route_shape_precheck` | Tests a concrete follow-up generator family on `des/jpeg`: minimal endpoint-side transplant from the selected partition plus alternate-axis pair repair from the endpoint-changing alternate partition. It enumerates `6561` candidates per target case and preserves child counts, but native-safe generated shapes are `0/2`. The best `des` candidate still fails score and total-child-diameter, and the best `jpeg` candidate still fails total-child-diameter. This closes the current split/materialized-route shape family before production C++; next work should pivot to targeted endpoint-order cluster diagnosis unless a materially different generator with a stronger invariant is first proven read-only. |
| `candidate_101a_targeted_endpoint_order_cluster_diagnosis` | Closed as read-only cluster scope filter | `summary/candidate_101a_targeted_endpoint_order_cluster_diagnosis` | Joins the cluster/final-local residual subset with endpoint-order ranks, skew-hierarchy attribution, sink distributions, and boundary-move traces. The target subset has `6` cases and `17.3416%` residual share. `iwls2005__wb_conmax`, `iwls2005__spi`, and `openroad__spi` are cluster-dominant path-context cases with no endpoint-cluster boundary move; `iwls2005__usb_phy` and `openroad__gcd` have accepted boundary moves touching Innovus endpoint clusters; `iwls2005__systemcdes` is htree-dominant and should be deferred. Next precheck should focus on cluster endpoint path-context for the three cluster-dominant/no-boundary cases before production code. |
| `candidate_102a_cluster_endpoint_path_context_precheck` | Closed as read-only path-context evidence | `summary/candidate_102a_cluster_endpoint_path_context_precheck` | Checks `iwls2005__wb_conmax`, `iwls2005__spi`, and `openroad__spi` before production code. All three are cluster-buffer path-delay-context cases, not same-cluster local endpoint-rank cases. The observed local/path delay difference is large enough to cover the endpoint-order correction window, so the next question is cluster path-delay source rather than local endpoint rank. |
| `candidate_103a_cluster_buffer_path_delay_source` | Closed as read-only delay-source evidence | `summary/candidate_103a_cluster_buffer_path_delay_source` | Decomposes the 102a endpoint pair paths into cluster cell delay, cluster input wire, final wire, slew, and load context. All three target cases are dominated by cluster buffer cell delay: `iwls2005__wb_conmax` has `30.0714 ps` cell delta out of `30.2301 ps` cluster delta, `iwls2005__spi` has `7.5836 ps` out of `7.6011 ps`, and `openroad__spi` has `3.9044 ps` out of `3.9115 ps`. This closes final-wire RC and local rank as first-order explanations; the next precheck should test a native cluster-buffer load/slew predictor. |
| `candidate_104a_cluster_buffer_load_slew_predictor_precheck` | Closed as read-only native-predictor evidence | `summary/candidate_104a_cluster_buffer_load_slew_predictor_precheck` | Joins all clusters in the three 103a target cases with clustering-time cap/geometry proxies and actual cluster-buffer cell/load/slew context. `fast_total_cap_pf` is the strongest native predictor: endpoint direction hits are `3/3`, mean Pearson correlation to cluster cell delay is `0.915906`, minimum per-case correlation is `0.884798`, and mean correlation to driver total cap is `0.993293`. Input slew is much weaker as a cluster-shape function, so the next precheck should test total-cap/load-balanced clustering alternatives rather than fitting slew or delay. |
| `candidate_105a_cluster_total_cap_balance_candidate_precheck` | Closed as read-only no-production evidence | `summary/candidate_105a_cluster_total_cap_balance_candidate_precheck` | Enumerates single-move and single-swap candidates from the late endpoint cluster in the three 104a target cases. Pair-max total cap can improve in `2/3` cases, but strict native hits that also preserve pair total-cap sum and max geometry appear in only `1/3` cases, and only `iwls2005__wb_conmax` has endpoint-preserving strict hits. `iwls2005__spi` needs pair total-cap giveback, while `openroad__spi` has no pair-max improvement. Do not implement direct cluster total-cap balancing from this evidence; pivot to the endpoint-cluster boundary-move cases from 101a. |
| `candidate_106a_endpoint_cluster_boundary_move_source` | Closed as read-only source evidence | `summary/candidate_106a_endpoint_cluster_boundary_move_source` | Joins the two endpoint-cluster boundary-move cases with endpoint clusters, moved sinks, local proxy deltas, and cluster distribution context. `iwls2005__usb_phy` has a local-proxy-improving move that depletes the Innovus-min cluster and increases the max-minus-min endpoint proxy; `openroad__gcd` has a local-proxy-regressing move that depletes the Innovus-max cluster and reduces the max-minus-min endpoint proxy. This proves local proxy alone is the wrong guard. The next precheck must prove an ECC-native substitute for endpoint-spread direction before production code. |
| `candidate_107a_native_endpoint_spread_observability` | Closed as read-only observability evidence | `summary/candidate_107a_native_endpoint_spread_observability` | Tests whether ECC-native signals can replace the Innovus-labeled endpoint side from 106a. Current boundary-stage local proxy sign hits `0/2`, and source depletion alone is non-discriminating. After timing is materialized, moved-sink arrival rank and source-cluster mean-arrival/cluster-delay ranks hit `2/2`, including from `after_route_tree`. Therefore immediate boundary-polish production at the current fast-clustering stage is closed; the only viable follow-up is a deferred timing-aware boundary-polish feasibility precheck, not another local proxy guard. |
| `candidate_108a_deferred_timing_boundary_polish_feasibility` | Closed as read-only feasibility evidence | `summary/candidate_108a_deferred_timing_boundary_polish_feasibility` | Scans all retained-82b accepted boundary moves with route-tree source-cluster timing ranks. Boundary polish remains sparse: `21` moves across all 23 cases and `15` moves across `10` core16 cases. The timing/local conflict scope is real, with `9/15` core moves disagreeing between local proxy and route-tree endpoint-spread proxy. This supports a counterfactual deferred boundary-polish candidate, but not immediate production: the timing signal is currently observed after boundary polish, so the next step needs preliminary-timing/counterfactual flow evidence before changing CTS behavior. |
| `candidate_109a_counterfactual_deferred_boundary_polish` | Closed as read-only prototype gate | `summary/candidate_109a_counterfactual_deferred_boundary_polish` | Tests the narrow policy implied by 108a: keep existing accepted boundary moves unless a local-proxy-improving move depletes an early source cluster under route-tree timing rank. The counterfactual would roll back `7` core16 moves across `6` cases, covering `38.6821%` of retained-82b core16 average residual share. The rollback endpoint-spread proxy gain is `36867.4`, above local-proxy giveback `22791`. This is enough to implement a small deferred timing-aware boundary-polish prototype, but only as a benchmarked candidate; it is not yet production proof. |
| `candidate_77a_htree_materialization_pressure` | Closed as read-only next-axis evidence | `summary/candidate_77a_htree_materialization_pressure` | Joins the 75a materialized-buffer audit, 76a latency-preserving front scan, and layered 71a/75b/75c Innovus-reference deltas. `core16` has `12` split-materialization-dominant cases and `4` edge-materialization-dominant cases, but `0/16` current-front lower-edge hits once root/split latency proxies are preserved. The next H-tree candidate must generate new same-depth edge/split materialization alternatives instead of retuning the existing selector cost. |
| `candidate_78a_htree_split_preserving_opportunity` | Closed as read-only no-selector evidence | `summary/candidate_78a_htree_split_preserving_opportunity` | Scans retained 71a front for lower split-extra candidates. `core16` has six cases with unguarded lower-split alternatives, but all trade into more materialized edge buffers and worse delay or weighted-arrival proxy; every split-saving policy from relaxed delay/power/arrival through same-depth materialization-safe has `0/16` hits. This closes current-front split-saving selection and reinforces that the next production candidate must create a new H-tree materialization/topology shape. |
| `candidate_79a_local_split_minimality_audit` | Closed as read-only root-cause evidence | `summary/candidate_79a_local_split_minimality_audit` | Audits materialized `htree_split_buffer` trees against the max-fanout lower bound. Retained 71a has `core16` split buffers `942` versus lower bound `768`, with `174` reducible buffers concentrated in `openroad__jpeg`, `iwls2005__des`, and `openroad__dynamic_node`. The root cause is local split generation over-materialization, not a selector hidden in the current H-tree feasible front. |
| `candidate_80a_minimal_local_split_generation` | Rejected as over-aggressive production shape | `summary/candidate_80a_minimal_local_split_generation`, `summary/benchmark_layer_candidate_80a_vs_remote` | Replaces recursive equal-size local split generation with a fanout-minimal split tree and removes all 71a lower-bound excess on `core16` (`768/768`). It improves buffer, cap, power, and WNS versus remote, but regresses `core16` skew alignment versus remote by `+2.75 ps`; pure minimum split count is therefore too aggressive because it removes useful local path-depth symmetry. |
| `candidate_81a_uniform_depth_local_split_generation` | Retained as intermediate node; superseded by 82b | `summary/candidate_81a_uniform_depth_local_split_generation`, `summary/benchmark_layer_candidate_81a_vs_remote` | Builds local split leaves by max-fanout, then packs them through a uniform-depth split shape instead of recursively over-splitting every child. It cuts 71a `core16` split buffers from `942` to `638` while leaving only `28` lower-bound excess buffers, and remains closer to Innovus than remote on all primary `core16` metrics: skew `-0.875 ps`, latency `-5.8125 ps`, buffer `-1.6875`, wire `-29.6434 um`, cap `-0.019375 pF`, power `-0.005926 mW`, and WNS `-115.1875 ps` MAE delta versus remote. Versus 71a it improves latency, buffer, and WNS but gives back some skew/wire/cap/power; 82b keeps this shape and fixes the leaf-packing giveback. |
| `candidate_82a_local_split_giveback_attribution` | Closed as read-only 81a follow-up evidence | `summary/candidate_82a_local_split_giveback_attribution` | Joins 71a/80a/81a local split materialization, same-sink role-delay movement, unique-net physical movement, and Innovus-reference deltas. It shows 81a's giveback is concentrated in large split-dominant cases and correlates with physical htree-split routing and split-delay spread, so the next production attempt should improve local leaf packing/placement rather than minimize split count further. |
| `candidate_82b_local_recursive_uniform_split_packing` | Retained as current technical node candidate | `summary/candidate_82b_local_recursive_uniform_split_packing`, `summary/benchmark_layer_candidate_82b_vs_remote` | Keeps 81a's fanout-derived uniform-depth split shape, but fills each internal shape by re-sorting and repartitioning its local load subset instead of using a one-time global leaf order. It completes `23/23` CTS/evaluation and improves every `core16` primary MAE versus 81a: skew `-0.25 ps`, latency `-0.625 ps`, wire `-72.658 um`, cap `-0.0169375 pF`, power `-0.0025625 mW`, and WNS `-11.5 ps`, with buffer unchanged. It is also closer than remote on all `core16` primary metrics. |
| `candidate_83a_82b_residual_htree_buffer_frontier_precheck` | Closed as read-only no-selector evidence | `summary/candidate_83a_82b_residual_htree_buffer_frontier_precheck` | Rechecks 82b residual buffer-loss cases against the current H-tree feasible front. In `core16` buffer-loss cases, only `openroad__uart` has a lower-buffer candidate, and that candidate adds `32.6796 ps` delay; delay+power no-worse and strict no-worse hit counts are `0`. Lower-buffer selector-only recovery remains closed after 82b. |
| `candidate_83b_82b_residual_htree_selector_precheck` | Closed as read-only no-selector evidence | `summary/candidate_83b_82b_residual_htree_selector_precheck` | Rechecks branch-length, physical-load, and exposure selector rules on the 82b trace. All tested rules have `0/23` hits, so selector-only H-tree retuning remains closed. |
| `candidate_83c_82b_level_length_coverage_audit` | Closed as read-only diagnostic evidence | `summary/candidate_83c_82b_level_length_coverage_audit` | Selected H-tree levels still under-cover branch maxima after 82b (`22/23` cases, `32/44` selected levels), and weighted undercoverage correlates with endpoint-order correction (`r=0.693`). However 42j/42k/42l/54a already closed global or broad max-branch production use, so undercoverage remains diagnostic only unless a new native no-regression invariant is found. |
| `candidate_84a_82b_core16_residual_priority_precheck` | Closed as read-only residual prioritization | `summary/candidate_84a_82b_core16_residual_priority_precheck` | Ranks retained 82b residuals against commercial Innovus on `core16`. The dominant residual mass is `iwls2005__des` (`20.80%` average share, max metric `clock_power_mw`) and `openroad__jpeg` (`19.49%`, max metric `clock_wire_um`), followed by edge/materialization-sensitive `systemcaes`, `dynamic_node`, and `wb_dma`. Joined 83a/83c fields keep lower-buffer selector recovery and broad max-branch planning closed; the next attempt must change bounded generation/materialization behavior. |
| `candidate_85a_next_generation_axis_precheck` | Closed as read-only next-axis decision | `summary/candidate_85a_next_generation_axis_precheck` | Joins 84a residual ranking, 77a materialization pressure, and 82a local-split giveback. `core16` split-materialization-dominant cases account for `69.57%` residual-share sum, while edge-materialization-dominant cases account for `27.41%`. The strongest next production axis is `local_split_physical_anchor_generation`: `des` and `jpeg` alone contribute `40.29%` residual-share sum and both are split-dominant physical cap/power/wire cases. Same-depth edge materialization is deferred as the next axis for `systemcaes`, `wb_dma`, and `tv80`. |
| `candidate_85b_local_split_physical_anchor_generation` | Rejected and production code reverted | `summary/candidate_85b_local_split_physical_anchor_generation`, `summary/benchmark_layer_candidate_85b_vs_remote`, `summary/candidate_85b_local_split_physical_anchor_generation/decision_compare_85b_vs_82b` | Tested coordinate-wise median anchoring for local split-buffer centers inside the retained 82b recursive uniform-depth shape. It completes `23/23` CTS and `47/47` conversion/evaluation and is still better than remote on most `core16` primary MAEs, but it regresses versus retained 82b on every `core16` primary metric except buffer count: skew `+0.5625 ps`, latency `+1.0625 ps`, wire `+12.5256 um`, cap `+0.010625 pF`, power `+0.007373 mW`, WNS `+13.1875 ps`; DRC worsens by `+1.375` versus 82b and by `+0.5` versus remote. The L1-median anchor idea is therefore not a production-safe local split physical anchor. |
| `candidate_86a_edge_materialization_next_axis_precheck` | Closed as read-only edge-generation contract | `summary/candidate_86a_edge_materialization_next_axis_precheck` | Confirms the deferred edge axis is real but not selector-ready. Four edge-dominant `core16` cases contribute `27.4109%` retained-82b residual share; `systemcaes`, `wb_dma`, and `tv80` have relaxed lower-edge candidates, but root/effective-delay, latency-preserving, and same-depth hit counts are all `0`. Level trace shows the relaxed candidates reduce H-tree depth by `1`, increase split-extra buffers by `19` to `57`, and regress effective split delay by `38.6` to `87.7 ps`. The next production attempt must generate a new same-depth edge-materialization point and pre-reject it unless native delay, weighted branch-arrival, cap/power, physical depth, and materialized complexity are no worse. |
| `remote_branch_algo_ablation_vs_origin_65d670007` | Closed as active baseline re-anchor | `summary/remote_branch_algo_ablation_vs_origin_65d670007`, `summary/benchmark_layer_*_vs_remote` | Re-anchors the optimization gate to the remote branch before unpushed changes. Layering shows `45a` is the meaningful Core16 direction, while `62c` has no Core16 standalone effect and mainly moves quarantine cases. |
| `candidate_40a_path_safe_local_split_axis` | Rejected | `summary/candidate_40a_path_safe_local_split_axis` | Improved skew, wire, and power, but regressed latency, cap, DRV, and WNS. |
| `candidate_40b_path_safe_local_split_axis_span_guard` | Rejected and production code reverted | `summary/candidate_40b_path_safe_local_split_axis_span_guard` | Added physical span guards but still regressed latency/cap/WNS alignment and raw DRV/WNS on affected cases. |
| `candidate_42a_htree_frontier_opportunity_precheck` | Closed as read-only evidence | `summary/candidate_42a_htree_frontier_opportunity_precheck` | Existing H-tree selection front has no strict no-worse lower-power/cap selector-only replacement. |
| `candidate_42b_shrink_only_topology_balance` | Rejected and production code reverted | `summary/candidate_42b_shrink_only_topology_balance` | Improved buffer-count/wire alignment and some same-tree residuals, but regressed final skew/latency MAE and clock cap/power. |
| `candidate_42c_preserve_leaf_cluster_centers` | Rejected and production code reverted | `summary/candidate_42c_preserve_leaf_cluster_centers` | Improved clock cap/power/wire, but regressed final skew/latency MAE and same-tree timing closure. |
| `candidate_42e_multi_topology_pareto_opportunity` | Closed as read-only evidence | `summary/candidate_42e_multi_topology_pareto_opportunity` | Historical topology candidates contain final-safe oracle opportunities, but no tested ECC-visible Pareto rule found a non-empty false-positive-free selector. |
| `candidate_43a_cluster_root_policy_precheck` | Closed as read-only evidence | `summary/candidate_43a_cluster_root_policy_precheck` | L1-median cluster roots reduce final-cluster Manhattan proxy in `23/23` cases, proving a real physical opportunity. |
| `candidate_43a_l1_median_cluster_root` | Rejected and production code reverted | `summary/candidate_43a_l1_median_cluster_root` | Improved latency, buffer count, clock cap, and power, but regressed final skew MAE and clock wire-length alignment. |
| `candidate_45a_pareto_split_axis` | Retained as current core-QoR candidate | `summary/candidate_45a_pareto_split_axis` | Improves all six core all-23 alignment metrics and all six core raw QoR means, with no core regression; guard side has a small DRV/DRC residual tracked below. |
| `candidate_45b_pareto_split_axis_cap_guard` | Rejected and production code reverted | `summary/candidate_45b_pareto_split_axis_cap_guard` | Direct child cap guard is not a valid invariant: it regressed skew and latency alignment and worsened DRV guard metrics. |
| `candidate_45c_leaf_envelope_split_axis` | Rejected and production code reverted | `summary/candidate_45c_leaf_envelope_split_axis` | Leaf-envelope guard improved same-tree closure and skew, but regressed latency, cap, and power core metrics. |
| `candidate_45d_leaf_tail_split_axis` | Rejected and production code reverted | `summary/candidate_45d_leaf_tail_split_axis` | Leaf-tail guard further improved skew, but still regressed latency alignment/raw latency and guard metrics, so it violates the no-deterioration rule. |
| `candidate_46a_guard_residual_scope` | Closed as read-only evidence | `summary/candidate_46a_guard_residual_scope` | New 45a max-cap/max-tran residuals classify as design-signal nets, not clock CTS nets; no production CTS guard should be added for this post-route noise. |
| `candidate_47a_timing_semantic_actionability` | Closed as read-only evidence | `summary/candidate_47a_timing_semantic_actionability` | Current same-tree residual is dominated by slew/driver-cap cell context, and the post-context residual is too small to justify scalar timing, threshold, DMP, or wire/RC correction. |
| `candidate_42f_htree_generation_precheck` | Closed as read-only evidence | `summary/candidate_42f_htree_generation_precheck` | H-tree split/branch spread remains a strong structural signal, but strict no-worse existing-front alternatives are not sufficient for a selector-only production change. |
| `candidate_42f_htree_branch_spread_selector_precheck` | Closed as read-only evidence | `summary/candidate_42f_htree_branch_spread_selector_precheck` | Same/less complex branch-spread selector found `0/23` strict hits on the retained 45a trace, so P42 must generate new topology behavior rather than reselect old candidates. |
| `candidate_42f_local_sibling_balance` | Rejected and production code reverted | `summary/candidate_42f_local_sibling_balance` | Improved skew and clock-wire alignment versus 45a, but regressed latency, buffer-count, clock-cap, and clock-power alignment, so it violates the clarified no-deterioration gate. |
| `candidate_43b_cluster_root_no_worse_precheck` | Closed as read-only evidence | `summary/candidate_43b_cluster_root_no_worse_precheck` | Guarded median cluster roots exist in `22/23` cases and `4/5` cluster-final cases, but the rule requires full production validation because it changes H-tree inputs broadly. |
| `candidate_43b_source_no_worse_median_cluster_root` | Rejected and production code reverted | `summary/candidate_43b_source_no_worse_median_cluster_root` | Improved buffer-count alignment versus 45a, but regressed skew, latency, clock-wire, clock-cap, and clock-power alignment, so it fails the no-deterioration gate. |
| `candidate_42g_split_local_generation_precheck` | Closed as read-only evidence | `summary/candidate_42g_split_local_generation_precheck` | Retained 45a high-skew H-tree cases mostly have active local split and low internal-level proxy share, so the next H-tree attempt should test local split generation rather than selector-only retuning. |
| `candidate_42g_recursive_local_split_generation` | Rejected and production code reverted | `summary/candidate_42g_recursive_local_split_generation` | Improved skew and buffer-count alignment versus 45a, but regressed latency, clock-wire, clock-cap, and clock-power alignment plus raw wire/cap/power, so it fails the no-deterioration gate. |
| `candidate_42h_local_split_generation_delta_audit` | Closed as read-only evidence | `summary/candidate_42h_local_split_generation_delta_audit` | 42g has `9/23` core-fit no-regression cases but raw physical cost regresses in `11/23`; any recovery needs a native wire/cap/power no-worse invariant. |
| `candidate_42i_guarded_recursive_local_split_generation` | Rejected and production code reverted | `summary/candidate_42i_guarded_recursive_local_split_generation` | Guarding recursive split by local physical no-worse proxies reduced it to a near-45a equivalent; latency alignment improved by only `0.0435 ps` while wire/cap alignment and raw latency/wire had tiny regressions. |
| `candidate_43c_cluster_final_outlier_audit` | Closed as read-only evidence | `summary/candidate_43c_cluster_final_outlier_audit` | Cluster-final cases are real, but the refreshed all-23 trace still shows H-tree/split contribution dominates the final skew gap. |
| `candidate_43c_per_cluster_buffer_delay_choice` | Rejected and production code reverted | `summary/candidate_43c_per_cluster_buffer_delay_choice` | Liberty delay-cost per-cluster buffer sizing improved skew MAE and buffer-count MAE, but selected `BUFX20H7L` for nearly all cluster buffers and regressed latency, wire, cap, and power, so cluster sizing is not a safe standalone next axis. |
| `candidate_42j_htree_split_topology_next_axis` | Closed as read-only evidence | `summary/candidate_42j_htree_split_topology_next_axis` | Selector-only H-tree retuning remains empty on retained 45a: all tested native no-worse rules have `0/23` hits, so the next lever must change generation/level planning. |
| `candidate_42j_htree_level_length_coverage_audit` | Closed as read-only evidence | `summary/candidate_42j_htree_level_length_coverage_audit` | Selected H-tree levels under-cover actual branch maxima in `22/23` cases, proving a real level-length modeling issue, but not yet a safe production rule. |
| `candidate_42j_max_branch_length_level_plan` | Rejected and production code reverted | `summary/candidate_42j_max_branch_length_level_plan` | Global max-branch level planning improved several metrics versus baseline38, but regressed skew/latency/cap/power relative to retained 45a, so it is too broad. |
| `candidate_42k_mean_plus_max_branch_level_plan_frontier` | Rejected and production code reverted | `summary/candidate_42k_mean_plus_max_branch_level_plan_frontier` | Exposing mean and max-branch level plans to the native frontier improved latency/buffer/wire versus 45a, but regressed skew, cap, power, and WNS; the max-branch opportunity needs a narrower invariant. |
| `candidate_42l_max_branch_char_coverage_only` | Rejected and production code reverted | `summary/candidate_42l_max_branch_char_coverage_only` | Narrow branch-max characterization coverage improved latency and clock-wire MAE versus 45a, but regressed skew, cap, power, WNS, and max-cap guard metrics, so it is not Pareto-safe under the clarified benchmark gate. |
| `candidate_48a_cluster_buffer_legality_precheck` | Closed as read-only evidence | `summary/candidate_48a_cluster_buffer_legality_precheck` | Per-cluster buffer upsizing is not electrically justified: BUFX8 Liberty max-cap is `0.325346 pF`, while the maximum traced cluster load is `0.0214115 pF`, only `6.58%` of that limit. |
| `candidate_49a_timing_cell_context_counterfactual_precheck` | Closed as read-only evidence | `summary/candidate_49a_timing_cell_context_counterfactual_precheck` | Native cell-delay substitutions already present in trace do not justify a production timing change: table-at-Ceff only improves retained-45a cell-stage MAE by `0.0218 ps` and same-tree cell-skew contribution by `0.0145 ps`, while table-at-driver-total-cap worsens both. |
| `candidate_50a_clarified_gate_candidate_frontier` | Closed as read-only evidence | `summary/candidate_50a_clarified_gate_candidate_frontier` | Re-audited `75` already-run non-retained all-23 candidates under the clarified partial-improvement/no-regression gate; `0` pass core-alignment-only, core+raw, or core+guard gates relative to retained 45a. |
| `candidate_51a_split_decision_trace_refresh` | Closed as read-only evidence | `summary/candidate_51a_split_decision_trace_refresh`, `summary/candidate_51a_split_decision_trace_analysis` | Adds default-off recursive split-decision trace for retained 45a; accepted alternate-axis splits are sparse (`747/11133`) and correlate with endpoint correction/H-tree path deltas, giving the next iteration split-level evidence without changing production behavior. |
| `candidate_52a_rms_level_length_plan` | Rejected and production code reverted | `summary/candidate_52a_rms_level_length_plan` | RMS level-length planning improved skew MAE by `0.087 ps` and clock-wire MAE by `2.07 um`, but regressed latency MAE by `2.89 ps` plus buffer/cap/power, so it fails the clarified partial-improvement/no-regression gate. |
| `candidate_53a_power_preserving_branch_arrival_overlay` | Rejected and production code reverted | `summary/candidate_53a_power_preserving_branch_arrival_overlay` | Power-preserving branch-arrival overlay improved cap/power alignment, but regressed skew MAE by `2.52 ps`, latency MAE by `15.28 ps`, and timing guard counts. |
| `candidate_54a_leaf_max_branch_level_plan` | Rejected and production code reverted | `summary/candidate_54a_leaf_max_branch_level_plan` | Leaf-only max-branch level planning was an all-23 no-op versus retained 45a on final metrics and guards, with only runtime noise; it produces no benchmark optimization. |
| `candidate_55a_disable_boundary_cap_polish` | Rejected and production code reverted | `summary/candidate_55a_disable_boundary_cap_polish` | Disabling boundary-load cap polish improved skew/buffer/wire/cap/power plus WNS/TNS, but regressed latency by `1.17 ps` and small DRV/DRC guards; keep only as a strong follow-up signal. |
| `candidate_56a_boundary_local_proxy_guard` | Rejected and production code reverted | `summary/candidate_56a_boundary_local_proxy_guard` | Guarding boundary moves by local routing-cap proxy no-worse still regressed skew, latency, cap, power, and DRV while only improving wire; simple local proxy guard is not sufficient. |
| `candidate_57a_boundary_move_trace_refresh` | Closed as read-only evidence | `summary/candidate_57a_boundary_move_trace_refresh`, `summary/candidate_57a_boundary_move_trace_analysis` | Adds default-off accepted boundary-move trace. The trace is QoR-neutral versus 45a and shows only `21` accepted moves across `13/23` cases, proving boundary polish is sparse but physically visible. |
| `candidate_58a_boundary_pareto_geometry_cap` | Rejected and production code reverted | `summary/candidate_58a_boundary_pareto_geometry_cap` | Requiring boundary moves to be geometry- and cap-penalty Pareto no-worse improved cap/power slightly but regressed skew, latency, wire, and DRV versus retained 45a. |
| `candidate_59a_binary_spatial_htree_branching` | Rejected and production code reverted | `summary/candidate_59a_binary_spatial_htree_branching` | Forcing global binary spatial H-tree branching did not pass the all-23 CTS-only gate: only `19/23` cases reached normal runtime output, and the resumed large-case run reached about `60 GB` RSS before abort. |
| `candidate_60a_boundary_nearest_root_move_guard` | Rejected and production code reverted | `summary/candidate_60a_boundary_nearest_root_move_guard` | Nearest-root boundary move guarding completed all-23 and improved skew MAE by `0.174 ps`, but regressed latency by `0.478 ps`, wire by `2.62 um`, cap by `0.00717 pF`, and WNS alignment by `44.9 ps`. |
| `candidate_61a_boundary_endpoint_context_precheck` | Closed as read-only evidence | `summary/candidate_61a_boundary_endpoint_context_precheck` | Move-level endpoint context shows boundary moves include final max endpoints and middle-rank sinks; local source/target predicates cannot reliably preserve endpoint order. Pause standalone boundary-polish guards unless timing/path context is exposed before the move decision. |
| `candidate_61b_weighted_branch_arrival_selector_precheck` | Closed as read-only evidence | `summary/candidate_61b_weighted_branch_arrival_selector_precheck` | Weighted branch-arrival selector precheck found `0/23` same-or-less-complex lower-proxy substitutions under the retained 45a frontier; selector-only H-tree retuning remains closed. |
| `candidate_61c_analytical_htree_mode_precheck` | Closed as workflow failure evidence | `summary/candidate_61c_analytical_htree_mode_precheck` | Temporary analytical-H-tree wrapper produced `23/23` CTS artifact failures and no evaluation summary. Treat this as an unusable precheck run, not a production algorithm signal. |
| `candidate_62a_htree_local_split_pareto_axis` | Rejected and production code reverted | `summary/candidate_62a_htree_local_split_pareto_axis` | Completed all-23 and improved retained-45a core alignment/raw QoR on skew, latency, wire, cap, and power with buffer unchanged, but failed the no-deterioration gate due DRV max-cap alignment and DRC marker regressions. |
| `candidate_62b_route_envelope_local_split_axis` | Rejected and production code reverted | `summary/candidate_62b_route_envelope_local_split_axis` | Added a worst route-envelope guard to 62a's local split-axis generation, but the all-23 result was exactly equivalent to 62a and still failed DRV/DRC guard metrics. |
| `candidate_62c_spread_guarded_local_split_axis` | Retained as current incremental candidate | `summary/candidate_62c_spread_guarded_local_split_axis` | Adds root-wire, child-diameter, and route-envelope spread no-worse guards to local split-axis generation. It improves retained-45a latency, wire, cap, power, and WNS alignment/raw QoR with no core or guard regression. |
| `candidate_63a_anchor_aware_local_split` | Rejected and production code reverted | `summary/candidate_63a_anchor_aware_local_split` | Passing relative to 45a is not enough for an incremental keep: versus current retained 62c, the only moved case (`iwls2005__pci`) regressed latency, wire, cap, power, and WNS. |
| `candidate_64b_local_split_generation_scope_trace_refresh` | Closed as read-only evidence | `summary/candidate_64b_local_split_generation_scope_trace_refresh` | Adds scoped local split-generation trace and is QoR-equivalent to 62c; materialization trace showed aggregate child-diameter and route-envelope-sum relaxations are the next local-split axis to test. |
| `candidate_65a_relaxed_aggregate_local_split_guard` | Rejected and production code reverted | `summary/candidate_65a_relaxed_aggregate_local_split_guard` | Dropping both aggregate child-diameter and route-envelope-sum guards improved skew MAE by `0.304 ps` versus 62c, but regressed latency, wire, cap, and power alignment. |
| `candidate_66a_drop_child_total_local_split_guard` | Rejected and production code reverted | `summary/candidate_66a_drop_child_total_local_split_guard` | Keeping route-envelope sum but dropping total child diameter improved skew, but still regressed latency/cap alignment and added a DRC marker on a moved case. |
| `candidate_67a_multi_child_total_diameter_relaxation` | Rejected and production code reverted | `summary/candidate_67a_multi_child_total_diameter_relaxation` | Multi-child-only relaxation isolated movement to `iwls2005__vga_lcd` and improved skew/latency/cap/WNS/DRV, but clock wire length regressed by about `1.37 um` all-23 mean/MAE versus 62c. |
| `candidate_68a_weighted_branch_arrival_exposure` | Rejected and production code reverted | `summary/candidate_68a_weighted_branch_arrival_exposure` | Replaced branch-arrival exposure selection with a topology-multiplicity-weighted proxy, but all checked all-23 final metrics were exactly equivalent to retained 62c, so it produced no benchmark improvement. |
| `candidate_69a_branch_exposure_pareto_dimension` | Rejected and production code reverted | `summary/candidate_69a_branch_exposure_pareto_dimension` | Promoted branch-arrival exposure to a physical Pareto frontier dimension, but all checked all-23 final metrics were exactly equivalent to retained 62c, so it produced no benchmark improvement. |
| `candidate_70a_materialized_local_split_wire_guard` | Rejected and production code reverted | `summary/candidate_70a_materialized_local_split_wire_guard` | Added a recursive materialized local-tree wire no-worse guard before relaxing 62c's total-child-diameter invariant; CTS succeeded, but one Innovus evaluation crashed and the 22-case summary already showed skew gains with wire/WNS/DRV regressions. |

Key 40b deltas versus baseline38:

| Metric | Baseline MAE | 40b MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `16.6087 ps` | `-0.0435 ps` |
| Latency | `48.2174 ps` | `48.3043 ps` | `+0.0869 ps` |
| Clock wire | `1757.39 um` | `1756.99 um` | `-0.4059 um` |
| Clock total cap | `0.26713 pF` | `0.26987 pF` | `+0.00274 pF` |
| Clock power | `0.119869 mW` | `0.119347 mW` | `-0.000522 mW` |
| WNS alignment | `285.609 ps` | `289.087 ps` | `+3.478 ps` |

40b improved a few metrics, but the regressions violate the current acceptance rule. The reports are retained as negative evidence; the production code path was restored to the baseline local split behavior.

Key 42b deltas versus baseline38:

| Metric | Baseline MAE | 42b MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `17.0870 ps` | `+0.4348 ps` |
| Latency | `48.2174 ps` | `49.3261 ps` | `+1.1087 ps` |
| Buffer count | `24.2609` | `22.1739` | `-2.0870` |
| Clock wire | `1757.39 um` | `1667.57 um` | `-89.82 um` |
| Clock total cap | `0.26713 pF` | `0.314783 pF` | `+0.047653 pF` |
| Clock power | `0.119869 mW` | `0.234539 mW` | `+0.11467 mW` |

42b confirms topology-space changes can move wire/buffer metrics, but pure shrink-only balance is too broad: fewer buffers and shorter wire can still force larger clock-buffer area/cap on large cases. The production diff was reverted and the next H-tree attempt must preserve a clearer physical invariant.

Key 42c deltas versus baseline38:

| Metric | Baseline MAE | 42c MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `17.0000 ps` | `+0.3478 ps` |
| Latency | `48.2174 ps` | `48.6957 ps` | `+0.4783 ps` |
| Buffer count | `24.2609` | `24.2609` | `0.0000` |
| Clock wire | `1757.39 um` | `1743.37 um` | `-14.02 um` |
| Clock total cap | `0.26713 pF` | `0.264348 pF` | `-0.002782 pF` |
| Clock power | `0.119869 mW` | `0.119477 mW` | `-0.000392 mW` |

42c confirms leaf-center preservation is a real cap/power/wire lever, but using it alone sacrifices skew/latency. The direction is not accepted unless combined with a native balancing mechanism that prevents endpoint-order regression.

Key 42e readings:

| Signal | Result |
| --- | ---: |
| Historical candidate/case pairs audited | `138` |
| Pairs with at least one final metric improvement | `74` |
| Pairs with improvement and no key-metric regression | `6` |
| Safe cases by final metrics | `4` |
| Tested ECC-visible Pareto rules with no false positives | `0` |

42e confirms that topology-space changes have real upper-bound opportunity, but the currently available historical alternatives cannot be safely selected with the tested ECC-native rules. The final-metric oracle uses Innovus evaluation outcomes and is not production logic. The next topology effort must either generate new ECC-native alternatives with clearer no-worse invariants or move back to timing semantics / targeted cluster outliers.

Key 43a deltas versus baseline38:

| Metric | Baseline MAE | 43a MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `19.6087 ps` | `+2.9565 ps` |
| Latency | `48.2174 ps` | `47.0652 ps` | `-1.1522 ps` |
| Buffer count | `24.2609` | `22.7391` | `-1.5218` |
| Clock wire | `1757.39 um` | `2111.29 um` | `+353.9 um` |
| Clock total cap | `0.26713 pF` | `0.257217 pF` | `-0.009913 pF` |
| Clock power | `0.119869 mW` | `0.0969848 mW` | `-0.0228842 mW` |

43a validates the user's relaxed acceptance framing in the negative direction: it has real partial wins, but the final skew and wire-length regressions are key-metric regressions, so the change is not acceptable as a standalone production algorithm. Median cluster roots remain useful evidence for cap/power/latency, but only as a future sub-mechanism guarded by endpoint-order or H-tree branch-balance improvements.

Key 45a deltas versus baseline38:

| Metric | Baseline MAE | 45a MAE | Delta | Raw mean delta |
| --- | ---: | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `15.4348 ps` | `-1.2174 ps` | `-0.001478 ns` |
| Latency | `48.2174 ps` | `46.8478 ps` | `-1.3696 ps` | `-0.002413 ns` |
| Buffer count | `24.2609` | `23.6957` | `-0.5652` | `-0.5652` |
| Clock wire | `1757.39 um` | `1640.46 um` | `-116.93 um` | `-116.93 um` |
| Clock total cap | `0.26713 pF` | `0.222913 pF` | `-0.044217 pF` | `-0.050391 pF` |
| Clock power | `0.119869 mW` | `0.0890196 mW` | `-0.030849 mW` | `-0.030849 mW` |

45a is the only current trial that passes the all-23 core benchmark gate on both commercial-alignment MAE and raw QoR direction. Its production change is algorithmic: when recursive clustering considers the alternate split axis, it is accepted only if the native split score improves while child count, split distance, routing-cap balance/spread, utilization penalty, and child-diameter envelope are no worse than the longest-axis baseline. It introduces no behavior config, fitted scalar, magic threshold, or case-specific rule.

Guard residual for 45a:

| Guard | Baseline MAE | 45a MAE | Delta | Raw mean delta |
| --- | ---: | ---: | ---: | ---: |
| DRV max-cap real nets | `0.434783` | `0.347826` | `-0.0869565` | `+0.173913` |
| DRV max-tran real nets | `0.0869565` | `0.217391` | `+0.130435` | `+0.130435` |
| DRC total markers | `30.087` | `29.913` | `-0.173913` | `+0.0869565` |
| WNS | `285.609 ps` | `194.783 ps` | `-90.8261 ps` | `+0.011348 ns` |

Interpretation: under the clarified acceptance target, 45a is an effective core-QoR improvement. It is not a full core+guard closure: DRV transition alignment and small raw DRV/DRC counts still need a follow-up guard-oriented algorithmic fix. The 45b/45c/45d attempts prove that naive cap or leaf-envelope guards are not reliable enough to keep.

46a guard-scope audit:

| Signal | Count |
| --- | ---: |
| Added max-cap clock CTS nets | `0` |
| Added max-cap design-signal nets | `15` |
| Added max-tran clock CTS nets | `0` |
| Added max-tran design-signal nets | `8` |
| Named DRC clock CTS marker delta | `0` |

Interpretation: the remaining 45a DRV side effect is not on CTS-created clock nets. The detailed DRC report does not enumerate every summary marker, but the named-net smoke test also shows no clock CTS marker delta. Therefore adding another CTS clustering/H-tree guard for this residual would be a low-quality optimization: it would tune post-route side effects on ordinary signal/power nets and risks losing the proven core CTS improvements. The 45a guard residual is recorded as an evaluation caveat, not as an in-scope CTS algorithm blocker.

47a timing-semantics actionability:

| Signal | Reading |
| --- | ---: |
| Same-tree FastSTA-vs-Innovus skew MAE / R2 | `8.44899 ps` / `0.802146` |
| Endpoint match | min `1/23`, max `2/23`, both `0/23` |
| Skew-pair final-arrival residual MAE | `13.2078 ps` |
| Cell-step delta MAE | `12.0931 ps` |
| Wire-step delta MAE | `1.44118 ps` |
| Slew+driver-total-cap delay term MAE | `11.921 ps` |
| Residual after slew/driver-total-cap+wire MAE | `1.55921 ps` |
| Residual-after-context / final-arrival MAE ratio | `0.118052` |

Interpretation: P41 does not currently justify a production timing-model correction. The remaining same-tree mismatch is mostly a cell-context/slew/driver-cap semantic observer issue, not a direct wire/RC delay gap. Because the unexplained residual after context is only about `11.8%` of the final-arrival residual, this stage should not introduce scalar timing correction, threshold tuning, DMP switching, or wire/RC adjustment. The timing caveat stays visible, but it should not block structural H-tree and targeted clustering work.

49a timing cell-context counterfactual:

| Signal | Current 45a | Table at Ceff | Table at driver-total cap |
| --- | ---: | ---: | ---: |
| Cell-stage MAE | `2.55323 ps` | `2.53143 ps` | `2.6894 ps` |
| Cell-stage RMSE | `3.91545 ps` | `3.8963 ps` | `4.08421 ps` |
| Endpoint cell-path sum MAE | `9.06123 ps` | `9.03296 ps` | `9.36715 ps` |
| Same-tree cell-skew contribution MAE | `12.0931 ps` | `12.0786 ps` | `12.2109 ps` |

Interpretation: the existing trace-visible native delay alternatives are not actionable production fixes. Table-at-Ceff has only a tiny observer-level improvement and splits same-tree cases almost evenly (`12` improved, `11` regressed); table-at-driver-total-cap moves more but worsens the retained candidate. Keep timing semantics as a monitoring path until a deterministic non-scalar mismatch is found.

50a clarified-gate historical frontier:

| Gate | Non-retained all-23 candidates passing |
| --- | ---: |
| Core alignment only | `0/75` |
| Core alignment plus raw QoR | `0/75` |
| Core plus guard metrics | `0/75` |

Interpretation: the user's clarified goal does not resurrect an old rejected candidate as-is. Several historical trials improve one or two metrics, but each introduces at least one core, raw-QoR, or guard regression relative to retained 45a. Future work needs a new trace-backed first-principles candidate, not a reclassification of existing negative trials.

42f H-tree generation follow-up:

| Metric | 45a MAE | 42f MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `14.6522 ps` | `-0.7826 ps` |
| Latency | `46.8478 ps` | `47.3478 ps` | `+0.5000 ps` |
| Buffer count | `23.6957` | `23.7826` | `+0.0869` |
| Clock wire | `1640.46 um` | `1608.69 um` | `-31.77 um` |
| Clock total cap | `0.222913 pF` | `0.235565 pF` | `+0.012652 pF` |
| Clock power | `0.0890196 mW` | `0.101741 mW` | `+0.012721 mW` |

Interpretation: 42f validates that H-tree branch-local behavior can move final skew and wire in the desired direction, but local sibling balancing is not monotonic enough for production because it trades away latency/cap/power/buffer alignment. Under the clarified goal, future candidates are judged against the current retained code as well as the original baseline: a partial improvement is effective only when the remaining key metrics do not regress relative to the code it replaces.

43b cluster-root follow-up:

| Metric | 45a MAE | 43b MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.5652 ps` | `+0.1304 ps` |
| Latency | `46.8478 ps` | `48.2826 ps` | `+1.4348 ps` |
| Buffer count | `23.6957` | `23.4783` | `-0.2174` |
| Clock wire | `1640.46 um` | `1772.39 um` | `+131.93 um` |
| Clock total cap | `0.222913 pF` | `0.250609 pF` | `+0.027696 pF` |
| Clock power | `0.0890196 mW` | `0.111067 mW` | `+0.022047 mW` |

Interpretation: 43b confirms the cluster-root opportunity is not a standalone safe production rule, even with local source/distance/cap/wire no-worse guards. It produces a small buffer-count alignment improvement, but regresses all other core metrics relative to retained 45a, so the code was reverted. Future cluster work must be targeted to cluster-final outliers and gated by endpoint-order or bounded H-tree contribution, not by local root geometry alone.

43c cluster-final sizing follow-up:

- Read-only audit found cluster-final outliers in `5/23` 45a cases and confirmed that high-final-cap/high-cluster-delay endpoints exist.
- Production trial selected each cluster center buffer from configured `BUFX8H7L/12/16/20` using native Liberty delay cost and exact cluster downstream cap, without new config or fitted threshold.
- All-23 workflow passed (`23/23` CTS-only, `23/23` conversion, `23/23` Innovus evaluation).
- Result: skew MAE improved from `15.4348 ps` to `13.9565 ps`, and buffer-count MAE improved from `23.6957` to `22.9565`, but latency MAE regressed by `+9.1739 ps`, clock-wire MAE by `+534.329 um`, clock-cap MAE by `+1.23439 pF`, and clock-power MAE by `+1.04616 mW`.
- Root cause: the native delay-only selector upsized `11152/11156` cluster buffers to `BUFX20H7L`, so reduced local delay came at unacceptable input-cap, wire, latency, and power cost.

Decision: rejected and reverted. Cluster-final sizing remains useful diagnostic evidence, but the next production axis should focus on H-tree/split topology scoring or generation where the refreshed reports show dominant structural contribution.

42j/42k H-tree level-length follow-up:

- Selector-only audits on retained 45a found no strict no-worse H-tree replacement: tested branch length, exposure, and physical-load rules all had `0/23` hits.
- Level-length coverage audit found selected levels under-cover actual branch maxima in `22/23` cases and `34/45` selected levels; weighted undercoverage correlates with endpoint window (`r=0.3536`) and skew error (`r=0.3829`).
- Global max-branch level planning improved against baseline38, but relative to retained 45a it regressed raw skew, raw latency, cap, and power.
- Dual mean+max-branch frontier selection improved latency MAE by `6.6087 ps`, buffer-count MAE by `0.1739`, and clock-wire MAE by `17.5865 um` versus 45a, but regressed skew MAE by `1.3478 ps`, cap MAE by `0.01517 pF`, power MAE by `0.00302 mW`, and raw WNS by `0.10997 ns`.

Decision: rejected and reverted. Max-branch coverage is a valid root-cause clue, but it cannot be used as a global or broad dual-candidate planning rule. A future attempt must localize the condition that needs max coverage while preserving the mean-length plan as the default physical/cap/power-safe behavior.

42g local-split-generation follow-up:

Read-only precheck:

- `candidate_42g_split_local_generation_precheck` found active local split in `19/23` retained 45a cases.
- `15/18` H-tree-structural cases also had active local split, while selected internal-level proxy share averaged only `0.139878`.
- This supported a production trial that changed same-count local split generation instead of global H-tree selector retuning.

Code direction tested:

- recursively partition each local split node into the same requested child count;
- rerun the longest-axis cut inside each recursive subregion so multi-child split groups become more spatially compact;
- preserve max-fanout, local split child count, split-buffer counting, and electrical legality checks;
- add no behavior config, fitted scalar, threshold margin, or case rule.

Decision: rejected and reverted.

| Metric | 45a MAE | 42g MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.3043 ps` | `-0.1305 ps` |
| Latency | `46.8478 ps` | `47.9130 ps` | `+1.0652 ps` |
| Buffer count | `23.6957` | `22.4348` | `-1.2609` |
| Clock wire | `1640.46 um` | `1679.88 um` | `+39.42 um` |
| Clock total cap | `0.222913 pF` | `0.237652 pF` | `+0.014739 pF` |
| Clock power | `0.0890196 mW` | `0.119923 mW` | `+0.0309034 mW` |

Raw mean movement versus 45a:

| Metric | 45a Mean | 42g Mean | Delta |
| --- | ---: | ---: | ---: |
| Skew | `44.6522 ps` | `44.2609 ps` | `-0.3913 ps` |
| Latency | `223.957 ps` | `223.543 ps` | `-0.4130 ps` |
| Buffer count | `684.304` | `683.043` | `-1.2609` |
| Clock wire | `13404.17 um` | `13443.60 um` | `+39.42 um` |
| Clock total cap | `7.03448 pF` | `7.07817 pF` | `+0.04370 pF` |
| Clock power | `1.95917 mW` | `1.99008 mW` | `+0.03090 mW` |

Interpretation: recursive local split generation is a real skew/buffer lever, but it is not a no-regression production invariant. It makes groups locally more compact for endpoint ordering, yet increases clock wire/cap/power relative to retained 45a. Future H-tree work should first audit 42g-vs-45a deltas and derive a native gate that separates skew-safe cases from physical-cost regressions before another production change.

42h local-split delta audit:

| Signal | Result |
| --- | ---: |
| Core fit no-regression cases | `9/23` |
| Raw physical cost regression cases | `11/23` |
| Selected topology signature changed | `4/23` |
| All-case skew abs-error delta | `-0.130435 ps` |
| All-case latency abs-error delta | `+1.06522 ps` |
| All-case raw wire/cap/power deltas | `+39.425 um` / `+0.0436957 pF` / `+0.0309035 mW` |

Interpretation: 42h confirms that the 42g failure is not caused by a timing observer artifact. Recursive split movement can produce skew/buffer wins, but it also perturbs topology selection and physical cost. A recoverable production variant must compare recursive groups against the existing direct groups using native local physical proxies and keep the recursive groups only when those proxies are no worse.

42i guarded local-split generation:

Code direction tested:

- keep direct local split as the default;
- construct recursive local groups only as an alternative;
- accept recursive groups only when total/max parent-to-child center wire and total/max child bbox diameter are no worse than direct groups;
- add no behavior config, scalar, fitted threshold, or case rule.

Decision: rejected and reverted.

| Metric | 45a MAE | 42i MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.4348 ps` | `0.0000 ps` |
| Latency | `46.8478 ps` | `46.8043 ps` | `-0.0435 ps` |
| Buffer count | `23.6957` | `23.6957` | `0.0000` |
| Clock wire | `1640.46 um` | `1640.53 um` | `+0.07 um` |
| Clock total cap | `0.222913 pF` | `0.222957 pF` | `+0.000044 pF` |
| Clock power | `0.0890196 mW` | `0.0890196 mW` | `0.0000 mW` |

Raw movement versus 45a was limited to `iwls2005__vga_lcd`: latency `+1 ps`, wire `+1.535 um`, cap `-0.001 pF`. The all-23 mean raw wire also increased by `0.0667 um`. This does not pass the no-deterioration gate and is too much code complexity for a single tiny latency-alignment movement.

## Current Root-Cause Reading

Current decision: `core_qor_split_axis_pareto_candidate_retained_with_guard_residual`.

Evidence:

- Same-tree endpoint order remains weak: both endpoints match in `0/23`.
- Delay residual is cell/context dominated; direct wire/RC residual is smaller.
- H-tree/split dominates structural attribution after carrying the timing caveat.
- Sink clustering remains a targeted outlier direction, not the suite-wide first lever.
- Pareto split-axis alternatives can improve core skew/latency/buffer/wire/cap/power together; the observed guard side effects are post-route non-clock-net effects, not direct clock CTS net regressions.
- Selection-front counterfactuals found no lower-power alternative that also preserves delay, branch-arrival proxy, weighted proxy, physical depth, and physical buffer count.
- L1-median cluster roots are physically meaningful for local final-net proxy, but the standalone all-23 run moved H-tree endpoint ordering the wrong way, regressing final skew and wire-length alignment.
- Multi-topology historical oracle analysis found `6` final-safe candidate/case opportunities, but tested ECC-visible Pareto rules either missed them or introduced false positives; this closes current-selector retuning as a safe broad path.
- Stronger 45b/45c/45d guards were not kept because each shifted regression into another core or guard metric, especially latency.
- 46a closes extra CTS guard tuning for 45a: the residual max-cap/max-tran nets are design-signal nets, so further split-axis guard changes would be indirect and likely overfit detail-route behavior.
- 47a closes the immediate native timing-semantics fix path: current evidence shows no isolated production code fix for timing residual without adding scalar or fitted behavior.
- 49a closes the concrete cell-delay substitution counterfactual: neither table-at-Ceff nor table-at-driver-total-cap materially improves same-tree endpoint-order evidence for the retained code.
- 50a confirms that no already-run rejected all-23 candidate satisfies the clarified partial-improvement/no-regression gate relative to retained 45a.
- 51a adds recursive split-decision tracing for retained 45a. Initial all-23 CTS trace shows the alternate axis is accepted sparsely and only under native Pareto checks, giving the next iteration a concrete clustering decision level instead of inferring split behavior only from final metrics.
- 52a closes RMS requested level length as a standalone H-tree level-planning rule. It only changes `openroad__jpeg`; the skew/wire improvement is accompanied by latency/cap/power/buffer regression, so the production code was restored to mean level length.
- 53a closes power-preserving branch-arrival overlay as a selector-only follow-up. It improves cap/power alignment but regresses skew/latency alignment and timing guards, so production code was restored to the retained 45a selector.
- 54a closes leaf-only max-branch level planning. The trace premise was real, but the all-23 run is exactly equivalent to 45a on final metrics and guards, so changing only the leaf requested-length bin does not create a useful new H-tree solution.
- 55a shows the first strong post-45a cluster-side signal: boundary-load cap polish is not free. Removing it improves five core metrics and timing guards, but the latency and DRV/DRC regressions require a guarded variant rather than direct removal.
- 56a closes the simplest guarded boundary-polish attempt. Local routing-cap proxy no-worse is too weak: it preserves some wire improvement but still worsens skew/latency/cap/power and DRV, so further boundary work needs move-level trace and a stronger endpoint-order/latency model before another production candidate.
- 57a closes the boundary-polish instrumentation gap. Accepted boundary moves are sparse (`21` moves across `13/23` cases), and the default-off trace is exactly QoR-neutral against retained 45a on core and guard metrics.
- 58a closes local Pareto boundary guarding as a standalone rule. Geometry/cap no-worse filtering still regresses skew, latency, wire, and DRV relative to retained 45a; boundary-polish work needs endpoint-order evidence, not only local source/target geometry and cap-balance facts.
- 59a closes global binary H-tree branching as a standalone generation rule. The first-principles idea of separating electrical fanout from spatial partitioning remains valid, but forcing every generated tree to binary branching failed all-23 CTS-only completion and inflated large-case topology depth/search; the resumed run reached about `60 GB` RSS on `openroad__dynamic_node`.
- 60a closes nearest-root boundary move guarding as a standalone rule. It is a natural spatial-affinity invariant and completes all-23, but the small skew improvement comes with latency, wire, cap, power, WNS, and max-cap guard regressions, so it fails the clarified no-deterioration gate.
- 61a closes standalone boundary-polish guarding as the next production direction. Boundary moves can touch final max endpoints, but the current production boundary-polish stage does not have endpoint/path timing context. Repeating local cap, geometry, or root-distance predicates is now low value.
- 43b closes standalone source/local no-worse median cluster roots: local root geometry can improve buffer count, but it is not sufficient to preserve skew, latency, wire, cap, and power relative to 45a.
- 42g closes naive recursive local split generation: it improves skew and buffer count, but the extra local spatial refinement is not free and regresses wire, cap, and power relative to 45a.
- 42h identifies the recovery condition for another local split attempt: recursive grouping must be guarded by direct-vs-recursive local physical no-worse proxies before it is allowed to affect topology selection.
- 42i closes the guarded-recursive recovery attempt: the guard prevents broad damage but leaves only a tiny latency alignment movement with small physical regressions, so local split generation is not the next productive lever.
- 42j/42k close broad max-branch H-tree level planning: undercoverage is real, but global or broad frontier exposure improves some latency/wire/buffer metrics while regressing skew/cap/power/WNS relative to retained 45a.
- 62a reopens local split generation with a better native premise: alternate-axis local split can improve five core metrics, but without spread guards it causes DRV/DRC guard regressions.
- 62b proves worst route-envelope improvement alone is not enough: it is exactly equivalent to 62a on all checked all-23 metrics.
- 62c is the current retained incremental improvement. Requiring root-wire, child-diameter, and route-envelope spread to be no worse keeps only the safe `iwls2005__pci` movement: latency `-1 ps`, clock wire `-14.16 um`, clock cap `-0.03 pF`, clock power `-0.005 mW`, and WNS `+2 ps`, with no other case movement and no guard regression.
- 63a tested an anchor-aware root-wire proxy for H-tree local split generation. It still passes the broad gate versus retained 45a, but it is worse than 62c on the same `iwls2005__pci` movement: latency `+0.5 ps`, clock wire `+8.06 um`, clock cap `+0.017 pF`, clock power `+0.003 mW`, and WNS `-2 ps` versus 62c. Keep 62c's center-based local split proxy and treat anchor-aware generation as closed negative evidence.
- 64b adds scoped local split-generation trace without QoR movement. It showed that relaxing aggregate guards is an observable next axis, but not yet a production rule.
- 65a/66a/67a/70a close the aggregate-relaxation family for now. Removing both aggregate guards is too broad; keeping route-envelope sum but dropping total child diameter still admits unsafe movement; limiting the relaxation to multi-child split isolates positive skew movement but regresses clock wire; adding a materialized local-tree wire guard still regresses wire/WNS/DRV and triggers one Innovus evaluation crash. Keep 62c's stricter aggregate guards unless a future candidate generates a different local topology.
- 75a/75b/75c close direct materialized-H-tree-buffer cost correction as a standalone production direction. 75a proves a real semantic mismatch: parent-level weighted buffer count underestimates edge-materialized H-tree buffers. 75b's production correction improves core16 skew, buffer, wire, cap, power, WNS, max-tran, and DRC versus 71a, but broad latency regression makes it a deferred, not retained, node. 75c's split-aware complexity follow-up does not fix the tradeoff; it improves wire/cap/power but regresses skew, latency, buffer count, WNS, and DRC versus 71a. Production code remains on the retained 71a selector semantics; the evidence is kept for a future endpoint-order-preserving topology-space change.
- 76a closes the current-front latency-preserving recovery for materialized-edge selection. It finds `5/16` relaxed core hits when only delay/power/arrival proxies are checked, but `0/16` hits once effective split delay and exact segment delay are required no-worse. This confirms that the next H-tree attempt needs a new topology/materialization point, not another selector over the current feasible front.
- 77a classifies the next topology-generation target. `core16` has `12` split-materialization-dominant cases and `4` edge-materialization-dominant cases; all current-front latency-preserving lower-edge hit counts remain `0/16`. Split-dominant cases explain the largest residuals (`openroad__jpeg`, `openroad__dynamic_node`, `iwls2005__des`), while edge-dominant cases (`iwls2005__systemcaes`, `iwls2005__wb_dma`, `iwls2005__tv80`, `iwls2005__spi`) show that materialized edge semantics must be handled during generation. The next candidate should create same-depth edge/split materialization alternatives with no-worse effective split delay, exact segment delay, and weighted branch-arrival proxy.
- 78a closes the current-front split-saving recovery. Six `core16` cases have unguarded lower-split candidates, but none preserve delay/power/arrival; same-depth lower-split candidates are `0/16`, and all materialization-safe split-saving policies are also `0/16`. Lower split in the current front is achieved only by moving to a different/deeper shape with more materialized edge buffering and worse timing proxies, so a selector cannot solve the split-dominant residual.
- 79a identifies a concrete generation-level root cause: retained 71a creates `942` `core16` local split buffers where the max-fanout lower bound is `768`. The excess is concentrated in the same split-dominant residual cases surfaced by 77a/78a, so the next production attempt can change local split tree generation rather than selector scoring.
- 80a proves the lower-bound direction is real but incomplete. A pure fanout-minimal tree removes all `core16` lower-bound excess and improves cost metrics, but it regresses skew because it discards uniform local path-depth symmetry.
- 81a is the retained intermediate local split generation node. It uses fanout-derived leaf groups and a uniform-depth shape, with no fitted constants or behavior config. It reduces split over-materialization substantially while preserving enough local depth symmetry to keep all primary `core16` metrics closer to Innovus than the remote baseline.
- 82a explains 81a's giveback. The split-count reduction is useful, but the residual loss is tied to local htree-split physical packing and split-delay spread in a few large core cases, not to the fanout-derived depth itself.
- 82b becomes the current retained local split generation node. It preserves 81a's fanout-derived uniform depth but recursively re-sorts/repartitions each child shape by its own physical load subset. This recovers 81a's wire/cap/power/skew giveback while keeping split count unchanged.

Evidence:

- `summary/candidate_75a_materialized_htree_buffer_count_audit/materialized_htree_buffer_count_audit_report.md`
- `summary/candidate_75b_materialized_htree_edge_buffer_cost/decision_compare_75b_vs_71a/candidate_75b_decision_compare_report.md`
- `summary/benchmark_layer_candidate_75b_vs_remote`
- `summary/candidate_75c_split_aware_htree_complexity/decision_compare_75c/candidate_75c_decision_compare_report.md`
- `summary/benchmark_layer_candidate_75c_vs_remote`
- `summary/candidate_76a_htree_latency_preserving_edge_opportunity/latency_preserving_edge_opportunity_report.md`
- `summary/candidate_77a_htree_materialization_pressure/htree_materialization_pressure_report.md`
- `summary/candidate_78a_htree_split_preserving_opportunity/split_preserving_opportunity_report.md`
- `summary/candidate_79a_local_split_minimality_audit/local_split_minimality_audit_report.md`
- `summary/candidate_80a_minimal_local_split_generation/local_split_minimality_after/local_split_minimality_audit_report.md`
- `summary/candidate_80a_minimal_local_split_generation/decision_compare_80a_vs_71a/candidate_metric_delta_report.md`
- `summary/candidate_81a_uniform_depth_local_split_generation/local_split_minimality_after/local_split_minimality_audit_report.md`
- `summary/candidate_81a_uniform_depth_local_split_generation/decision_compare_81a_vs_71a/candidate_metric_delta_report.md`
- `summary/benchmark_layer_candidate_81a_vs_remote`
- `summary/candidate_82a_local_split_giveback_attribution/local_split_giveback_attribution_report.md`
- `summary/candidate_82b_local_recursive_uniform_split_packing/local_split_minimality_after/local_split_minimality_audit_report.md`
- `summary/candidate_82b_local_recursive_uniform_split_packing/decision_compare_82b_vs_81a/candidate_metric_delta_report.md`
- `summary/candidate_82b_local_recursive_uniform_split_packing/decision_compare_82b_vs_71a/candidate_metric_delta_report.md`
- `summary/benchmark_layer_candidate_82b_vs_remote`

## Required Workflow

Every production candidate follows:

`Innovus benchmark gap -> Innovus flow-log review -> sequential evidence summary -> code/trace implementation -> build ecc_bin -> all-23 CTS-only -> all-23 Innovus evaluation -> L1/L2/L3/L4/case analysis -> keep/retry/reject -> retrospective`

The task-level experiment framework is:

| Step | Required artifact | Gate |
| --- | --- | --- |
| Benchmark-gap analysis | NFS report under `summary/<candidate>/...` | Starts from commercial Innovus reference, remote-branch result, current retained result, and case-level metric gaps. |
| Innovus flow-log review | Compact report under `summary/<candidate>/...` when logs exist | Summarizes observable Innovus stages and maps them to ECC missing capabilities or weaker approximations. |
| Reverse analysis | NFS report under `summary/<candidate>/...` | Explains the proposed change from timing/topology/cap/load evidence, not from final metric tuning. |
| Code candidate | Production diff or read-only analyzer diff | No behavior config, fitted scalar, magic threshold, or case rule. |
| Bench collection | all-23 CTS-only run under `runs/<candidate>` | `23/23` CTS success before evaluation. |
| Evaluation collection | all-23 Innovus evaluation summary | `23/23` evaluation success and comparable `ecc-tools.summary.csv`. |
| Decision analysis | metric fit, QoR direction, case win/loss, L2/L3/L4 reports | Keep only if candidate-vs-Innovus error improves versus remote-branch-vs-Innovus error, with case-by-case support and acceptable guards. |
| Retrospective | PRD or compact summary report | Updates successful paths, failed paths, closed directions, and next evidence-backed direction. |
| Cleanup | reverted rejected code or retained accepted code | Rejected production changes leave reports only; accepted changes keep tests and concise rationale. |

Rules:

- Use all 23 designs for keep/reject decisions.
- Compare every new production candidate against commercial Innovus as the reference: candidate-vs-Innovus must improve over remote-branch-vs-Innovus on at least one primary metric.
- Also compare against the current retained production candidate to avoid stacking changes that are weaker than the current branch unless the tradeoff is explicit and worth accepting.
- Check both aggregate metrics and case-by-case behavior. A mean improvement driven by one unrelated outlier is not enough; weak but plausible small-case movement should get at most one bounded follow-up attempt before keep/reject.
- Periodically summarize successful and failed paths so the next direction is based on accumulated benchmark evidence rather than isolated candidate memories.
- Keep diagnostic trace default-off behind `ICTS_TIMING_TRACE_DIR`.
- Do not add fitted scalars, magic thresholds, case-specific behavior, or behavior-changing configs.
- Do not accept changes from WNS/TNS/slack summaries alone.
- Do not tune final metrics directly; reason from topology, load distribution, RC/cap/slew, stage delay, endpoint ordering, then skew/QoR.
- If Innovus behavior is not explainable from ECC trace or Innovus flow logs, add read-only analysis scripts before changing production code.

## Next Direction

The next accepted change should come from one of these first-principles paths:

1. H-tree topology generation: expand physically meaningful topology alternatives, not selector-only retuning over historical candidates, and keep delay/branch-arrival/complexity envelopes non-worse. Do not force global binary branching; 59a shows that any spatial/electrical fanout decoupling must be localized or bounded by a native workload/depth envelope.
2. H-tree level planning: use the max-branch undercoverage evidence only as a localized diagnostic signal; do not globally replace mean level length, broadly add max-length plan sets, or add max-only characterization coverage unless skew/cap/power no-regression can be proven from native physical invariants.
3. Clustering: handle only the `cluster_final` outliers if H-tree contribution is bounded and local cap/fanout/wirelength distribution clearly dominates; do not keep standalone median-root, source/local no-worse root rewrites, or cluster-buffer upsizing without direct electrical necessity and all-23 no-regression evidence.
4. Native timing semantics: revisit only when future trace identifies a deterministic slew, driver-cap, Liberty, or RC semantic mismatch that can be fixed natively without scalar correction.

Current cluster-local evidence after 102a/103a/104a/105a/106a closes direct total-cap single-move/swap balancing for the path-context group and identifies endpoint-spread direction as the source signal for the boundary-move group. Do not implement boundary polish, final-wire RC correction, input-slew fitting, local endpoint-rank guarding, direct total-cap balancing, or an Innovus-labeled endpoint guard before a native endpoint-spread observability precheck is complete.

Ordered iteration queue:

1. `P42 H-tree topology-space generation`: create new topology alternatives when existing frontier and historical-candidate counterfactuals show no safe selector-only replacement; reject broad changes that improve skew/wire but worsen cap/power/buffer, final latency, or the all-23 CTS-only completion/workload gate.
2. `P43 cluster-final outliers`: optimize local clustering only for cases whose trace remains cluster-dominant after timing and H-tree gates. Standalone L1-median, source/local no-worse median-root, boundary-polish local guards, delay-driven buffer upsizing, and cap-legality buffer upsizing are closed as rejected or read-only negative evidence.
3. `P41 timing-semantics/native observer`: keep as a monitoring track only; implement a native timing fix only if later trace proves a deterministic non-scalar modeling gap.

Each item may start with a read-only precheck script. A precheck can close a direction without production code when all-23 evidence shows no safe opportunity.

## Acceptance

- [x] Task assets are compact and point to NFS evidence.
- [x] Trace records algorithm-stage and post-design-apply FastSTA timing states.
- [x] L3 analysis compares ECC FastSTA with Innovus arrival/path delay, cell delay, wire delay, slew, cap, and RC context.
- [x] First-principles reports quantify timing observer, H-tree, and clustering contributions.
- [x] Rejected candidates keep evidence but do not remain in production code.
- [x] A production algorithm change improves core all-23 benchmark targets without core metric regression.
- [x] The retained core candidate is algorithmic and first-principles based, with no fitted parameters, magic numbers, or case-specific rules.
- [x] Follow-up work classifies the remaining 45a DRV/DRC guard residual and closes extra CTS-side guard tuning when the residual is not on clock CTS nets.
- [x] P41 timing-semantics precheck is closed without production timing correction because the current evidence does not isolate a native non-scalar fix.
- [x] Recursive split-decision trace is available for first-principles clustering/H-tree follow-up analysis.
- [x] A second incremental production algorithm change (`candidate_62c_spread_guarded_local_split_axis`) improves latency, wire, cap, power, and WNS versus retained 45a with no core/raw/guard regression.
- [x] Anchor-aware local split generation (`candidate_63a_anchor_aware_local_split`) was tested, rejected, and reverted because it regresses the current retained 62c result.
- [x] Scoped local split-generation trace (`candidate_64b_local_split_generation_scope_trace_refresh`) is available for legality/materialization split diagnosis.
- [x] Aggregate local split guard relaxations (`candidate_65a`, `candidate_66a`, `candidate_67a`) were tested, rejected, and reverted because each produced at least one key benchmark regression versus retained 62c.
- [x] Local split over-materialization is quantified by `candidate_79a`; pure fanout-minimal generation (`candidate_80a`) is rejected as too aggressive, uniform-depth generation (`candidate_81a`) establishes the fanout-derived shape, and recursive local physical packing (`candidate_82b`) is retained as the current technical node candidate.
