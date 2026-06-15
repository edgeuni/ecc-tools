# Phase 4 CTS Algorithm Distribution Baseline

## Scope

- Cases: 23 matched DAC synthesized CTS cases.
- ECC source: Phase 1 native ground/coupling detail-run under the current task.
- Innovus source: DAC commercial strict detail-route evaluation, read-only.
- Purpose: explain final CTS metric gaps through algorithm behavior, not through timing/RC retuning.

## All-Case Signals

- ECC report max depth is greater than Innovus in 23 / 23 cases.
- ECC trunk wire length is greater than 1.4x Innovus in 23 / 23 cases.
- ECC leaf wire length is shorter than Innovus in 21 / 23 cases.
- H-tree depth pruning has a first monotone hard fail in 21 / 23 cases.
- The first hard fail exceeds the current single-stage local split capacity (`max_fanout^2`) in 21 / 23 cases.
- Sink clustering is packed to the minimum legal cluster count in 20 / 23 cases.
- Source-to-root segment inserts at least one buffer in 2 / 23 cases.

## Metric Summary

| metric | unit | count | mean | median | min | max | rmse |
| --- | --- | --- | --- | --- | --- | --- | --- |
| latency avg delta | ns | 23 | 0.0619565 | 0.0535 | 0.0155 | 0.181 | 0.0714703 |
| skew max delta | ns | 23 | 0.0178696 | 0.016 | -0.021 | 0.053 | 0.0254806 |
| ECC max depth minus Innovus | level | 23 | 1.30435 | 1 | 1 | 3 | 1.41421 |
| HTree selected depth minus Innovus max depth | level | 23 | -0.0434783 | 0 | -2 | 2 | 1.23359 |
| trunk wire ratio | x | 23 | 1.64151 | 1.6277 | 1.42081 | 1.86345 | 1.64543 |
| leaf wire ratio | x | 23 | 0.924568 | 0.888243 | 0.773361 | 1.47853 | 0.933935 |
| cluster compression ratio | loads/cluster | 23 | 3.94872 | 3.98889 | 3.57143 | 4 | 3.94986 |
| cluster leaf mean distance | um | 23 | 8.3487 | 7.83 | 6.43 | 17.26 | 8.61321 |
| hard fail load count | loads | 21 | 19.3333 | 19 | 17 | 25 | 19.4887 |
| hard fail over max_fanout^2 | loads | 21 | 3.33333 | 3 | 1 | 9 | 4.14039 |

## Correlations

The correlation rows report `r` in the `mean` column and `R2` in the `median` column.

| metric | count | mean | median |
| --- | --- | --- | --- |
| latency delta vs selected HTree depth delta | 23 | 0.307804 | 0.0947433 |
| latency delta vs trunk wire ratio | 23 | -0.00705711 | 4.98028e-05 |
| latency delta vs cluster leaf mean distance | 23 | 0.109004 | 0.0118818 |
| skew delta vs timing stage spread delta | 23 | 0.680077 | 0.462505 |
| skew delta vs cluster leaf max distance | 23 | 0.452829 | 0.205055 |

## Highest Gap Cases

| case | latency_avg_delta_ns | skew_max_delta_ns | tree_max_depth_delta | htree_selected_depth_minus_innovus_max_depth | first_hard_fail_load_count | first_hard_fail_loads_over_two_level | cluster_compression_ratio | cluster_leaf_mean_distance | trunk_wire_length_ratio |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| iwls2005__vga_lcd | 0.181 | 0.051 | 3 | 2 | 18 | 2 | 3.99929 | 7.83 | 1.60159 |
| openroad__dynamic_node | 0.132 | 0.018 | 2 | 1 | 20 | 4 | 4 | 7.94 | 1.54524 |
| iwls2005__usb_funct | 0.083 | 0.038 | 1 | -1 | 25 | 9 | 3.96429 | 17.26 | 1.47631 |
| iwls2005__spi | 0.093 | 0.023 | 2 | 0 | 17 | 1 | 3.94828 | 8.37 | 1.83484 |
| iwls2005__des | 0.082 | 0.032 | 2 | 1 | 20 | 4 | 4 | 7.84 | 1.54463 |
| openroad__fifo | 0.0715 | -0.021 | 1 | 0 | 19 | 3 | 4 | 7.92 | 1.76471 |
| iwls2005__mem_ctrl | 0.076 | 0.01 | 2 | 1 | 19 | 3 | 3.99145 | 7.35 | 1.6277 |
| openroad__uart | 0.0635 | -0.015 | 1 | -2 | 20 | 4 | 3.95 | 7.79 | 1.86345 |
| openroad__gcd | 0.0515 | 0.026 | 2 | -1 |  |  | 3.77778 | 7.77 | 1.64823 |
| iwls2005__systemcaes | 0.0645 | 0.01 | 1 | 0 | 19 | 3 | 3.9881 | 8.97 | 1.54457 |

## Phase 4 Root-Cause Reading

- The common blocker is structural feasibility: shallower H-tree candidates are eliminated when a terminal load group needs more than the current single-stage local split can legally drive.
- The fast sink clustering layer compresses original sinks to nearly `max_fanout` clusters, so the H-tree sees compact local-buffer loads but can still create 17-20 load terminal groups in shallower candidates.
- Existing clustering contains routing-cap balance terms and boundary-load movement. Those may improve cap equality while spreading cluster centers, so they must be tested as algorithm candidates against all 23 cases.
- Source trunk remains a secondary axis: trunk length is uniformly high, but the first fix should not hide the H-tree/local split capacity root cause.

## Candidate Iteration Order

1. Baseline extraction and invariant checks.
2. Multi-stage local split for sink-load regions that exceed `max_fanout^2`.
3. Compact clustering experiment by disabling boundary cap-balancing polish.
4. Compact clustering experiment by reducing recursive split cap-balance weight pressure.
5. Control experiment: build H-tree directly on original sinks.
6. H-tree selection using final physical depth including split/local branch buffers.
7. Source-trunk selection with distribution-aware cost, after H-tree/local feasibility is fixed.
8. Cluster-root policy check: median versus center for local-buffer placement.
9. Branch buffer sizing distribution check, without adding public drive-strength knobs.
10. Combined candidate acceptance run and rollback of any non-general change.
