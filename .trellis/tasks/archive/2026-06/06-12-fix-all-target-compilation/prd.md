# PRD: fix all target compilation with GCC 11

## Background
After cherry-picking nSTA into cts_refactor and removing ista namespace from CTS, the `all` build target is broken because:
1. CMakeLists reference deleted library targets (tool_api_ista, ista-engine, sta)
2. Source files include headers from moved/deleted iSTA engine
3. `ista::` namespace remnants in evaluation module (liberty is now `idb::`)

## Requirements
1. Fix CMakeLists: remove references to deleted targets (tool_api_ista, ista-engine, sta)
2. Disable py_ista / py_imp modules that depend on deleted iSTA engine
3. Fix `ista::` → `idb::` namespace in evaluation module
4. Verify nSTA module compiles cleanly
5. `all` target compiles successfully with GCC 11

## Acceptance Criteria
- [ ] `cmake --build build --target all` passes (or at least configuration passes)
- [ ] No linker errors referencing deleted iSTA targets
- [ ] No `ista::` namespace compile errors outside refactor/
- [ ] nSTA module compiles without error
