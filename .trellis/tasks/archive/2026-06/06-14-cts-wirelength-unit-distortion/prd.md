# CTS Wirelength Unit Distortion Analysis

## Goal

Determine whether the current H-tree characterization wirelength unit introduces algorithm-visible distortion across DAC CTS cases.

The suspected failure mode is:

- H-tree topology has several physical level wire lengths.
- Characterization maps each level to `length_idx = ceil(level_length / wirelength_unit)`.
- The modeled length is then `length_idx * wirelength_unit`.
- For some cases/levels, the modeled length may be much larger than the actual level length.
- That length quantization may perturb RC/timing estimates and solver/candidate selection.

## Inputs

- Existing Phase 1 timing trace summaries under `.trellis/tasks/06-13-commercial-cts-capability-align/summary/`.
- Existing Phase 4 Iter09 CTS-only and Innovus evaluation assets under `.trellis/tasks/06-13-commercial-cts-capability-align/runs/phase4_iter09_kary_buffer_first_cts`.
- Existing Phase 4 score/structural summaries under `.trellis/tasks/06-13-commercial-cts-capability-align/summary/phase4_iter09_kary_buffer_first_*`.
- Source implementation in `src/operation/iCTS/source/flow/synthesis/htree/**`.

## Constraints

- Do not modify `/home/liweiguo/project/DAC-27-CTS`.
- Do not commit unless explicitly requested.
- Do not stage large temporary run assets.
- Temporary trace/instrumentation is allowed only for diagnosis and must not be treated as production algorithm behavior.
- No public config knob or magic tuning parameter may be introduced.

## Acceptance

- Produce a per-case and per-level report showing:
  - resolved wirelength unit
  - actual/requested level length
  - aligned `length_idx`
  - modeled length `unit * idx`
  - absolute and relative over-modeling error
- Correlate wirelength distortion with:
  - L3 row-delay/timing error from existing Phase 1 traces
  - Innovus evaluation final metrics such as latency, skew, buffer count, clock cap/power
  - H-tree selected depth, selected level count, and selected buffer pattern summaries
- State whether the issue exists, whether it is large enough to explain current timing/final metric deltas, and what algorithmic repair direction is justified.
- If repair is implemented, it must:
  - avoid public knobs, case-specific constants, or magic cap parameters
  - work for both native H-tree and analytical H-tree
  - reduce selected-path wirelength quantization error on the 23-case benchmark
  - preserve Innovus evaluation compatibility and avoid catastrophic CTS runtime growth

## Non-Goals

- Running a full new 23-case Innovus evaluation unless existing assets are insufficient.
- Reintroducing hardcoded wirelength/grid knobs.
