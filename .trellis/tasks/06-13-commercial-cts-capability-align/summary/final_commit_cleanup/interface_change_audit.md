# CTS Alignment Commit Cleanup Audit

## Staged Asset Scope

Before cleanup the staged task assets included raw rerun and L3 trace material:

- staged files: 430
- staged bytes: 118,644,706

After cleanup the commit scope keeps only compact task PRDs and summary evidence:

- staged files: 116
- staged bytes: 1,131,915
- removed from staged scope: `research/`, `scripts/`, raw `row_matches` CSVs, `run_manifest.json`, and intermediate candidate run folders
- spec changes: current staged change restores `.trellis/spec/backend/database-guidelines.md` to the remote version, neutralizing an earlier local committed spec edit; the final diff against `origin/cts_refactor` contains no `.trellis/spec/` changes

## Interface And Naming Audit

| Area | Interface change | Current naming / style | Issue found | Action |
| --- | --- | --- | --- | --- |
| FastSTA parasitic view | `FastStaRcNode`, `FastStaRcEdge`, `FastStaNetParasitic` now carry ground cap, coupling cap, timing coupling factor, driver cap, and driver PI fields. | Unit suffixes use `_pf`, `_ohm`, `_ns`; field names describe electrical quantity. | Needed by phase 1 timing L3 alignment; this is a data model extension, not a tuning knob. | Keep. |
| Wrapper RC query | Added `Wrapper::WireCapacitanceProfile`, `queryRequiredWireCapacitanceProfile`, and `queryRequiredClockTimingWireCapacitanceProfile`. | Renamed from `*Breakdown` to `*CapacitanceProfile`. | `Breakdown` was generic and read like report/debug vocabulary. | Renamed. |
| Optimization target skew | `ResolveClockTargetSkewNs` now takes only `const Config&`. | Function name remains scoped to skew target derivation. | Previous `Clock*` parameter was unused after removing period-derived skew behavior. | Removed unused parameter and simplified tests. |
| H-tree global selection | `GlobalSelectionDetail` renamed to `GlobalSelectionDecision`. | Names now describe the selected policy and reason. | `Detail` was vague for a struct that records selection decision state. | Renamed. |
| Delay-margin selection helper | Removed `SelectionPolicy.hh`, `SelectDelayBoundedIndex`, and `SelectionPolicyTest.cc` from build. | No remaining production include. | It exposed old margin-driven selection semantics that are no longer used by adaptive timing/physical selection. | Deleted. |
| Config surface | Removed `htree_depth_explore_window` getter, setter, parser handling, runtime report row, and default-config key. Kept old removed keys in deprecated-key warnings. | Existing config accessors keep `get_`, `set_`, `is_` style. | Hardcoded/user-facing tuning knobs should not remain as commit API. | Keep removal; deprecated warnings preserve migration behavior. |
| Wirelength grid | Added `ResolveCharacterizationGridPlan(config, direct_lengths, coverage_lengths)`. | Parameter names distinguish exact direct lengths from coverage lengths. | Needed to avoid DBU grid distortion while preserving library coverage. | Keep. |
| H-tree topology generator | Added `TopologyGenConfig::branching_factor` and `TopologyGen::resolveBranchingFactor`; depth/leaf helpers now receive branching factor explicitly. | CamelCase method names match existing class style; config member is lower snake case. | Algorithmic branching behavior is explicit instead of hidden in local constants. | Keep. |
| Sink load region split | Added `SinkLoadRegionSplitNode`, `SinkLoadRegionSplitPlan`, and `SplitSinkLoadRegionGroup`. | Names describe local split remediation rather than a tuning parameter. | Required to model legal local fanout repair when sink load groups exceed max fanout. | Keep. |
| Topology pruning refs | `CandidateCharRef` carries split metrics used by physical selection. | `Ref` is acceptable because it stores a pointer to the char entry plus metadata. | Could be renamed later to `GlobalCandidateRef` if this interface becomes broader. | Keep for now; no functional cleanup needed. |
| Wrapper RC tests | Test fixture renamed from `WrapperRcHarnessInterface` to `WrapperRcTestInterface`. | Matches checker requirement for GTest fixtures while staying readable. | `HarnessInterface` was awkward and added no meaning. | Renamed. |
| FastSTA report adapter | Deleted `FastStaReport.cc/.hh` and removed CMake source entries. | No public replacement interface. | Report class was diagnostic-only and unused after native parasitic trace/report flow moved elsewhere. | Delete kept. |
| Task assets | Kept final compact summaries only. | Task paths remain under `.trellis/tasks/.../summary`. | Raw rerun data and experimental scripts are too large/noisy for commit. | Left untracked/unstaged locally. |

## Residual Notes

- Remaining staged code changes are algorithm/data-model changes: native ground/coupling parasitics, adaptive wirelength characterization grid, H-tree topology branching, sink-load split remediation, and adaptive timing/physical selection.
- No new user-facing magic tuning parameter is retained in the config surface.
- The old removed config names remain listed as deprecated keys only so stale JSON files get a clear warning instead of silently failing in unclear ways.
