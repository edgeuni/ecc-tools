# iCTS ownership matrix and business call map

This is the implementation ownership contract captured before product edits. It translates the current runtime/Flow state into the approved global DataManager and module-local execution model. It is not a file-move map: every row defines the final mutation authority and commit boundary.

## Stable-state ownership matrix

| State / service | Current owner and behavior | Final owner | Readers | Writer / commit authority | Reset order |
|-----------------|----------------------------|-------------|---------|---------------------------|-------------|
| Run lifecycle/readiness | `Flow` booleans `_setup_ready`, `_evaluation_ready`, `_runtime_setup_emitted`; status also split across summaries | DataManager explicit run state; interface owns transitions | Interface and top-level module facades | DataManager input and stage-specific commit methods only | First invalidate readiness; then clear results in reverse stage order |
| Config and derived paths | `CTSRuntime::config`; Setup loads config and derives work/log/visualization/statistics paths | DataManager | All stage-local model builders through narrow const views | `DataManager::input()` only | Last, after every dependent service/result |
| Logger stream/buffer/path | `CTSRuntime::reporter` plus glog; Setup opens SchemaWriter | Logger owns stream/buffer/current path; Config retains only the resolved work/log path | All runtime call sites through `CTSLOG` | Interface initializes/opens/closes/destroys; no module lifecycle access | After all module/DataManager output and workers are quiescent |
| Canonical Design | `CTSRuntime::design`; clock read, synthesis and optimization mutate it directly | DataManager | Module local-model builders, evaluation, output | Input builder creates initial clocks/design; synthesis and optimization use validated commits; instantiation performs final projection | After committed results and external/timing bindings are cleared |
| Clock/Inst/Pin/Net ownership | `Design` owns final objects; Clock/Wrapper/topology links borrow pointers | DataManager-owned Design retains this ownership rule | All CTS domain behaviors through borrowed const/narrow views | DataManager commit operations transfer validated local ownership | Borrowed views invalidated before Design reset |
| ClockLayout | `Flow::_clock_layout`; synthesis builds/merges, optimization edits, instantiation marks completion, evaluation/report read | DataManager committed topology/physical view | Optimization, instantiation, evaluation, output | Synthesis commit creates; optimization commit replaces accepted edits; instantiation commit marks projection state | After evaluation/instantiation/optimization summaries; before Design |
| Wrapper/iDB binding and indexes | `CTSRuntime::wrapper`; Setup binds `dmInst` builder; read/synthesis/optimization/instantiation/evaluation/report query it | DataManager external-system boundary | Input builders and top-level local-model builders; low-level modules receive CTS values/narrow queries | DataManager input owns binding/index construction; instantiation commit owns final iDB writeback | After stage results, before Design/Config |
| SDC clock input | ClockDataRead reads SDC, traces targets through Wrapper, then mutates Design | DataManager input builder | Canonical input construction only | `DataManager::input()` validates all trace targets, then publishes canonical clocks once | With external bindings before Design |
| Liberty facts | Wrapper/FastSTA adapters read raw Liberty; synthesis characterization and optimization query them | DataManager external boundary and stable CTS-domain library facts | Synthesis/optimization local-model builders | DataManager input owns raw binding; synthesis may commit stable characterization facts required later | FastSTA/Liberty state clears before Design/Config |
| FastSTA | `CTSRuntime::fast_sta`; synthesis characterization and optimization bind/mutate contexts | DataManager shared service with explicit context lifetime | Synthesis and optimization top-level facades/local models | DataManager initializes service; each module owns temporary clock contexts and releases them before commit/return | Before external bindings and Design |
| CharacterizationLibrary | `Flow::_char_library`; synthesis builds it and optimization consumes it | DataManager committed cross-stage characterization | Optimization and later read-only consumers if required | Synthesis commit only | With synthesis result, after optimization result |
| SynthesisTraceSummary | `Flow::_run_summary` | DataManager committed synthesis outcome | Interface, later stages, feature/output queries | Synthesis commit only | After later-stage summaries; with synthesis result |
| Selected logical topology | Currently assembled by synthesis directly in Design/ClockLayout while candidates execute | DataManager committed synthesis result; candidates stay local | Optimization, instantiation, evaluation, output | Synthesis validates complete selected topology then commits once | With synthesis result |
| OptimizationSummary / clock timing | `Flow::_optimization_summary`; optimizer also applies accepted edits directly | DataManager committed optimization outcome | Interface feature timing, instantiation, evaluation, output | Optimization validates accepted edit set and commits it once | After evaluation/instantiation, before synthesis state |
| InstantiationSummary | `Flow::_instantiation_summary`; IdbConversion writes directly through Wrapper | DataManager committed writeback outcome | Interface, evaluation, output | Instantiation validates a complete `ClockWritebackPlan`, then DataManager/real boundary applies one transaction | After evaluation/output metadata |
| EvaluationState / QorSummary | `Flow::_evaluation_state`; evaluation mutates it and report may recompute it | DataManager committed evaluation/QoR result | CTSAPI feature queries and output/artifact behavior | Evaluation computes locally from const committed state and commits one stable result; output is readonly | First result group cleared during reset |
| Artifact paths/results | Config plus Report local path resolution; Report may mutate evaluation state | DataManager owns stable paths; output module owns per-call writer state | Interface/report callers | Output writes artifacts from committed state and records only stable success/result metadata | Before Config paths |
| SchemaWriter/default/detail/runtime metrics | `CTSRuntime::reporter`, SchemaScope and nested metric/report state | Removed | None | Necessary runtime lines use Logger; artifact formats keep their real owners | No final state exists |

## State that must remain module-local

| Module | Local model/workspace | Result eligible for commit |
|--------|-----------------------|----------------------------|
| Synthesis | Per-clock layouts, sink-domain contexts, H-tree candidates, characterization sampling work, segment/topology libraries, solver/search/frontier state, unselected buffers/nets | One validated selected logical topology, ClockLayout, stable characterization library, synthesis summary |
| Optimization | Route-tree cache, clock FastSTA context, sizing candidates, cap/slew baselines, batch trials, rejected edits, iteration profiles | Accepted edit set, resulting committed topology/layout, clock timing and optimization summary |
| Instantiation | Logical-to-Design/iDB conversion plan, name/connectivity/placement checks, incomplete external objects | One complete writeback result and instantiation summary |
| Evaluation | Temporary metric accumulators and per-clock calculations | Stable EvaluationState/QoR summary only |
| Output | Formatting buffers and writer-local state | Artifact files and stable output status/path metadata; never another runtime log |

No candidate, trial, cache, incomplete conversion, debug/detail row, or reporter state may be added to DataManager merely because multiple functions currently share it.

## Final business call map

### 1. Interface initialization

```text
CTSAPI::init(config_file, work_dir)
  -> reset prior run if present
  -> Logger::initInst()
  -> DataManager::initInst()
  -> CTSDM.input(DataManagerInput{config_file, work_dir})
       -> validate/load Config
       -> derive/create output paths
       -> bind iDB/SDC/Liberty/FastSTA boundaries
       -> trace and validate SDC clock targets
       -> build canonical Design and indexes
       -> publish input-ready state atomically
  -> CTSLOG.openLogFileStream(CTSDM.getLogPath())
  -> map DataManagerStatus to existing CTSStatus
```

Input failure clears partial builders/bindings and leaves DataManager empty. Setup and ClockDataRead do not remain as facades or forwarding calls.

### 2. Synthesis

```text
interface -> Synthesis::run()
  -> build SynthesisModel from CTSDM canonical facts
  -> execute per-clock distribution / H-tree / topology work locally
  -> select one result and validate topology, names, ownership and ClockDAG
  -> CTSDM.commitSynthesis(SynthesisCommit)
  -> return synthesis status to interface
```

The existing low-level CTS algorithms remain algorithmically unchanged but consume fields owned by `SynthesisModel`. They do not receive an old `SynthesisInput` runtime pointer bundle through a compatibility conversion and do not call `CTSDM` themselves.

### 3. Optimization

```text
interface -> Optimization::run()
  -> build OptimizationModel from committed synthesis topology, characterization and timing facts
  -> build route/FastSTA trial context locally
  -> generate/evaluate candidates without canonical mutation
  -> validate accepted edit set and final timing/legal state
  -> CTSDM.commitOptimization(OptimizationCommit)
  -> release temporary FastSTA contexts
```

Current direct per-trial/accepted mutation must become local/transactional. A failed solver or commit leaves the committed synthesis topology unchanged.

### 4. Instantiation

```text
interface -> Instantiation::run()
  -> build ClockWritebackPlan from committed logical CTS state
  -> validate every inst/net/pin name, connection and placement
  -> perform one DataManager-owned Design/iDB projection transaction
  -> CTSDM.commitInstantiation(InstantiationCommit)
```

IdbConversion remains a legitimate external-system boundary implementation, not a bridge between old and new CTS architectures.

### 5. Evaluation

```text
interface -> Evaluation::run()
  -> read committed Design/ClockLayout/timing state
  -> compute QoR in a local EvaluationModel
  -> CTSDM.commitEvaluation(EvaluationCommit)
```

Evaluation does not mutate synthesis/optimization workspaces or iDB.

### 6. Output and feature queries

```text
CTSAPI::report(save_dir) -> Output::run(save_dir)
  -> read committed Config/Design/ClockLayout/Evaluation/QoR
  -> write statistics and visualization artifacts through their real writers
  -> log only necessary success/failure paths through CTSLOG

CTSAPI::outputSummary/outputClockTiming
  -> read committed DataManager summaries
```

Output may trigger evaluation only through the same evaluation business operation and commit contract; it may not own a second evaluation state.

### 7. Reset and teardown

```text
CTSAPI::resetAPI / process teardown
  -> wait for module work and callbacks to finish
  -> CTSDM.output() when required
  -> CTSDM.reset() in reverse dependency order
  -> DataManager::destroyInst()
  -> log completion/path while stream is valid
  -> close Logger stream
  -> Logger::destroyInst()
```

Repeated reset/destroy is safe. A new sequential run observes no previous Config, Design, path, pointer, stage summary, metric, or log buffer.

## Current call graph that must disappear

```text
CTSAPI
  -> owned CTSRuntime {Config, Design, Wrapper, FastSTA, SchemaWriter}
  -> owned Flow
       -> Setup::initializeRuntime / emitRuntimeSetup
       -> ClockDataRead::read
       -> Synthesis::run(SynthesisInput pointer bundle)
       -> Optimization::run(OptimizationInput pointer bundle)
       -> Instantiation::run(InstantiationInput pointer bundle)
       -> Evaluation::run(EvaluationState, EvaluationInput)
       -> Report::run(ReportInput pointer bundle)
       -> Flow-owned summaries/layout/library/readiness flags
```

Final review must find none of `CTSRuntime`, Flow, Setup forwarding, SchemaWriter, old stage runtime pointer bundles that exist only for plumbing, or compatibility conversions between this graph and the final graph.
