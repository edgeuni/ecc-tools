# Optimize iCTS clang-format integration

## Goal

Give `src/operation/iCTS/` an explicit module-local formatting contract aligned with iRT, make `ecc_dev_tools` apply that contract deterministically without hard-coding a CTS-only file path, and mechanically reformat the iCTS C++ scope without semantic changes.

## Background

- The repository root `.clang-format` uses the Google base style with `ColumnLimit: 140`.
- The local configurations under iDRC, iEMIR, iFP, iLVS, iRCX, iRT, iSTA, and iZH are byte-identical and differ from the root only by `ColumnLimit: 160`.
- iCTS currently has no local configuration, so clang-format resolves the root configuration and uses 140 columns.
- `ecc_dev_tools` passes each absolute source-file path to clang-format. Clang-format's default `--style=file` lookup already selects the closest parent `.clang-format` for each file.
- Applying the sibling-module/iRT configuration would reformat 269 of the 445 `.cc`/`.hh` files currently collected by the iCTS profile.
- `src/operation/iCTS/source/data_manager/result/StageResult.hh` is required by CMake and iCTS headers but is currently untracked because the repository `result` ignore rule hides it. Its contents are cross-stage summaries/state, so the reviewed resolution is a semantic path rename rather than an ignore exception.

## Requirements

1. Add `src/operation/iCTS/.clang-format` using the reviewed iRT-aligned configuration.
2. Keep formatting ownership local: the iCTS configuration applies only below `src/operation/iCTS/`; root, sibling-module, interface, and third-party configurations remain unchanged.
3. Preserve clang-format's nearest-parent configuration discovery. Do not add a profile field or hard-coded `file:<path>` CTS override.
4. Make `ecc_dev_tools` invoke `--style=file` explicitly for both check and fix modes, report that nearest-parent discovery is in use, and use configuration-neutral finding text.
5. Add focused `ecc_dev_tools` tests for the format command contract and update its README to document local configuration discovery.
6. Reformat the complete iCTS `.cc`/`.hh` scope through the tool after the configuration and tool contract are in place.
7. Keep the bulk source reformat mechanical and separate from the explicitly approved stage-contract path rename. Do not combine other semantic edits, renames, include-policy changes, or unrelated cleanup with it.
8. Rename `source/data_manager/result/StageResult.hh` to `source/data_manager/stage/StageSummary.hh`, update all CMake/include references, and remove the old ignored directory without changing the declared types or CTS behavior.
9. Build the updated code and run the exact iCTS binary acceptance script after formatting and the path rename.
10. After binary acceptance, audit the complete iCTS codebase against every current `.trellis/spec` authority, including file/header metadata, license/copyright, naming, directory, dependency, logger/monitor, error-handling, and quality contracts.
11. Report all spec-audit findings with file/line evidence; do not silently fix findings outside the approved implementation scope.
12. Do not modify `.trellis/spec/**` or any other module's `.clang-format` without a separate reviewed scope change.

## Acceptance Criteria

- [ ] `clang-format --dump-config` for an iCTS source resolves the local configuration and reports `ColumnLimit: 160`.
- [ ] The new iCTS configuration is behaviorally identical to the current iRT/sibling-module configuration.
- [ ] `ecc_dev_tools` check and fix invocations explicitly use `--style=file` and do not hard-code the iCTS configuration path.
- [ ] Focused `ecc_dev_tools` unit tests pass.
- [ ] Running the iCTS format fix changes only the approved local configuration, tool/tests/docs, the stage-contract path rename, and mechanical formatting in iCTS C++ files.
- [ ] A subsequent format check reports zero in-scope findings.
- [ ] The full iCTS `ecc_dev_tools` check reports zero in-scope findings.
- [ ] The iCTS build and 19 iCTS CTest cases pass after the mechanical reformat.
- [ ] `source/data_manager/result/` no longer exists; `stage/StageSummary.hh` is visible to Git, all references use the new path, and the declared stage types are unchanged.
- [ ] `cd scripts/design/ics55_dev && ./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl` exits successfully and emits the expected CTS log/report artifacts.
- [ ] A full spec audit is completed after binary acceptance, and every finding or clean category is reported with evidence.
- [ ] `.trellis/spec/**`, root formatting policy, sibling-module formatting policy, and third-party code are unchanged.

## Out of Scope

- Changing the repository root or sibling-module `.clang-format` files.
- Reformatting Tcl/Python CTS interfaces or any code outside `src/operation/iCTS/`.
- Reformatting third-party code.
- Changing clang-tidy, header, CMake, IWYU, or compiler-warning policy.
- CTS behavior, architecture, algorithm, logger, monitor, or report changes.
- Automatically fixing spec-audit findings or unrelated repository-wide findings that are outside the reviewed implementation scope.

## Review Gate

Implementation starts only after the user approves the configuration, tool behavior, expected reformat scope, `stage/StageSummary.hh` path migration, binary acceptance, and full spec-audit boundary described in the planning artifacts.
