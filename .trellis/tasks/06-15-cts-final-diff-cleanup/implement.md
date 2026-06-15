# Implementation Checklist

## Cleanup Items

- [x] Create the Trellis task and capture the final-diff cleanup scope.
- [x] Recompute full diff from `origin/cts_refactor` including local commits and
      staged changes.
- [x] Restore `.trellis/spec/backend/database-guidelines.md` so the final branch
      diff has no `.trellis/spec/` changes.
- [x] Reconfirm staged assets are compact and exclude experimental/raw paths.
- [x] Produce a final full-diff cleanup report with interface/behavior/risk
      summary.
- [x] Re-run targeted compile and tests.
- [x] Re-run full `ecc_dev_tools` check for `src/operation/iCTS`.
- [x] Update this checklist with completed evidence.

## Completed Evidence

- Full code diff from remote merge-base: 60 files, +2493/-343.
- Full `.trellis/spec/` diff from remote merge-base: empty.
- Staged scope after cleanup: 124 files, 1,150,542 bytes.
- Staged raw/experimental filter: no `row_matches`, no `run_manifest.json`, no
  `research/`, no `scripts/`.
- Unstaged tracked files: none.
- Report: `summary/final_diff_cleanup_report.md`.
- Targeted build passed:
  `cmake --build build-gcc11-release --target ecc_bin icts_test_database_io
  icts_test_flow_optimization icts_test_flow_synthesis_htree
  icts_test_module_topology_gen
  icts_test_flow_synthesis_htree_analytical_solver -j 8`.
- Targeted tests passed: `icts_test_database_io`,
  `icts_test_flow_optimization`, `icts_test_flow_synthesis_htree`,
  `icts_test_module_topology_gen`,
  `icts_test_flow_synthesis_htree_analytical_solver`.
- Full `ecc_dev_tools` passed for `src/operation/iCTS`: in-scope findings 0.
  The checker still reports out-of-scope diagnostics from external database and
  parser headers triggered by iCTS translation units.

## Validation Commands

```bash
cmake --build build-gcc11-release --target ecc_bin icts_test_database_io icts_test_flow_optimization icts_test_flow_synthesis_htree icts_test_module_topology_gen icts_test_flow_synthesis_htree_analytical_solver -j 8
./bin/icts_test_database_io
./bin/icts_test_flow_optimization
./bin/icts_test_flow_synthesis_htree
./bin/icts_test_module_topology_gen
./bin/icts_test_flow_synthesis_htree_analytical_solver
python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS
```

## Notes

- Use `git diff $(git merge-base HEAD @{u})` for final branch scope.
- Use `git diff --cached` only to verify what the next commit would include.
- Do not stage raw rerun assets or temporary scripts.
