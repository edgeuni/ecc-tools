# Phase 4 Candidate Score: phase4_iter09_kary_buffer_first

## Inputs

- Candidate summary: `.trellis/tasks/06-13-commercial-cts-capability-align/summary/phase4_iter09_kary_buffer_first_eval/ecc-tools.summary.csv`
- Candidate run: `.trellis/tasks/06-13-commercial-cts-capability-align/runs/phase4_iter09_kary_buffer_first_cts`
- Innovus summary: `/home/liweiguo/project/DAC-27-CTS/experiments/summary/synthesized_cts_eval.commercial_strict/innovus.summary.csv`

## Final Metric Fit

| metric | unit | run | case_count | mae | rmse | r2 | geomean_ratio |
| --- | --- | --- | --- | --- | --- | --- | --- |
| latency_avg_ns | ns | phase1 | 23 | 0.0619565 | 0.0714703 | 0.514746 | 1.40284 |
| latency_avg_ns | ns | phase4_iter09_kary_buffer_first | 23 | 0.0444783 | 0.0519914 | 0.743208 | 1.30273 |
| latency_avg_ns | ns | phase3_full_depth_focus | 7 | 0.0857143 | 0.0954863 | 0.450824 | 1.55296 |
| skew_max_ns | ns | phase1 | 23 | 0.0212609 | 0.0254806 | -1.2344 | 1.58908 |
| skew_max_ns | ns | phase4_iter09_kary_buffer_first | 23 | 0.021087 | 0.0283296 | -1.76199 | 1.67002 |
| skew_max_ns | ns | phase3_full_depth_focus | 7 | 0.0285714 | 0.0309931 | -1.84812 | 1.69514 |
| clock_total_cap_pf | pF | phase1 | 23 | 0.200913 | 0.295744 | 0.99954 | 1.06741 |
| clock_total_cap_pf | pF | phase4_iter09_kary_buffer_first | 23 | 0.176217 | 0.286673 | 0.999568 | 1.06651 |
| clock_total_cap_pf | pF | phase3_full_depth_focus | 7 | 0.173714 | 0.219061 | 0.999886 | 1.09058 |
| clock_power_mw | mW | phase1 | 23 | 0.098997 | 0.17143 | 0.997871 | 1.08483 |
| clock_power_mw | mW | phase4_iter09_kary_buffer_first | 23 | 0.0806217 | 0.125228 | 0.998864 | 1.09033 |
| clock_power_mw | mW | phase3_full_depth_focus | 7 | 0.137521 | 0.228548 | 0.998285 | 1.10699 |

## Structural Movement

| field | case_count | phase1_mean | candidate_mean | candidate_minus_phase1_mean | abs_error_improved_cases | abs_error_worsened_cases |
| --- | --- | --- | --- | --- | --- | --- |
| latency_avg_delta_ns | 23 | 0.0619565 | 0.0404783 | -0.0214783 | 16 | 6 |
| skew_max_delta_ns | 23 | 0.0178696 | 0.0205652 | 0.00269565 | 10 | 10 |
| tree_max_depth_delta | 23 | 1.30435 | 0.913043 | -0.391304 | 8 | 0 |
| trunk_wire_length_ratio | 23 | 1.64151 | 1.61755 | -0.0239657 | 13 | 9 |
| leaf_wire_length_ratio | 23 | 0.924568 | 0.929392 | 0.00482374 | 7 | 15 |
| selected_depth_row_split_buffers | 23 | 113.652 | 147.435 | 33.7826 | 14 | 6 |
| first_hard_fail_loads_over_two_level | 0 |  |  |  | 0 | 0 |
