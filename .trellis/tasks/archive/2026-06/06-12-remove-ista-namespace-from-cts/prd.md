# PRD: remove iSTA namespace from CTS

## Background
After merging nSTA, liberty parser is in `namespace idb`, but CTS code references `ista::LibCell`, `ista::LibPort`, etc. CTS needs to use `idb::` types instead.
`ista_io` (StaIO bridge) was deleted by nSTA merge — needs to be restored with `idb::` type references.

## Requirements
1. Replace all `ista::` liberty type references with `idb::` in CTS (~9 files)
2. Change forward declares from `namespace ista` to `namespace idb`
3. Restore `ista_io` (StaIO) from cts_refactor and fix its `ista::` → `idb::` references
4. Fix CMakeLists paths if needed
5. Do NOT commit

## Acceptance Criteria
- [ ] Zero `ista::` references remain in CTS source files
- [ ] Zero `ista::` references remain in ista_io
- [ ] All forward declares use `namespace idb`
- [ ] CMakeLists paths are consistent
