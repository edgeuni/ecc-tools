# Implementation Plan

## Preconditions

- User approves this plan or explicitly asks to implement.
- Start the task with `python3 ./.trellis/scripts/task.py start .trellis/tasks/06-12-fix-gcc11-all-target-clean-build`.
- Before source edits, run the `trellis-before-dev` workflow and read the relevant backend specs.

## Ordered Changes

1. Confirm active stale links.
   - Run:
     `rg -n "\\bivec_db\\b|\\bivec_api\\b|\\bivec_layout_db\\b|\\bsta-solver\\b|\\bshell-cmd\\b|include\\(cmake/operation/ista\\.cmake\\)|HOME_ISTA" CMakeLists.txt cmake src/platform/tool_manager src/evaluation/src/util src/database/manager/parser/liberty src/interface/tcl src/operation/iCTS -S`

2. Remove vectorization link dependencies.
   - Edit `src/platform/tool_manager/CMakeLists.txt` and remove `ivec_db`.
   - Edit `src/evaluation/src/util/CMakeLists.txt` and remove `ivec_api` and `ivec_layout_db`.
   - Keep `ivec::VecLayout` forward declarations for the first build pass unless they cause compile errors.

3. Remove stale Liberty solver dependency.
   - Edit `src/database/manager/parser/liberty/CMakeLists.txt`.
   - Change `target_link_libraries(liberty str sta-solver log dl)` to remove `sta-solver`.

4. Remove active Tcl `shell-cmd` links.
   - First verify actual usage:
     `find src/interface/tcl -maxdepth 2 \\( -name '*.h' -o -name '*.cpp' -o -name '*.cc' \\) -print | xargs rg -n "shell-cmd/|ShellCmd|CmdReadLiberty|CmdReadSdc|CmdReportTiming|CmdSetPwr|CmdReportPower" -S`
   - Treat this as a link cleanup only. Do not remove current Tcl command registrations from `src/interface/tcl/tcl_register.h`.
   - Edit `src/interface/tcl/CMakeLists.txt` and remove `shell-cmd`.
   - Edit active Tcl module CMake files and remove `shell-cmd` where it is only a link dependency:
     - `tcl_workspace`
     - `tcl_config`
     - `tcl_idb`
     - `tcl_flow`
     - `tcl_icts`
     - `tcl_idrc`
     - `tcl_instance`
     - `tcl_irt`
     - `tcl_izh`
     - `tcl_ircx`
     - `tcl_ifp`
     - `tcl_ipdn`
     - `tcl_report`
     - `tcl_feature`
     - `tcl_eval`
     - `tcl_contest` if preserving conditional `CONTEST` build.
   - Do not change inactive `tcl_ipw` command code in the first pass unless it becomes part of the active build.

5. Remove obsolete iSTA CMake wiring.
   - Edit top-level `CMakeLists.txt` and remove `include(cmake/operation/ista.cmake)`.
   - Edit `src/operation/iCTS/CMakeLists.txt` and remove unused `HOME_ISTA`.
   - If compilation then reports missing generic include paths from the removed `ista.cmake`, add the needed dependency to the nearest target-specific CMake file instead of restoring iSTA global wiring.

6. Re-scan active build references.
   - Run:
     `rg -n "\\bivec_db\\b|\\bivec_api\\b|\\bivec_layout_db\\b|\\bsta-solver\\b|\\bshell-cmd\\b|include\\(cmake/operation/ista\\.cmake\\)|HOME_ISTA" CMakeLists.txt cmake src/platform/tool_manager src/evaluation/src/util src/database/manager/parser/liberty src/interface/tcl src/operation/iCTS -S`
   - Expected remaining matches, if any, must be inactive legacy source paths or documented comments only.

## Validation Commands

Run from repository root:

```bash
rm -rf build
cmake -S . -B build -GNinja \
  -DCMAKE_C_COMPILER=gcc-11 \
  -DCMAKE_CXX_COMPILER=g++-11 \
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$PWD/bin" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMD_BUILD=ON
cmake --build build --target all -j16
cmake --build build --target ecc_bin -j16
```

If `all` fails before final link, fix the first compile error and rerun the same clean build sequence when the CMake dependency shape changes.

If `all` fails at link with new missing libraries, trace them with:

```bash
rg -n "<missing-library-name>" CMakeLists.txt cmake src -S
```

## Review Checklist

- No active CMake path relinks old refactor iSTA/vectorization targets.
- `src/operation/refactor/iSTA` and `src/operation/refactor/vectorization` remain inactive.
- The source/CMake diff is separable from `.trellis` metadata for a later nSTA branch transfer.
- Any inactive legacy reference left behind is explicitly understood as outside the active `all` build.
