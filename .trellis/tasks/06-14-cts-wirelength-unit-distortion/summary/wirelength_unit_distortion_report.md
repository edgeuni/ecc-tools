# CTS Wirelength Unit Distortion Analysis

## Scope

- Run root: `.trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/wirelength_trace_cts`.
- Evidence: temporary H-tree level quantization tables in `cts_detail.log`, existing Phase4 Innovus evaluation summary, and Phase1 L3 timing alignment summary.
- Note: the CTS baseline wrapper returned failure after CTS because `feature_summary` is unavailable in the save step. The CTS logs and DEF outputs were generated and are usable for this diagnostic.

## Main Finding

- The wirelength lattice distortion exists in 23/23 cases. All selected H-tree levels are mapped through `ceil(actual_length / unit) * unit`, so the characterization length is always a covering over-model.
- 3/23 cases have at least one selected level with more than 100% relative length over-model. 17/23 cases have more than 25% accumulated selected path length over-model.
- Worst selected-path absolute over-model: 131.481 um. Worst estimated network extra cap from selected-level quantization: 0.5173 pF.
- Source trunk quantization is not the issue: max source-trunk absolute delta is 0.000200 um because source-to-root lengths are already included as characterization requests.

## Worst Cases By Selected Path Over-Model

| case | path delta um | path rel error | path extra cap pF | max selected rel error | selected idx bins | L3 mean arrival delta ns | latency delta ns | skew delta ns |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| iwls2005__vga_lcd | 131.481 | 29.4% | 0.02639 | 103.3% | 3/4 | 0.02645 | 0.1810 | 0.0510 |
| openroad__ethmac | 123.195 | 33.8% | 0.02473 | 86.0% | 3/3 | 0.00953 | 0.0195 | 0.0330 |
| iwls2005__des | 114.941 | 66.4% | 0.02307 | 96.2% | 3/3 | 0.02072 | 0.0820 | 0.0320 |
| iwls2005__ac97_ctrl | 105.512 | 70.0% | 0.02118 | 99.0% | 3/3 | 0.01779 | 0.0420 | 0.0300 |
| openroad__dynamic_node | 96.622 | 50.3% | 0.01939 | 273.1% | 2/4 | 0.00580 | 0.1320 | 0.0180 |
| openroad__jpeg | 80.765 | 26.4% | 0.01621 | 52.4% | 3/3 | 0.02086 | 0.0370 | 0.0160 |

## Worst Cases By Relative Selected-Level Error

| case | max selected rel error | path rel error | selected tail idx=1 | selected idx bins | path delta um |
| --- | ---: | ---: | ---: | ---: | ---: |
| openroad__dynamic_node | 273.1% | 50.3% | 3 | 2/4 | 96.622 |
| iwls2005__tv80 | 116.0% | 43.4% | 1 | 2/3 | 38.752 |
| iwls2005__vga_lcd | 103.3% | 29.4% | 2 | 3/4 | 131.481 |
| iwls2005__ac97_ctrl | 99.0% | 70.0% | 1 | 3/3 | 105.512 |
| iwls2005__systemcdes | 96.6% | 96.6% | 0 | 1/1 | 29.586 |
| iwls2005__des | 96.2% | 66.4% | 1 | 3/3 | 114.941 |

## Correlation Highlights

The strongest correlations are useful as triage signals, not proof of direct STA error. L3 post-build timing uses the final geometry, while this quantization acts earlier in characterization and candidate selection.

| predictor | target | n | pearson r | r2 | spearman r |
| --- | --- | ---: | ---: | ---: | ---: |
| network_extra_length_um | buffer_count_delta | 23 | 0.886 | 0.784 | 0.877 |
| network_extra_length_um | abs_buffer_count_delta | 23 | 0.885 | 0.784 | 0.837 |
| path_delta_um | buffer_count_delta | 23 | 0.781 | 0.610 | 0.824 |
| path_extra_cap_pf | buffer_count_delta | 23 | 0.781 | 0.610 | 0.824 |
| path_extra_cap_to_maxcap_ratio | buffer_count_delta | 23 | 0.781 | 0.610 | 0.824 |
| path_extra_cap_to_maxcap_ratio | abs_buffer_count_delta | 23 | 0.776 | 0.602 | 0.774 |
| path_delta_um | abs_buffer_count_delta | 23 | 0.776 | 0.602 | 0.774 |
| path_extra_cap_pf | abs_buffer_count_delta | 23 | 0.776 | 0.602 | 0.774 |
| max_full_rel_error_ratio | buffer_count_delta | 23 | 0.717 | 0.515 | 0.689 |
| max_full_rel_error_ratio | abs_buffer_count_delta | 23 | 0.711 | 0.506 | 0.637 |
| network_extra_length_um | l3_max_abs_arrival_delta_ns | 23 | 0.659 | 0.434 | 0.619 |
| max_full_rel_error_ratio | l3_mean_abs_arrival_delta_ns | 23 | 0.650 | 0.423 | 0.541 |

## Interpretation

1. The distortion is real and case-dependent. Large designs such as `iwls2005__vga_lcd`, `openroad__ethmac`, and `openroad__jpeg` accumulate large absolute path and network RC over-model. Small designs can have extreme relative errors on one short level, but much smaller absolute RC impact.
2. The direct Innovus-vs-FastSTA L3 timing gap is not explained mainly by source-trunk quantization. Source trunk alignment is effectively exact in this run.
3. The main algorithm impact is pre-STA: multiple physical levels collapse into the same `length_idx`, so the segment frontier and solver see the same characterized delay/cap/power for materially different level lengths. This can bias buffer pattern choice, terminal branch buffering, candidate depth scoring, and power/cap estimates.
4. The existing L3 timing summary can remain valid while this problem still exists, because L3 checks post-build timing on the emitted clock tree. The wirelength unit problem affects which clock tree is chosen before that final timing evaluation.

## Recommended Next Step

Replace the auto-derived coarse uniform unit policy with a characterization lattice that preserves requested level lengths directly or uses a bounded-error adaptive grid. This should be algorithmic, not a new user knob: the solver should avoid collapsing distinct physical levels into the same RC/delay bin when their requested lengths differ materially.

## Output Assets

- `summary/wirelength_unit_level_distortion.csv`
- `summary/wirelength_unit_case_summary.csv`
- `summary/wirelength_unit_metric_correlation.csv`
