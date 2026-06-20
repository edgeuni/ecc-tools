# Phase 3 CTS Alignment Report

## Scope

- Focus cases: 7
- Reference: Innovus commercial strict detail-route evaluation.
- Compared ECC runs: Phase 1 timing baseline, Phase 3 adaptive-shape, Phase 3 full-depth.

## Implemented Changes

- H-tree adaptive global selection now chooses the fastest shape only when it is no deeper and has no more buffered levels than the Pareto median.
- Production H-tree assembly now uses full-depth candidate search (`depth_explore_window = 0`) and the old public `htree_depth_explore_window` key is deprecated/ignored.
- No new public QoR knob, per-case setting, fixed delay margin, or magic-number threshold was added.

## Metric Summary

| metric | run | mean_delta | mae | mse | rmse | r2 | geomean_ratio |
| --- | --- | --- | --- | --- | --- | --- | --- |
| latency_avg_ns | phase1 | 0.098 | 0.098 | 0.0113045 | 0.106323 | 0.319105 | 1.6124 |
| latency_avg_ns | phase3_shape | 0.0857143 | 0.0857143 | 0.00911764 | 0.0954863 | 0.450824 | 1.55296 |
| latency_avg_ns | phase3_full_depth | 0.0857143 | 0.0857143 | 0.00911764 | 0.0954863 | 0.450824 | 1.55296 |
| skew_max_ns | phase1 | 0.0247143 | 0.029 | 0.000974714 | 0.0312204 | -1.89005 | 1.70719 |
| skew_max_ns | phase3_shape | 0.0242857 | 0.0285714 | 0.000960571 | 0.0309931 | -1.84812 | 1.69514 |
| skew_max_ns | phase3_full_depth | 0.0242857 | 0.0285714 | 0.000960571 | 0.0309931 | -1.84812 | 1.69514 |
| buffer_count | phase1 | 54.7143 | 56.7143 | 10901.9 | 104.412 | 0.99712 | 1.07556 |
| buffer_count | phase3_shape | 54.4286 | 56.4286 | 10868.7 | 104.253 | 0.997129 | 1.07518 |
| buffer_count | phase3_full_depth | 54.4286 | 56.4286 | 10868.7 | 104.253 | 0.997129 | 1.07518 |
| clock_total_cap_pf | phase1 | 0.164286 | 0.164286 | 0.0411426 | 0.202836 | 0.999902 | 1.08934 |
| clock_total_cap_pf | phase3_shape | 0.173714 | 0.173714 | 0.0479877 | 0.219061 | 0.999886 | 1.09058 |
| clock_total_cap_pf | phase3_full_depth | 0.173714 | 0.173714 | 0.0479877 | 0.219061 | 0.999886 | 1.09058 |
| clock_power_mw | phase1 | 0.116379 | 0.116379 | 0.0439462 | 0.209633 | 0.998557 | 1.09707 |
| clock_power_mw | phase3_shape | 0.137521 | 0.137521 | 0.0522342 | 0.228548 | 0.998285 | 1.10699 |
| clock_power_mw | phase3_full_depth | 0.137521 | 0.137521 | 0.0522342 | 0.228548 | 0.998285 | 1.10699 |

## Case Summary

| case | policy | depth_candidates | selected_depth | latency_gap_ps_phase1 | latency_gap_ps_phase3 | skew_gap_ps_phase1 | skew_gap_ps_phase3 | depth_delta_phase1 | depth_delta_phase3 | trunk_ratio_phase3 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| iwls2005__des | adaptive_front_midpoint | 8 | 6 | 82 | 82 | 32 | 32 | 2 | 2 | 1.54463 |
| iwls2005__spi | adaptive_front_midpoint | 5 | 3 | 93 | 93 | 23 | 23 | 2 | 2 | 1.83484 |
| iwls2005__usb_funct | adaptive_front_midpoint | 5 | 2 | 83 | 83 | 38 | 38 | 1 | 1 | 1.47631 |
| iwls2005__vga_lcd | adaptive_front_midpoint | 12 | 9 | 181 | 181 | 51 | 51 | 3 | 3 | 1.60159 |
| openroad__dynamic_node | adaptive_shape_timing | 9 | 6 | 132 | 46 | 18 | 15 | 2 | 1 | 1.54879 |
| openroad__gcd | adaptive_front_midpoint | 3 | 1 | 51.5 | 51.5 | 26 | 26 | 2 | 2 | 1.64823 |
| openroad__uart | adaptive_shape_median | 4 | 1 | 63.5 | 63.5 | 15 | 15 | 1 | 1 | 1.86345 |

## Conclusions

- The adaptive-shape production patch has a real but narrow effect. `openroad__dynamic_node` latency gap improved from 132 ps to 46 ps and tree max-depth delta improved from 2 to 1.
- Full-depth search is necessary as a correctness fix because the previous fixed window clipped the candidate space, but it does not by itself move most final metrics.
- `iwls2005__vga_lcd` proves the remaining blocker: full-depth search evaluated 12 depth candidates but selected depth 9; shallower depths failed sink-load-region legality with `htree_load_group_node_15 anchor=(126592,486581) fanout_violation load_count=18, max_fanout=4, split=infeasible`.
- For 6 / 7 focus cases, final latency/skew/tree-report deltas remained unchanged after full-depth search, so the remaining alignment gap is not another timing-model fix and not a simple global depth-window issue.
- The next scoped repair should target sink-load-region split/local buffering legality and source-trunk/segment candidate scoring, with Innovus distribution evidence as the guard.

## Decision

Do not expand this Phase 3 patch directly to a full 23-case acceptance claim. Keep the code changes that remove the hard depth-window and improve adaptive selection, but split the next repair around sink-load-region fanout/local-buffer distribution and source-trunk scoring.
