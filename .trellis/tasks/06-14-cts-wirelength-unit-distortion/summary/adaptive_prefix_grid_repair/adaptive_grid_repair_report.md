# Adaptive Wirelength Grid Repair Summary

## Quantization

- Cases analyzed: 23
- Selected-path relative over-model mean: 0.4727 -> 0.0375
- Selected-path absolute over-model mean: 43.823 um -> 7.113 um
- Cases with lower selected-path relative error: 23; worsened: 0
- CTS total runtime sum: 344.210 s -> 392.157 s (+13.93%)

## Innovus Metric Fit

| metric | phase1 MAE | candidate MAE | MAE delta | phase1 RMSE | candidate RMSE | R2 delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| latency_avg_ns | 0.0620 | 0.0590 | -0.0029 | 0.0715 | 0.0699 | 0.0208 |
| skew_max_ns | 0.0213 | 0.0190 | -0.0022 | 0.0255 | 0.0248 | 0.1204 |
| buffer_count | 38.0000 | 24.2609 | -13.7391 | 78.2099 | 44.2051 | 0.0024 |
| clock_wire_length_um | 1852.7200 | 1752.7700 | -99.9500 | 3804.8300 | 3459.7500 | 0.0050 |
| clock_total_cap_pf | 0.2009 | 0.2345 | 0.0336 | 0.2957 | 0.3890 | -0.0003 |
| clock_power_mw | 0.0990 | 0.0843 | -0.0147 | 0.1714 | 0.1502 | 0.0005 |

## Structural Side Effects

| field | phase1 mean | candidate mean | delta | improved cases | worsened cases |
| --- | ---: | ---: | ---: | ---: | ---: |
| latency_avg_delta_ns | 0.0620 | 0.0544 | -0.0076 | 13 | 9 |
| skew_max_delta_ns | 0.0179 | 0.0185 | 0.0007 | 9 | 10 |
| trunk_wire_length_ratio | 1.6415 | 1.6187 | -0.0228 | 13 | 9 |
| leaf_wire_length_ratio | 0.9246 | 0.9281 | 0.0036 | 9 | 13 |
| clock_total_cap_ratio | 1.0684 | 1.0695 | 0.0011 | 11 | 10 |
| buffer_count_delta | 37.3913 | 23.6522 | -13.7391 | 12 | 8 |
| htree_selected_depth_minus_innovus_max_depth | -0.0435 | -2.0435 | -2.0000 | 0 | 15 |

## Interpretation

- The repair reduces the proven pre-STA grid distortion without adding a public knob or case-specific parameter.
- Runtime impact is bounded in this 23-case run; the direct characterized point count remains sparse while coverage iterations may increase for source-to-root lengths.
- Final Innovus metric fit improves on latency, skew, buffer count, wirelength, and power; clock total cap is the main metric that regresses and remains a follow-up algorithm target.
