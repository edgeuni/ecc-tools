# 全面对齐商业 CTS 能力

## Goal

Use the DAC-27-CTS 23-case synthesized CTS evaluation as the commercial-tool baseline and align iCTS toward Innovus final CTS behavior.

The accepted timing model from Phase 1 remains the guard. Phase 4 has completed the first algorithm-level CTS-structure alignment stage across all 23 cases, especially H-tree construction, sink clustering controls, and sink-load/local buffering.

No new user-facing hardcoded QoR knobs are allowed. Deprecated or ignored public/config-style tuning keys remain out of the production control path:

- `selection_delay_margin`
- `skew_period_fraction`
- `auto_direct_bins_cap`
- `htree_depth_explore_window`

## Current Stage Status

- Phase 1: **complete**. L3 row-delay and RC/timing-model alignment against Innovus postRoute reports is accepted as the timing baseline.
- Phase 2: **complete**. The remaining final-metric gap is structural: depth, trunk/leaf distribution, local buffering, and source-trunk behavior.
- Phase 3: **complete for first production alignment iteration**. It removed a hard H-tree depth-window and added shape-aware global selection, but did not close the 23-case final-metric gap.
- Phase 4: **complete**. Ten algorithm-level iterations were run over all 23 cases, with final code -> bench -> Innovus evaluation -> score evidence for Iter09.

## Phase 1-3 Retained Evidence

Phase 1 timing guard:

- `summary/l3_native_ground_coupling_full_detail_l3/l3_timing_case_summary.csv`
- `summary/l3_native_ground_coupling_full_detail_l3/l3_timing_row_matches.csv`
- `summary/l3_native_ground_coupling_full_error_metrics/error_metrics_report.md`
- accepted gate: standard detail-route row-delay mean abs `2.862 ps`, p95 abs `9.770 ps`, max abs `30.062 ps`, all 23 cases below 50 ps max row-delay

Phase 2 structural evidence:

- `summary/phase2_clock_tree_qor/phase2_clock_tree_qor_report.md`
- `summary/phase2_clock_tree_qor/phase2_case_tree_diff.csv`
- accepted diagnosis:
  - ECC max logical buffer depth is greater than Innovus in 23 / 23 cases.
  - ECC trunk wire length is more than 1.4x Innovus in 23 / 23 cases.
  - ECC leaf wire length is shorter than Innovus in 21 / 23 cases.
  - ECC average latency is late in 21 / 23 cases.
  - ECC skew is worse in 18 / 23 cases.
  - buffer count alone is not the root cause; distribution/topology is.

Phase 3 first production iteration:

- code changes:
  - shape-aware H-tree global candidate selection
  - production full-depth H-tree candidate search
  - public `htree_depth_explore_window` deprecated/ignored
- assets:
  - `runs/phase3_full_depth_focus`
  - `runs/phase3_full_depth_focus_eval`
  - `summary/phase3_full_depth_focus_qor`
  - `summary/phase3_alignment_final/phase3_alignment_report.md`
- outcome:
  - `openroad__dynamic_node` improved: latency average gap `132 ps -> 46 ps`, skew gap `18 ps -> 15 ps`, max-depth delta `2 -> 1`
  - most focus cases did not move, so Phase 4 must address sink-load/local-buffer distribution and source-trunk/segment scoring

## Phase 4 Final Result

Promoted candidate:

- Iter09: `phase4_iter09_kary_buffer_first`
- CTS-only run: `runs/phase4_iter09_kary_buffer_first_cts`
- Innovus evaluation: `summary/phase4_iter09_kary_buffer_first_eval/ecc-tools.summary.csv`
- score report: `summary/phase4_iter09_kary_buffer_first_score/phase4_candidate_score_report.md`
- final report: `summary/phase4_final/phase4_final_report.md`
- iteration log: `summary/phase4_iterations/phase4_iteration_log.md`

Final all-23 effect versus Innovus:

- latency avg RMSE improved from `71.47 ps` to `51.99 ps`
- latency avg R2 improved from `0.515` to `0.743`
- buffer-count RMSE improved from `78.21` to `40.09`
- clock total cap RMSE improved from `0.2957 pF` to `0.2867 pF`
- clock power RMSE improved from `0.1714 mW` to `0.1252 mW`
- tree max-depth mean delta improved from `+1.30` to `+0.91` levels
- skew max MAE was nearly flat (`21.26 ps` to `21.09 ps`), but skew max RMSE worsened (`25.48 ps` to `28.33 ps`)

Phase 4 therefore completes latency/topology/buffer alignment, but not final skew alignment. The next stage should focus on skew spread, leaf/load balance, and local branch distribution.

## Phase 4 Requirements

Phase 4 must use all 23 synthesized CTS cases, not only focus cases.

The optimization loop for each promoted candidate is:

1. **Reverse analysis**: inspect Innovus/ECC report distributions for all 23 cases.
2. **Algorithm hypothesis**: explain the first-principles reason for the change, using geometry, cap/load balance, fanout legality, H-tree depth, source-trunk/leaf distribution, or drive distribution.
3. **Code implementation**: change production iCTS algorithms without per-case tuning, public magic knobs, fixed delay margins, or fixed skew targets.
4. **Build/test**: build `ecc_bin` and targeted iCTS tests.
5. **Bench run**: run DAC CTS-only bench for all 23 cases when the candidate is executable.
6. **Evaluation run**: run Innovus detail-route evaluation for all 23 cases for promoted candidates.
7. **Analysis report**: compare candidate versus Phase 1, Phase 3, and Innovus on final metrics and structural distributions.
8. **Decision**: keep, refine, revert, or split the repair family based on evidence.

Phase 4 attempted 10 iterations. The final Phase 4 report includes all 10 iteration records with rationale and outcome.

## Phase 4 Algorithm Scope

Allowed repair families:

- H-tree depth/topology selection beyond the Phase 3 shape policy
- sink clustering and pre-H-tree sink distribution
- local sink-load-region split and local-buffer legality
- source-trunk buffer/segment selection and source-to-root behavior
- branch/local buffering and sizing distribution
- drive-strength distribution when backed by Innovus cell histogram evidence
- cap/load balance scoring derived from observed sink/load distributions

Examples of Phase 4 questions:

- Does current sink clustering spread sinks to satisfy internal cap/fanout heuristics at the cost of Innovus-like compactness?
- Does disabling or tightening sink clustering improve trunk/leaf distribution and latency without causing DRV regressions?
- Does local split legality force deeper trees because split remediation is too shallow or too conservative?
- Does segment/source-trunk selection under-buffer or over-buffer relative to Innovus trunk/leaf cap and slew distributions?
- Are H-tree candidate scores over-prioritizing median power over insertion delay, cap balance, or skew risk?

Non-goals:

- timing-model retuning
- per-case/default config tuning
- fixed numerical margins or period fractions
- public QoR knobs
- report-only improvements without production algorithm behavior change
- accepting a change that improves one metric while clearly destroying DRV/power/cap without a defensible CTS reason

## Phase 4 Metrics

Primary final metrics versus Innovus:

- `latency_avg_ns` RMSE, MAE, geomean ratio, and case improvement count
- `skew_max_ns` RMSE, MAE, geomean ratio, and case improvement count
- WNS/TNS/DRV guard metrics

Primary structural metrics versus Innovus:

- max logical buffer depth delta
- timing-path clock stage average/max delta
- trunk wire length and cap ratio
- leaf wire length and cap ratio
- clock total cap, clock power, buffer area
- buffer count and cell/drive histogram distance
- sink-load-region fanout/split failure signatures
- source-trunk selected buffer count and candidate policy

## Phase 4 Acceptance Criteria

- [x] PRD/design/implement describe Phase 4 and retain only concise Phase 1-3 history.
- [x] A 23-case Phase 4 distribution analysis is generated before code changes.
- [x] At least 10 Phase 4 iteration records are written with hypothesis, evidence, action, result, and decision.
- [x] At least one production iCTS algorithm candidate is implemented without user-facing tuning knobs.
- [x] `ecc_bin` and targeted iCTS tests pass for the final candidate.
- [x] DAC CTS-only bench completes for all 23 cases for the final candidate.
- [x] Innovus detail-route evaluation completes for all 23 cases for the final candidate.
- [x] Final Phase 4 report compares Phase 1, Phase 3, Phase 4, and Innovus on final and structural metrics.
- [x] The final decision states whether to keep Phase 4 code, continue Phase 4, or split Phase 5.

## Out Of Scope

- Modifying `/home/liweiguo/project/DAC-27-CTS` assets directly.
- Reverting the accepted Phase 1 timing model to improve secondary metrics.
- Adding new public QoR knobs to replace removed hardcoded knobs.
- Committing unless explicitly requested.

## Execution Constraint

The current task remains no-commit unless explicitly requested. Validation should use focused build/test/evaluation/summary commands during iteration; broad `ecc dev` is reserved for final finish-work, not the normal implementation loop.
