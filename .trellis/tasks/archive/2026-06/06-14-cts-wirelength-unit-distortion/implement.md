# Implementation Plan

1. Inspect H-tree wirelength grid and selected embedding code.
2. Parse existing CTS logs and Phase 1/4 CSV assets.
3. If needed, add a diagnostic-only extraction script or temporary trace for per-level requested/aligned lengths.
4. Generate per-level and per-case distortion summaries.
5. Correlate distortion with L3 timing and Innovus evaluation metrics.
6. Write the final report and state whether a production repair is justified.

## Validation

- Run the analysis script over all available 23 Phase 4 Iter09 cases.
- Verify output CSVs are generated and contain all cases with available logs.
- Run `git diff --check` for touched task/code files.

## Staging

This task should not stage large run directories or build outputs.

## Execution Notes

- Temporary C++ trace was added only to emit:
  - `HTree Full Level Wirelength Quantization`
  - `HTree Selected Level Wirelength Quantization`
- Built trace binary:

```bash
cmake --build build-gcc11-release --target ecc_bin -j 8
```

- Ran all 23 CTS-only cases:

```bash
python3 .trellis/tasks/06-13-commercial-cts-capability-align/scripts/run_phase4_cts_only.py \
  --run-root .trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/wirelength_trace_cts \
  --summary-dir .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/wirelength_trace_cts \
  --ecc-bin /home/liweiguo/project/ecc-tools-dev/bin/ecc_bin \
  --jobs 4 \
  --continue-on-error \
  --command-timeout-seconds 3600
```

- Wrapper status note: CTS itself generated logs and DEF outputs for all 23 cases, but `run-cts-baseline` returned nonzero after save because `feature_summary` is unavailable in the Tcl save step.
- Parsed quantization trace and joined with Phase4 evaluation and Phase1 L3 timing evidence:

```bash
python3 .trellis/tasks/06-14-cts-wirelength-unit-distortion/scripts/analyze_wirelength_unit_distortion.py \
  --run-root .trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/wirelength_trace_cts \
  --structural-csv .trellis/tasks/06-13-commercial-cts-capability-align/summary/phase4_iter09_kary_buffer_first_structural_probe/phase4_cts_algorithm_features.csv \
  --l3-csv .trellis/tasks/06-13-commercial-cts-capability-align/summary/l3_native_ground_coupling_full_detail_l3/l3_timing_case_summary.csv \
  --output-dir .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary
```

- Temporary C++ trace was then removed and `ecc_bin` was rebuilt successfully.

## Results

- Main report: `summary/wirelength_unit_distortion_report.md`
- Per-level CSV: `summary/wirelength_unit_level_distortion.csv`
- Per-case CSV: `summary/wirelength_unit_case_summary.csv`
- Correlation CSV: `summary/wirelength_unit_metric_correlation.csv`

Key findings:

- Wirelength unit distortion exists in 23/23 cases.
- 49 selected H-tree levels were observed; 32/49 selected levels have relative over-model above 25%, 3/49 above 100%.
- 17/23 cases have accumulated selected-path length over-model above 25%.
- Worst selected path over-model is `131.481 um` on `iwls2005__vga_lcd`.
- Source trunk quantization is not material: max source-trunk absolute delta is `0.000200 um`.
- Strongest measured relationship is with buffering pressure: `network_extra_length_um -> buffer_count_delta` has Pearson `r=0.886`, `R2=0.784`.
- Timing relation is moderate rather than conclusive: `path_delta_um -> L3 mean abs arrival delta` has Pearson `r=0.602`, `R2=0.363`.

## Repair Execution Notes

Implemented production repair in:

- `src/operation/iCTS/source/flow/synthesis/htree/characterization/wirelength/WirelengthGrid.cc`
- `src/operation/iCTS/source/flow/synthesis/htree/characterization/wirelength/WirelengthGrid.hh`
- `src/operation/iCTS/source/flow/synthesis/htree/characterization/Characterization.cc`
- `src/operation/iCTS/test/flow/synthesis/htree/WirelengthGridTest.cc`

Behavior:

- auto-derived grid now scores candidate units by cumulative topology-prefix over-model error
- source-to-root length is coverage-only for H-tree characterization, not a direct unit-shaping length
- direct characterization remains sparse and topology-driven
- no public CTS config knob or case-specific cap was added
- analytical H-tree keeps an explicit unit-length direct bin

Validation:

```bash
cmake --build build-gcc11-release --target ecc_bin icts_test_flow_synthesis_htree -j 8
./bin/icts_test_flow_synthesis_htree --gtest_filter='WirelengthGridTest.*'
cmake --build build-gcc11-release --target icts_test_flow_synthesis_htree_analytical_solver -j 8
./bin/icts_test_flow_synthesis_htree_analytical_solver
python3 .trellis/tasks/06-13-commercial-cts-capability-align/scripts/run_phase4_cts_only.py \
  --run-root .trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/adaptive_prefix_grid_23_cts \
  --summary-dir .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_cts \
  --ecc-bin /home/liweiguo/project/ecc-tools-dev/bin/ecc_bin \
  --jobs 4 \
  --continue-on-error \
  --command-timeout-seconds 3600
python3 .trellis/tasks/06-13-commercial-cts-capability-align/scripts/evaluate_existing_cts_run.py \
  --run-root .trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/adaptive_prefix_grid_23_cts \
  --summary-dir .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_eval \
  --tool-name ecc-tools \
  --continue-on-error \
  --command-timeout-seconds 3600
python3 .trellis/tasks/06-13-commercial-cts-capability-align/scripts/score_phase4_candidate.py \
  --candidate-name adaptive_wirelength_grid \
  --candidate-summary .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_eval/ecc-tools.summary.csv \
  --candidate-run .trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/adaptive_prefix_grid_23_cts \
  --output-dir .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_score
python3 .trellis/tasks/06-14-cts-wirelength-unit-distortion/scripts/analyze_adaptive_grid_repair.py \
  --old-level-csv .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/wirelength_unit_level_distortion.csv \
  --old-case-csv .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/wirelength_unit_case_summary.csv \
  --old-run-root .trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/wirelength_trace_cts \
  --candidate-run-root .trellis/tasks/06-14-cts-wirelength-unit-distortion/runs/adaptive_prefix_grid_23_cts \
  --candidate-eval-summary .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_eval/ecc-tools.summary.csv \
  --score-dir .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_score \
  --output-dir .trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_repair
```

Final repair result:

- selected-path relative over-model mean: `0.4727 -> 0.0375`
- selected-path absolute over-model mean: `43.823 um -> 7.113 um`
- lower selected-path relative error: `23/23` cases
- direct characterized wirelength points sum: `74 -> 54`
- required coverage iterations sum: `92 -> 147`
- CTS total runtime sum from `CTS Runtime Overview`: `344.210 s -> 392.157 s`, `+13.93%`
- Innovus metric fit improved for latency MAE, skew MAE, buffer-count MAE, clock-wirelength MAE, and clock-power MAE
- clock total cap MAE regressed slightly and remains a follow-up algorithm target

Final evidence:

- `.trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_repair/adaptive_grid_repair_report.md`
- `.trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_eval/ecc-tools.summary.csv`
- `.trellis/tasks/06-14-cts-wirelength-unit-distortion/summary/adaptive_prefix_grid_23_score/phase4_candidate_score_report.md`
