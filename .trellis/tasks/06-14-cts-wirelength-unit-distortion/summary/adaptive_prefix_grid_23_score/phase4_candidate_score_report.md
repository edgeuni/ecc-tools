# Phase 4 Candidate Score: adaptive_wirelength_grid

## Inputs

- Candidate summary: `.trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_eval/ecc-tools.summary.csv`
- Candidate run: `.trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/adaptive_prefix_grid_23_cts`
- Innovus summary: `/home/liweiguo/project/DAC-27-CTS/experiments/summary/synthesized_cts_eval.commercial_strict/innovus.summary.csv`

## Final Metric Fit

| metric | unit | run | case_count | mae | rmse | r2 | geomean_ratio |
| --- | --- | --- | --- | --- | --- | --- | --- |
| latency_avg_ns | ns | phase1 | 23 | 0.0619565 | 0.0714703 | 0.514746 | 1.40284 |
| latency_avg_ns | ns | adaptive_wirelength_grid | 23 | 0.0590435 | 0.0699191 | 0.535582 | 1.36336 |
| latency_avg_ns | ns | phase3_full_depth_focus | 7 | 0.0857143 | 0.0954863 | 0.450824 | 1.55296 |
| skew_max_ns | ns | phase1 | 23 | 0.0212609 | 0.0254806 | -1.2344 | 1.58908 |
| skew_max_ns | ns | adaptive_wirelength_grid | 23 | 0.0190435 | 0.0247843 | -1.11395 | 1.62712 |
| skew_max_ns | ns | phase3_full_depth_focus | 7 | 0.0285714 | 0.0309931 | -1.84812 | 1.69514 |
| clock_total_cap_pf | pF | phase1 | 23 | 0.200913 | 0.295744 | 0.99954 | 1.06741 |
| clock_total_cap_pf | pF | adaptive_wirelength_grid | 23 | 0.234522 | 0.388983 | 0.999204 | 1.0684 |
| clock_total_cap_pf | pF | phase3_full_depth_focus | 7 | 0.173714 | 0.219061 | 0.999886 | 1.09058 |
| clock_power_mw | mW | phase1 | 23 | 0.098997 | 0.17143 | 0.997871 | 1.08483 |
| clock_power_mw | mW | adaptive_wirelength_grid | 23 | 0.0843435 | 0.150243 | 0.998365 | 1.09606 |
| clock_power_mw | mW | phase3_full_depth_focus | 7 | 0.137521 | 0.228548 | 0.998285 | 1.10699 |

## Structural Movement

| field | case_count | phase1_mean | candidate_mean | candidate_minus_phase1_mean | abs_error_improved_cases | abs_error_worsened_cases |
| --- | --- | --- | --- | --- | --- | --- |
| latency_avg_delta_ns | 23 | 0.0619565 | 0.0543913 | -0.00756522 | 13 | 9 |
| skew_max_delta_ns | 23 | 0.0178696 | 0.0185217 | 0.000652174 | 9 | 10 |
| tree_max_depth_delta | 23 | 1.30435 | 1.21739 | -0.0869565 | 5 | 3 |
| trunk_wire_length_ratio | 23 | 1.64151 | 1.61869 | -0.0228278 | 13 | 9 |
| leaf_wire_length_ratio | 23 | 0.924568 | 0.928144 | 0.00357587 | 9 | 13 |
| selected_depth_row_split_buffers | 23 | 113.652 | 153.696 | 40.0435 | 12 | 8 |
| first_hard_fail_loads_over_two_level | 0 |  |  |  | 0 | 0 |
