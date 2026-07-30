# iCTS iRT-style systematic refactor design

## Status

Correction complete. The approved global DataManager and one-active-CTS-session-per-process contract remain implemented. Owner-stage observability now uses one stateless iRT-style ASCII table renderer, `QorReport` reuses the same canonical table text for `.rpt` and `CTSLOG`, and the designated dev script executes `cts_report` after `run_cts`.

## Final directory architecture

```text
src/operation/iCTS/
├── CMakeLists.txt
├── interface/
│   ├── CMakeLists.txt
│   └── CTSAPI.{hh,cc}
├── source/
│   ├── CMakeLists.txt
│   ├── data_manager/
│   ├── module/
│   └── toolkit/
└── test/
```

This matches iRT at both outer levels:

| iRT | iCTS final responsibility |
|---|---|
| `interface/RTInterface` | `interface/CTSAPI`, external API, root lifecycle and ordered orchestration |
| `source/data_manager` | one `DataManager` owning stable CTS runtime data and real external-system boundaries |
| `source/module/<behavior>` | synthesis/optimization/instantiation/evaluation/output and reusable CTS behaviors |
| `source/toolkit/logger` | iRT-style CTS Logger |
| `source/toolkit/monitor` | iRT-style CTS Monitor |
| `source/toolkit/utility` | stateless generic CTS utilities |
| `test` | tests grouped by interface/data_manager/module/toolkit responsibility |

The directory shape is the result of ownership refactoring, not its substitute. Migration is replacement, not coexistence:

- `api` moves to `interface` and is deleted;
- current Design/Config/external-service code is reorganized by canonical-data ownership under `data_manager`; the old `CTSRuntime` aggregation is deleted rather than wrapped;
- root sequencing moves to `interface`; stage behaviors are rebuilt as business modules under `module`, and the old `Flow` call graph is deleted rather than forwarded;
- `utils` responsibilities move to `toolkit` or the owning module, and `utils` is deleted;
- external dependency declarations move to their owner targets, and `external_libs` is deleted;
- all repository consumers use final include paths; no forwarding header, alias target or duplicate facade remains.

`CTSAPI` retains its public methods and status behavior even though its folder changes. Interface owns global DataManager lifecycle and invokes the CTS business modules in the existing order. Each module builds a local model from canonical global data, executes and validates locally, then commits a selected result; it does not call back into interface.

## Business architecture and global DataManager

### Why the current CTS supports it

Production already has one function-local singleton `CTSAPI`, one runtime/Flow pair, and `init()` resets the current run before loading another. There is no public session/context handle. A global DataManager therefore matches production behavior. The incompatible capability is the test-only construction of two live runtime/Flow pairs in one process; detailed evidence is in `research/global-data-manager-feasibility.md`.

### DataManager is a business owner, not a renamed runtime bag

```text
DataManager
├── run lifecycle/state machine
├── validated CTS Config and work/output paths
├── canonical CTS database
│   ├── Design: clocks/instances/pins/nets/ClockDAG
│   ├── ClockLayout: committed cross-stage physical/topology view
│   └── external-object indexes/bindings
├── shared execution services
│   ├── iDB/SDC/Liberty input-output boundary
│   └── FastSTA contexts with explicit reset rules
└── committed results
    ├── characterization data required across stages
    ├── synthesis outcome/summary
    ├── optimization outcome/timing summary
    ├── instantiation/writeback outcome
    └── evaluation/QoR state
```

The global API follows iRT:

```cpp
#define CTSDM (icts::DataManager::getInst())

class DataManager
{
 public:
  static void initInst();
  static DataManager& getInst();
  static void destroyInst();

  auto input(const DataManagerInput& input) -> DataManagerStatus;
  auto reset() -> void;
  // Canonical queries and stage-specific validated commit operations.
};
```

`input()` directly owns config parsing, output-directory initialization, external clock/library/design ingestion and canonical CTS database construction. The old Setup/ClockDataRead orchestration is decomposed into DataManager builders; DataManager must not call a retained old facade.

DataManager exposes stable domain queries and stage-specific commit operations, not `CTSRuntime&`, a generic object registry or public mutable access to every member. Reset order is explicit and tested.

### Business module lifecycle

```text
interface init
  -> CTSDM.input()
interface runCTS
  -> synthesis:     init local model -> execute/select -> validate -> commit synthesis result
  -> optimization:  init from committed topology -> trial edits -> validate -> commit accepted edits
  -> instantiation: init transactional projection -> validate -> commit Design/iDB writeback
  -> evaluation:    read committed state -> compute -> commit QoR summary
interface report
  -> output/report: read committed state -> write non-log artifacts
interface reset/destroy
  -> CTSDM teardown
```

Global state is limited to stable input, shared services and committed cross-stage results. The following remain module-local:

- H-tree synthesis state, depth/level candidates and pattern libraries;
- solver models, search frontiers and unselected topologies;
- optimization trial edits and temporary timing observations;
- incomplete Design/iDB conversion objects;
- formatting buffers, iteration traces and debug/detail data.

Top-level module facades may use `CTSDM` to initialize/commit business state. Nested algorithms receive narrow domain inputs derived from the local model; they do not use `CTSDM` as an unrestricted service locator.

### Forbidden surface refactors

The following fail design acceptance even if the tree compiles:

- DataManager contains or returns the old `CTSRuntime`;
- a new facade only forwards to old `Flow`, Setup or static stage `run(Input)` paths;
- old and new Input/context types are connected by compatibility translators;
- directory moves preserve the same hidden ownership/mutation graph;
- include forwarders, target aliases or umbrella roots make the new tree appear layered;
- a generic adapter is introduced only to avoid redesigning the owning business boundary.

External iDB/SDC/Liberty conversion remains valid because it is a real system boundary. Compatibility conversion between old and new CTS architectures is forbidden.

## Single logging contract

### Files and API

```text
source/toolkit/logger/
├── CMakeLists.txt
├── LogLevel.hh
├── Logger.hh
└── Logger.cc
```

The public grammar follows iRT:

```cpp
using Loc = std::experimental::source_location;

#define CTSLOG (icts::Logger::getInst())

CTSLOG.info(Loc::current(), ...);
CTSLOG.warn(Loc::current(), ...);
CTSLOG.error(Loc::current(), ...);
```

Logger lifecycle is root-owned:

```text
CTSAPI init
  -> Logger::initInst()
  -> startup messages buffered for file and emitted to console
  -> DataManager resolves/creates work directory
  -> CTSLOG.openLogFileStream(<work_dir>/cts.log)
  -> module execution
CTSAPI destroy/reset
  -> final messages and file path
  -> closeLogFileStream()
  -> Logger::destroyInst()
```

Logical line format:

```text
[CTS YYYYMMDD HH:MM:SS <base62-thread-id> <file:line> <Info|Warn|Error> <function>] <message>
```

The console colors only the level token. `cts.log` contains the same lines without ANSI. Messages are fully assembled before a synchronized complete-line write; pre-open lines drain in original order; the file flushes per line. `Error` is terminal and exits with failure status.

### Exactly one sink

The final runtime produces one log file only:

```text
<work_dir>/cts.log
```

The following are removed, not adapted:

- `cts_detail.log`;
- the previously proposed `cts_runtime.log`;
- `SchemaWriter`, `ReportSink`, `StageScope`, `RuntimeMetricScope`;
- default/detail routing and nested writer state;
- runtime/report dual-write helpers;
- glog and repository `Log.hh`.

Necessary information is emitted through `CTSLOG` only:

| Current report intent | Final iRT-style expression |
|---|---|
| stage scope/start/finish | local Monitor plus `Starting...` / `Completed...` |
| recoverable diagnostic | `CTSLOG.warn` plus existing typed status/diagnostic |
| terminal invariant | `CTSLOG.error` |
| runtime/config/output path | concise `CTSLOG.info` fragments |
| stage input, constraint and business profile | high-density `CTSLOG.info` table/summary |
| bounded candidate/iteration progress | one aggregate row per meaningful candidate depth or accepted optimization iteration |
| selected solution/final QoR | `CTSLOG.info` summary/table |
| artifact path | owner emits one `CTSLOG.info` line after successful creation |
| per-sample/per-trial/per-net debug detail | no production log emission; retain typed local data only if the algorithm requires it |

The pre-refactor primary `cts.log`, rather than `cts_detail.log`, is the semantic observability baseline. The final single log must retain the information needed to explain clock ownership, clustering, characterization coverage, H-tree candidate feasibility and selection, optimization evolution, writeback, evaluation and report status. Module-local state remains local, but its bounded aggregate summaries are observable through `CTSLOG`.

`cts_report` builds each table once from committed evaluation data. The same canonical rendered lines are written to the corresponding `.rpt` file and emitted line-by-line through `CTSLOG.info`, matching iRT's `printSummary -> RTLOG` behavior without restoring SchemaWriter or a second report/log schema.

DEF, Verilog, JSON, CSV, SVG and GDS writers remain artifact-specific code. They are not Logger alternatives and cannot write another CTS runtime log.

## Monitor contract

```text
source/toolkit/monitor/
├── CMakeLists.txt
├── Monitor.hh
└── Monitor.cc
```

Monitor follows iRT's local stack pattern and uses `gettimeofday` plus `getrusage(RUSAGE_SELF)`:

- elapsed wall time;
- process user + system CPU time;
- peak RSS delta in MB;
- exact ` (elapsed = HH:MM:SS, cpu = HH:MM:SS, mem = NN.NNMB) ` layout;
- construction-to-first-call total, then lap after each `getStatsInfo()`;
- getters do not advance the baseline;
- nested monitors are independent.

Every meaningful interface/module operation uses:

```cpp
Monitor monitor;
CTSLOG.info(Loc::current(), "Starting...");
// operation
CTSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
```

No SchemaWriter metric scope or `ieda::Stats` CTS runtime sampler remains.

## Architecture and dependency rules

```text
platform / Tcl / Python
          |
          v
       interface
          |
          +------> global DataManager <------ module facades
                         |                          |
                         v                          v
             canonical data / commits       local business models
                         |                          |
                         +-------------> toolkit <-+
```

- Interface alone owns DataManager/module lifecycle and the established stage order.
- Global DataManager alone owns validated Config, canonical Design/ClockLayout, shared external/timing services and committed run results.
- Each module exposes one business facade, builds one local model and keeps workspace/candidate/search state private.
- Module facades may use `CTSDM` only to initialize their model and commit validated outputs; nested algorithms receive domain-specific inputs.
- Raw external database/tool types remain inside the real DataManager input/output boundary.
- Module output is validated before one explicit stage-specific commit; evaluation/report/artifact export read committed state.
- Logger and DataManager are the only approved global infrastructure/state accessors; module-local lifecycle does not create another persistent state repository.
- Existing `.hh/.cc` conventions remain. The alignment target is iRT's two-level folder model and behavior pattern, not routing-specific names or unsafe singleton mechanics.

## CMake design

Top-level CMake includes exactly:

```cmake
add_subdirectory(${ICTS_INTERFACE})
add_subdirectory(${ICTS_SOURCE})
add_subdirectory(${ICTS_TEST})
```

Source CMake includes exactly:

```cmake
add_subdirectory(${ICTS_DATA_MANAGER})
add_subdirectory(${ICTS_MODULE})
add_subdirectory(${ICTS_TOOLKIT})
```

Each behavior implementation has one owner target. External dependencies are linked by that owner, visibility defaults to PRIVATE, aggregators depend only downward, and no target form may hide a cycle. The final iCTS target closure contains no glog logging symbol.

## Compatibility and intentional changes

Frozen behavior:

- CTSAPI methods, status codes and feature/timing projections;
- Tcl/Python/tool-manager behavior;
- configuration values, algorithms, topology, naming, connectivity, placement and writeback;
- DEF/Verilog and non-log artifact content;
- QoR.

Intentional compatibility changes:

- include path changes from `iCTS/api/...` to `iCTS/interface/...`, with repository consumers updated and no old-path shim;
- old structured/default/detail `cts.log` format is replaced by the iRT-style plain runtime mirror;
- `cts_detail.log` is no longer generated;
- no `cts_runtime.log` is introduced;
- high-value primary-log candidate and iteration summaries are preserved in the single `cts.log`; only the former detail sink and unbounded per-object debug output are removed.
- only one CTS session may be active per process; repeated sequential runs and parallel CTest processes remain supported, while the two-live-runtime test contract is removed.

## Baseline and acceptance comparison

Before product edits, the exact command was run and commit/config/binary identity, exit status, DEF, normalized Verilog, topology, names, connectivity, placement, non-log artifact inventory and QoR were persisted. Existing logs were captured only to classify necessary messages; their schema is intentionally not a compatibility baseline.

Final comparison permits only the approved logging change and nondeterministic timestamp/thread/source-path/runtime-metric fields. It does not permit algorithm, design, artifact or QoR drift.

Required final scans include:

```bash
! test -e src/operation/iCTS/api
! test -e src/operation/iCTS/external_libs
! test -e src/operation/iCTS/source/database
! test -e src/operation/iCTS/source/flow
! test -e src/operation/iCTS/source/utils
! rg -n '#include[[:space:]]*[<"]glog/logging\.h[>"]|#include[[:space:]]*[<"]Log\.hh[>"]|\bLOG_(INFO|WARNING|ERROR|FATAL)' src/operation/iCTS
! rg -n 'SchemaWriter|ReportSink|StageScope|RuntimeMetricScope|cts_detail\.log|cts_runtime\.log' src/operation/iCTS
! rg -n '\bCTSRuntime\b|class[[:space:]]+Flow\b|struct[[:space:]]+Flow\b' src/operation/iCTS
! rg -n -i '\bglog\b|\bieda_log\b' src/operation/iCTS -g 'CMakeLists.txt' -g '*.cmake'
```

Path-specific tests verify that `<work_dir>/cts.log` is the only CTS log file. Architecture tests verify DataManager input/reset/commit, readonly output, one local model per major module, validate-before-commit, failed-stage non-mutation and sequential re-init isolation. Final review also inspected DataManager/module implementations to reject forwarding or compatibility layers that cannot be proven by grep alone.

## Review decision

The business-level transformation remains accepted: DataManager owns input, canonical state and committed results; modules own local models and upload validated results; old Runtime/Flow/logging call graphs are absent; and the top two directory levels reflect those final responsibilities. Observability acceptance is reopened until owner-stage ASCII tables replace the current long text summaries, the designated dev script executes `cts_report`, and the resulting single `cts.log` passes binary, functional/QoR and post-binary ecc dev tools validation.
