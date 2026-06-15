# Design

## Analysis Model

The relevant production code derives a characterization grid from topology level lengths:

```text
requested_level_lengths_um -> wirelength_unit_um -> length_idx -> modeled_length_um
```

The distortion metric is:

```text
modeled_length_um = ceil(actual_level_length_um / wirelength_unit_um) * wirelength_unit_um
absolute_error_um = modeled_length_um - actual_level_length_um
relative_error = absolute_error_um / actual_level_length_um
```

This is an upper-bounding grid, so the expected error is non-negative. The risk is not sign but magnitude and whether the magnitude is case-dependent.

## Data Sources

Primary:

- `cts_detail.log` for `resolved_wirelength_unit`, selected depth/levels, selected segment pattern IDs, and selected level buffer counts.
- iCTS source code for exact grid semantics.
- Phase 4 Iter09 structural and final-metric summaries.
- Phase 1 L3 timing summaries for timing error guard.

Fallback:

- add temporary trace to emit per-level `requested_length_um`, `aligned_length_idx`, and `aligned_length_um` if logs do not contain enough per-level data.

## Output

The task writes only lightweight analysis artifacts:

- `summary/wirelength_unit_level_distortion.csv`
- `summary/wirelength_unit_case_summary.csv`
- `summary/wirelength_unit_metric_correlation.csv`
- `summary/wirelength_unit_distortion_report.md`
- `summary/adaptive_prefix_grid_repair/adaptive_grid_repair_report.md`
- `summary/adaptive_prefix_grid_repair/adaptive_grid_quantization_comparison.csv`
- `summary/adaptive_prefix_grid_repair/adaptive_grid_metric_delta.csv`

## Decision Criteria

Treat the issue as proven if all are true:

- large per-level relative distortion exists in multiple non-trivial cases
- the distortion aligns with candidate/timing evidence, such as inflated modeled delay/cap at affected levels
- cases with larger distortion show materially worse L3 timing or Innovus final metric deltas after controlling for topology depth/load size

Treat it as a secondary effect if:

- distortion exists but is bounded to small-geometry or low-load cases
- correlations with L3 timing/final metrics are weak
- the existing Phase 1 timing guard remains strong in high-distortion cases

## Repair Design

The production repair keeps CharBuilder's uniform integer lattice, but changes how the H-tree flow derives that lattice when runtime config does not provide a usable unit:

- derive the auto unit from topology level lengths, not from source-to-root coverage length
- treat source-to-root length as coverage-only so the lattice can synthesize its frontier without forcing a long direct characterization slot
- choose the auto unit by minimizing cumulative prefix quantization error across topology levels, because native and analytical H-tree depth candidates consume prefixes of the full level list
- keep direct characterization sparse: direct points are only the topology level bins plus analytical unit bin when needed
- keep `wirelength_iterations` as the coverage range for required source/topology lengths

This is algorithmic rather than parameter tuning: the budget is the topology level count already present in the H-tree, and the objective is physical over-model error induced by the lattice itself.
