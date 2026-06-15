# Phase 4 Final Report

## Problem

After Phase 1 timing/RC alignment, ECC CTS still diverged from Innovus in final QoR because the generated clock-tree structure differed:

- ECC clock trees were deeper than Innovus in all 23 Phase 2 baseline cases.
- ECC trunk wire length was consistently high and leaf distribution was consistently short.
- The old H-tree construction was effectively binary-biased, while Innovus non-leaf fanout distribution was closer to a 4-way CTS topology.
- Sink clustering was already packed to the minimum legal cluster count in 20/23 cases, so simply changing clustering boundary polish was unlikely to close the main gap.

## Root Cause

The dominant Phase4 root cause is H-tree topology structure and candidate selection, not the Phase1 timing model.

The all-23 experiments showed:

- A recursive local split is required to make shallow candidates feasible, but using it alone over-inserts local buffers and worsens final latency.
- A k-ary H-tree is a better structural model than a binary H-tree for the target Innovus baseline, but it must be paired with correct target-depth semantics.
- The first k-ary implementation accidentally built tree height with binary `log2(leaf_count)` logic; this was fixed and pinned by test.
- After k-ary correction, choosing by physical depth first made the global H-tree too shallow. Sorting first by actual physical buffer count and then by physical depth gave the best all-23 final result.
- Disabling boundary cap-balance polish did not improve compactness; under the final k-ary candidate it slightly worsened cluster distance and split-buffer count.

## Solution

The Phase4 promoted candidate is Iter09:

- `TopologyGen` now supports an internal k-ary H-tree branching factor.
- Production H-tree resolves branching from `max_fanout`, capped by the 2D quadrant nature of H-tree splitting.
- H-tree target depth now builds the exact requested tree height for k-ary topologies.
- Topology-pattern fanout legality and weighted buffer accounting use the actual branching factor/topology level size.
- Global candidate selection accounts for local split depth and split buffer count, and orders physical complexity by actual buffer count before physical depth.
- Recursive local split is retained as a feasibility primitive.
- Cluster root policy stays at center to match where local buffers are materialized.
- Boundary polish remains enabled; the no-boundary Iter10 control was rejected.

## Validation Assets

- Final CTS-only run: `runs/phase4_iter09_kary_buffer_first_cts`
- Final Innovus evaluation: `summary/phase4_iter09_kary_buffer_first_eval/ecc-tools.summary.csv`
- Final score report: `summary/phase4_iter09_kary_buffer_first_score/phase4_candidate_score_report.md`
- Iteration log: `summary/phase4_iterations/phase4_iteration_log.md`
- Structural probes:
  - `summary/phase4_iter08_kary_height_fix_structural_probe`
  - `summary/phase4_iter09_kary_buffer_first_structural_probe`
  - `summary/phase4_iter10_kary_no_boundary_structural_probe`

All 23 CTS-only cases completed for the final candidate. All 23 Innovus detail-route evaluation cases and the dataset summary completed successfully.

## Effect

| Metric | Phase 1 | Phase 4 Iter09 | Direction |
| --- | ---: | ---: | --- |
| latency avg MAE | 61.96 ps | 44.48 ps | improved |
| latency avg RMSE | 71.47 ps | 51.99 ps | improved |
| latency avg R2 | 0.515 | 0.743 | improved |
| latency geomean ratio | 1.403x | 1.303x | improved |
| skew max MAE | 21.26 ps | 21.09 ps | nearly flat |
| skew max RMSE | 25.48 ps | 28.33 ps | worse |
| buffer-count RMSE | 78.21 | 40.09 | improved |
| clock total cap RMSE | 0.2957 pF | 0.2867 pF | improved |
| clock power RMSE | 0.1714 mW | 0.1252 mW | improved |
| tree max-depth mean delta | +1.30 levels | +0.91 levels | improved |
| timing clock-stage avg delta | +1.22 stages | +0.77 stages | improved |

Case-level highlights:

- `iwls2005__vga_lcd`: latency gap improved from `181 ps` to `17.5 ps`.
- `openroad__dynamic_node`: latency gap improved from `132 ps` to `33.5 ps`.
- `iwls2005__spi`: latency gap improved from `93 ps` to `47 ps`.
- `openroad__fifo`: latency gap improved from `71.5 ps` to `25.5 ps`.
- Main latency regressions remain `iwls2005__usb_funct`, `openroad__ethmac`, `iwls2005__ac97_ctrl`, and `iwls2005__des`.
- Main skew regressions remain `iwls2005__ss_pcm`, `iwls2005__wb_conmax`, `iwls2005__des`, and `iwls2005__ac97_ctrl`.

## Phase4 Status

Phase 4 is complete as a latency/topology/buffer alignment stage:

- It completed at least 10 all-23 algorithm iterations.
- It produced a production algorithm candidate without new public QoR knobs.
- It passed targeted iCTS build/tests.
- It passed all-23 CTS-only and Innovus detail-route evaluation.
- It improved latency, tree-depth, stage-count, buffer-count, cap, and power alignment.

Phase 4 is not a final skew solution. The next stage should target skew spread and leaf/local load balance using Innovus distribution evidence, not timing-model retuning or hardcoded margins.
