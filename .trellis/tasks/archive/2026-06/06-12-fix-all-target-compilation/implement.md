# Implement: fix all target compilation

## Step 1: CMakeLists fixes
- [ ] `src/platform/data_manager/CMakeLists.txt:28`: remove `tool_api_ista`
- [ ] `src/evaluation/src/util/CMakeLists.txt:61`: remove `ista-engine`
- [ ] `src/interface/python/CMakeLists.txt:18`: comment `add_subdirectory(py_ista)`
- [ ] `src/interface/python/CMakeLists.txt:45`: comment `py_ista`
- [ ] `src/interface/python/py_imp/CMakeLists.txt:18`: remove `sta`

## Step 2: Disable py_imp files depending on iSTA engine
- [ ] Comment out `invoke_ista/` sources and idb_to_imp_db files with iSTA deps

## Step 3: Fix evaluation namespace
- [ ] `evaluation/api/timing_api.hh`: `ista::AnalysisMode` → `idb::AnalysisMode`
- [ ] `evaluation/api/timing_api.cc`: `ista::AnalysisMode` → `idb::AnalysisMode`
- [ ] `evaluation/src/module/timing/timing_eval.hh`: `ista::AnalysisMode` → `idb::AnalysisMode`
- [ ] `evaluation/src/module/timing/timing_eval.cc`: `ista::AnalysisMode` → `idb::AnalysisMode`

## Step 4: Verify
- [ ] cmake configure passes
