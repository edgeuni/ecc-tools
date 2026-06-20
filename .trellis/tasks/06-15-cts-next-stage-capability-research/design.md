# Design: First-Principles CTS Skew Diagnosis

## Diagnostic Model

Skew is endpoint ordering:

`skew = max_sink_arrival - min_sink_arrival`

The diagnosis order is:

`same-tree skew error -> endpoint extrema agreement -> path stage residual -> role contribution -> H-tree branch/split spread -> cluster/local spread -> optimizer movement`

H-tree and clustering changes are accepted only after timing-observer gaps are quantified and carried as explicit caveats.

## Trace Contract

All trace is default-off behind `ICTS_TIMING_TRACE_DIR`.

| Trace | Purpose |
| --- | --- |
| `timing_checkpoint_summary_*.csv` | FastSTA skew/min/max endpoints at route-tree, before optimization, after optimization, accepted optimizer batches, and after design apply. |
| `*_sink_paths.csv` | Per-sink path stages with arrival, slew, cell delay, wire delay, cap/Ceff, driver-total cap, RC, DMP, load-wire timing, and role. |
| `*_skew_pair_stage_deltas.csv` | Signed cell/wire/context contribution for FastSTA min/max endpoints. |
| `*_skew_pair_role_deltas.csv` | H-tree versus local skew contribution for FastSTA extrema. |
| `*_sink_distribution.csv` | Endpoint rank, branch key, cluster key, final-net cap, and per-sink role totals. |
| `*_branch_distribution.csv`, `*_cluster_distribution.csv` | Branch and cluster spread after timing gate. |
| `htree_selection_*.csv`, `htree_candidate_levels_*.csv` | H-tree topology-space and selected-candidate facts. |
| `fast_clustering_*.csv` | Raw clustering groups and local sink allocation. |
| `fast_clustering_split_decisions_*.csv` | Recursive split longest-axis versus alternate-axis decision metrics. |
| `optimization_clock_*_iterations.csv`, `optimization_clock_*_trials.csv` | Optimizer checkpoint movement and accepted batches. |

The trace is diagnostic only. It must not add production behavior, fitted constants, or user-facing tuning knobs.

## Alignment Levels

| Level | Question | Evidence |
| --- | --- | --- |
| L1 final metrics | Does Innovus evaluation of ECC CTS approach commercial Innovus CTS? | Skew, latency, buffer, wire, cap, power MAE/MSE/RMSE/R2/bias. |
| L2 same-tree skew | Does ECC FastSTA agree with Innovus timing reports on the same ECC tree? | Same-tree skew fit, endpoint match, endpoint correction window. |
| L3 stage residual | Which delay stage explains mismatch? | Matched path cell/wire delay, arrival, slew, cap/Ceff, RC, DMP, load-wire fields. |
| L4 structural attribution | If timing error is carried, is the symptom H-tree or clustering? | H-tree edge/split versus cluster/final role deltas; branch/cluster distribution spread. |
| L5 optimizer movement | Does buffering after topology materially change endpoint order? | Checkpoint skew movement and accepted-batch count. |

## Current Reading

Baseline38 and 40b establish:

- endpoint-order gate is still active: both endpoint match is `0/23`;
- stage residual remains cell/context dominated;
- direct wire/RC/NDR remains secondary in the current evidence;
- H-tree/split is the dominant structural symptom after carrying the timing caveat;
- local split-axis changes are not a safe broad lever because the all-23 benefit is narrow and WNS/DRV side effects appear;
- current global selection front has no obvious no-worse-complexity, no-worse-delay, no-worse-branch-arrival lower-power alternative.
- broad shrink-only topology balancing can reduce wire and buffer-count alignment, but it regresses final skew/latency and clock cap/power, especially through larger buffering on large cases.
- preserving leaf cluster centers can reduce clock cap/power/wire, but it regresses final skew/latency and same-tree timing closure when used without a compensating endpoint-order-preserving balance mechanism.
- L1-median cluster roots reduce local final-cluster Manhattan proxy across all 23 cases and improve latency/buffer/cap/power in a production run, but standalone use regresses final skew and clock wire-length alignment.
- historical topology candidates contain a small final-safe oracle opportunity (`6` candidate/case pairs across `4` cases), but no tested ECC-visible Pareto rule selected those opportunities without false positives.
- alternate-axis recursive split selection is a valid core-QoR lever when guarded by native Pareto invariants: `candidate_45a_pareto_split_axis` improves all six core alignment metrics and all six core raw QoR means on the all-23 benchmark.
- direct child cap guards and leaf-envelope guards are not reliable standalone protections: 45b/45c/45d each moved regression into latency or guard metrics.
- the remaining 45a max-cap/max-tran residuals classify as design-signal nets, not CTS clock nets; DRC named-net smoke testing also shows no clock CTS marker delta.
- current same-tree timing residual is mostly explained by slew/driver-total-cap cell context; the post-context residual is small enough that scalar timing correction, threshold tuning, DMP switching, or wire/RC adjustment is not justified.
- H-tree local sibling balancing can improve final skew and clock-wire alignment, but the tested 42f implementation regressed latency, buffer-count, clock-cap, and clock-power alignment relative to retained 45a, so broad local branch balancing is not an accepted production invariant.
- Max-branch level-length undercoverage is a real diagnostic signal, but 42j/42k/42l show that global max planning, broad mean+max frontier exposure, and max-branch characterization-only coverage move latency/wire at the cost of skew/cap/power or guard metrics. Do not keep max-branch changes without a native endpoint-order and cap/power no-regression invariant.
- Cluster-final outliers exist, but buffer upsizing is not currently a native legality need: the smallest configured CTS buffer already covers every traced cluster load by a wide Liberty max-cap margin. Do not revisit cluster-buffer sizing unless future traces show actual electrical overload or a local rule that improves a benchmark target without cap/power/latency regression.
- Native cell-delay substitutions already visible in the trace are not a production lever: table-at-Ceff has only a tiny retained-45a L3 improvement and does not materially change same-tree cell-skew contribution, while table-at-driver-total-cap worsens fit.
- The clarified acceptance rule does not make a rejected historical candidate acceptable: a full scan of already-run all-23 candidates found no non-retained candidate with partial improvement and no core/raw/guard regression relative to retained 45a.
- The retained 45a split-axis behavior now has default-off recursive split-decision trace. This records longest-axis and alternate-axis metrics per recursive node, so future clustering/H-tree changes can be checked before relying on final skew/latency summaries.
- RMS requested level length is not a safe standalone H-tree planning rule. `candidate_52a_rms_level_length_plan` moved only `openroad__jpeg`; it improved skew and wire but regressed latency, buffer count, cap, and power, so retained production code stays on mean level length while branch-max/RMS evidence remains diagnostic only.
- Power-preserving branch-arrival overlay is not a safe selector-only follow-up. `candidate_53a_power_preserving_branch_arrival_overlay` improves clock cap and power alignment, but the price is worse skew/latency alignment and more timing guard violations, so retained production code stays on the 45a selector.
- Leaf-only max-branch level planning is not a useful production rule. `candidate_54a_leaf_max_branch_level_plan` was final-metric equivalent to 45a across all 23 cases, so the remaining branch undercoverage signal needs a real topology-space or endpoint-order mechanism, not another requested-length-only variant.
- Boundary-load cap polish is a real but unsafe cluster-side lever. `candidate_55a_disable_boundary_cap_polish` improves skew, buffer count, wire, cap, power, WNS, TNS, and violating paths, but it regresses latency and small DRV/DRC guards. The next clustering attempt should not remove boundary polish wholesale; it should prevent only moves that improve cap balance by spreading local geometry or worsening endpoint-order proxies.
- The first guarded boundary-polish attempt is closed. `candidate_56a_boundary_local_proxy_guard` shows that pairwise local routing-cap proxy no-worse is not enough; it worsens skew, latency, cap, power, and DRV. Boundary-polish work now needs move-level trace evidence before another production rule.
- The boundary move trace is now available. `candidate_57a_boundary_move_trace_refresh` is QoR-neutral against retained 45a and records accepted boundary moves at move level; only `21` moves occur across `13/23` cases, so boundary polish is sparse but influential enough to explain 55a/56a side effects.
- The second guarded boundary-polish attempt is closed. `candidate_58a_boundary_pareto_geometry_cap` shows that requiring local geometry and cap-variance no-worse still regresses skew, latency, wire, and DRV versus retained 45a. Boundary polishing cannot be recovered by local source/target Pareto checks alone; it needs endpoint-order or path-level context before another production rule.
- Global binary H-tree branching is closed as too broad. `candidate_59a_binary_spatial_htree_branching` decoupled electrical max leaf load from spatial branching, but it failed the all-23 CTS-only gate and inflated large-case topology depth/search. Future H-tree generation can still test spatial/electrical decoupling, but it must be bounded or localized instead of globally binary.
- Nearest-root boundary move guarding is closed as too weak. `candidate_60a_boundary_nearest_root_move_guard` completed all-23 and slightly improved skew, but regressed latency, wire, cap, power, WNS alignment, and max-cap guards. Root-distance affinity alone does not preserve enough physical/electrical context for boundary polish.
- Boundary move endpoint-context precheck is closed. `candidate_61a_boundary_endpoint_context_precheck` maps all `21` retained boundary moves to final endpoint/path context and shows that moves can touch final max endpoints as well as middle-rank sinks. The current boundary-polish production stage lacks endpoint/path timing context, so standalone local guards are paused.
- Weighted branch-arrival selector and analytical-H-tree prechecks did not produce a safe next production lever. `candidate_61b_weighted_branch_arrival_selector_precheck` found `0/23` strict same-or-less-complex substitutions, and `candidate_61c_analytical_htree_mode_precheck` failed the CTS artifact gate.
- Local split generation is not a retained primary H-tree lever under the layered benchmark gate. `candidate_62a_htree_local_split_pareto_axis` improved several all-23 metrics but failed DRV/DRC guards; `candidate_62b_route_envelope_local_split_axis` was exactly equivalent to 62a. The later `candidate_62c_spread_guarded_local_split_axis` has no `core16` standalone effect after multi-clock/outlier cases are quarantined, so it remains stress evidence rather than a primary single-clock technical node.
- Anchor-aware local split generation is closed as an incremental regression. `candidate_63a_anchor_aware_local_split` still passes relative to 45a, but versus current retained 62c it worsens the only moved `iwls2005__pci` case on latency, wire, cap, power, and WNS.
- Local split aggregate-guard relaxation is closed as unsafe relative to retained 62c. `candidate_65a` improved skew but regressed latency/wire/cap/power; `candidate_66a` kept route-envelope sum but still regressed latency/cap and DRC; `candidate_67a` isolated the positive `vga_lcd` movement but still regressed clock wire length. `candidate_70a` added a recursive materialized local-tree wire guard, but still produced wire/WNS/DRV regressions on the successfully evaluated cases and one Innovus evaluation crash. The retained 62c aggregate guards remain necessary.
- Weighted branch-arrival exposure is closed as a standalone H-tree selector lever. `candidate_68a` used native topology branch multiplicity to weight level arrival-spread exposure, but the all-23 result was final-metric equivalent to retained 62c, so it did not meet the partial-improvement gate.
- Branch-arrival exposure Pareto-front retuning is also closed. `candidate_69a` made branch exposure an additional physical-frontier dominance dimension, but all checked all-23 final metrics stayed exactly equivalent to retained 62c.
- Final-cluster extra-cluster guarding is closed as a no-op. `candidate_72a_final_cluster_extra_cluster_guard` selected the retained 45a Pareto path in every benchmark case and produced final metrics identical to 71a, so the extra selector should not remain in production code.
- H-tree sink-envelope fallback guarding is closed as too blunt. `candidate_73a_htree_sink_envelope_guard` fixes a small part of the 71a buffer/DRC regression, but it also reverts profitable 45a behavior on large cases and regresses core16 skew, latency, wire, cap, power, and WNS versus 71a; cap and power become worse than the remote baseline.
- The 74a/74b/74c read-only checks close the current selector-only H-tree follow-up. 74a shows longest-axis fallback loses the retained 45a cap/power/wire/skew value, 74b finds `0/23` no-worse branch-spread substitutions in the retained H-tree front, and 74c shows the five `core16` buffer-loss cases do not have safe lower-buffer alternatives already present in the selected feasible front.
- Materialized H-tree buffer-count semantics are a real diagnostic mismatch but not a retained production lever yet. `candidate_75a_materialized_htree_buffer_count_audit` proves the selector undercounts edge buffers by counting buffered H-tree levels with parent multiplicity, while the embedding materializes buffers per nonempty parent-child edge. `candidate_75b_materialized_htree_edge_buffer_cost` improves core16 skew, buffer, wire, cap, power, WNS, max-tran, and DRC, but its broad latency regression fails the incremental gate. `candidate_75c_split_aware_htree_complexity` separates H-tree edge and split-extra buffer complexity but still regresses core16 skew, latency, buffer count, WNS, and DRC. Keep the evidence, but do not retain either production formulation.
- `candidate_76a_htree_latency_preserving_edge_opportunity` closes the current-front recovery variant for materialized-edge cost. The relaxed delay/power/arrival rule has `5/16` core hits, but once effective split delay and exact segment delay are required no-worse, the hit count becomes `0/16`; adding physical-depth and split-extra no-worse also stays `0/16`. The next path must generate a new topology/materialization point instead of selecting another current-front edge-cost candidate.
- `candidate_77a_htree_materialization_pressure` identifies the next H-tree generation axis. On `core16`, `12` cases are split-materialization-dominant and `4` cases are edge-materialization-dominant; all root/split latency-preserving current-front lower-edge hit counts are still `0/16`. The next production attempt should generate same-depth edge/split materialization alternatives and pre-reject them unless effective split delay, exact segment delay, and weighted branch-arrival proxy are no worse.
- `candidate_78a_htree_split_preserving_opportunity` closes current-front split-saving selection. Six `core16` cases have unguarded lower-split candidates, but every such candidate fails the relaxed delay/power/arrival gate; same-depth lower-split and materialization-safe split-saving policies are `0/16`. The next H-tree attempt must therefore create a new materialization/topology shape, not choose another retained-front candidate.
- `candidate_79a_local_split_minimality_audit` finds a generation-level split over-materialization issue in the retained 71a local split tree. `core16` has `942` actual materialized local split buffers against a fanout lower bound of `768`, with excess concentrated in the same split-dominant cases highlighted by 77a/78a.
- `candidate_80a_minimal_local_split_generation` validates that reducing local split materialization is a real physical lever, but pure fanout-minimal generation is too aggressive: it removes all lower-bound excess, improves several cost metrics, and still regresses skew by removing useful same-depth path symmetry.
- `candidate_81a_uniform_depth_local_split_generation` is the intermediate local split shape node. It generates max-fanout leaf groups, then populates a uniform-depth local split shape so split count is substantially reduced without collapsing local path-depth symmetry. This is algorithmic and uses only existing fanout-derived constraints; it adds no fitted scalar, case rule, or behavior config.
- `candidate_82a_local_split_giveback_attribution` shows the remaining 81a giveback is a local physical-packing issue: wire/cap/skew loss concentrates in large split-dominant cases and correlates with htree-split route movement and split-delay spread.
- `candidate_82b_local_recursive_uniform_split_packing` is the current retained technical node candidate. It preserves 81a's fanout-derived uniform-depth shape but recursively re-sorts and repartitions each internal shape's load subset, recovering the local physical packing while keeping split materialization count unchanged.
- `candidate_85b_local_split_physical_anchor_generation` closes coordinate-wise median anchoring for local split buffers. It keeps the 82b shape but regresses retained-82b `core16` skew, latency, wire, cap, power, WNS, and DRC, so local split physical-anchor work needs a stronger endpoint-order/materialized-routing invariant before reopening.
- `candidate_86a_edge_materialization_next_axis_precheck` closes selector-only edge-materialization recovery after 82b. The edge-dominant residual cases are real (`27.4109%` residual share), but relaxed lower-edge current-front candidates shorten H-tree depth and move complexity into split-extra buffers, regressing effective split delay. The next attempt must generate a same-depth edge-materialization point rather than select an existing lower-edge candidate.
- `candidate_86b_same_depth_edge_materialization_generation_precheck` shows the retained 82b front has no current-front same-depth lower-edge substitute for the target edge cases. Each target exposes only the selected leaf-terminal sequence under strict effective-delay, exact-delay, branch-spread, power, and complexity guards, so a real production 86b would require new generation semantics.
- `candidate_87a_compact_local_split_tree` closes mixed-depth local split minimization. It reduces some physical cost but regresses `core16` skew/latency/WNS/DRC because local split buffers cannot be minimized independently of local depth symmetry and effective split delay.
- `candidate_88a_effective_split_delay_global_selection` proves effective split delay is a real selector signal: it improves retained-82b `core16` skew and latency and improves several remote-vs-Innovus primary metrics. The direct formulation is closed because it regresses buffer, wire, cap, and power broadly; effective split delay should only return as a guarded/tie-break feature with no-worse exact-delay, branch-spread, cap/power, and materialized-complexity checks.
- `candidate_88b_effective_timing_guarded_no_power` closes the current guarded effective-delay selector follow-up. Strict exact-delay, branch-spread, cap/power, shape, and materialized-complexity guards have `0/16` `core16` hits. Dropping only the power guard creates `14/16` hits and improves retained-82b skew/latency, but the all-23 run regresses cap, power, wire, and WNS; effective delay cannot be promoted without a native cap/power-preserving generation or selection invariant.
- `candidate_89a_effective_delay_power_source_precheck` shows the 88b opportunities are mostly segment-strength substitutions, not new geometry. Physical buffer count and sink-cap spread stay unchanged, but selected segment masters move toward stronger cells and final Innovus cap/power error worsens. This closes effective-delay current-front selection unless a future generator creates a lower-effective-delay topology/materialization point without stronger segment cells.
- `candidate_90a_effective_delay_geometry_substitution_precheck` shows what that future generator would need to do. Replacing the stronger-cell timing gain with selected-cell geometry shortening requires absorbing about `61.1%` of changed-level branch spread on average and `68.8%` on a weighted basis; half of the hits need more than `50%` weighted-spread absorption, and one exceeds the budget. This is too demanding for another broad level-length rule, but it identifies localized H-tree/materialization generation as the only first-principles continuation.
- `candidate_91a_effective_delay_changed_level_scope` narrows the generation scope: stronger-cell substitutions are mostly leaf-side and terminal-branch-buffered. This points to leaf-side/near-leaf terminal materialization as the next research target; root/trunk-wide H-tree planning is not the right next production surface.
- `candidate_92a_leaf_terminal_materialization_current_front_precheck` closes current-front leaf-terminal selection. The retained 82b feasible front has `773` lower-effective candidates across `14/16` core cases, but every lower-effective candidate fails topology power no-worse and strict leaf/near-leaf terminal geometry hits are `0/16`. The next step must either expose lower-level segment-frontier choices with default-off trace or add localized generation/materialization semantics; do not continue with another selector over the generated 82b front.
- `candidate_93a_segment_frontier_trace_opportunity` closes the lower-level segment-frontier selector path as well. Full default-off trace exposes `all`, `terminal_branch_buffered`, and `terminal_leaf_unbuffered` frontiers, but `core16` has no hidden semantic patterns and no same-boundary/source-boundary lower-delay, power-no-worse leaf-side opportunity. Apparent hits disappear once source-boundary switching-power state is included, matching the native frontier-pruning contract.
- `candidate_94a/95a/96a/96b` refine the post-82b continuation. The residual is still mostly H-tree/split-edge, and existing alternate X/Y split axes can change endpoint-side assignment on `des/jpeg`, but those changes are non-Pareto. A principal-direction split generator creates many physical-score Pareto nodes, yet none are endpoint-changing under the same native guard. The production 96b run improved `core16` wire slightly versus 82b but regressed skew, latency, buffer, cap, power, WNS, and DRC, so broad extra split-axis generation is closed.
- `candidate_97a_same_depth_edge_generation_feasibility_precheck` closes direct same-depth edge generation from current segment-frontier evidence. Strict same-state lower-buffer segment opportunities are `0/4` target cases; relaxed lower-buffer rows exist but require boundary/terminal/slew/cap/source-power/fanout state changes already filtered by native topology composition. The next edge-axis evidence must instrument composition or sink-load rejection reasons before production code.
- `candidate_98a_topology_composition_rejection_trace_precheck` closes direct edge-axis production for the current iteration. On the four edge target cases, selected-depth sink-load rejected entries are `0`, so the relaxed 97a rows are not lost at sink-load legality. The pressure is instead native composition/root legality: composition monotonic rejects are `46.5939%`, composition fanout rejects are `21.5401%`, and root fanout rejects are `19.7024%`. Reopening edge generation now requires a new monotonic-boundary-safe and source/root-fanout-safe state generator; otherwise move to the larger split-materialization residual axis.
- `candidate_99a_split_materialization_endpoint_order_generator_precheck` closes direct production from the current split-materialization axis. The largest residual bucket (`des/jpeg`, `40.2938%`) is real and both cases have endpoint-changing existing alternates, but every endpoint-changing alternate is non-Pareto; `des` breaks score and total-child-diameter, while `jpeg` breaks total-child-diameter. Principal-axis generation contributes `84` physical Pareto nodes and `0` endpoint-changing Pareto nodes. The next split/materialization attempt must create a new materialized-route shape with endpoint-side intent and native no-worse guards, not select existing X/Y alternates or add another broad projection axis.
- `candidate_100a_endpoint_materialized_route_shape_precheck` closes the most direct new split-shape follow-up. It enumerates minimal endpoint-side transplant and alternate-axis pair-repair candidates on the `des/jpeg` endpoint-changing nodes (`6561` candidates per case) and still finds `0/2` native-safe shapes. `des` remains blocked by score and total-child-diameter; `jpeg` remains blocked by total-child-diameter. Do not proceed to split/materialized-route production code from this generator family.
- `candidate_101a_targeted_endpoint_order_cluster_diagnosis` scopes the smaller cluster/final-local subset. The target has `6` cases and `17.3416%` residual share: three cluster-dominant path-context cases without endpoint-cluster boundary moves (`wb_conmax`, `spi`, `openroad__spi`), two endpoint-cluster boundary-move cases (`usb_phy`, `openroad__gcd`), and one htree-dominant deferred case (`systemcdes`). Start with cluster endpoint path-context before revisiting boundary polish.
- `candidate_102a_cluster_endpoint_path_context_precheck` closes the same-cluster local endpoint-rank explanation for `wb_conmax`, `spi`, and `openroad__spi`. The endpoint correction windows are covered by cross-cluster path delay differences, so a local per-cluster rank guard is not the right first production lever.
- `candidate_103a_cluster_buffer_path_delay_source` identifies the concrete source of the 102a signal. In all three target cases, the max-minus-min cluster delay delta is dominated by cluster buffer cell delay; cell-delay share is about `99.5%` to `99.8%`, while cluster input/final wire delay is secondary. The next evidence step should look for a native predictor of cluster buffer input slew and downstream load before changing clustering or boundary polish.
- `candidate_104a_cluster_buffer_load_slew_predictor_precheck` identifies the strongest native predictor for that source. `fast_total_cap_pf` hits the endpoint direction in `3/3` target cases and correlates with actual cluster buffer cell delay in every target case (`0.8848` to `0.9530`, mean `0.9159`). It almost directly predicts driver-total-cap/load context, while input slew is only weakly predicted by cluster shape. A production follow-up should therefore test cluster total-cap/load balance, keeping input slew as a monitor or guard rather than a fitted objective.
- `candidate_105a_cluster_total_cap_balance_candidate_precheck` closes the direct single-move/single-swap total-cap balance production path for the path-context group. Pair-max cap can improve in two target cases, but pair-sum-safe and geometry-safe strict native hits appear only in `iwls2005__wb_conmax`; `iwls2005__spi` requires pair total-cap giveback, and `openroad__spi` has no pair-max improvement. This is too sparse for production code without a stronger generation mechanism.
- `candidate_106a_endpoint_cluster_boundary_move_source` explains why the endpoint-cluster boundary cases escaped earlier local guards. `iwls2005__usb_phy` accepts a local-proxy-improving move that depletes the Innovus-min endpoint cluster and increases the max-minus-min endpoint proxy; `openroad__gcd` accepts a local-proxy-regressing move that depletes the Innovus-max endpoint cluster and reduces the max-minus-min endpoint proxy. Endpoint-spread direction is the diagnostic source, but it is not production-ready until an ECC-native observable substitute is proven.
- `candidate_107a_native_endpoint_spread_observability` proves that the current boundary-polish decision stage still lacks a safe native endpoint-spread signal. Boundary-stage local proxy sign hits `0/2`, and source depletion without endpoint side is non-discriminating. Native timing/rank signals explain both moves once timing exists: moved-sink arrival rank hits `2/2` after design apply, and source-cluster mean-arrival/cluster-delay ranks hit `2/2` from route-tree timing onward. Boundary polish should not be changed at the current fast-clustering stage; only a deferred timing-aware boundary-polish feasibility path remains open.
- `candidate_108a_deferred_timing_boundary_polish_feasibility` shows the deferred path has enough scope to justify a counterfactual candidate. Accepted boundary polish has `21` all-case moves and `15` core16 moves; `9/15` core moves have local/timing conflicts under route-tree source-cluster timing rank. This is targeted enough to avoid a broad clustering rewrite, but it is not production proof because the observed route-tree timing already includes current boundary polish.
- `candidate_109a_counterfactual_deferred_boundary_polish` narrows the prototype policy: roll back only accepted local-proxy-improving moves that deplete an early source cluster under route-tree timing rank. This would touch `7` core16 moves across `6` cases and covers `38.6821%` of retained-82b core residual share. The endpoint-spread proxy gain (`36867.4`) is larger than local-proxy giveback (`22791`), so a small benchmarked prototype is justified.
- `candidate_110a_deferred_boundary_polish_flow_contract` closes the post-route sink-net-only shortcut. The 109a rollback source/target clusters are observable at route-tree timing (`7/7` core hits), but every core rollback changes source or target cluster centers and net-only post-route rewiring is equivalent in `0/7` moves. A production 110a must create preliminary timing before final cluster-buffer/HTree commit; do not use trace CSVs or `ICTS_TIMING_TRACE_DIR` as behavior inputs.
- `candidate_110b_boundary_polish_timing_guidance_hook` adds the dormant native decision hook for that flow-level prototype. `ClusterConfig` now can carry `BoundaryPolishTimingGuidance`, and the existing boundary-move search rejects a locally improving move only when the supplied source-cluster arrival rank predicts larger arrival spread. With null guidance, behavior is unchanged; this is not a benchmark node until a preliminary-timing pass supplies ranks from committed route-tree timing.

## Current Design Direction

Do not keep 40a/40b local split-axis code. Treat those runs as negative evidence.

Keep the 45a split-axis Pareto version as the current core-QoR base candidate:

- evaluate both longest-axis and alternate-axis recursive split plans;
- keep the longest-axis plan as the default fallback;
- accept the alternate-axis plan only when the native split score is better and child count, split distance, routing-cap balance/spread, utilization penalty, and child-diameter envelope are no worse;
- do not add configs, fitted scalars, or case-specific gates.

This is a core benchmark improvement. The remaining DRV/DRC residual is carried as an evaluation caveat because the affected max-cap/max-tran nets are ordinary design-signal nets, not CTS-created clock nets. Do not add another split-axis, clustering, or H-tree heuristic just to tune this post-route side effect.

Do not keep the 62c local split generation guard as a primary single-clock technical node on top of 45a:

- for H-tree sink-load local split generation, evaluate longest-axis and alternate-axis child plans;
- keep longest-axis as fallback;
- accept the alternate-axis plan only when child size spread, root wirelength sum/max/spread, child diameter total/max/spread, and route-envelope sum/spread are no worse while worst route envelope strictly improves;
- add no config, fitted scalar, magic threshold, or case-specific rule.

This intentionally narrowed 62a/62b, but the layered benchmark shows its movement is outside `core16`. Keep it as historical evidence for quarantine/stress behavior only; future production code should not depend on it unless a later mechanism gives the same idea measurable Core16 benefit.

Do not replace this local split proxy with an anchor-aware driver-position proxy. The 63a all-23 run proved that using the upstream anchor for first-level root-wire scoring can undo part of 62c's safe `iwls2005__pci` improvement without helping any other checked case.

Do not relax 62c's aggregate child-diameter or route-envelope-sum guards as a standalone production rule. The scoped 64b trace identified the opportunity, but 65a/66a/67a prove that immediate local route-envelope and child-spread proxies can still hide final clock-wire or cap/latency regressions after materialization. The 70a materialized-wire guard also failed, so reopening this path requires generating a different local topology rather than admitting extra choices from the same two split axes.

Do not retry branch-arrival exposure as selector/frontier retuning. The weighted proxy and Pareto dimension are physically reasonable, but 68a/69a prove they do not change retained 62c final metrics under the current physical-depth, buffer-count, and delay envelope.

Next production candidates should be designed in one of two ways:

1. H-tree topology-space expansion: generate physically different H-tree alternatives and select only when delay, branch-arrival proxy, weighted proxy, physical depth, and physical buffer count are non-worse. The 74b/74c prechecks show that selecting among the current front is insufficient; the production change must create a new feasible topology point or a new materialization shape.
2. Targeted cluster-final handling: change local clustering only for trace-proven cluster-dominant outliers and preserve endpoint order.

Native timing semantics remains a monitoring path, not the next production lever. Change FastSTA/Liberty/RC behavior only when the trace shows a deterministic slew, driver-cap, Liberty, or RC semantic mismatch and an all-23 same-tree timing improvement without scalar correction.

Do not replace the current FastSTA cell-delay path with table-at-Ceff, table-at-driver-total-cap, or raw waveform delay as a broad rule. The 49a counterfactual shows those substitutions either move too little to affect endpoint order or regress cell-stage/cell-skew fit.

The next H-tree topology generation candidate must preserve endpoint-order behavior, not only reduce physical cost. The tested leaf-center and L1-median-root invariants are useful evidence for cap/power/wire, but they are not acceptable alone because skew or latency/wire regress.

Do not implement a selector that chooses among the historical topology candidates from final Innovus outcomes. The multi-topology audit shows that final-safe cases exist, but the tested ECC-native rules are not predictive enough. A production topology change needs either new generated candidates with stronger native invariants or a new trace feature that explains final-safe selection without fitted thresholds.

Do not keep adding selector-only guard terms to the retained H-tree front unless a read-only all-23 precheck shows a no-regression opportunity. The 53a power guard proves that a locally attractive power invariant can still discard endpoint-order-preserving candidates and regress skew/latency.

Do not spend another production iteration on standalone requested-length variants. Mean, max, RMS, mean+max, max-characterization-only, and leaf-only max coverage have all been tested or bounded; they either regress skew/latency/cap/power or do not change the final solution.

Do not add a final selector that compares retained 45a clustering against longest-axis clustering only by final cluster count or H-tree sink envelope. The 72a selector is behaviorally neutral, and the 73a selector is predictive only for a small buffer/DRC improvement while losing the cap/power/wire/skew benefits that made 45a worth retaining. The 74a/74b/74c read-only checks further show that neither longest-axis fallback nor current-front lower-buffer/branch-spread selection is enough. The next attempt must introduce a new topology-space or endpoint-order-aware feature before changing production selection.

Do not switch the retained H-tree selector directly to materialized edge-buffer or split-buffer cost. The 75a audit shows the weighted-buffer metric is physically approximate, but 75b and 75c prove that correcting the cost alone changes depth/split choices in ways that regress latency or endpoint order. 76a further shows there is no current-front lower-edge candidate that also preserves effective split delay and exact segment delay on `core16`, and 78a shows current-front lower-split candidates also disappear once delay/power/arrival or same-depth materialization guards are applied. 77a separates the remaining gap into split-dominant and edge-dominant generation targets. Reopen this path only by generating a same-depth topology/materialization point with simultaneous native latency/root-delay and endpoint-order preservation, not by changing the buffer-count cost dimension alone.

Keep the recursive uniform-depth local split packing direction from 82b as the current production candidate:

- split oversized local load groups into max-fanout leaf nodes;
- choose the shallowest fanout-derived uniform depth that can host all leaves;
- populate that shape by recursively sorting and repartitioning each internal shape's local load subset;
- preserve local split depth symmetry instead of collapsing to the absolute minimum internal-node count;
- avoid new configs, margins, fitted thresholds, or case-specific gates.

Do not keep the pure fanout-minimal 80a shape. Its lower split count is physically attractive, but the benchmark shows local split count cannot be minimized independently of endpoint-order and depth symmetry.

The 82b follow-up target should not be another split-count reduction. It should focus only on residual case-specific structural gaps, especially `openroad__jpeg` skew and `iwls2005__des` cap/power/WNS behavior, using native topology or timing evidence. Do not reintroduce scalar margins or final-metric tuning.

Do not replace 82b local split-buffer centers with coordinate-wise L1 medians. The 85b all-23 evaluation shows this physical anchor improves a few local cases but broadly gives back 82b's endpoint-order and DRC gains. Any future local split-anchor candidate must preserve native endpoint-order and materialized-route evidence, not only Manhattan center optimality.

Do not implement another selector-only edge-buffer-cost change. 86a shows `systemcaes`, `wb_dma`, and `tv80` relaxed lower-edge candidates reduce H-tree depth by one level, increase split-extra buffers by `19` to `57`, and regress effective split delay by `38.6` to `87.7 ps`; `spi` has no relaxed lower-edge current-front opportunity. 86b further shows the retained same-depth front has no lower-edge leaf-boundary-preserving substitute. Reopen edge materialization only with new generation semantics that preserve effective split delay, exact segment delay, weighted branch-arrival proxy, cap/power proxy, physical depth, and materialized buffer complexity.

Do not switch global H-tree selection directly from raw candidate delay to effective split delay. 88a demonstrates the missing signal, but direct substitution buys skew/latency with higher cap/power and larger branch-spread on important cases such as `openroad__jpeg` and `openroad__dynamic_node`. A future candidate may use effective split delay only as a guarded or tie-break signal after exact segment delay, weighted branch-arrival spread, cap/power, physical depth, and materialized buffer complexity are no worse.

Do not drop the cap/power guard from effective-delay selection. The 88b ablation shows that power is the guard that exposes most of the opportunity, but the production run confirms this converts into worse final cap/power alignment and WNS giveback. The next H-tree direction should generate a new topology/materialization point with simultaneous effective-delay and cap/power preservation, or add a first-principles cap/power proxy that predicts final Innovus cap/power movement without fitted thresholds.

Do not treat stronger segment-cell choice as an acceptable substitute for geometry. 89a shows the current-front effective-delay wins keep physical buffer count and sink cap unchanged but increase segment/topology power and often swap into stronger `BUFX20H7L` masters. A real follow-up needs a geometry/materialization mechanism that shortens effective paths or balances branch delay before segment strength is increased.

Do not retry global max/mean/RMS level-length or selector-only branch-spread policies as the geometry substitute. 90a shows the required geometry movement is localized to changed segment levels and often consumes most of their weighted branch-spread budget. A viable production candidate would need to create a localized materialization/topology alternative around those levels while preserving cap/power, not change the whole level plan.

Do not retain principal-axis recursive split generation as a broad third-axis generator. The read-only 96a precheck showed physical Pareto opportunities but no endpoint-changing Pareto hit, and the all-23 96b production run gave back retained-82b quality across most `core16` metrics. Future split/materialization work must be endpoint-order-aware or same-depth materialization-specific, not another general spatial projection axis.

Do not broaden the next attempt to root/trunk H-tree planning. 91a shows the changed effective-delay levels are mainly leaf-side, 92a shows the generated feasible front cannot supply a cap/power-safe leaf-terminal selector, and 93a shows the lower segment-frontier choices also have no strict same-state opportunity. Do not implement localized leaf-terminal materialization from the existing segment/frontier space. Reopen this only with a generator that creates genuinely new geometry/topology states and proves cap/power preservation before production evaluation.

The retained-82b reanchor is now complete. 94a shows that 82b is still closer than the remote branch on every `core16` primary metric, while the remaining residual mass is mostly H-tree/split-edge (`92.24%`) rather than cluster-final (`7.76%`). The next optimization iteration should therefore choose only among these non-selector axes:

1. Split-materialization endpoint-order-aware generation for `iwls2005__des` and `openroad__jpeg` (`40.29%` residual-share sum). Coordinate-wise median local anchoring is closed by 85b, so a follow-up must preserve endpoint order and materialized-route evidence, not simply move centers toward an L1 median.
2. New same-depth edge-materialization generation for `iwls2005__systemcaes`, `iwls2005__wb_dma`, and `iwls2005__tv80` (`24.04%`). Current-front edge selection is closed by 86a/86b, and 97a shows direct lower-buffer segment generation is not justified from the existing `kAll` segment-frontier evidence. Reopen this only with topology composition or sink-load rejection trace proving where relaxed lower-buffer rows are lost.
3. Targeted endpoint-order cluster diagnosis for the smaller cluster/final-local subset (`17.34%`). This is not a broad clustering rewrite: standalone median-root changes and buffer upsizing are already closed.

95a further narrows the first axis. The retained-82b split decisions do affect Innovus endpoint side assignment in both `des` and `jpeg`, but the existing alternate axes that change endpoint side are non-Pareto under the native split score/cap-spread/diameter/utilization guards. Do not implement a production selector that promotes those alternates. A valid split-materialization follow-up must generate a new candidate shape or add a first-principles invariant strong enough to explain why endpoint order is preserved without giving back physical safety.

Selector-only H-tree, leaf-terminal, and segment-frontier retuning remains closed until a new read-only precheck proves a native no-regression opportunity. Direct same-depth edge generation is also closed after 98a until a new generator can prove monotonic-boundary safety, source/root fanout safety, effective-delay preservation, exact segment-delay preservation, cap/power preservation, and materialized-complexity safety.

After 102a/103a/104a/105a/106a/107a/108a/109a/110a/110b, the cluster-local path-context subset should not go directly to current-stage boundary polish, final-wire RC adjustment, input-slew fitting, local endpoint-rank guarding, direct total-cap single-move/swap balancing, an Innovus-labeled endpoint guard, or post-route sink-net rewiring. Native endpoint-spread observability appears only after timing is materialized, and the 110a flow-contract check shows the rollback changes cluster centers. The 110b policy hook is intentionally dormant until synthesis can supply preliminary timing ranks. The next allowed production attempt is therefore a flow-level preliminary-timing boundary-polish prototype before final cluster-buffer/HTree commit. Keep it only if `core16` moves closer to Innovus and guard metrics do not show broad cap/power/wire or DRC damage.

The 110c prototype proves that this flow-level preliminary-timing formulation is implementable and has real remote-vs-Innovus value, but it is not a promoted technical node yet. It improves every `core16` primary MAE versus the remote branch, but gives back latency and WNS versus retained 82b. The 110d pairwise source-plus-target rank variant is closed and reverted: endpoint-spread reduction alone can improve skew while making critical-path/WNS alignment worse. The next timing-aware boundary-polish attempt must preserve critical-path, WNS, or path-stage delay context explicitly, or the work should return to the H-tree/split-edge residual queue.

The 110e critical-path precheck closes immediate production boundary-polish follow-up. After aligning to the benchmark `wns_default_ns` metric, skew-win/WNS-loss conflicts remain visible in `4/16` `core16` cases under 110d, and two severe WNS regressions (`wb_conmax`, `tv80`) do not have moved boundary source/target clusters on the parsed post-route top timing paths. A source/target cluster timing-rank predicate is therefore not a native WNS-protection rule. Reopen timing-aware boundary polish only after a stronger read-only join maps Innovus launch/capture data-path endpoints to ECC clock sink clusters and proves a threshold-free predictor; otherwise return to the retained 82b H-tree/split-edge residual queue.

The 110f launch/capture endpoint join performs that stronger read-only check and still does not justify production code. Endpoint-cluster touch catches some 110d WNS losses, but it also creates false positives and false negatives on moved `core16` cases; the largest WNS losses (`wb_conmax`, `tv80`) remain untouched by launch/capture endpoint clusters. Timing-aware boundary polish is therefore deferred after 110f. The next algorithmic work should resume from retained-82b H-tree/split-edge residuals rather than adding another boundary-move guard.

111a is the stopping analysis for this goal iteration. It rechecks the retained-82b queue after 110f and keeps the H-tree/split-edge diagnosis dominant: `92.2366%` residual share is still H-tree/split-edge, while cluster-final is `7.76333%`. The split-materialization bucket remains the largest (`des/jpeg`, `40.2938%`), but the current alternate-axis, broad principal-axis, and minimal endpoint-transplant/repair generator families are closed by 99a/100a. The more actionable restart point is the same-depth edge-materialization bucket (`systemcaes/wb_dma/tv80`, `24.0414%`) because 98a identifies the native pruning mechanism: composition monotonic, source-fanout, and root-fanout rejection with zero selected-depth sink-load rejection. If a later goal resumes this work, start with a read-only monotonic-boundary/source-fanout-safe same-depth edge generator precheck; do not restart from selector-only H-tree retuning, boundary polish, scalar timing correction, or another current split-axis variant.

Do not resurrect old production diffs solely because they improve one metric under the clarified goal. A candidate still needs all-23 no-regression against retained 45a; 50a shows the old candidate set has no such direct replacement.

Clustering remains a targeted path for `cluster_final` outliers. Standalone median-root rewriting is closed; any future clustering candidate needs an endpoint-order guard or must be naturally limited to cluster-dominant outliers. Optimizer/buffering remains deferred because accepted optimizer movement is sparse and small in the current traces.

For clustering, prefer guarded boundary-move behavior over root rewriting or buffer upsizing only if the guard has endpoint-order or path-level evidence. Local geometry/electrical checks alone have now failed in 56a and 58a, so do not add another boundary-polish guard from source/target proxy facts alone.

Do not add another boundary-polish guard from final metrics alone. The 57a move-level diagnostics have now been joined with endpoint-order/path context in 61a. Local source/target proxy, geometry/cap Pareto checks, and nearest-root affinity are all closed as standalone guards; reopening boundary polish requires timing/path context before the move decision, not another local predicate.

Do not force all generated H-trees to binary spatial branching. The 59a negative run proves that this preserves a first-principles idea but violates the all-23 practicality gate by increasing large-case topology depth and characterization/search cost. A future topology-generation change must keep a native complexity envelope such as depth, candidate count, buffer count, or characterization workload no worse.

Future candidates may keep historical baseline38 tables for continuity, but the keep/reject gate is now remote-branch versus commercial Innovus on the layered benchmark. A candidate must improve at least one `core16` primary metric versus the remote branch, with case-by-case support and no broad `extended23` or `quarantine7` damage. This partial-improvement gate is intentional; a single candidate does not need to fully match commercial CTS behavior.

## Experiment Architecture

Candidate names use a monotonic prefix and a descriptive direction:

`candidate_41a_<direction>`, `candidate_41b_<direction>`, ...

The experiment loop is intentionally uniform:

1. Analyze the current all-23 baseline and commercial Innovus reports.
2. Add a read-only analyzer when the existing summaries do not prove the candidate's first-principles premise.
3. Implement production code only after the analyzer identifies a native timing or topology-space opportunity.
4. Run all-23 CTS-only and all-23 Innovus evaluation.
5. Compare both alignment metrics and raw QoR direction against the remote-branch baseline and commercial Innovus reference using `core16`, `extended23`, and `quarantine7` views.
6. Keep or revert production code immediately after the decision.

The decision layer must separate two questions:

- Alignment fit: whether ECC moves closer to commercial Innovus on skew, latency, buffer, wire, cap, power, DRV, DRC, and WNS.
- Raw QoR direction: whether ECC's own skew, latency, wire, cap, power, DRV, DRC, and WNS worsened independent of commercial fit.

An accepted change must have a physical/timing explanation and pass both questions.

## Non-Goals

- No scalar cap/delay/slew/wire correction factors.
- No case-specific rules.
- No behavior-changing config.
- No endpoint-window magic thresholds.
- No selector-only metric retuning without physical topology-space evidence and an ECC-native no-regression selector.
- No acceptance based only on WNS/TNS/slack.
