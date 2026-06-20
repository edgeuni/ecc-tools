# Phase 4 Iteration Log

## Scope

Phase 4 iterated on CTS algorithm behavior over all 23 DAC synthesized CTS cases. The active goal was structural alignment against Innovus final CTS behavior after Phase 1 had locked the RC/timing model.

## Iterations

| Iteration | Hypothesis | Action | Evidence | Decision |
| --- | --- | --- | --- | --- |
| 01 baseline distribution | Remaining final gap is structural rather than RC/timing. | Generated all-23 algorithm distribution probe from Phase 1 run and Innovus reports. | `summary/phase4_cts_algorithm_alignment/phase4_distribution_report.md`; ECC max depth > Innovus in 23/23, trunk WL > 1.4x in 23/23, leaf WL shorter in 21/23. | Keep as Phase4 baseline. |
| 02 multi-stage local split | Shallow H-tree candidates fail because local sink-load groups exceed one-stage split capacity. | Added recursive local split plan/materialization and ran all-23 CTS/eval. | `runs/phase4_iter02_multistage_split_fast_cts`, `summary/phase4_iter02_multistage_split_fast_eval`, `summary/phase4_iter02_multistage_split_score/phase4_candidate_score_report.md`. Hard fails removed, but latency RMSE worsened `71.47 ps -> 99.77 ps`. | Keep recursive split capability as a feasibility primitive, reject standalone policy as QoR fix. |
| 03 physical complexity selection | Global selection must see local split depth/buffer cost. | Added candidate physical depth/buffer accounting and structural probe. | `summary/phase4_iter03_physical_shape_structural_probe`. Physical-depth RMSE improved versus Iter02 but remained worse than baseline. | Refine. |
| 04 cluster root center | Cluster buffer is physically materialized at geometric center, so clustering/evaluation root should match. | Changed cluster root policy to center and ran structural probe. | `summary/phase4_iter04_cluster_center_structural_probe`. Nearly neutral versus Iter03. | Keep for semantic consistency. |
| 05 no boundary polish, binary path | Boundary cap-balancing might spread clusters and hurt compactness. | Temporarily disabled boundary polish and ran all-23 structural probe. | `summary/phase4_iter05_no_boundary_polish_structural_probe`. No meaningful structural improvement. | Reject and restore boundary polish. |
| 06 physical-depth-first sorting | Selection sorted first by physical depth may avoid deep candidates. | Changed global physical-complexity ordering to depth first and ran all-23 CTS-only/probe. | `runs/phase4_iter06_physical_depth_first_cts`, `summary/phase4_iter06_physical_depth_first_structural_probe`. No material improvement over Iter03/04. | Reject as primary ordering; later reverted to buffer-first in Iter09. |
| 07 k-ary H-tree | Innovus non-leaf fanout is closer to 4, while ECC was binary-biased. | Added max-fanout-derived k-ary topology generation and branching-factor-aware fanout/pattern accounting. | `runs/phase4_iter07_kary_htree_cts`, `summary/phase4_iter07_kary_htree_structural_probe`. Local split depth improved, but implementation exposed tree-height inconsistency. | Reject Iter07 result as invalid implementation; keep hypothesis. |
| 08 k-ary height fix | k-ary target depth must build actual tree height `depth`, not binary `log2(leaf_count)`. | Fixed `TopologyGen` height calculation and added 4-way target-depth test. | `runs/phase4_iter08_kary_height_fix_cts`, `summary/phase4_iter08_kary_height_fix_structural_probe`. All 23 CTS-only succeeded; correct semantics restored, but selected H-tree remained too shallow. | Keep correctness fix; refine selection. |
| 09 k-ary buffer-first selection | With k-ary topology, physical depth alone over-penalizes global depth; actual buffer count should lead ordering. | Restored physical buffer count before physical depth in global candidate ordering and ran all-23 CTS-only plus Innovus eval. | `runs/phase4_iter09_kary_buffer_first_cts`, `summary/phase4_iter09_kary_buffer_first_eval`, `summary/phase4_iter09_kary_buffer_first_score/phase4_candidate_score_report.md`. Latency RMSE `71.47 ps -> 51.99 ps`, latency R2 `0.515 -> 0.743`, buffer-count RMSE `78.21 -> 40.09`, tree max-depth mean delta `1.30 -> 0.91`. | Promote as Phase4 final candidate. |
| 10 no boundary polish, k-ary path | If cap-balance boundary movement is the dominant spread source, disabling it should compact clusters under the new H-tree. | Temporarily disabled boundary polish on top of Iter09 and ran all-23 CTS-only/probe. | `runs/phase4_iter10_kary_no_boundary_cts`, `summary/phase4_iter10_kary_no_boundary_structural_probe`. Cluster mean distance worsened `8.87 -> 8.99 um`, split buffers worsened `147.43 -> 147.87`, selected depth remained under Innovus. | Reject and restore boundary polish. |

## Final Decision

Keep the Iter09 production candidate:

- k-ary H-tree generation derived from max fanout and the 2D H-tree quadrant structure
- k-ary target-depth height fix
- branching-factor-aware topology-pattern fanout accounting
- physical candidate selection that sees local split cost and sorts by actual buffer count before physical depth
- recursive local split capability and center cluster-root policy retained

Do not keep Iter10 no-boundary-polish behavior. Phase 4 improved latency/topology/buffer alignment but did not solve skew. The next alignment stage should focus on skew spread, leaf/load balance, and local branch distribution instead of further timing-model or public-knob tuning.
