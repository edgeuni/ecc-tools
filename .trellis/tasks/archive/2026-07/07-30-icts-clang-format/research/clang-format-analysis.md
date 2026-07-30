# clang-format configuration and tool-resolution analysis

## Repository configuration inventory

The first-party configuration set has two effective policies:

| Scope | Base style | Column limit | Relationship |
| --- | --- | ---: | --- |
| Repository root | Google | 140 | Project fallback |
| iDRC, iEMIR, iFP, iLVS, iRCX, iRT, iSTA, iZH | Google | 160 | All eight files are byte-identical |
| iCTS | Google | 140 | No local file; inherits the repository root today |

The root and first-party module files are otherwise identical. The iRT history shows an intentional 2025 change from 140 to 160 columns. A byte-identical iCTS copy therefore matches both the established operation-module convention and the requested iRT alignment.

Third-party configurations are ownership-local and are not useful as first-party policy inputs:

- abseil uses the plain Google preset.
- gdstk uses a detailed 100-column custom style.
- HiGHS uses Google with its own pointer/include settings.
- pybind11 uses a 99-column LLVM-derived style.

They should remain untouched.

## Current ecc_dev_tools behavior

`run_format_check()` collects the iCTS profile's `.cc` and `.hh` files, resolves them to absolute paths, and invokes clang-format with each real path while using the repository root as the process working directory.

No style path is currently supplied. In clang-format 22.1.2, `--style=file` is the default. For a file argument, clang-format searches from the file's directory toward its parents and selects the closest `.clang-format`. The working directory matters only for standard-input formatting.

Observed resolution with the installed binary:

| Probe file | Resolved column limit |
| --- | ---: |
| `src/operation/iCTS/interface/CTSAPI.cc` | 140 |
| `src/operation/iRT/test/test_topo_builder/test_topo_builder.cpp` | 160 |

The existing tool therefore already redirects iCTS automatically once `src/operation/iCTS/.clang-format` exists. A CTS-specific profile path is unnecessary and would suppress future nearest-parent overrides.

The minimal tool improvement is to make the existing contract explicit and observable:

1. Pass `--style=file` in check and fix modes.
2. State in the result notes that the closest parent configuration is selected per source file.
3. Replace the inaccurate phrase `repository .clang-format` with `applicable .clang-format`.
4. Cover the command contract with focused tests.

Primary references:

- [LLVM clang-format documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [LLVM style option documentation](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)

## Configuration representation decision

Two valid local-file representations were considered:

1. Full byte-identical copy of the iRT configuration.
2. `BasedOnStyle: InheritParentConfig` plus `ColumnLimit: 160`.

The full configuration is recommended because it:

- matches every existing first-party operation-module local file;
- gives iCTS an explicit, stable contract rather than inheriting future root changes implicitly;
- avoids adding a newer inheritance feature to the project's tool-version compatibility surface;
- allows direct byte comparison with iRT during review.

The cost is duplicated configuration text. That cost already exists uniformly across sibling operation modules and is smaller than introducing a one-off representation only for iCTS.

## Reformat impact

The iCTS profile currently collects 445 `.cc`/`.hh` files. A dry-run using the iRT configuration reports formatting replacements in 269 files. This is expected because changing 140 to 160 columns allows clang-format to recombine many previously wrapped declarations, calls, expressions, and tables.

The implementation must treat this as a mechanical formatting migration:

- generate the source changes only through clang-format;
- keep tool/config/tests/docs changes separately reviewable from the bulk source diff;
- inspect the final file set and reject any file outside the approved scope;
- rebuild and rerun iCTS tests after formatting.

## Repository-integrity finding and reviewed resolution

`src/operation/iCTS/source/data_manager/result/StageResult.hh` is the only ignored, untracked C++ file under iCTS. The root `.gitignore` rule `result` hides the directory. The file is directly referenced by `source/data_manager/CMakeLists.txt`, `DataManager.hh`, and multiple stage headers.

It is already formatted under the recommended style and does not contribute to the 269 changed files, but a push cannot include it while it remains ignored.

The reviewed direction rejects an ignore exception and replaces the generic path with `source/data_manager/stage/StageSummary.hh`. This name follows the actual cross-stage summary responsibility, avoids collision with the generic repository `result` ignore policy, and does not overlap the existing optimization-local `source/module/optimization/state/` responsibility. CMake and include paths change; the declared types remain intact.
