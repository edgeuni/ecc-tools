# Fix GCC11 all target clean build

## Goal

Make the current `cts_refactor` source tree pass a clean GCC 11 `all` target build without relying on stale libraries from an old local `build/lib`.

The fix must align the nSTA integration direction: active builds should not re-enable old `src/operation/refactor/iSTA` or `src/operation/refactor/vectorization` targets just to satisfy linker inputs.

## Requirements

- Keep a single Trellis task for the build-fix closeout.
- Do not rewrite commit history or push.
- Fix current-branch source/CMake only after the task is started.
- Preserve the ability to separate code changes from Trellis task assets for a later nSTA cherry-pick.
- Use a clean GCC 11 build as the source of truth, not the pre-existing local `build/` directory.
- Remove stale active-build link references to missing old-library targets:
  - `ivec_db`
  - `ivec_api`
  - `ivec_layout_db`
  - `sta-solver`
  - `shell-cmd`
- Do not add active `add_subdirectory()` calls for `src/operation/refactor/iSTA` or `src/operation/refactor/vectorization`.
- Remove or neutralize obsolete iSTA CMake wiring that is still active, including the top-level `cmake/operation/ista.cmake` include if it can be replaced without reintroducing old iSTA dependencies.
- Keep changes narrowly scoped to build wiring and directly related stale iSTA/vectorization references needed for the clean build.

## Acceptance Criteria

- [x] Active CMake files no longer link `ivec_db`, `ivec_api`, `ivec_layout_db`, `sta-solver`, or `shell-cmd` unless the target is actually produced by the active build.
- [x] Current branch configures from a clean `build/` with GCC 11.4.0.
- [x] `cmake --build build --target all -j16` succeeds after `rm -rf build`.
- [x] `cmake --build build --target ecc_bin -j16` succeeds after the clean build.
- [x] `rg` confirms no active build path still depends on the missing old libraries.
- [x] The final source diff is code/CMake-only separable from `.trellis` task metadata for later nSTA transfer.
- [x] If disabled legacy directories still contain old iSTA/vectorization references, they are documented as out of the active build path instead of silently treated as fixed.

## Confirmed Facts

- The previous local `build/` contained stale May-era artifacts for all five missing libraries. Those artifacts could mask source-level link defects.
- After `rm -rf build`, clean GCC 11 configure succeeds.
- Clean `all` and `ecc_bin` links fail with:
  - `cannot find -livec_db`
  - `cannot find -lshell-cmd`
  - `cannot find -livec_api`
  - `cannot find -livec_layout_db`
  - `cannot find -lsta-solver`
- The old target definitions live under inactive refactor paths:
  - `src/operation/refactor/vectorization/database`
  - `src/operation/refactor/vectorization/api`
  - `src/operation/refactor/vectorization/src/layout/database`
  - `src/operation/refactor/iSTA/source/solver`
  - `src/operation/refactor/iSTA/source/module/shell-cmd`
- Active source paths still introduce stale link inputs:
  - `src/platform/tool_manager/CMakeLists.txt`
  - `src/evaluation/src/util/CMakeLists.txt`
  - `src/database/manager/parser/liberty/CMakeLists.txt`
  - `src/interface/tcl/CMakeLists.txt`
  - active `src/interface/tcl/tcl_*` module CMake files.

## Notes

- The preferred fix is dependency cleanup, not restoration of old modules.
- `ivec::VecLayout` currently appears in evaluation public APIs via forward declarations. This does not by itself require linking `ivec_*` libraries; any compile failure after removing the links should be handled at the narrow call site.
- `src/interface/tcl/tcl_ipw` has real `shell-cmd/PowerShellCmd.hh` includes but is not currently added by the active top-level Tcl CMake file. Treat it separately from the active `all` target unless the build graph changes.
- Global Tcl command registration in `src/interface/tcl/tcl_register.h` does not register the old iSTA `shell-cmd` commands. The `shell-cmd` cleanup in this task means removing stale link dependencies from active Tcl targets, not deleting current Tcl command implementations.

## Verification Result

- Initial clean verification reproduced the failure after removing `build/`: final links could not find `ivec_db`, `shell-cmd`, `ivec_api`, `ivec_layout_db`, and `sta-solver`.
- Final clean verification removed `build/` again and configured successfully with GCC 11.4.0:
  `cmake -S . -B build -GNinja -DCMAKE_C_COMPILER=gcc-11 -DCMAKE_CXX_COMPILER=g++-11 -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$PWD/bin" -DCMAKE_BUILD_TYPE=Release -DCMD_BUILD=ON`
- Final clean `cmake --build build --target all -j16` completed with exit code 0.
- Final `cmake --build build --target ecc_bin -j16` completed with exit code 0 after the clean build.
- Incremental `cmake --build build --target all -j16` and `cmake --build build --target ecc_bin -j16` also completed with exit code 0 after the last iCTS CMake dependency fix.
- Active-path `rg` scan found no remaining `ivec_db`, `ivec_api`, `ivec_layout_db`, `sta-solver`, `shell-cmd`, top-level `include(cmake/operation/ista.cmake)`, or `HOME_ISTA` references.
- Full Tcl scan still finds `shell-cmd` only in disabled legacy directories not added by active `src/interface/tcl/CMakeLists.txt`: `tcl_eco`, `tcl_ino`, `tcl_ipl`, `tcl_ipnp`, `tcl_ipw`, and `tcl_ito`. `tcl_ipw` also still has a real `shell-cmd/PowerShellCmd.hh` include and remains outside this build fix.
- `git diff --check` completed with exit code 0.
- `python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS` completed with exit code 0. It reported 0 in-scope findings and 4183 out-of-scope findings from external headers.
