# Phase 2 Clock Tree QoR Analysis

## Scope

- Mode: detail-route/postRoute Innovus evaluation.
- Cases: 23 matched synthesized CTS cases.
- ECC input: Phase 1 native ground/coupling detail-run.
- Innovus reference: commercial strict Innovus CTS run.
- Timing guard: Phase 1 L3 row-delay alignment summary.

## Phase 2A Evidence

The generated case table joins scalar QoR, `clock_trees.rpt` tree evidence, top timing-path stage statistics, skew/latency report pins, and the Phase 1 L3 guard.

Outputs:

- `phase2_case_tree_diff.csv`
- `phase2_aggregate_summary.csv`
- `phase2_classification_summary.csv`
- `phase2_repair_candidates.csv`
- `phase2_top_movements.csv`

Structural aggregate signals:

| metric | unit | mean | median | min | max |
| --- | --- | --- | --- | --- | --- |
| ECC max depth minus Innovus |  | 1.30435 | 1 | 1 | 3 |
| top-path stage avg delta |  | 1.22417 | 1 | 0 | 3 |
| trunk wire length ratio | x | 1.64151 | 1.6277 | 1.42081 | 1.86345 |
| leaf wire length ratio | x | 0.924568 | 0.888243 | 0.773361 | 1.47853 |
| clock total cap ratio | x | 1.06837 | 1.0613 | 1.00462 | 1.19328 |
| buffer count delta |  | 37.3913 | 11 | -7 | 265 |

- ECC has greater max logical buffer depth in 23 / 23 cases.
- ECC trunk wire length is more than 1.4x Innovus in 23 / 23 cases.
- ECC leaf wire length is shorter than Innovus in 21 / 23 cases.

## Aggregate Metrics

| metric | unit | geomean_ratio_ecc_to_innovus | mean_delta | rmse_delta | phase1_vs_pre_improved_cases | phase1_vs_pre_worsened_cases | phase1_vs_pre_neutral_cases |
| --- | --- | --- | --- | --- | --- | --- | --- |
| skew_max | ns | 1.58908 | 0.0178696 | 0.0254806 | 8 | 9 | 6 |
| latency_avg | ns | 1.40284 | 0.0619565 | 0.0714703 | 2 | 15 | 6 |
| buffer_count | count | 1.07098 | 37.3913 | 78.2099 | 2 | 4 | 17 |
| buffer_area_um2 | um2 | 1.08466 | 144.723 | 294.302 | 17 | 1 | 5 |
| clock_total_cap_pf | pF | 1.06741 | 0.200913 | 0.295744 | 13 | 5 | 5 |
| clock_power_mw | mW | 1.08483 | 0.098997 | 0.17143 | 17 | 1 | 5 |

## Phase 2B Latency Classification

- `ecc_latency_late`: 21 cases
- `latency_near_innovus`: 2 cases

## Phase 2C Skew Classification

- `ecc_skew_worse`: 18 cases
- `skew_near_innovus`: 3 cases
- `ecc_skew_better`: 2 cases

## Phase 2D Repair Families

- `tree_level_candidate_scoring_or_topology_depth_balance`: 23 cases
- `source_trunk_buffering_or_sizing`: 21 cases
- `rc_drv_aware_selection`: 20 cases
- `buffer_distribution_not_count_repair`: 2 cases
- `branch_buffering_or_sizing`: 1 cases

Phase 2D decision:

- Dominant repair family by case count: `tree_level_candidate_scoring_or_topology_depth_balance`.
- The evidence is report-backed and structural; no public fixed skew/latency target or delay margin is introduced.
- If more than one repair family has significant support, production changes should be split into scoped patches rather than one broad heuristic.

## Top Repair Candidates

| case | priority_score | latency_avg_delta_ps | skew_max_delta_ps | tree_max_depth_delta | buffer_count_delta | latency_classification | skew_classification | repair_recommendation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| iwls2005__vga_lcd | 262.281 | 181 | 51 | 3 | 265 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| openroad__dynamic_node | 170.33 | 132 | 18 | 2 | 59 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| iwls2005__spi | 136.062 | 93 | 23 | 2 | 12 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| iwls2005__des | 134.252 | 82 | 32 | 2 | 49 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| iwls2005__usb_funct | 131.178 | 83 | 38 | 1 | -7 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;branch_buffering_or_sizing;rc_drv_aware_selection |
| iwls2005__mem_ctrl | 106.136 | 76 | 10 | 2 | 39 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| openroad__fifo | 102.736 | 71.5 | -21 | 1 | 21 | ecc_latency_late | ecc_skew_better | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| openroad__gcd | 97.523 | 51.5 | 26 | 2 | 2 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| openroad__uart | 88.524 | 63.5 | -15 | 1 | 3 | ecc_latency_late | ecc_skew_better | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| iwls2005__systemcaes | 84.681 | 64.5 | 10 | 1 | 12 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| iwls2005__ac97_ctrl | 82.377 | 42 | 30 | 1 | 65 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |
| iwls2005__pci | 80.572 | 40 | 30 | 1 | 37 | ecc_latency_late | ecc_skew_worse | tree_level_candidate_scoring_or_topology_depth_balance;source_trunk_buffering_or_sizing;rc_drv_aware_selection |

## Phase 2E Scoreboard Interpretation

- Phase 1 timing remains the guard; this report does not retune RC/timing model parameters.
- `buffer_count` alone is insufficient: many cases show count-neutral but depth/route/cap/drive distribution differences.
- Latency repair should prioritize structural evidence from depth, trunk/leaf RC, and drive histograms.
- Skew repair should prioritize spread evidence rather than average latency offset.
