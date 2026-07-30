# Design: iCTS local clang-format integration

## Formatting ownership

Add a complete module-local configuration at `src/operation/iCTS/.clang-format`. It is byte-identical to `src/operation/iRT/.clang-format` and the seven other first-party operation-module configurations.

The selected explicit options are:

| Option | Value |
| --- | --- |
| `Language` | `Cpp` |
| `BasedOnStyle` | `Google` |
| `ColumnLimit` | `160` |
| `AlignConsecutiveAssignments` | `false` |
| `AlignConsecutiveDeclarations` | `false` |
| `AllowAllParametersOfDeclarationOnNextLine` | `false` |
| `AllowShortFunctionsOnASingleLine` | `InlineOnly` |
| `AllowShortIfStatementsOnASingleLine` | `false` |
| `AllowShortLoopsOnASingleLine` | `false` |
| `BinPackArguments` | `true` |
| `BinPackParameters` | `true` |
| `BreakBeforeBinaryOperators` | `All` |
| `BreakBeforeBraces` | `Custom` |
| `DerivePointerAlignment` | `false` |
| `SpaceAfterCStyleCast` | `true` |
| `Standard` | `c++20` |

`BraceWrapping` uses the sibling-module values:

- wrap after class, enum, function, struct, and union definitions;
- do not wrap after control statements, namespaces, Objective-C declarations, or extern blocks;
- do not place catch/else on a new brace line;
- do not indent braces;
- split empty functions, records, and namespaces.

No additional formatting preference is introduced in this task.

## Tool resolution

The format data flow remains file-oriented:

```text
requested iCTS scope
  -> collect absolute .cc/.hh paths
  -> clang-format --style=file <absolute-file>
  -> closest parent .clang-format
  -> src/operation/iCTS/.clang-format
```

The profile does not gain a `clang_format_config` field, and the command does not use `--style=file:<fixed-path>`. This preserves standard clang-format hierarchy semantics and avoids applying a CTS style accidentally if a caller ever supplies a different in-repository path.

The tool change is limited to:

- explicit `--style=file` in check and fix commands;
- one result note describing per-file nearest-parent lookup;
- configuration-neutral finding text;
- focused tests and README documentation.

## Stage-contract path migration

Do not add a `.gitignore` exception. Rename the ignored, generic path to the business-responsibility path:

```text
source/data_manager/result/StageResult.hh
  -> source/data_manager/stage/StageSummary.hh
```

`stage` is preferred over `state` because `source/module/optimization/state/` already owns optimization-local mutable state, while this data-manager file defines cross-stage public summaries and one evaluation state committed by `DataManager`. `StageSummary.hh` matches the existing `{Stage}Summary` contracts and the backend naming spec more closely than `StageResult.hh`.

The move updates the owning data-manager CMake list and every include from `result/StageResult.hh` to `stage/StageSummary.hh`. Type names and definitions remain unchanged; this is a path/ownership correction, not an API or behavior redesign. The old ignored directory is removed and no `.gitignore` file changes.

## Migration boundary

The mechanical reformat target is exactly the `.cc` and `.hh` set collected under `src/operation/iCTS/`. Tcl/Python interfaces, sibling operation modules, root sources, and third-party trees retain their owning configurations and are not reformatted.

The source diff is expected to touch approximately 269 files. This volume is accepted only as clang-format output; any semantic or cross-scope change fails review.

## Validation and audit sequence

1. Unit-test the changed `ecc_dev_tools` format command behavior.
2. Probe an iCTS file with `clang-format --dump-config` and verify 160 columns.
3. Run format fix for the iCTS scope.
4. Run format check and inspect the complete changed-file set.
5. Run the full iCTS `ecc_dev_tools` gate.
6. Build the iCTS targets and run all 19 iCTS CTest cases.
7. Run the exact `scripts/design/ics55_dev` iEDA/iCTS binary acceptance and verify the CTS log/report artifacts.
8. Audit all iCTS `.cc`, `.hh`, CMake files, and directory boundaries against the complete backend spec set. The audit reports findings; it does not expand implementation scope by fixing unrelated violations.
9. Confirm `result/` is absent, `stage/StageSummary.hh` is tracked, all references use the new path, and no declared type changed.
10. Confirm no spec, root/sibling format configuration, `.gitignore`, or non-iCTS source changed.

The spec audit covers at least:

- permitted extensions, PascalCase file naming, `#pragma once`, license/copyright block, and required file metadata;
- interface/source/test and data_manager/module/toolkit directory boundaries;
- DataManager ownership and cross-stage commit contracts;
- `CTSLOG`, `Monitor`, table/report mirroring, and absence of alternate logger semantics;
- typed recoverable errors and terminal-error handling;
- include self-containment/order, namespace rules, forbidden traversal, CMake target naming/visibility, and the full quality gate.
