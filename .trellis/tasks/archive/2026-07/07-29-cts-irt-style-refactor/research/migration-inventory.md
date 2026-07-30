# iCTS pre-refactor migration inventory

Snapshot date: 2026-07-29, before product edits, HEAD `31da2c9cfe86e63791171418fa1ff202400a1f5e`.

## Source and build surface

| Measure | Count |
|---------|------:|
| Total files under `src/operation/iCTS` | 626 |
| `.cc` files | 250 |
| `.hh` files | 220 |
| `CMakeLists.txt` files | 135 |
| Direct glog include files | 107 |
| Direct `Log.hh` include files | 107 |
| `LOG_*` calls | 827 |
| Files mentioning `SchemaWriter` or `reporter` | 113 |
| CMake files mentioning log/glog/ieda_log tokens | 65 |
| `CurrentRuntime()` calls / files | 441 / 36 |
| CTS-owned thread/async/OpenMP launch sites | 0 |

`LOG_*` classification before semantic migration:

| Form | Count | Migration rule |
|------|------:|----------------|
| `LOG_INFO` | 70 | Keep only necessary lifecycle/result/path messages as `CTSLOG.info`; remove iteration/detail noise |
| `LOG_WARNING` | 134 | `CTSLOG.warn` when the control flow continues safely |
| `LOG_WARNING_IF` | 1 | Replace with explicit condition plus `CTSLOG.warn` |
| `LOG_ERROR` | 123 | Inspect control flow; recoverable paths become Warn plus typed status, unsafe invariants become terminal Error |
| `LOG_ERROR_IF` | 1 | Same classification; no conditional logging API remains |
| `LOG_FATAL` | 24 | Terminal `CTSLOG.error` unless repository evidence proves an existing typed recoverable path |
| `LOG_FATAL_IF` | 474 | Explicit guard plus terminal `CTSLOG.error`; no macro-only translation |

The highest direct-glog concentrations are synthesis/H-tree/topology, routing, report/visualization, FastSTA, clustering, characterization, optimization, SDC, iDB/Wrapper, and the root flow stages. Logging conversion must follow the owning business slice so severity and result ownership are reviewed together.

## External API consumers

| Consumer | Current use | Required final change |
|----------|-------------|-----------------------|
| `src/platform/tool_manager/tool_api/icts_io/icts_io.cpp` | Includes `iCTS/api/CTSAPI.hh`; calls init/run/report | Include `iCTS/interface/CTSAPI.hh`; preserve statuses and sequencing |
| `src/interface/python/py_icts/py_icts.cpp` | Includes old API path | Use final interface path; preserve binding behavior |
| `src/interface/tcl/tcl_icts` | Links `icts_api`; Tcl commands reach tool manager/API | Keep command behavior; link final interface target |
| `src/feature/builder/feature_builder_tool.cpp` | Calls `CTS_API_INST.outputSummary()` | Preserve fields, read committed DataManager QoR |
| iCTS tests | Direct API and Flow/runtime fixtures | Use production interface/DataManager contracts and global-lifecycle fixture |

The target name `icts_api` may remain as the stable external link contract, but it must be owned by `interface/` and cannot be an alias to an old target. Old include paths are updated atomically and no forwarding header remains.

## Current target and external-dependency ownership

| Current dependency/target | Final owner |
|---------------------------|-------------|
| `icts_api` | `interface/CTSAPI` concrete target |
| `icts_source` | Removed as an old broad source umbrella or rebuilt only as a downward interface aggregate with no implementation ownership/cycle |
| `icts_source_flow` | Removed; root orchestration moves to interface and stage implementations to module |
| `icts_source_database` | Replaced by DataManager-owned targets under `source/data_manager` |
| `icts_source_utils` | Replaced by toolkit leaf targets and owning module targets |
| `icts_api_external_libs` | Removed; each real dependency moves to its owner target |
| `idm` | DataManager iDB input/output boundary |
| repository `log` / glog | Removed from every iCTS-owned target and final link closure |
| repository `usage` | Removed from CTS runtime metrics; Monitor owns POSIX sampling directly |
| `feature_db` | Interface target because CTSAPI publishes `ieda_feature::CTSSummary` |
| SDC parser dependencies | DataManager SDC input boundary |
| Liberty parser/data dependencies | DataManager Liberty/FastSTA boundary; no iSTA engine ownership |
| geometry/solver/routing libraries | The module or toolkit implementation that directly uses them |
| GoogleTest | Test targets only |

Every existing internal implementation target migrates with its business source directory before the old aggregator is removed. Dependency visibility defaults to PRIVATE; PUBLIC is retained only when a final public header exposes the dependency.

## Old runtime/report graph to eliminate

- `CTSAPI` owns `std::unique_ptr<CTSRuntime>` and `std::unique_ptr<Flow>`.
- `CTSRuntime` aggregates Config, Design, Wrapper, FastSTA, and SchemaWriter.
- Flow owns synthesis/evaluation/instantiation/optimization summaries, ClockLayout, CharacterizationLibrary, and readiness flags.
- Setup opens structured logs and initializes Config/Wrapper.
- ClockDataRead mutates canonical clocks before synthesis.
- Stage facades receive broad pointer bundles, including the reporter.
- Report may recompute and mutate Flow's EvaluationState.
- Tests provide a separate thread-local runtime path and one test constructs two live runtimes.

Final scans must show no old runtime/Flow/Setup/reporting path or old/new compatibility translator.

## Baseline and validation assets

- Exact pre-edit command passed with exit `0`.
- Authoritative actual output path and artifact/QoR hashes are recorded in `baseline/README.md`.
- Compressed baseline DEF and Verilog are retained for semantic/exact comparison.
- Old `cts.log` and `cts_detail.log` are retained only to classify necessary messages; detail/default schema compatibility is intentionally removed.
