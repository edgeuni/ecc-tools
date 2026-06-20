# Implementation Plan

## Current Constraint

- Do not commit unless the user explicitly asks.
- Do not modify `/home/liweiguo/project/DAC-27-CTS`.
- Do not run broad `ecc dev` checks during the normal loop.
- Use Phase 1 L3 timing results as a guard, not as the active optimization target.
- Phase 4 must consider all 23 synthesized CTS cases.

## Retained History

Phase 1 complete:

- timing/RC row-delay alignment accepted
- retained guard assets under `summary/l3_native_ground_coupling_full_detail_l3*` and `summary/l3_native_ground_coupling_full_error_metrics`

Phase 2 complete:

- all-23 structural diagnosis under `summary/phase2_clock_tree_qor`
- dominant gap: deeper ECC tree, longer trunk, shorter leaf, late latency, worse skew

Phase 3 complete:

- adaptive H-tree global selection
- production full-depth H-tree search
- final report under `summary/phase3_alignment_final`
- decision: continue with sink-load/local-buffer and source-trunk/segment scoring

## Phase 4 Workflow

For every promoted candidate:

1. analyze all 23 cases
2. write hypothesis and expected structural movement
3. implement production algorithm change
4. build `ecc_bin`
5. run targeted iCTS tests
6. run all-23 CTS-only bench
7. run all-23 Innovus detail-route evaluation
8. regenerate Phase 4 scoreboards
9. update iteration log
10. decide keep/refine/revert/split

## Phase 4 Completed Work Items

- [x] Load Trellis task/spec context.
- [x] Compact Phase 1-3 PRD/design/implement.
- [x] Add all-23 Phase 4 distribution analysis script.
- [x] Generate all-23 Phase 4 distribution probes.
- [x] Create `summary/phase4_iterations/phase4_iteration_log.md`.
- [x] Complete 10 Phase 4 iteration records.
- [x] Implement production CTS algorithm candidates without new public QoR knobs.
- [x] Build `ecc_bin` and targeted iCTS tests for final candidate.
- [x] Run all-23 DAC CTS-only bench for final candidate.
- [x] Run all-23 Innovus detail-route evaluation for final candidate.
- [x] Generate `summary/phase4_final/phase4_final_report.md`.
- [x] Update PRD/design/implement with Phase 4 result.

## Phase 4 Iteration Slots

| Iteration | Hypothesis Area | Status | Decision |
| --- | --- | --- | --- |
| 01 | all-23 baseline distribution and clustering evidence | completed | keep as baseline |
| 02 | recursive local sink-load split | completed | keep primitive, reject standalone QoR result |
| 03 | physical complexity selection | completed | refine |
| 04 | cluster-root center consistency | completed | keep |
| 05 | no-boundary-polish binary control | completed | reject |
| 06 | physical-depth-first selection | completed | reject as primary ordering |
| 07 | k-ary H-tree first implementation | completed | reject result due tree-height bug |
| 08 | k-ary target-depth height fix | completed | keep correctness fix |
| 09 | k-ary buffer-first physical selection | completed | promote final Phase4 candidate |
| 10 | no-boundary-polish k-ary control | completed | reject and restore boundary polish |

Detailed iteration evidence is in `summary/phase4_iterations/phase4_iteration_log.md`.

## Final Phase 4 Validation

```bash
cmake --build build-gcc11-release --target icts_test_module_topology_gen icts_test_flow_synthesis_htree ecc_bin -j 8
./bin/icts_test_module_topology_gen
./bin/icts_test_flow_synthesis_htree
python3 .trellis/tasks/06-13-commercial-cts-capability-align/scripts/run_phase4_cts_only.py --run-root .trellis/tasks/06-13-commercial-cts-capability-align/runs/phase4_iter09_kary_buffer_first_cts --summary-dir .trellis/tasks/06-13-commercial-cts-capability-align/summary/phase4_iter09_kary_buffer_first_cts --ecc-bin /home/liweiguo/project/ecc-tools-dev/bin/ecc_bin --jobs 4 --continue-on-error --command-timeout-seconds 3600
python3 .trellis/tasks/06-13-commercial-cts-capability-align/scripts/evaluate_existing_cts_run.py --run-root .trellis/tasks/06-13-commercial-cts-capability-align/runs/phase4_iter09_kary_buffer_first_cts --summary-dir .trellis/tasks/06-13-commercial-cts-capability-align/summary/phase4_iter09_kary_buffer_first_eval --continue-on-error --command-timeout-seconds 3600
python3 .trellis/tasks/06-13-commercial-cts-capability-align/scripts/score_phase4_candidate.py --candidate-name phase4_iter09_kary_buffer_first --candidate-summary .trellis/tasks/06-13-commercial-cts-capability-align/summary/phase4_iter09_kary_buffer_first_eval/ecc-tools.summary.csv --candidate-run .trellis/tasks/06-13-commercial-cts-capability-align/runs/phase4_iter09_kary_buffer_first_cts --output-dir .trellis/tasks/06-13-commercial-cts-capability-align/summary/phase4_iter09_kary_buffer_first_score
```

All 23 CTS-only cases, all 23 Innovus detail-route evaluations, and the dataset summary succeeded.

## Finish-Work Cleanup Validation

Code cleanup:

- Removed/verified absence of temporary H-tree timing and wirelength quantization trace strings from production iCTS code.
- Replaced recursive local split traversal/materialization helpers with iterative traversal where the final checker flagged recursion.
- Fixed checker-reported naming, optional access, initializer, ranges, and allocation issues in touched CTS tests/source.
- Kept algorithm comments only where they document production behavior.

Targeted build/test after cleanup:

```bash
cmake --build build-gcc11-release --target ecc_bin icts_test_flow_synthesis_htree icts_test_database_io icts_test_module_topology_gen icts_test_flow_synthesis_htree_analytical_solver -j 8
./bin/icts_test_flow_synthesis_htree
./bin/icts_test_database_io
./bin/icts_test_module_topology_gen
./bin/icts_test_flow_synthesis_htree_analytical_solver
```

Full CTS finish-work check:

```bash
python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS
```

Final result: in-scope findings `0`; format, tidy, headers, CMake, and IWYU all passed. Remaining diagnostics are out-of-scope external-header/compiler warnings reported by the checker.

## Validation Commands

Targeted build/test:

```bash
cmake --build build-gcc11-release --target icts_test_flow_synthesis_htree icts_test_database_config ecc_bin -j 8
./bin/icts_test_flow_synthesis_htree
./bin/icts_test_database_config
```

All-23 DAC run shape:

```bash
python3 experiments/scripts/run_experiment.py \
  --dataset experiments/configs/synthesis_dataset.default.json \
  --run-root /home/liweiguo/project/ecc-tools-dev/.trellis/tasks/06-13-commercial-cts-capability-align/runs/<run_name> \
  --summary-dir /home/liweiguo/project/ecc-tools-dev/.trellis/tasks/06-13-commercial-cts-capability-align/summary/<run_name> \
  --innovus-runtime-config /home/liweiguo/project/benchmark_ecc/configs/runtime/innovus_offline.json \
  --openroad-bin third_party/OpenROAD/build/bin/openroad \
  --ecc-bin /home/liweiguo/project/ecc-tools-dev/bin/ecc_bin \
  --tools ecc-tools \
  --continue-on-error \
  --command-timeout-seconds 3600
```
