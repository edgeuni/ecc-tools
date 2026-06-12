# Design

## Problem

The current `cts_refactor` branch compiles much of the tree with GCC 11, but clean final linking fails because active targets still refer to libraries that are no longer produced by the active build graph.

The missing libraries are old iSTA/vectorization artifacts:

- `ivec_db`
- `ivec_api`
- `ivec_layout_db`
- `sta-solver`
- `shell-cmd`

The previous local `build/lib` contained stale copies of those libraries, so builds in that workspace could appear successful without proving that the current source tree is self-contained.

## Boundary Decision

Do not restore old refactor targets into the active build:

- Do not add `src/operation/refactor/iSTA`.
- Do not add `src/operation/refactor/vectorization`.
- Do not use stale local static archives as external inputs.

The correct boundary is to remove stale links from active targets and keep old refactor code inactive unless a separate product decision reintroduces it.

## Fix Strategy

### 1. Remove stale vectorization links

`src/platform/tool_manager/CMakeLists.txt` links `ivec_db`, but active `tool_manager` code does not use `ivec` symbols. Remove the link.

`src/evaluation/src/util/CMakeLists.txt` links `ivec_api` and `ivec_layout_db`. Active evaluation code only forward-declares `ivec::VecLayout*` in public APIs and has the old vectorized STA implementation commented out. Remove those links.

Keep the function signatures initially to minimize downstream API churn. If later compilation shows a hard dependency, fix that call site explicitly instead of relinking old vectorization libraries.

### 2. Remove stale iSTA solver link from Liberty

`src/database/manager/parser/liberty/CMakeLists.txt` links `sta-solver`.

The active Liberty parser is under `idb` and should not depend on old iSTA solver targets. `Interpolation.cc` is compiled directly into the `liberty` target, so remove `sta-solver` from `target_link_libraries`.

### 3. Remove stale shell command links from active Tcl targets

The active Tcl aggregate and active Tcl submodules still link `shell-cmd`.

This is a link-graph cleanup, not a command removal. Current global Tcl registration comes from `src/interface/tcl/tcl_register.h` and registers the active module command groups (`config`, `workspace`, `flow`, `db`, `CTS`, `DRC`, `RCX`, reports, evaluation, and others). It does not include the old iSTA shell command registry.

The old `shell-cmd` target belongs to `src/operation/refactor/iSTA/source/module/shell-cmd` and contains legacy iSTA-style commands such as Liberty/SDC/SPEF read and timing report commands. That target is not produced by the active build graph.

Remove `shell-cmd` from:

- `src/interface/tcl/CMakeLists.txt`
- active subdirectories listed by that top-level CMake file
- conditionally active `tcl_contest` if it is kept buildable under `CONTEST`

Before removing a `shell-cmd` link from a Tcl target, confirm that the target's active source files do not include `shell-cmd/...` headers and do not reference symbols from that old target. Do not delete Tcl command source files as part of this step.

Do not treat inactive Tcl directories such as `tcl_ipw` as part of the clean build failure unless they are added to the active build graph. `tcl_ipw` still includes `shell-cmd/PowerShellCmd.hh` and would need a separate migration if re-enabled.

### 4. Neutralize obsolete global iSTA CMake wiring

The top-level `CMakeLists.txt` still includes `cmake/operation/ista.cmake`, which injects old iSTA include directories and `link_directories(${CMAKE_BINARY_DIR}/lib)`.

Preferred cleanup:

- Remove `include(cmake/operation/ista.cmake)`.
- Remove unused `HOME_ISTA` from `src/operation/iCTS/CMakeLists.txt`.
- If removing `ista.cmake` exposes missing generic dependencies such as Eigen include paths, add those dependencies to the actual target that needs them, or move generic setup to a neutral non-iSTA CMake file.

Do not keep a file named `ista.cmake` as the generic place for active non-iSTA dependencies.

## Compatibility

The intended source compatibility is narrow:

- Keep public evaluation APIs stable in the first build fix unless they directly block compilation.
- Keep nSTA integration intact.
- Avoid broad renames of `eval_util_init_ista` and `init_sta.*` in this task unless required to pass the clean build.

Semantic cleanup can continue after the clean build is restored, but it should not be mixed with broad API churn unless required by the linker fix.

## Risks

- Removing `cmake/operation/ista.cmake` may reveal accidental dependency on its global include directories.
- Removing `shell-cmd` from active Tcl CMake files may reveal a hidden include or symbol use in a currently active Tcl module.
- Evaluation still exposes `ivec::VecLayout*` as a forward-declared type. That is acceptable for compilation but remains a semantic cleanup item.

## Rollback

If a link removal exposes real symbol usage, do not restore the old library wholesale. Instead:

1. Identify the exact source file and symbol.
2. Decide whether the source should use nSTA/current infrastructure or be disabled from active build.
3. Add only the nearest current target dependency needed by that source.
