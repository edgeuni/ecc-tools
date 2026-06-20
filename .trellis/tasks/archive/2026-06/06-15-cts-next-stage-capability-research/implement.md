# Implementation: First-Principles CTS Skew Alignment

## Checklist

- [x] Read current task and iCTS backend specs.
- [x] Treat a candidate as effective when it improves at least one all-23 benchmark target without regressing other key benchmark metrics; full Innovus parity is not required in one iteration.
- [x] Keep generated evidence under the NFS evidence root.
- [x] Keep default-off timing checkpoint trace for algorithm-stage and final FastSTA states.
- [x] Keep path/stage trace with source/root, cap/Ceff, driver-total cap, slew, RC, DMP, and load-wire fields.
- [x] Keep all-23 L3 analysis comparing ECC FastSTA with Innovus timing reports.
- [x] Compact Trellis assets so completed trial detail lives in reports/NFS instead of design/implement.
- [x] Add benchmark-layer analysis from remote-vs-Innovus gaps plus SDC/netlist clock evidence; use `core16` as the primary single-clock gate, keep `extended23` as a guard, and report `quarantine7` separately.
- [x] Add remote baseline case-suitability report from SDC/netlist and remote-vs-Innovus contribution evidence; exclude six unconstrained multi-clock cases plus `vga_lcd` from the primary benchmark.
- [x] Re-evaluate `45a-only`, `62c-only`, and `45a+62c` under the layered benchmark policy.
- [x] Run all-23 `candidate_71a_core16_gate_refresh` after cleanup; confirm current worktree behaves like layered `45a-only` on `core16`.
- [x] Run all-23 `candidate_72a_final_cluster_extra_cluster_guard`; reject and revert because it is final-metric equivalent to 71a and adds production selector complexity without benchmark movement.
- [x] Run all-23 `candidate_73a_htree_sink_envelope_guard`; reject and revert because the small buffer/DRC gain versus 71a comes with worse skew, latency, wire, cap, power, and WNS, and cap/power become worse than remote.
- [x] Run read-only `candidate_74a_htree_precost_direction_precheck`; close longest-axis fallback as a production selector because its small buffer/DRC benefit loses the retained 71a skew/latency/wire/cap/power/WNS value on the selected `core16` group.
- [x] Run read-only `candidate_74b_htree_branch_spread_selector_precheck`; close current-front branch-spread selection because retained 71a traces have `0/23` physical no-worse or strict timing no-worse substitutions.
- [x] Run read-only `candidate_74c_htree_buffer_frontier_gap`; close current-front lower-buffer selection because four of five `core16` buffer-loss cases have no lower-buffer feasible-front alternative, and the remaining case only has lower-buffer alternatives with worse delay/power/arrival-spread proxies.
- [x] Run read-only `candidate_75a_materialized_htree_buffer_count_audit`; confirm current H-tree selector undercounts materialized H-tree edge buffers when it uses parent-level weighted buffer count, with broad undercount evidence across `core16` and `extended23`.
- [x] Run all-23 `candidate_75b_materialized_htree_edge_buffer_cost`; defer and revert because the edge-buffer semantic correction improves core16 skew, buffer, wire, cap, power, WNS, max-tran, and DRC versus 71a, but introduces broad latency regression.
- [x] Run all-23 `candidate_75c_split_aware_htree_complexity`; reject and revert because separating H-tree edge buffers and split-extra buffers as dominance dimensions improves wire/cap/power but regresses core16 skew, latency, buffer count, WNS, and DRC versus 71a.
- [x] Run read-only `candidate_76a_htree_latency_preserving_edge_opportunity`; close current-front materialized-edge selector follow-up because lower-edge candidates disappear under effective split delay / exact segment delay / split-extra no-worse guards.
- [x] Run read-only `candidate_77a_htree_materialization_pressure`; classify core16 pressure as split-dominant `12/16` and edge-dominant `4/16`, with `0/16` latency-preserving current-front lower-edge hits, so the next candidate must generate same-depth materialization/topology alternatives rather than retune selector cost.
- [x] Run read-only `candidate_78a_htree_split_preserving_opportunity`; close current-front split-saving selection because six core16 cases have unguarded lower-split candidates, but all fail relaxed delay/power/arrival guards and same-depth materialization-safe split-saving remains `0/16`.
- [x] Run read-only `candidate_79a_local_split_minimality_audit`; confirm retained 71a over-materializes local split buffers on `core16` (`942` actual versus `768` fanout lower bound, `174` reducible).
- [x] Run all-23 `candidate_80a_minimal_local_split_generation`; reject pure fanout-minimal split generation because it removes all lower-bound excess but regresses `core16` skew alignment versus remote.
- [x] Run all-23 `candidate_81a_uniform_depth_local_split_generation`; retain as current technical node candidate because it reduces local split over-materialization while keeping all primary `core16` metrics closer to Innovus than the remote branch.
- [x] Run read-only `candidate_82a_local_split_giveback_attribution`; confirm 81a giveback is tied to local htree-split physical packing and split-delay spread, not to fanout-derived uniform depth itself.
- [x] Run all-23 `candidate_82b_local_recursive_uniform_split_packing`; retain as current technical node candidate because it improves every `core16` primary MAE versus 81a while preserving 81a's split count and all-23 completion.
- [x] Run read-only `candidate_83a_82b_residual_htree_buffer_frontier_precheck`; close lower-buffer selector recovery after 82b because `core16` buffer-loss cases have no delay/power-safe lower-buffer current-front alternative.
- [x] Run read-only `candidate_83b_82b_residual_htree_selector_precheck`; close residual selector-only H-tree retuning after 82b because tested physical/load/exposure/branch-length rules have `0/23` hits.
- [x] Run read-only `candidate_83c_82b_level_length_coverage_audit`; keep branch-max undercoverage as diagnostic evidence only because previous max-branch production uses regressed key metrics or no-oped.
- [x] Run read-only `candidate_84a_82b_core16_residual_priority_precheck`; rank retained 82b residual mass and confirm the top gaps are physical cap/power/wire on `des` and `jpeg`, with selector-only lower-buffer and broad max-branch recovery still closed.
- [x] Run read-only `candidate_85a_next_generation_axis_precheck`; join 84a residual ranking, 77a materialization pressure, and 82a local-split giveback to select `local_split_physical_anchor_generation` as the next bounded production axis and defer same-depth edge materialization for edge-dominant residual cases.
- [x] Implement and evaluate all-23 `candidate_85b_local_split_physical_anchor_generation`; reject and revert production code because coordinate-wise median local split anchoring regresses every `core16` primary MAE versus retained 82b except buffer count, and also worsens `core16` DRC versus remote.
- [x] Run read-only `candidate_86a_edge_materialization_next_axis_precheck`; confirm edge-dominant residual cases are real but current-front lower-edge candidates are not safe because relaxed hits shorten H-tree depth, increase split-extra buffers, and regress effective split delay, while strict same-depth latency-preserving hits remain `0/4`.
- [x] Run read-only `candidate_86b_same_depth_edge_materialization_generation_precheck`; close current-front same-depth lower-edge recovery because the target cases expose only the selected leaf-terminal materialization sequence under retained 82b, so a real production attempt would need new generation semantics.
- [x] Attempt all-23 `candidate_87a_compact_local_split_tree`; reject and revert because mixed-depth minimal local split generation improves some cost metrics but regresses `core16` skew, latency, WNS, and DRC by losing local depth symmetry and increasing effective split delay in large cases such as `des`.
- [x] Attempt all-23 `candidate_88a_effective_split_delay_global_selection`; reject and revert because direct effective-delay selection improves skew/latency but broadly regresses buffer, wire, cap, and power versus retained 82b, and cap/power also become worse than the remote branch.
- [x] Run read-only `candidate_88b_effective_delay_guarded_tiebreak_precheck`; strict guarded/tie-break effective-delay selection has `0/16` `core16` hits, while removing only the power guard exposes `14/16` opportunities.
- [x] Attempt all-23 `candidate_88b_effective_timing_guarded_no_power`; reject and revert because the no-power guard improves retained-82b skew/latency but regresses `core16` wire, cap, power, and WNS, and cap/power are worse than the remote branch.
- [x] Run read-only `candidate_89a_effective_delay_power_source_precheck`; confirm no-power effective-delay opportunities are mostly stronger segment-cell substitutions with unchanged physical buffer count/sink cap, so relaxing the power guard is closed.
- [x] Run read-only `candidate_90a_effective_delay_geometry_substitution_precheck`; confirm geometry/materialization is the correct substitute in principle, but it needs localized changed-level movement rather than another broad level-length or branch-spread selector.
- [x] Run read-only `candidate_91a_effective_delay_changed_level_scope`; classify changed effective-delay levels as mostly leaf-side/terminal-branch substitutions, narrowing the next research target.
- [x] Run read-only `candidate_92a_leaf_terminal_materialization_current_front_precheck`; close current-front leaf-terminal selector work because all `773` lower-effective current-front alternatives fail power no-worse and strict hits are `0/16`.
- [x] Add default-off full segment-frontier trace under `ICTS_TIMING_TRACE_DIR`, exposing `all`, `terminal_branch_buffered`, and `terminal_leaf_unbuffered` entries without changing normal production behavior.
- [x] Run read-only `candidate_93a_segment_frontier_trace_opportunity`; close lower-level segment-frontier selector work because `core16` has `0` same-boundary/source-boundary lower-delay, power-no-worse opportunities after matching the native frontier-pruning state.
- [x] Run read-only `candidate_94a_retained_82b_residual_reanchor`; confirm retained 82b still improves every `core16` primary metric versus remote and reclassify the remaining open work as split-materialization endpoint-order-aware generation, new same-depth edge-materialization generation, and smaller targeted endpoint-order cluster diagnosis.
- [x] Run read-only `candidate_95a_endpoint_split_order_precheck`; confirm existing alternate split axes can change endpoint side on the top split-materialization residuals, but those alternates are non-Pareto under native physical/electrical guards, so direct alternate-axis selection is not a safe production candidate.
- [x] Run read-only `candidate_96a_principal_axis_split_generation_precheck`; confirm principal-direction split generation has native Pareto opportunities across top residual axes, but no endpoint-changing Pareto hit, so it is only a broad physical-cost generator hypothesis.
- [x] Implement/evaluate all-23 `candidate_96b_principal_axis_split_generation`; reject and revert production code because it improves `core16` wire slightly versus retained 82b but regresses retained-82b skew, latency, buffer, cap, power, WNS, and DRC alignment.
- [x] Run read-only `candidate_97a_same_depth_edge_generation_feasibility_precheck`; close direct same-depth edge-materialization production from current segment-frontier evidence because strict same-state lower-buffer opportunities are `0/4` target cases, while relaxed lower-buffer rows require boundary/terminal/fanout state changes already pruned by the production `kAll` H-tree search.
- [x] Run read-only `candidate_98a_topology_composition_rejection_trace_precheck`; close direct edge-axis production for now because selected-depth sink-load rejection is `0`, while composition monotonic rejects are `46.5939%`, composition fanout rejects are `21.5401%`, and root fanout rejects are `19.7024%` on the target edge cases.
- [x] Run read-only `candidate_99a_split_materialization_endpoint_order_generator_precheck`; close the existing alternate-axis selector and broad principal-axis generator because `des/jpeg` endpoint-changing alternates are all non-Pareto, blocked by score and/or total-child-diameter, while principal-axis generation has `84` physical Pareto nodes and `0` endpoint-changing Pareto nodes.
- [x] Run read-only `candidate_100a_endpoint_materialized_route_shape_precheck`; close the minimal endpoint-transplant and alternate-repair split-shape family because each target case enumerates `6561` candidates and native-safe generated shape count is `0/2`.
- [x] Run read-only `candidate_101a_targeted_endpoint_order_cluster_diagnosis`; split the `17.3416%` cluster/final-local subset into three cluster endpoint path-context cases, two endpoint-cluster boundary-move cases, and one htree-dominant deferred case.
- [x] Run read-only `candidate_102a_cluster_endpoint_path_context_precheck`; confirm the three target cases are cluster-buffer path-delay-context cases, not same-cluster local endpoint-rank cases.
- [x] Run read-only `candidate_103a_cluster_buffer_path_delay_source`; confirm the 102a path-context delta is dominated by cluster buffer cell delay and associated input slew/downstream load, not final-wire RC.
- [x] Run read-only `candidate_104a_cluster_buffer_load_slew_predictor_precheck`; confirm `fast_total_cap_pf` is the strongest native pre-materialization proxy for the target cluster cell-delay/load context.
- [x] Run read-only `candidate_105a_cluster_total_cap_balance_candidate_precheck`; reject direct single-move/single-swap total-cap balance production because strict native hits appear in only one target case.
- [x] Run read-only `candidate_106a_endpoint_cluster_boundary_move_source`; identify endpoint-spread direction as the diagnostic source, but do not promote it directly because the endpoint side is Innovus-labeled.
- [x] Run read-only `candidate_107a_native_endpoint_spread_observability`; close current-stage boundary-polish guarding because boundary-local proxy hits `0/2`, while route-tree-or-later native timing/rank signals hit `2/2` but are unavailable at the current fast-clustering move decision.
- [x] Run read-only `candidate_108a_deferred_timing_boundary_polish_feasibility`; confirm route-tree timing/local conflicts exist in `9/15` core16 accepted boundary moves, enough for a targeted counterfactual candidate but not immediate production because timing is observed after current boundary polish.
- [x] Run read-only `candidate_109a_counterfactual_deferred_boundary_polish`; confirm the narrow rollback policy would touch `7` core16 moves across `6` cases, covering `38.6821%` of retained-82b residual share with endpoint-spread proxy gain greater than local-proxy giveback.
- [x] Run read-only `candidate_110a_deferred_boundary_polish_flow_contract`; close the post-route sink-net-only implementation shortcut because it is equivalent in `0/7` core rollback moves and every rollback changes source or target cluster centers.
- [x] Implement `candidate_110b_boundary_polish_timing_guidance_hook`; add a dormant native boundary-polish timing-guidance input and unit test the 109a early-source depletion rejection policy without changing default behavior.
- [x] Implement and evaluate all-23 `candidate_110c_preliminary_timing_boundary_polish_flow`; defer promotion because it improves all `core16` primary MAEs versus remote but regresses latency and WNS versus retained 82b.
- [x] Attempt all-23 `candidate_110d_pairwise_timing_boundary_polish_flow`; reject and revert because source-plus-target endpoint-spread guidance improves skew but causes broad WNS/critical-path alignment regression.
- [x] Run read-only `candidate_110e_critical_path_boundary_guard_precheck`; close immediate production boundary-polish follow-up because local source/target timing-rank evidence does not natively protect benchmark WNS, especially `wb_conmax` and `tv80`.
- [x] Run read-only `candidate_110f_data_path_endpoint_boundary_join`; keep timing-aware boundary polish deferred because launch/capture endpoint-cluster touch has both WNS-safe false positives and WNS-loss false negatives.
- [x] Run read-only `candidate_111a_htree_split_edge_residual_queue_refresh`; end the current goal iteration with no production C++ change, keep retained 82b as the anchor, close boundary-polish continuation for now, and identify same-depth edge materialization around composition/fanout-safe state generation as the most actionable restart point.
- [x] Run all-23 `candidate_38a_local_split_physical_guard_refresh` as the current baseline.
- [x] Run all-23 `candidate_40a_path_safe_local_split_axis`; reject due latency/cap/DRV/WNS regressions.
- [x] Run all-23 `candidate_40b_path_safe_local_split_axis_span_guard`; reject due remaining latency/cap/WNS regressions and raw DRV/WNS side effects.
- [x] Revert rejected 40a/40b production local split-axis code.
- [x] Add reusable read-only prechecks for frontier/timing/topology opportunities before production changes.
- [x] Run read-only `candidate_42a_htree_frontier_opportunity_precheck`; close selector-only H-tree retuning because no strict no-worse replacement exists in the current frontier.
- [x] Run all-23 `candidate_42b_shrink_only_topology_balance`; reject because wire/buffer improvements came with skew/latency/cap/power regressions.
- [x] Revert rejected 42b production topology-balance code.
- [x] Run all-23 `candidate_42c_preserve_leaf_cluster_centers`; reject because cap/power/wire improvements came with skew/latency and same-tree closure regressions.
- [x] Revert rejected 42c production topology-balance code.
- [x] Run read-only `candidate_43a_cluster_root_policy_precheck`; confirm L1-median cluster roots reduce final-cluster Manhattan proxy in `23/23` cases.
- [x] Run all-23 `candidate_43a_l1_median_cluster_root`; reject because latency/buffer/cap/power improvements came with final skew and clock-wire regressions.
- [x] Revert rejected 43a production cluster-root code.
- [x] Run read-only `candidate_42e_multi_topology_pareto_opportunity`; close historical topology selector reuse because tested ECC-visible Pareto rules had false positives or missed safe cases.
- [x] Run all-23 `candidate_45a_pareto_split_axis`; retain as current core-QoR candidate because all six core alignment metrics and all six core raw QoR means improve.
- [x] Run all-23 `candidate_45b_pareto_split_axis_cap_guard`; reject because direct cap guarding regressed skew/latency and DRV guard metrics.
- [x] Run all-23 `candidate_45c_leaf_envelope_split_axis`; reject because leaf-envelope guarding regressed latency/cap/power core metrics.
- [x] Run all-23 `candidate_45d_leaf_tail_split_axis`; reject because leaf-tail guarding regressed latency and guard metrics.
- [x] Revert rejected 45b/45c/45d production code and keep the 45a split-axis Pareto implementation.
- [x] Accept a production algorithm change for core all-23 benchmark QoR under the clarified partial-improvement goal.
- [x] Run read-only `candidate_46a_guard_residual_scope`; close extra CTS-side guard tuning because 45a added max-cap/max-tran residuals are design-signal nets, not clock CTS nets.
- [x] Run read-only `candidate_47a_timing_semantic_actionability`; close immediate scalar/native timing correction because the current residual does not isolate a non-fitted production fix.
- [x] Run read-only `candidate_42f_htree_generation_precheck`; confirm branch/split spread remains structural signal, but selector-only reuse is not enough.
- [x] Run read-only `candidate_42f_htree_branch_spread_selector_precheck`; close strict same/less-complex branch-spread selector because it has `0/23` hits on retained 45a traces.
- [x] Run all-23 `candidate_42f_local_sibling_balance`; reject because skew/wire improvements relative to 45a come with latency, buffer-count, clock-cap, and clock-power regressions.
- [x] Revert rejected 42f production topology-balance code.
- [x] Run read-only `candidate_43b_cluster_root_no_worse_precheck`; confirm guarded median cluster roots exist but require all-23 production validation.
- [x] Run all-23 `candidate_43b_source_no_worse_median_cluster_root`; reject because buffer-count improves relative to 45a but skew, latency, wire, cap, and power regress.
- [x] Revert rejected 43b production cluster-root code.
- [x] Run read-only `candidate_42j_htree_split_topology_next_axis`; close selector-only H-tree retuning because all tested no-worse selector rules have `0/23` hits on retained 45a.
- [x] Run read-only `candidate_42j_htree_level_length_coverage_audit`; confirm selected levels under-cover branch maxima in `22/23` cases.
- [x] Run all-23 `candidate_42j_max_branch_length_level_plan`; reject because global max-branch level planning regresses retained-45a skew/latency/cap/power despite baseline38 improvements.
- [x] Run all-23 `candidate_42k_mean_plus_max_branch_level_plan_frontier`; reject because broad mean+max frontier exposure improves latency/buffer/wire but regresses skew/cap/power/WNS relative to retained 45a.
- [x] Revert rejected 42j/42k production level-planning code and keep the retained 45a implementation.
- [x] Run all-23 `candidate_42l_max_branch_char_coverage_only`; reject because latency/wire gains regress skew/cap/power/WNS and max-cap guard metrics relative to retained 45a.
- [x] Revert rejected 42l production level-planning coverage code and keep the retained 45a implementation.
- [x] Run read-only `candidate_48a_cluster_buffer_legality_precheck`; close cluster-buffer upsizing because traced cluster load is only `6.58%` of BUFX8 Liberty max-cap.
- [x] Run read-only `candidate_49a_timing_cell_context_counterfactual_precheck`; close broad native cell-delay substitution because table-at-Ceff is too small and table-at-driver-total-cap regresses fit.
- [x] Run read-only `candidate_50a_clarified_gate_candidate_frontier`; close direct resurrection of old rejected candidates because `0/75` non-retained all-23 candidates pass the clarified gates relative to retained 45a.
- [x] Add default-off recursive split-decision trace for retained 45a so future clustering/H-tree candidates can be analyzed at the split-decision level.
- [x] Complete all-23 `candidate_51a_split_decision_trace_refresh` workflow and join split-decision trace with L3/hierarchy/evaluation context.
- [x] Attempt `candidate_59a_binary_spatial_htree_branching`; reject and revert because global binary H-tree branching fails all-23 CTS-only completion and inflates large-case topology/search workload.
- [x] Run all-23 `candidate_60a_boundary_nearest_root_move_guard`; reject and revert because skew improves slightly but latency, wire, cap, power, WNS alignment, and max-cap guards regress versus retained 45a.
- [x] Run read-only `candidate_61a_boundary_endpoint_context_precheck`; pause standalone boundary-polish guards because endpoint/path timing context is required but unavailable at the current move-decision stage.
- [x] Run read-only `candidate_61b_weighted_branch_arrival_selector_precheck`; close selector-only weighted branch-arrival retuning because strict same/less-complex substitutions are `0/23`.
- [x] Attempt `candidate_61c_analytical_htree_mode_precheck`; close as workflow failure evidence because the temporary analytical-H-tree wrapper produced `23/23` CTS artifact failures and no evaluation.
- [x] Run all-23 `candidate_62a_htree_local_split_pareto_axis`; reject and revert because core metrics improve versus retained 45a, but DRV max-cap alignment and DRC marker regressions fail the no-deterioration gate.
- [x] Remove the temporary analytical-H-tree wrapper and revert the 62a local split-axis production diff.
- [x] Run all-23 `candidate_62b_route_envelope_local_split_axis`; reject and revert because worst route-envelope guarding is exactly equivalent to 62a and still fails DRV/DRC guards.
- [x] Run all-23 `candidate_62c_spread_guarded_local_split_axis`; initially retained under all-23 evidence, then reclassified after layered benchmark analysis because it has no `core16` standalone effect and mainly moves quarantine cases.
- [x] Run all-23 `candidate_63a_anchor_aware_local_split`; reject and revert because it regresses the current retained 62c result on the only moved case.
- [x] Run read-only `candidate_64b_local_split_generation_scope_trace_refresh`; close as QoR-neutral scoped trace evidence for legality/materialization local split generation.
- [x] Run all-23 `candidate_65a_relaxed_aggregate_local_split_guard`; reject and revert because skew improves but latency, wire, cap, and power regress versus retained 62c.
- [x] Run all-23 `candidate_66a_drop_child_total_local_split_guard`; reject and revert because skew improves but latency/cap alignment and DRC regress versus retained 62c.
- [x] Run all-23 `candidate_67a_multi_child_total_diameter_relaxation`; reject and revert because only `vga_lcd` moves and skew/latency/cap improve, but clock wire regresses versus retained 62c.
- [x] Run all-23 `candidate_68a_weighted_branch_arrival_exposure`; reject and revert because the topology-multiplicity-weighted branch-arrival exposure proxy is final-metric equivalent to retained 62c and produces no benchmark improvement.
- [x] Run all-23 `candidate_69a_branch_exposure_pareto_dimension`; reject and revert because adding branch-arrival exposure as a physical-frontier Pareto dimension is final-metric equivalent to retained 62c and produces no benchmark improvement.
- [x] Run `candidate_70a_materialized_local_split_wire_guard`; reject and revert because the materialized-wire guard still trades skew improvement for wire/WNS/DRV regressions, and one Innovus evaluation crashed before all-23 completion.

## Validation Commands

Build:

```bash
cmake --build build-gcc11-release --target ecc_bin -j 8
```

Targeted checks:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -m py_compile .trellis/tasks/06-15-cts-next-stage-capability-research/scripts/*.py
git diff --check -- src/operation/iCTS .trellis/tasks/06-15-cts-next-stage-capability-research
ctest --test-dir build-gcc11-release -R "icts_test_flow_synthesis_htree|icts_test_database_adapter_fast_sta" --output-on-failure
```

All-23 workflow:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 .trellis/tasks/06-15-cts-next-stage-capability-research/scripts/run_candidate_workflow.py --candidate <candidate_name> --jobs 4
```

Split-decision trace analysis:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 .trellis/tasks/06-15-cts-next-stage-capability-research/scripts/analyze_split_decision_trace.py --summary-dir <summary_dir> --out-dir <analysis_dir>
```

Benchmark-layer analysis:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 .trellis/tasks/06-15-cts-next-stage-capability-research/scripts/analyze_benchmark_layers.py --out-dir /nfs/share/home/liweiguo/ecc_cts_innovus_align/evaluations/06-15-cts-next-stage-capability-research/summary/benchmark_layer_remote_baseline
PYTHONDONTWRITEBYTECODE=1 python3 .trellis/tasks/06-15-cts-next-stage-capability-research/scripts/analyze_benchmark_layers.py --candidate-summary <candidate_summary>/ecc-tools.summary.csv --out-dir <layered_analysis_dir>
```

Full `ecc_dev_tools` remains reserved for finish-work.

## Remote Baseline Suitability

Compact local report:

`remote_baseline_case_suitability_report.md`

NFS evidence:

- `summary/remote_baseline_case_suitability_analysis/remote_baseline_case_suitability_report.md`
- `summary/benchmark_layer_remote_baseline`
- `summary/benchmark_layer_candidate_71a_vs_remote`

Decision:

- primary benchmark is `core16`;
- `extended23` remains a completion and catastrophic-regression guard;
- `quarantine7` contains `iwls2005__ac97_ctrl`, `iwls2005__mem_ctrl`, `iwls2005__pci`, `iwls2005__usb_funct`, `openroad__ethmac`, `openroad__fifo`, and `iwls2005__vga_lcd`;
- `iwls2005__ss_pcm` remains in `core16` as a watchlist case.

## Current Baseline Snapshot

Baseline summary:

`/nfs/share/home/liweiguo/ecc_cts_innovus_align/evaluations/06-15-cts-next-stage-capability-research/summary/candidate_38a_local_split_physical_guard_refresh`

| Signal | Reading |
| --- | ---: |
| Final skew MAE / RMSE / R2 | `16.6522 ps` / `22.854 ps` / `-0.797488` |
| Latency MAE / RMSE / R2 | `48.2174 ps` / `54.8094 ps` / `0.714618` |
| Same-tree skew MAE / RMSE / R2 | `8.48199 ps` / `13.6809 ps` / `0.726535` |
| Endpoint match | min `2/23`, max `3/23`, both `0/23` |
| Cell-stage delay MAE / RMSE | `2.81787 ps` / `4.42254 ps` |
| Wire-stage delay MAE / RMSE | `0.391373 ps` / `0.861325 ps` |
| Structural attribution after timing caveat | H-tree/split `20/23`, cluster/local `3/23` |

## Latest Trial: `candidate_40b_path_safe_local_split_axis_span_guard`

Code direction tested:

- allow a local split to choose the alternate axis only when branch-distance spread improves;
- add guards for max branch distance, weighted branch distance, child HPWL sum, child HPWL max, and child HPWL spread;
- keep no user config, no scalar margin, no case rule.

Decision: rejected and reverted.

| Metric | Baseline MAE | 40b MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `16.6087 ps` | `-0.0435 ps` |
| Latency | `48.2174 ps` | `48.3043 ps` | `+0.0869 ps` |
| Clock wire | `1757.39 um` | `1756.99 um` | `-0.4059 um` |
| Clock total cap | `0.26713 pF` | `0.26987 pF` | `+0.00274 pF` |
| Clock power | `0.119869 mW` | `0.119347 mW` | `-0.000522 mW` |
| WNS alignment | `285.609 ps` | `289.087 ps` | `+3.478 ps` |

Supporting files:

- `summary/candidate_40b_path_safe_local_split_axis_span_guard/decision_compare/candidate_40b_vs_baseline_metric_fit.csv`
- `summary/candidate_40b_path_safe_local_split_axis_span_guard/decision_compare/candidate_40b_vs_baseline_qor_direction.csv`
- `summary/candidate_40b_path_safe_local_split_axis_span_guard/l3_alignment/l3_alignment_report.md`

## Counterfactual Check

Baseline38 H-tree selection trace was queried for alternatives that satisfy:

`delay <= selected`, `branch-arrival proxy <= selected`, `weighted branch-arrival proxy <= selected`, `physical depth <= selected`, `physical buffer count <= selected`, and lower power.

Result: `0/23` cases had such a replacement in the existing front.

Implication: the next production candidate should not be selector-only retuning over the current front. It should either improve native timing semantics or generate new physical H-tree topology alternatives.

## Latest Trial: `candidate_42b_shrink_only_topology_balance`

Code direction tested:

- change `TopologyGen::balanceTopology` from bidirectional normalization to shrink-only branch projection;
- keep no new config, no scalar margin, and no case-specific rule.

Decision: rejected and reverted.

| Metric | Baseline MAE | 42b MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `17.0870 ps` | `+0.4348 ps` |
| Latency | `48.2174 ps` | `49.3261 ps` | `+1.1087 ps` |
| Buffer count | `24.2609` | `22.1739` | `-2.0870` |
| Clock wire | `1757.39 um` | `1667.57 um` | `-89.82 um` |
| Clock total cap | `0.26713 pF` | `0.314783 pF` | `+0.047653 pF` |
| Clock power | `0.119869 mW` | `0.234539 mW` | `+0.11467 mW` |

Interpretation: broad shrink-only topology generation can reduce wire and buffer count, but it perturbs large-case buffering enough to increase clock cap/power and final skew/latency. The next H-tree candidate must preserve a narrower physical invariant.

## Latest Trial: `candidate_42c_preserve_leaf_cluster_centers`

Code direction tested:

- keep leaf topology nodes at sink-cluster centers by applying topology balance only to internal H-tree junction nodes;
- keep no new config, no scalar margin, and no case-specific rule.

Decision: rejected and reverted.

| Metric | Baseline MAE | 42c MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `17.0000 ps` | `+0.3478 ps` |
| Latency | `48.2174 ps` | `48.6957 ps` | `+0.4783 ps` |
| Buffer count | `24.2609` | `24.2609` | `0.0000` |
| Clock wire | `1757.39 um` | `1743.37 um` | `-14.02 um` |
| Clock total cap | `0.26713 pF` | `0.264348 pF` | `-0.002782 pF` |
| Clock power | `0.119869 mW` | `0.119477 mW` | `-0.000392 mW` |

Interpretation: preserving leaf cluster centers is a real cap/power/wire lever, but it weakens final skew/latency and same-tree timing closure when used alone. Do not keep it without an accompanying endpoint-order-preserving H-tree balancing mechanism.

## Latest Trial: `candidate_43a_l1_median_cluster_root`

Read-only precheck:

- L1-median cluster roots reduce final-cluster Manhattan proxy in `23/23` cases.
- Mean proxy delta was `-331164 dbu`, or `-5.23202%` relative.
- The effect also held for the `cluster_local` bucket: `3/3` cases improved.

Code direction tested:

- use the same L1-median root policy for fast clustering objective/electrical evaluation and materialized cluster-buffer location;
- keep no new config, no scalar margin, and no case-specific rule.

Decision: rejected and reverted.

| Metric | Baseline MAE | 43a MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `19.6087 ps` | `+2.9565 ps` |
| Latency | `48.2174 ps` | `47.0652 ps` | `-1.1522 ps` |
| Buffer count | `24.2609` | `22.7391` | `-1.5218` |
| Clock wire | `1757.39 um` | `2111.29 um` | `+353.9 um` |
| Clock total cap | `0.26713 pF` | `0.257217 pF` | `-0.009913 pF` |
| Clock power | `0.119869 mW` | `0.0969848 mW` | `-0.0228842 mW` |

Interpretation: median roots are a valid local cap/power/latency lever, but standalone use perturbs endpoint ordering and wire alignment enough to fail the current acceptance gate. Do not retry this as a standalone clustering rewrite; only revisit it as a guarded sub-mechanism after H-tree endpoint-order behavior is improved.

## Latest Read-Only Audit: `candidate_42e_multi_topology_pareto_opportunity`

Scope:

- compare historical topology and clustering candidates against baseline38 at candidate/case level;
- test whether final-safe opportunities can be explained by non-fitted ECC-visible Pareto rules from H-tree selection traces;
- avoid production code changes until a native safe selector exists.

Decision: closed as read-only evidence.

| Signal | Result |
| --- | ---: |
| Candidate/case pairs audited | `138` |
| Pairs with at least one final metric improvement | `74` |
| Pairs with improvement and no key-metric regression | `6` |
| Safe cases by final metrics | `4` |
| Tested Pareto rules with no false positives | `0` |

Interpretation: historical topology rewrites contain useful upper-bound signal, but the tested ECC-visible rules are not reliable selectors. Do not build a production chooser from final Innovus/evaluation outcomes. The next topology attempt must expose new generated candidates or new native trace observables that explain no-regression selection.

Supporting files:

- `summary/candidate_42e_multi_topology_pareto_opportunity/multi_topology_pareto_opportunity_report.md`
- `summary/candidate_42e_multi_topology_pareto_opportunity/multi_topology_candidate_case_rows.csv`
- `summary/candidate_42e_multi_topology_pareto_opportunity/multi_topology_rule_summary.csv`

## Current Retained Candidate: `candidate_45a_pareto_split_axis`

Code direction retained:

- evaluate longest-axis and alternate-axis recursive split plans in fast clustering;
- keep longest-axis as the default fallback;
- accept alternate-axis only when native split score improves and child count, split distance, routing-cap balance/spread, utilization penalty, and child-diameter envelope are no worse;
- keep no new config, scalar margin, fitted threshold, or case-specific rule.

Core all-23 result:

| Metric | Baseline MAE | 45a MAE | Delta | Raw mean delta |
| --- | ---: | ---: | ---: | ---: |
| Skew | `16.6522 ps` | `15.4348 ps` | `-1.21739 ps` | `-0.00147826 ns` |
| Latency | `48.2174 ps` | `46.8478 ps` | `-1.36957 ps` | `-0.00241304 ns` |
| Buffer count | `24.2609` | `23.6957` | `-0.565217` | `-0.565217` |
| Clock wire | `1757.39 um` | `1640.46 um` | `-116.932 um` | `-116.932 um` |
| Clock total cap | `0.26713 pF` | `0.222913 pF` | `-0.0442174 pF` | `-0.0503913 pF` |
| Clock power | `0.119869 mW` | `0.0890196 mW` | `-0.0308491 mW` | `-0.0308491 mW` |

Guard residual:

- DRV max-tran alignment MAE regresses by `+0.130435` real nets.
- Raw DRV max-cap, DRV max-tran, and DRC marker means increase slightly.
- WNS alignment improves by `-90.8261 ps`, and raw WNS mean improves by `+0.0113478 ns`.

Decision: retain 45a as the core-QoR base production candidate. Do not retain 45b/45c/45d because their stronger guards trade away latency or other guard metrics. Later retained local split-generation work is documented separately in the 62c section.

## Read-Only Guard Scope Audit: `candidate_46a_guard_residual_scope`

Scope:

- parse 45a versus baseline38 Innovus `*.cap.gz`, `*.tran.gz`, and `drc.rpt`;
- classify residual nets as `clock_cts`, `design_signal`, or `power`;
- avoid adding a CTS heuristic until the residual is proven to be a CTS-clock net effect.

Result:

| Signal | Count |
| --- | ---: |
| Added max-cap clock CTS nets | `0` |
| Added max-cap design-signal nets | `15` |
| Added max-tran clock CTS nets | `0` |
| Added max-tran design-signal nets | `8` |
| Named DRC clock CTS marker delta | `0` |

Decision: close extra CTS-side guard tuning for 45a. The remaining guard deltas are post-route side effects on ordinary design-signal or unnamed/non-clock DRC context, not direct CTS clock-net regressions. Further split-axis/clustering/H-tree guards aimed at these residuals would be indirect and likely overfit detail-route behavior.

Supporting files:

- `summary/candidate_46a_guard_residual_scope/guard_residual_scope_report.md`
- `summary/candidate_46a_guard_residual_scope/guard_residual_scope_case_rows.csv`

## Read-Only Timing Semantic Audit: `candidate_47a_timing_semantic_actionability`

Scope:

- inspect retained 45a same-tree FastSTA-vs-Innovus reports;
- compare final-arrival residual against cell-step, wire-step, slew, driver-total-cap, Ceff, threshold, and DMP context;
- decide whether P41 has enough evidence for a native production timing fix.

Result:

| Signal | Reading |
| --- | ---: |
| Same-tree skew MAE / R2 | `8.44899 ps` / `0.802146` |
| Endpoint match | min `1/23`, max `2/23`, both `0/23` |
| Skew-pair final-arrival residual MAE | `13.2078 ps` |
| Cell-step delta MAE | `12.0931 ps` |
| Wire-step delta MAE | `1.44118 ps` |
| Slew+driver-total-cap term MAE | `11.921 ps` |
| Residual after slew/driver-total-cap+wire MAE | `1.55921 ps` |

Decision: do not add scalar timing correction, threshold tweak, DMP switch, or wire/RC adjustment. The current timing residual is mostly cell-context observer mismatch, and the unexplained post-context residual is too small to justify production timing-model behavior. Carry the timing caveat into the structural work instead.

Supporting files:

- `summary/candidate_47a_timing_semantic_actionability/timing_semantic_actionability_report.md`

## H-Tree Follow-Up: `candidate_42f`

Read-only prechecks:

- `candidate_42f_htree_generation_precheck` found H-tree split dominant endpoint pairs in `19/23` cases and no-worse delay/power/branch-spread alternatives in `14/23`, so branch/split spread remains a real structural signal.
- `candidate_42f_htree_branch_spread_selector_precheck` found `0/23` strict same/less-complex branch-spread selector hits on retained 45a traces, so selector-only branch-spread retuning is closed.

Code direction tested:

- change `TopologyGen::balanceTopology` from global per-level edge-distance balancing to parent-local sibling edge balancing;
- reuse existing topology tolerance and add no behavior config, scalar margin, fitted threshold, or case-specific rule.

Decision: rejected and reverted.

| Metric | 45a MAE | 42f MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `14.6522 ps` | `-0.7826 ps` |
| Latency | `46.8478 ps` | `47.3478 ps` | `+0.5000 ps` |
| Buffer count | `23.6957` | `23.7826` | `+0.0869` |
| Clock wire | `1640.46 um` | `1608.69 um` | `-31.77 um` |
| Clock total cap | `0.222913 pF` | `0.235565 pF` | `+0.012652 pF` |
| Clock power | `0.0890196 mW` | `0.101741 mW` | `+0.012721 mW` |

Interpretation: local sibling balancing is useful negative evidence. It proves branch-local topology movement can reduce skew and wire, but it is not a monotonic no-regression invariant relative to retained 45a. Future P42 work should not repeat broad local branch balancing unless a stronger endpoint-order and cap/power guard is proven from trace first.

Supporting files:

- `summary/candidate_42f_htree_generation_precheck/htree_split_rank_precheck_report.md`
- `summary/candidate_42f_htree_branch_spread_selector_precheck/htree_branch_spread_selector_precheck_report.md`
- `summary/candidate_42f_local_sibling_balance/first_principles_skew_alignment/first_principles_skew_alignment_report.md`
- `summary/candidate_42f_local_sibling_balance/htree_branch_balance/htree_branch_balance_analysis_report.md`

## Cluster-Root Follow-Up: `candidate_43b`

Read-only precheck:

- `candidate_43b_cluster_root_no_worse_precheck` tested a no-knob L1-median cluster root rule guarded by local root-to-sink Manhattan sum, source-to-root Manhattan distance, total cap, and wirelength.
- It found at least one no-worse median cluster in `22/23` cases and in `4/5` cluster-final cases, so the physical opportunity is real enough for a production trial.

Code direction tested:

- after default fast clustering, move a cluster buffer from geometric center to L1 median only when all local physical/electrical summaries are no worse;
- pass only execution context, not behavior config; add no scalar margin, fitted threshold, or case-specific rule.

Decision: rejected and reverted.

| Metric | 45a MAE | 43b MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.5652 ps` | `+0.1304 ps` |
| Latency | `46.8478 ps` | `48.2826 ps` | `+1.4348 ps` |
| Buffer count | `23.6957` | `23.4783` | `-0.2174` |
| Clock wire | `1640.46 um` | `1772.39 um` | `+131.93 um` |
| Clock total cap | `0.222913 pF` | `0.250609 pF` | `+0.027696 pF` |
| Clock power | `0.0890196 mW` | `0.111067 mW` | `+0.022047 mW` |

Interpretation: source/local no-worse median roots are still not a suite-wide safe invariant. They can reduce buffer-count alignment slightly, but relative to retained 45a they regress skew, latency, wire, cap, and power. Cluster-root movement should stay a second-order targeted direction and must be coupled to endpoint-order/H-tree evidence before another production attempt.

Supporting files:

- `summary/candidate_43b_cluster_root_no_worse_precheck/cluster_root_no_worse_precheck_report.md`
- `summary/candidate_43b_source_no_worse_median_cluster_root/first_principles_skew_alignment/first_principles_skew_alignment_report.md`
- `summary/candidate_43b_source_no_worse_median_cluster_root/skew_hierarchy_root_cause/skew_hierarchy_root_cause_report.md`

## Local Split Generation Follow-Up: `candidate_42g`

Read-only precheck:

- `candidate_42g_split_local_generation_precheck` showed that local split is active in most retained 45a H-tree-structural cases.
- The selected internal-level proxy share is low, so the first tested generation change should target `SplitSinkLoadRegionGroup`, not global H-tree selector retuning.

Code direction tested:

- replace same-level multi-child local split slicing with recursive same-count split-group generation;
- rerun the longest-axis split inside each recursive subregion;
- preserve max-fanout, child-count, split-buffer-count, and electrical legality contracts;
- add no behavior config, fitted threshold, scalar margin, or case rule.

Validation:

- all-23 CTS-only: `23/23` succeeded.
- all-23 conversion/evaluation/summary: `47/47` succeeded.
- local build and tests passed before and after rejection/revert.

Decision: rejected and reverted.

| Metric | 45a MAE | 42g MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.3043 ps` | `-0.1305 ps` |
| Latency | `46.8478 ps` | `47.9130 ps` | `+1.0652 ps` |
| Buffer count | `23.6957` | `22.4348` | `-1.2609` |
| Clock wire | `1640.46 um` | `1679.88 um` | `+39.42 um` |
| Clock total cap | `0.222913 pF` | `0.237652 pF` | `+0.014739 pF` |
| Clock power | `0.0890196 mW` | `0.119923 mW` | `+0.0309034 mW` |

Interpretation: 42g is useful negative evidence. It confirms recursive local split generation can improve skew and buffer-count alignment, but it regresses latency/wire/cap/power alignment relative to retained 45a and also worsens raw clock wire/cap/power. Under the clarified goal, this violates the no-deterioration gate.

Supporting files:

- `summary/candidate_42g_split_local_generation_precheck/htree_split_local_generation_precheck_report.md`
- `summary/candidate_42g_recursive_local_split_generation/first_principles_skew_alignment/first_principles_skew_alignment_report.md`
- `summary/candidate_42g_recursive_local_split_generation/skew_hierarchy_root_cause/skew_hierarchy_root_cause_report.md`
- `summary/candidate_42g_recursive_local_split_generation/ecc-tools.summary.csv`
- `summary/candidate_42h_local_split_generation_delta_audit/local_split_generation_delta_audit_report.md`
- `summary/candidate_42i_guarded_recursive_local_split_generation/first_principles_skew_alignment/first_principles_skew_alignment_report.md`
- `summary/candidate_42i_guarded_recursive_local_split_generation/skew_hierarchy_root_cause/skew_hierarchy_root_cause_report.md`

## Guarded Local Split Generation Follow-Up: `candidate_42i`

Evidence from 42h:

- 42g had `9/23` core-fit no-regression cases but raw physical cost regressed in `11/23`.
- Therefore the recovery attempt needed a native direct-vs-recursive physical no-worse invariant.

Code direction tested:

- build direct local split groups exactly as the retained implementation does;
- build recursive local split groups as a candidate only when `child_count > 2`;
- accept recursive groups only if total/max parent-to-child center wire and total/max child bbox diameter are all no worse than direct groups.

Validation:

- all-23 CTS-only: `23/23` succeeded.
- all-23 conversion/evaluation/summary: `47/47` succeeded.
- local build and tests passed before and after rejection/revert.

Decision: rejected and reverted.

| Metric | 45a MAE | 42i MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.4348 ps` | `0.0000 ps` |
| Latency | `46.8478 ps` | `46.8043 ps` | `-0.0435 ps` |
| Buffer count | `23.6957` | `23.6957` | `0.0000` |
| Clock wire | `1640.46 um` | `1640.53 um` | `+0.07 um` |
| Clock total cap | `0.222913 pF` | `0.222957 pF` | `+0.000044 pF` |
| Clock power | `0.0890196 mW` | `0.0890196 mW` | `0.0000 mW` |

Interpretation: the guard prevented the broad 42g physical-cost regression, but the remaining accepted movement was too small and still created tiny wire/cap alignment regressions relative to retained 45a. The local split generation branch is closed for now.

## Cluster-Final Buffer Sizing Follow-Up: `candidate_43c`

Evidence:

- `candidate_43c_cluster_final_outlier_audit` found `5/23` retained 45a cluster-final cases.
- In those cases, the max endpoint's cluster-delay and final-cap contribution can be comparable to or larger than H-tree contribution.
- The same refreshed hierarchy reports still rank H-tree/split as the dominant all-23 axis, so this was treated as a narrow standalone trial rather than the primary direction.

Code direction tested:

- collect all configured cluster buffer masters from `buffer_type`;
- estimate each cluster's downstream load with the existing exact electrical summary;
- choose a per-cluster buffer master by native Liberty direct-delay cost, without new config, fitted scalar, or case rule;
- preserve cluster membership, cluster center, max-fanout/max-cap legality, and H-tree generation code.

Validation:

- `cmake --build build-gcc11-release --target ecc_bin icts_test_flow_synthesis_htree icts_test_module_topology_fast_clustering -j 8` passed before the run.
- `ctest --test-dir build-gcc11-release -R "icts_test_module_topology_fast_clustering|icts_test_flow_synthesis_htree" --output-on-failure` passed before the run.
- `PYTHONDONTWRITEBYTECODE=1 python3 .trellis/tasks/06-15-cts-next-stage-capability-research/scripts/run_candidate_workflow.py --candidate candidate_43c_per_cluster_buffer_delay_choice --jobs 4` completed with `23/23` CTS-only, `23/23` conversion, `23/23` Innovus evaluation, and summary success.

Decision: rejected and reverted.

| Metric | 45a MAE | 43c MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `13.9565 ps` | `-1.4783 ps` |
| Latency | `46.8478 ps` | `56.0217 ps` | `+9.1739 ps` |
| Buffer count | `23.6957` | `22.9565` | `-0.7391` |
| Clock wire | `1640.46 um` | `2174.79 um` | `+534.33 um` |
| Clock total cap | `0.222913 pF` | `1.4573 pF` | `+1.23439 pF` |
| Clock power | `0.0890196 mW` | `1.13518 mW` | `+1.04616 mW` |

Root cause: 43c selected `BUFX20H7L` for `11152/11156` cluster buffers. It proves a real skew-reduction lever, but it is not an accepted optimization under the clarified gate because latency, wire, cap, and power regress broadly.

## H-Tree Level-Length Follow-Up: `candidate_42j` / `candidate_42k`

Evidence:

- `candidate_42j_htree_split_topology_next_axis` found no selector-only replacement on retained 45a: all tested no-worse H-tree rules had `0/23` hits.
- `candidate_42j_htree_level_length_coverage_audit` found selected levels under-cover branch maxima in `22/23` cases and `34/45` selected levels.
- Mean weighted undercoverage was `20.1092 um`; correlation with endpoint window was `0.353617`, and correlation with skew error was `0.382861`.

Code directions tested:

- `candidate_42j_max_branch_length_level_plan`: globally characterize/plan each level at branch max instead of average parent-child length.
- `candidate_42k_mean_plus_max_branch_level_plan_frontier`: keep the average plan as default and additionally expose a max-branch plan set to the existing global frontier selector.
- Both directions added no behavior config, fitted scalar, magic threshold, or case-specific rule.

42k incremental result versus retained 45a:

| Metric | 45a MAE | 42k MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `16.7826 ps` | `+1.3478 ps` |
| Latency | `46.8478 ps` | `40.2391 ps` | `-6.6087 ps` |
| Buffer count | `23.6957` | `23.5217` | `-0.1739` |
| Clock wire | `1640.46 um` | `1622.87 um` | `-17.59 um` |
| Clock total cap | `0.222913 pF` | `0.238087 pF` | `+0.015174 pF` |
| Clock power | `0.0890196 mW` | `0.0920432 mW` | `+0.003024 mW` |

Decision: rejected and reverted. Max-branch coverage is a valid diagnostic signal, but broad use is not a safe production invariant. It improves latency/buffer/wire while regressing skew/cap/power/WNS, so retained production code stays at 45a.

Run note: one 42k Innovus child process remained alive after all report files were generated for `iwls2005__ac97_ctrl`. The all-23 report set was complete, so the stale process tree was stopped and `ecc-tools.summary.csv` plus downstream analyses were regenerated from exactly the 23 directories containing `clock_timing_summary.rpt`.

Supporting files:

- `summary/candidate_42j_htree_split_topology_next_axis/htree_split_topology_next_axis_report.md`
- `summary/candidate_42j_htree_level_length_coverage_audit/htree_level_length_coverage_report.md`
- `summary/candidate_42j_max_branch_length_level_plan/ecc-tools.summary.csv`
- `summary/candidate_42k_mean_plus_max_branch_level_plan_frontier/ecc-tools.summary.csv`
- `summary/candidate_42k_mean_plus_max_branch_level_plan_frontier/first_principles_skew_alignment/first_principles_skew_alignment_report.md`

## Current Local Validation

After reverting rejected 45b/45c/45d, 42f, 43b, 42g, 42i, 43c, 42j, 42k, and 42l code to the retained 45a implementation:

- `PYTHONDONTWRITEBYTECODE=1 python3 -m py_compile .trellis/tasks/06-15-cts-next-stage-capability-research/scripts/*.py` passed.
- `git diff --check -- src/operation/iCTS .trellis/tasks/06-15-cts-next-stage-capability-research` passed.
- `cmake --build build-gcc11-release --target ecc_bin icts_test_module_topology_fast_clustering icts_test_flow_synthesis_htree -j 8` passed.
- `ctest --test-dir build-gcc11-release -R "icts_test_module_topology_fast_clustering|icts_test_flow_synthesis_htree" --output-on-failure` passed.

## Rejected Follow-up: `candidate_42l_max_branch_char_coverage_only`

Purpose: test whether the max-branch undercoverage issue can be used narrowly in characterization coverage without changing mean level planning or exposing a broad max-level frontier.

Evidence:

- workflow manifest: `summary/candidate_42l_max_branch_char_coverage_only/run_manifest.json`
- all-23 summary: `summary/candidate_42l_max_branch_char_coverage_only/ecc-tools.summary.csv`
- max-plan precheck: `summary/candidate_42l_max_branch_char_coverage_precheck/max_branch_level_plan_frontier_effect_report.md`

All-23 workflow completed with `23/23` CTS success and `23/23` Innovus evaluation success.

Incremental MAE versus retained 45a:

| Metric | 45a | 42l | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.6087 ps` | `+0.1739 ps` |
| Average latency | `46.8478 ps` | `40.4348 ps` | `-6.4130 ps` |
| Buffer count | `23.6957` | `23.6957` | `0.0000` |
| Clock wire | `1640.46 um` | `1637.96 um` | `-2.50 um` |
| Clock total cap | `0.222913 pF` | `0.225609 pF` | `+0.002696 pF` |
| Clock power | `0.089020 mW` | `0.089307 mW` | `+0.000287 mW` |

Raw guard deltas versus retained 45a:

| Metric | Delta |
| --- | ---: |
| WNS mean | `-11.70 ps` |
| max-cap real-net count | `+0.0435` |
| max-tran real-net count | `-0.0435` |
| DRC marker count | `-0.0435` |

Decision: rejected and production code reverted. The user clarified that partial benchmark improvement is sufficient only when other key metrics do not regress. 42l has real latency/wire benefit, but it is not Pareto-safe because skew, cap, power, WNS, and max-cap guard metrics regress.

Conclusion: close broad and narrow max-branch level-length coverage as an immediate production lever. Keep max-branch undercoverage as a diagnostic signal only; future H-tree work must preserve endpoint order and cap/power invariants natively.

## Closed Precheck: `candidate_48a_cluster_buffer_legality_precheck`

Purpose: test whether cluster-final outliers justify per-cluster buffer upsizing from electrical load legality, rather than final metric tuning.

Evidence:

- `summary/candidate_48a_cluster_buffer_legality_precheck/cluster_buffer_legality_precheck_report.md`
- `summary/candidate_48a_cluster_buffer_legality_precheck/cluster_buffer_legality_buffer_caps.csv`
- `summary/candidate_48a_cluster_buffer_legality_precheck/cluster_buffer_legality_case_summary.csv`

Key facts:

- Configured CTS buffers are `BUFX8H7L`, `BUFX12H7L`, `BUFX16H7L`, `BUFX20H7L`.
- Liberty output cap limit for the smallest configured buffer `BUFX8H7L` is `0.325346 pF`.
- Maximum traced cluster `total_cap_pf` across all 23 cases is `0.02141153028 pF`.
- The largest cluster load is only `6.58%` of the smallest buffer's Liberty cap limit.

Decision: no production code. Per-cluster cluster-buffer upsizing is not electrically required and would likely repeat the 43c delay-driven upsizing failure mode.

## Closed Precheck: `candidate_49a_timing_cell_context_counterfactual_precheck`

Purpose: test whether existing trace-visible native cell-delay alternatives can reduce ECC-vs-Innovus timing error enough to justify a production timing change.

Evidence:

- `summary/candidate_49a_timing_cell_context_counterfactual_precheck/timing_cell_context_counterfactual_report.md`
- `summary/candidate_49a_timing_cell_context_counterfactual_precheck/cell_delay_model_summary.csv`
- `summary/candidate_49a_timing_cell_context_counterfactual_precheck/cell_delay_model_by_role.csv`
- `summary/candidate_49a_timing_cell_context_counterfactual_precheck/skew_pair_cell_counterfactual_rows.csv`

Retained-45a counterfactual:

| Metric | Current ECC step | Table at Ceff | Table at driver-total cap |
| --- | ---: | ---: | ---: |
| Cell-stage MAE | `2.55323 ps` | `2.53143 ps` | `2.6894 ps` |
| Cell-stage RMSE | `3.91545 ps` | `3.8963 ps` | `4.08421 ps` |
| Endpoint cell-path sum MAE | `9.06123 ps` | `9.03296 ps` | `9.36715 ps` |
| Same-tree cell-skew contribution MAE | `12.0931 ps` | `12.0786 ps` | `12.2109 ps` |

Decision: no production code. Table-at-Ceff improves the retained candidate by only `0.0218 ps` at cell-stage MAE and `0.0145 ps` at same-tree cell-skew contribution MAE, with cases split `12/11` improved/regressed. Table-at-driver-total-cap worsens both. Timing semantics remains a monitoring direction until trace proves a deterministic non-scalar mismatch that improves endpoint-order evidence.

## Closed Precheck: `candidate_50a_clarified_gate_candidate_frontier`

Purpose: re-audit all already-run all-23 candidate summaries under the user's clarified acceptance rule: partial benchmark improvement is valid only if other key benchmark metrics do not regress relative to the current retained production code.

Evidence:

- `summary/candidate_50a_clarified_gate_candidate_frontier/clarified_gate_candidate_frontier_report.md`
- `summary/candidate_50a_clarified_gate_candidate_frontier/clarified_gate_candidate_rows.csv`
- `summary/candidate_50a_clarified_gate_candidate_frontier/clarified_gate_metric_leaders.csv`

Result:

| Gate | Passing non-retained candidates |
| --- | ---: |
| Core alignment only | `0/75` |
| Core alignment plus raw QoR | `0/75` |
| Core plus guard metrics | `0/75` |

Decision: no old rejected production diff should be resurrected as-is. The closest partial historical signal is `candidate_cc_objective_split_axis_eval`: it improves `5/6` core alignment metrics and all `6/6` raw core means, but regresses skew alignment and `3` guard alignment metrics and has no trace-backed first-principles report. It is only a direction hint, not production evidence.

## Next Iteration

## Rejected Follow-up: `candidate_52a_rms_level_length_plan`

Purpose: test whether RMS level length is a narrower first-principles response to branch-length variance than global max-branch planning.

Evidence:

- workflow manifest: `summary/candidate_52a_rms_level_length_plan/run_manifest.json`
- all-23 summary: `summary/candidate_52a_rms_level_length_plan/ecc-tools.summary.csv`
- first-principles reports under `summary/candidate_52a_rms_level_length_plan/*`

All-23 workflow completed with `23/23` CTS success and `23/23` Innovus evaluation success.

Incremental MAE versus retained 45a:

| Metric | 45a | 52a | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.3478 ps` | `-0.0870 ps` |
| Average latency | `46.8478 ps` | `49.7391 ps` | `+2.8913 ps` |
| Buffer count | `23.6957` | `23.7391` | `+0.0435` |
| Clock wire | `1640.46 um` | `1638.39 um` | `-2.07 um` |
| Clock total cap | `0.222913 pF` | `0.224348 pF` | `+0.001435 pF` |
| Clock power | `0.089020 mW` | `0.089498 mW` | `+0.000478 mW` |

Decision: rejected and production code reverted. The rule changed only `openroad__jpeg`: skew improved by `2 ps` and wire by `47.558 um`, but latency worsened by `66.5 ps`, buffer count by `1`, cap by `0.033 pF`, and clock power by `0.011 mW`.

Conclusion: RMS level length is a useful negative control for branch variance, but it is still too broad as production behavior. Future H-tree planning must localize branch-length coverage with endpoint-order and cap/power invariants rather than replacing mean level length.

## Rejected Follow-up: `candidate_53a_power_preserving_branch_arrival_overlay`

Purpose: test whether the retained branch-arrival exposure selector should be additionally constrained to candidates whose modeled H-tree pattern power is not higher than the selected midpoint candidate.

Evidence:

- workflow manifest: `summary/candidate_53a_power_preserving_branch_arrival_overlay/run_manifest.json`
- all-23 summary: `summary/candidate_53a_power_preserving_branch_arrival_overlay/ecc-tools.summary.csv`
- first-principles reports under `summary/candidate_53a_power_preserving_branch_arrival_overlay/*`

All-23 workflow completed with `23/23` CTS success and `23/23` Innovus evaluation success.

Incremental MAE versus retained 45a:

| Metric | 45a | 53a | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `17.9565 ps` | `+2.5217 ps` |
| Average latency | `46.8478 ps` | `62.1304 ps` | `+15.2826 ps` |
| Buffer count | `23.6957` | `23.6957` | `0.0000` |
| Clock wire | `1640.46 um` | `1643.85 um` | `+3.39 um` |
| Clock total cap | `0.222913 pF` | `0.189522 pF` | `-0.033391 pF` |
| Clock power | `0.089020 mW` | `0.059169 mW` | `-0.029851 mW` |

Guard movement versus retained 45a: WNS improved by `+0.063 ns`, but TNS worsened by `-0.007 ns` and violating paths increased from `0.0435` to `0.1739` on all-23 mean.

Decision: rejected and production code reverted. The candidate proves that power no-regression inside the existing selector front is not enough: it improves cap/power alignment by suppressing high-power topology candidates, but that also removes candidates carrying useful endpoint-order behavior, causing material skew and latency regression.

## Rejected Follow-up: `candidate_54a_leaf_max_branch_level_plan`

Purpose: test a narrower level-length response to branch undercoverage by using max branch length only on the terminal leaf level while leaving upper levels on the mean requested length.

Evidence:

- workflow manifest: `summary/candidate_54a_leaf_max_branch_level_plan/run_manifest.json`
- all-23 summary: `summary/candidate_54a_leaf_max_branch_level_plan/ecc-tools.summary.csv`
- first-principles reports under `summary/candidate_54a_leaf_max_branch_level_plan/*`

All-23 workflow completed with `23/23` CTS success and `23/23` Innovus evaluation success.

Incremental MAE versus retained 45a:

| Metric | 45a | 54a | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.4348 ps` | `0.0000 ps` |
| Average latency | `46.8478 ps` | `46.8478 ps` | `0.0000 ps` |
| Buffer count | `23.6957` | `23.6957` | `0.0000` |
| Clock wire | `1640.46 um` | `1640.46 um` | `0.0000 um` |
| Clock total cap | `0.222913 pF` | `0.222913 pF` | `0.000000 pF` |
| Clock power | `0.089020 mW` | `0.089020 mW` | `0.000000 mW` |

Guard movement versus retained 45a: WNS, TNS, violating paths, DRV max-cap/max-tran counts, and DRC markers were unchanged on all 23 cases. Mean CTS runtime changed by only `+0.194 s`.

Decision: rejected and production code reverted. The branch undercoverage premise is real, but changing only the leaf requested-length bin does not alter the final selected H-tree solutions. Future work should not repeat standalone requested-length variants; it needs new generated structure or a stronger endpoint-order mechanism.

## Rejected Follow-up With Strong Signal: `candidate_55a_disable_boundary_cap_polish`

Purpose: test the user's proposed clustering-direction hypothesis that cap-distribution balancing may spread local clock distribution and hurt final skew/physical compactness. The production trial removed only the boundary-load polish stage while preserving recursive spatial partitioning and small-cluster merge polish.

Evidence:

- workflow manifest: `summary/candidate_55a_disable_boundary_cap_polish/run_manifest.json`
- all-23 summary: `summary/candidate_55a_disable_boundary_cap_polish/ecc-tools.summary.csv`
- first-principles reports under `summary/candidate_55a_disable_boundary_cap_polish/*`

All-23 workflow completed with `23/23` CTS success and `23/23` Innovus evaluation success.

Incremental MAE versus retained 45a:

| Metric | 45a | 55a | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `14.1739 ps` | `-1.2609 ps` |
| Average latency | `46.8478 ps` | `48.0217 ps` | `+1.1739 ps` |
| Buffer count | `23.6957` | `23.2609` | `-0.4348` |
| Clock wire | `1640.46 um` | `1636.90 um` | `-3.55 um` |
| Clock total cap | `0.222913 pF` | `0.218435 pF` | `-0.004478 pF` |
| Clock power | `0.089020 mW` | `0.081135 mW` | `-0.007885 mW` |

Guard movement versus retained 45a: WNS improved by `+0.0416 ns`, TNS improved to `0`, and violating paths dropped from `0.0435` to `0`; however max-cap real nets increased by `+0.0435`, max-tran real nets by `+0.1304`, and DRC markers by `+0.1304` on all-23 mean.

Decision: rejected and production code reverted. The candidate is not acceptable as-is because latency and small guard regressions violate the clarified no-deterioration gate. It is still a strong direction signal: boundary-load cap polish should be guarded, not removed wholesale.

## Rejected Follow-up: `candidate_56a_boundary_local_proxy_guard`

Purpose: recover the useful 55a signal while preserving boundary polish. Each boundary move was accepted only when the source+target local `routing_cap_proxy` sum did not increase, so the global cap-balance objective could not buy improvement by increasing local wire/cap proxy.

Evidence:

- workflow manifest: `summary/candidate_56a_boundary_local_proxy_guard/run_manifest.json`
- all-23 summary: `summary/candidate_56a_boundary_local_proxy_guard/ecc-tools.summary.csv`
- first-principles reports under `summary/candidate_56a_boundary_local_proxy_guard/*`

All-23 workflow completed with `23/23` CTS success and `23/23` Innovus evaluation success.

Incremental MAE versus retained 45a:

| Metric | 45a | 56a | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `15.5652 ps` | `+0.1304 ps` |
| Average latency | `46.8478 ps` | `47.2609 ps` | `+0.4130 ps` |
| Buffer count | `23.6957` | `23.6957` | `0.0000` |
| Clock wire | `1640.46 um` | `1639.17 um` | `-1.2870 um` |
| Clock total cap | `0.222913 pF` | `0.227000 pF` | `+0.004087 pF` |
| Clock power | `0.089020 mW` | `0.089500 mW` | `+0.000481 mW` |

Guard movement versus retained 45a: WNS improved by `+0.0073 ns`, TNS and violating paths were unchanged, DRC markers were unchanged, but max-cap real nets worsened by `+0.1304` and max-tran real nets by `+0.0435`.

Decision: rejected and production code reverted. The local no-worse proxy preserved only a small wire improvement while regressing skew, latency, cap, power, and DRV. Further boundary-polish work needs move-level diagnostics before another production rule.

## Read-Only Follow-up: `candidate_57a_boundary_move_trace_refresh`

Purpose: close the instrumentation gap left by 55a/56a by recording the accepted boundary-polish moves that retained 45a actually applies.

Code direction:

- extend the existing default-off `FastClusteringTraceContext` with accepted boundary-move records;
- keep file output in `SinkLoadClustering` under `ICTS_TIMING_TRACE_DIR`;
- record source/target ids, moved entry, local proxy deltas, pair objective, source/target diameter/root movement, and moved-load distance to source/target geometry;
- add `analyze_boundary_move_trace.py` to join move features with 45a/55a/56a evaluation deltas.

Validation:

- build target `ecc_bin icts_test_flow_synthesis_htree icts_test_module_topology_fast_clustering` passed.
- targeted `ctest` for H-tree and fast clustering passed `3/3`.
- all-23 workflow completed with `23/23` CTS success and `47/47` conversion/evaluation success.
- QoR is exactly unchanged versus retained 45a on core and guard fields checked per case.

Evidence:

- workflow manifest and all-23 summary: `summary/candidate_57a_boundary_move_trace_refresh/*`
- analysis report: `summary/candidate_57a_boundary_move_trace_analysis/boundary_move_trace_report.md`
- accepted boundary moves: `21` rows across `13/23` cases.
- mean positive local proxy-delta ratio by traced case: `0.25641`.

Decision: closed as read-only evidence. Keep the default-off trace because it does not change production behavior and prevents another blind boundary-polish guard attempt.

## Rejected Follow-up: `candidate_58a_boundary_pareto_geometry_cap`

Purpose: test a no-knob local Pareto recovery for 55a/56a by accepting a boundary move only when source+target geometry score and routing-cap variance penalty are both no worse after the move.

Evidence:

- workflow manifest: `summary/candidate_58a_boundary_pareto_geometry_cap/run_manifest.json`
- all-23 summary: `summary/candidate_58a_boundary_pareto_geometry_cap/ecc-tools.summary.csv`
- first-principles reports under `summary/candidate_58a_boundary_pareto_geometry_cap/*`

All-23 workflow completed with `23/23` CTS success and `47/47` conversion/evaluation success.

Incremental MAE versus retained 45a:

| Metric | 45a | 58a | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.4348 ps` | `16.0000 ps` | `+0.5652 ps` |
| Average latency | `46.8478 ps` | `47.5652 ps` | `+0.7174 ps` |
| Buffer count | `23.6957` | `23.6957` | `0.0000` |
| Clock wire | `1640.46 um` | `1640.73 um` | `+0.2710 um` |
| Clock total cap | `0.222913 pF` | `0.222783 pF` | `-0.000130 pF` |
| Clock power | `0.089020 mW` | `0.087660 mW` | `-0.001360 mW` |

Guard movement versus retained 45a: WNS mean improved by `+3.78 ps`, TNS and violating paths were unchanged, DRC markers improved by `-0.0435`, but max-cap real nets worsened by `+0.0870` and max-tran real nets by `+0.0435`.

Decision: rejected and production code reverted. The candidate has small cap/power wins, but it violates the clarified no-deterioration gate through skew, latency, wire, and DRV regressions. This closes local source/target geometry+cap Pareto guarding as a standalone boundary-polish rule.

## Rejected Follow-up: `candidate_59a_binary_spatial_htree_branching`

Purpose: test whether H-tree generation should separate electrical max leaf load from spatial branching by forcing recursive spatial partitioning to binary branching.

Code direction tested:

- make `TopologyGen::resolveBranchingFactor` return binary branching independent of `max_leaf_load_count`;
- keep electrical legality and downstream split/characterization checks unchanged;
- add no config, scalar, fitted threshold, or case-specific rule.

Evidence:

- run directory: `runs/candidate_59a_binary_spatial_htree_branching`
- partial trace directory: `summary/candidate_59a_binary_spatial_htree_branching`
- normal `runtime.rpt` output exists for `19/23` cases.
- incomplete cases: `iwls2005__vga_lcd`, `openroad__dynamic_node`, `openroad__ethmac`, `openroad__jpeg`.
- resumed large-case run reached about `60 GB` RSS on `openroad__dynamic_node` after `2m41s` without producing `cts.def`.
- no `run_manifest.json` or `ecc-tools.summary.csv` was generated, so Innovus evaluation was not started.

Decision: rejected and production code reverted. The first-principles premise is useful, but global binary branching is too broad: large cases expand to deeper topology/search and do not satisfy the all-23 CTS-only gate. A future H-tree-generation candidate must keep a native workload/depth/complexity envelope no worse while testing any spatial/electrical fanout decoupling.

## Rejected Follow-up: `candidate_60a_boundary_nearest_root_move_guard`

Purpose: test a no-knob spatial-affinity guard for boundary-load polish. A boundary move remains legal only when the moved sink is closer to the target cluster root than to the source cluster root, so cap balancing cannot move a sink against its nearest local root.

Evidence:

- workflow manifest: `summary/candidate_60a_boundary_nearest_root_move_guard/run_manifest.json`
- all-23 summary: `summary/candidate_60a_boundary_nearest_root_move_guard/ecc-tools.summary.csv`
- first-principles reports under `summary/candidate_60a_boundary_nearest_root_move_guard/*`
- all-23 CTS-only: `23/23`; conversion: `23/23`; Innovus evaluation: `23/23`.

Key deltas versus retained 45a:

| Metric | 45a MAE | 60a MAE | Delta |
| --- | ---: | ---: | ---: |
| Skew | `15.435 ps` | `15.261 ps` | `-0.174 ps` |
| Latency | `46.848 ps` | `47.326 ps` | `+0.478 ps` |
| Clock wire | `1640.459 um` | `1643.080 um` | `+2.621 um` |
| Clock total cap | `0.222913 pF` | `0.230087 pF` | `+0.007174 pF` |
| Clock power | `0.089020 mW` | `0.089123 mW` | `+0.000103 mW` |
| WNS alignment | `194.783 ps` | `239.696 ps` | `+44.913 ps` |

Guard movement versus retained 45a: TNS and violating paths improve, DRC markers improve by `-0.0435`, max-tran alignment is unchanged, but max-cap real-net alignment regresses by `+0.0435` and raw max-tran real-net count worsens by `+0.0870`.

Decision: rejected and production code reverted. The root-affinity premise is physically reasonable and all-23 feasible, but as a standalone boundary-polish guard it trades a small skew gain for latency/physical/timing regressions. Future boundary work must include endpoint-order/path-level context rather than only local root distance.

## Read-Only Follow-up: `candidate_61a_boundary_endpoint_context_precheck`

Purpose: join accepted boundary moves from 57a with final endpoint/path context before attempting another boundary-polish production guard.

Evidence:

- report: `summary/candidate_61a_boundary_endpoint_context_precheck/boundary_move_endpoint_context_report.md`
- move rows: `summary/candidate_61a_boundary_endpoint_context_precheck/boundary_move_endpoint_rows.csv`
- case summary: `summary/candidate_61a_boundary_endpoint_context_precheck/boundary_move_endpoint_case_summary.csv`

Key facts:

- `21/21` boundary moves were mapped to final moved sinks.
- `21/21` moved sinks remained in the target final cluster, so source/target trace ids are usable for context.
- Endpoint role counts are `max=2`, `none=19`; boundary moves can directly touch final max endpoints.
- Branch kind counts are `htree_split=17`, `htree_edge=4`.
- Arrival rank ratio spans `0.06` to `1.0`, so moves cover both near-extreme and middle-rank sinks.

Decision: close standalone boundary-polish guarding as the next production direction. Source/target local proxy, geometry/cap Pareto, and nearest-root affinity have all been tested and failed the no-deterioration gate. The next boundary candidate would need endpoint-order or path-delay context before the move decision; adding another local predicate would be low-value and likely repeat the same regressions.

## H-Tree Follow-ups: `candidate_61b` / `candidate_61c` / `candidate_62a` / `candidate_62b` / `candidate_62c` / `candidate_63a` / `candidate_64b` / `candidate_65a` / `candidate_66a` / `candidate_67a`

`candidate_61b_weighted_branch_arrival_selector_precheck`:

- Evidence: `summary/candidate_61b_weighted_branch_arrival_selector_precheck/weighted_branch_arrival_selector_precheck_report.md`.
- Result: `0/23` same-or-less-complex substitutions under the weighted branch-arrival proxy. Selector-only retuning over retained 45a remains closed.

`candidate_61c_analytical_htree_mode_precheck`:

- Evidence: `summary/candidate_61c_analytical_htree_mode_precheck/run_manifest.json` and `summary/candidate_61c_analytical_htree_mode_precheck/cts_trace_status.csv`.
- Result: `23/23` CTS-only artifact failures and `0` evaluation rows. This is a failed temporary-wrapper precheck, not a production algorithm result.

`candidate_62a_htree_local_split_pareto_axis`:

- Evidence: `summary/candidate_62a_htree_local_split_pareto_axis/run_manifest.json`, `summary/candidate_62a_htree_local_split_pareto_axis/ecc-tools.summary.csv`, and `summary/candidate_62a_htree_local_split_pareto_axis/decision_vs_retained_45a/clarified_gate_candidate_rows.csv`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.

Key deltas versus retained 45a:

| Metric | 45a MAE | 62a MAE | Delta | Raw direction |
| --- | ---: | ---: | ---: | --- |
| Skew | `15.435 ps` | `15.217 ps` | `-0.217 ps` | improved |
| Latency | `46.848 ps` | `46.804 ps` | `-0.043 ps` | improved |
| Buffer count | `23.696` | `23.696` | `0.000` | unchanged |
| Clock wire | `1640.459 um` | `1638.002 um` | `-2.457 um` | improved |
| Clock total cap | `0.222913 pF` | `0.222043 pF` | `-0.000870 pF` | improved |
| Clock power | `0.089020 mW` | `0.088354 mW` | `-0.000665 mW` | improved |

Guard movement versus retained 45a: WNS alignment and raw WNS improve; DRV max-tran improves; DRV max-cap raw count improves but commercial-alignment MAE regresses by `+0.1739` real nets; DRC alignment/raw markers regress by `+0.0435`, caused by one extra marker on one case.

Decision: reject and revert production code. The candidate proves local split generation can produce a small core-QoR Pareto improvement, but it fails the clarified no-deterioration gate once guard metrics are included. Future local split-generation work needs a native guard that preserves final DRC/DRV behavior without scalar margins or case rules.

`candidate_62b_route_envelope_local_split_axis`:

- Evidence: `summary/candidate_62b_route_envelope_local_split_axis/run_manifest.json`, `summary/candidate_62b_route_envelope_local_split_axis/ecc-tools.summary.csv`, and `summary/candidate_62b_route_envelope_local_split_axis/decision_vs_retained_45a/clarified_gate_candidate_rows.csv`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.
- Result: exactly equivalent to 62a on the checked core and guard raw metrics. Worst route-envelope strict improvement alone does not filter the unsafe local split movements.

Decision: reject and revert production code. Keep only as negative evidence that the guard must preserve spread, not only worst local path envelope.

`candidate_62c_spread_guarded_local_split_axis`:

- Evidence: `summary/candidate_62c_spread_guarded_local_split_axis/run_manifest.json`, `summary/candidate_62c_spread_guarded_local_split_axis/ecc-tools.summary.csv`, and `summary/candidate_62c_spread_guarded_local_split_axis/decision_vs_retained_45a/clarified_gate_candidate_rows.csv`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.
- Code direction: compare longest-axis and alternate-axis local split plans, then accept alternate only when child size spread, root wirelength sum/max/spread, child diameter total/max/spread, and route-envelope sum/spread are no worse while route-envelope max strictly improves.

Key deltas versus retained 45a:

| Metric | 45a MAE | 62c MAE | Delta | Raw direction |
| --- | ---: | ---: | ---: | --- |
| Skew | `15.435 ps` | `15.435 ps` | `0.000 ps` | unchanged |
| Latency | `46.848 ps` | `46.804 ps` | `-0.043 ps` | improved |
| Buffer count | `23.696` | `23.696` | `0.000` | unchanged |
| Clock wire | `1640.459 um` | `1639.843 um` | `-0.616 um` | improved |
| Clock total cap | `0.222913 pF` | `0.221609 pF` | `-0.001304 pF` | improved |
| Clock power | `0.089020 mW` | `0.088802 mW` | `-0.000217 mW` | improved |

Guard movement versus retained 45a: WNS alignment/raw improve slightly; DRV max-cap, DRV max-tran, and DRC alignment/raw are unchanged. Clarified gate row: core alignment improvements `4`, core regressions `0`, core raw improvements `4`, core raw regressions `0`, guard alignment regressions `0`, guard raw regressions `0`, `passes_core_plus_guard_gate=True`.

Case movement versus retained 45a is intentionally narrow: only `iwls2005__pci` changes (`latency_avg_ns -0.001`, `clock_wire_length_um -14.16`, `clock_total_cap_pf -0.03`, `clock_power_mw -0.005`, `wns_all_ns +0.002`); the other `22/23` cases are unchanged on checked core/guard raw metrics.

Decision: retain production code. This satisfies the clarified partial-improvement goal with an algorithmic native invariant and no fitted parameters, magic thresholds, behavior config, or case-specific rule.

`candidate_63a_anchor_aware_local_split`:

- Evidence: `summary/candidate_63a_anchor_aware_local_split/run_manifest.json`, `summary/candidate_63a_anchor_aware_local_split/ecc-tools.summary.csv`, and `summary/candidate_63a_anchor_aware_local_split/decision_vs_retained_45a/clarified_gate_candidate_rows.csv`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.
- Code direction tested: use the true upstream driver/anchor position for first-level local split root-wire scoring instead of the local load centroid; add no config, scalar, fitted threshold, or case-specific rule.

Result versus retained 45a: still passes the broad clarified gate, with latency, wire, cap, and power alignment/raw improvements, no skew/buffer movement, and no guard regression.

Result versus current retained 62c: rejected. The only moved case is `iwls2005__pci`, and all moved metrics are worse than 62c: latency `+0.5 ps`, clock wire `+8.06 um`, clock cap `+0.017 pF`, clock power `+0.003 mW`, and WNS `-2 ps`. No other checked case improves.

Decision: reject and revert production code. The first-principles consistency premise is plausible, but the benchmark evidence shows that 62c's center-based local proxy is the better incremental invariant for the current retained candidate. Keep 63a as negative evidence and do not add the anchor-aware overload or embedding driver-position plumbing.

`candidate_64b_local_split_generation_scope_trace_refresh`:

- Evidence: `summary/candidate_64b_local_split_generation_scope_trace_refresh/run_manifest.json`, `summary/candidate_64b_local_split_generation_scope_trace_refresh/trace/*/htree_local_split_generation.csv`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.
- Result: QoR-equivalent to retained 62c. The new trace separates `legality` and `materialization` local split calls.
- Materialization trace finding: current 62c accepts only the two safe `iwls2005__pci` rows. Relaxing both aggregate child-diameter and route-envelope-sum guards would admit `18` rows across `7` cases, so a production trial was required.

Decision: close as read-only evidence. Keep the scoped trace while this task remains in diagnosis mode; it is default-off under `ICTS_TIMING_TRACE_DIR` and adds no production behavior.

`candidate_65a_relaxed_aggregate_local_split_guard`:

- Evidence: `summary/candidate_65a_relaxed_aggregate_local_split_guard/run_manifest.json`, `summary/candidate_65a_relaxed_aggregate_local_split_guard/decision_vs_retained_62c/candidate_65a_vs_retained_62c_decision_report.md`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.
- Code direction tested: drop both `total_child_diameter_dbu <= baseline` and `route_envelope_sum_dbu <= baseline` while keeping child-size, root-wire, child max/spread, route max/spread guards.
- Result versus retained 62c: skew MAE improves by about `0.304 ps`, but latency, clock wire, clock cap, and clock power alignment regress.

Decision: reject and revert production code. Route-envelope sum is a necessary aggregate guard; dropping it admits broad physical-cost movement.

`candidate_66a_drop_child_total_local_split_guard`:

- Evidence: `summary/candidate_66a_drop_child_total_local_split_guard/run_manifest.json`, `summary/candidate_66a_drop_child_total_local_split_guard/decision_vs_retained_62c/candidate_66a_drop_child_total_local_split_guard_vs_retained_62c_decision_report.md`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.
- Code direction tested: keep route-envelope sum, but drop total child-diameter while keeping child max/spread and route max/spread guards.
- Result versus retained 62c: skew MAE improves by about `0.739 ps`, but latency and cap alignment regress; one moved case also adds a DRC marker.

Decision: reject and revert production code. Binary local split movement remains unsafe when total child diameter is not guarded.

`candidate_67a_multi_child_total_diameter_relaxation`:

- Evidence: `summary/candidate_67a_multi_child_total_diameter_relaxation/run_manifest.json`, `summary/candidate_67a_multi_child_total_diameter_relaxation/decision_vs_retained_62c/candidate_67a_multi_child_total_diameter_relaxation_vs_retained_62c_decision_report.md`.
- Workflow: all-23 CTS-only `23/23`; conversion/evaluation/summary succeeded.
- Code direction tested: allow total child-diameter relaxation only for multi-child local split; binary split keeps the retained 62c total child-diameter guard.
- Result versus retained 62c: only `iwls2005__vga_lcd` moves. Skew improves `15 ps`, average latency improves `2 ps`, cap improves `0.021 pF`, WNS improves `207 ps`, and DRV counts improve, but clock wire length increases `31.615 um`; all-23 wire mean/MAE regresses by about `1.37 um`.

Decision: reject and revert production code. This is the cleanest positive signal in the aggregate-relaxation family, but it still violates the no-regression gate. The later 70a materialized local-tree wire guard also failed, so reopening this path requires generating a different local topology rather than admitting extra choices from the same two split axes.

Candidate selection order:

1. Localized H-tree generation/materialization: after 82b, 83a/83b prove the retained front has no delay/power-safe lower-buffer or physical/load/exposure selector-only substitute. Use 83c branch-max undercoverage only as a diagnostic signal; do not repeat global max-branch planning, broad dual-plan frontier exposure, max-branch characterization-only coverage, leaf-only max coverage, RMS coverage, global binary branching, or selector-only power/complexity guard additions without a native no-regression invariant that preserves endpoint order, skew, latency, cap, power, and all-23 completion.
2. Cluster boundary polish: 55a/56a/58a/60a prove this is a real but unsafe lever, and 61a shows endpoint/path context is necessary but absent from the current move-decision stage. Do not add another local source/target proxy or root-distance guard.
3. `candidate_41b`: native timing semantics only if later trace isolates a deterministic Liberty/RC/DMP/slew propagation mismatch without scalar correction. Do not retry broad table-at-Ceff, table-at-driver-total-cap, or raw waveform delay substitution from the current evidence.
4. Cluster-final root or buffer work remains parked unless a future trace proves H-tree contribution is bounded and a native cap/power-aware local clustering rule avoids root-placement and buffer-upsizing regressions.
4. Historical objective/split-axis directions may be re-tested only as new trace-backed candidates; old no-trace evaluation summaries are not sufficient evidence for production.

Any accepted candidate must run the all-23 workflow and be compared against both `candidate_38a_local_split_physical_guard_refresh` and the current retained production candidate.

## Decision Artifacts Per Candidate

Each candidate summary should include:

- `decision_compare/<candidate>_vs_baseline_metric_fit.csv`
- `decision_compare/<candidate>_vs_baseline_qor_direction.csv`
- one first-principles report explaining root cause, keep/reject decision, and next action

When a production candidate is rejected, revert the production diff in the same iteration and keep the NFS evidence.
