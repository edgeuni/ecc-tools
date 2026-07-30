# Implementation plan

## Review gate

- [ ] Obtain explicit user approval for the recommended configuration, nearest-parent tool behavior, approximately 269-file mechanical reformat, `stage/StageSummary.hh` migration, binary acceptance, and report-only full spec audit.
- [ ] Start the Trellis task only after that approval.

## Implementation

1. Add the iRT-identical `src/operation/iCTS/.clang-format`.
2. Move `source/data_manager/result/StageResult.hh` to `source/data_manager/stage/StageSummary.hh`; update CMake and all includes; remove the old directory; do not modify `.gitignore`.
3. Update `ecc_dev_tools` format check/fix commands to pass `--style=file` explicitly.
4. Update format result notes and finding wording to describe the applicable nearest-parent configuration.
5. Add focused unit tests and update the tool README.
6. Run the iCTS format-only fix through `ecc_dev_tools`.
7. Inspect the bulk diff for file-scope and mechanical-only compliance.
8. Build and run the iCTS tests.
9. Run the exact iEDA/iCTS binary acceptance command and verify log/report artifacts.
10. Perform a read-only, full-scope compliance audit against every current iCTS spec authority and prepare an evidence-backed findings report.

## Validation commands

```bash
python3 -m unittest .trellis/ecc_dev_tools/tests/test_core.py
/home/liweiguo/llvm/bin/clang-format --dump-config src/operation/iCTS/interface/CTSAPI.cc
python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS --kinds format --fix
python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS
cmake --build build/cleanup-latest-gcc11 -j 16
ctest --test-dir build/cleanup-latest-gcc11 -R '^icts_' --output-on-failure -j 4
cd scripts/design/ics55_dev && ./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl
git diff --check
rg -uu -n 'result/StageResult\.hh|data_manager/result' src/operation/iCTS
git ls-files src/operation/iCTS/source/data_manager/stage/StageSummary.hh
```

The old-path search must return no matches, and `git ls-files` must report the new header after it is staged/committed in the normal finish phase.

## Diff review

- Confirm only the approved config, tool/tests/docs, stage-contract path migration, and `src/operation/iCTS/**/*.cc|*.hh` formatting changed.
- Confirm the root and sibling `.clang-format` files are unchanged.
- Confirm `.gitignore` is unchanged.
- Confirm `.trellis/spec/**` is unchanged.
- Confirm the format-generated C++ diff contains no semantic edits.
- Rerun the full iCTS quality gate after any correction.
- Record spec-audit findings without mixing unapproved fixes into this task.

## Rollback points

- Tool/config changes are independently reversible before running the bulk format.
- The bulk formatting diff can be discarded without reverting the reviewed tool/config changes if it crosses the approved scope.
- Do not alter approved Trellis assets during implementation; return to review first if the scope or configuration must change.
