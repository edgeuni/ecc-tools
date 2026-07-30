# Research: global iRT-style DataManager feasibility for iCTS

- Query: Can the current iCTS runtime be replaced by an iRT-style global DataManager without changing CTS behavior?
- Scope: internal
- Date: 2026-07-29

## Verdict

Yes. The confirmed compatibility contract is exactly one active CTS session per process. This already matches the production API, but intentionally removes the test-only ability to construct two independent `CTSRuntime`/`Flow` pairs in one process. Sequential repeated runs and process-level parallelism remain supported.

The recommended target is a business-owned global DataManager: `DataManager::initInst/getInst/destroyInst` and `CTSDM` provide the iRT-style global owner; each module builds a local business model from canonical CTS data, executes the stage, validates the selected result and uploads it through a real DataManager commit operation. Low-level algorithms keep stage-local workspaces and typed domain inputs instead of turning every helper into an unrestricted service-locator consumer.

## Evidence that the production model supports it

1. `CTSAPI` is already a function-local singleton and owns exactly one runtime plus one Flow (`src/operation/iCTS/api/CTSAPI.hh:37-91`, `CTSAPI.cc:89-103`). There is no public context/session handle.
2. Every `CTSAPI::init()` first calls `resetAPI()`, and reset clears both runtime and flow state (`CTSAPI.cc:123-150`). Production therefore already assumes one replaceable active run.
3. Tool-manager calls `init -> runCTS` through `CTS_API_INST`; later report calls read the same retained state (`src/platform/tool_manager/tool_api/icts_io/icts_io.cpp:33-75`). Concurrent independent CTS runs are not exposed.
4. Stable state is already centralized in the small `CTSRuntime` aggregate: Config, Design, Wrapper, FastSTA and the soon-to-be-removed SchemaWriter (`src/operation/iCTS/source/flow/Flow.hh:77-93`). These members can move into DataManager without inventing new state.
5. Flow-local committed/cross-stage state is explicit: synthesis, clock layout, evaluation, instantiation, optimization, characterization library and readiness flags (`Flow.hh:129-138`). The stable subset can be absorbed into DataManager; per-operation candidates/workspaces remain module-local.
6. Current source contains no CTS-owned `std::thread`, `std::async`, parallel execution policy or OpenMP launch. A process-global owner does not conflict with a current CTS-owned multi-session execution model.

## Evidence that literal global access has costs

1. Major CTS stage/module boundaries currently use 36 typed Input structs. Whole `CTSRuntime` appears only at the root/Flow boundary; low-level algorithms do not receive it as a service locator.
2. Past refactors deliberately removed Config/Design/Wrapper/FastSTA/SchemaWriter singletons because hidden access made dependencies and test setup implicit. The archived evidence classifies API-owned runtime plus explicit references as the good pattern (`.trellis/tasks/archive/2026-05/05-24-cts-reporter-config-explicit/research/cts-singleton-scan.md:134-148`).
3. Production iRT uses `RTDM` heavily: about 902 macro occurrences across 14 interface/source files. Copying this access density into every iCTS helper would reverse the existing facade/input cleanup and create a broad service locator.
4. The current test harness uses thread-local owned runtime state in 36 files and 441 `CurrentRuntime()` calls (`src/operation/iCTS/test/common/CTSTestRuntime.cc:32-68`). This is migratable, but not free.
5. `FlowRuntimeIsolationTest` explicitly constructs two independent runtime/Flow pairs and verifies their Config, Design, FastSTA, reporter and reset isolation (`src/operation/iCTS/test/flow/FlowRuntimeIsolationTest.cc:41-97`). A true process-global DataManager cannot preserve that test contract without ceasing to be global.
6. CTest registers each iCTS test target as a separate executable (`src/operation/iCTS/test/CMakeLists.txt:44-64`). Parallel CTest processes remain isolated by process; only same-process concurrent/multiple active contexts are lost.

## Recommended target contract

```cpp
#define CTSDM (icts::DataManager::getInst())

class DataManager
{
 public:
  static void initInst();
  static DataManager& getInst();
  static void destroyInst();

  void input(/* config/work-dir */);
  void reset();
  void output();

  Config& getConfig();
  Design& getDesign();
  Wrapper& getWrapper();
  FastSTA& getFastSTA();
  // committed cross-stage state and summaries
};
```

Lifecycle:

```text
CTSAPI init
  -> Logger::initInst if needed
  -> DataManager::initInst
  -> CTSDM.reset/input
  -> open cts.log
CTSAPI run/report/query
  -> interface/module facades use the same CTSDM
next init/reset
  -> clear all run and stage state in a tested order
process/API teardown
  -> CTSDM.output if required
  -> DataManager::destroyInst
  -> Logger::destroyInst
```

Access rule:

- interface and top-level module facades may read/write CTSDM;
- data-manager adapters operate on DataManager-owned state;
- low-level algorithms receive narrow values/references/views from the facade;
- candidates, workspaces, caches and uncommitted results never live globally;
- only validated results are committed into CTSDM;
- no second `CTSRuntime`, Context, Registry or test-only production state path remains.

## Systematic business redesign requirement

The target is not a renamed `CTSRuntime` bag and not a directory adapter. The following transformations are required:

1. **Input/build database**: DataManager owns config parsing, work-directory creation, external clock/library/design ingestion and construction of the canonical CTS database. The old Setup + ClockDataRead orchestration is absorbed, not forwarded through a compatibility facade.
2. **Synthesis**: the synthesis module initializes a local synthesis model from canonical clocks, physical facts and policy; it owns characterization/search/candidates locally; only the validated selected topology is committed to DataManager.
3. **Optimization**: the optimizer reads the committed synthesis result, owns candidate edits/timing trials locally and commits only accepted edits plus the optimization summary.
4. **Instantiation**: the instantiation module converts the committed logical CTS result into one transactional Design/iDB writeback; failed conversion cannot leave a partial global result.
5. **Evaluation**: the evaluator reads committed state, computes QoR and commits a stable evaluation summary; it does not mutate synthesis/optimization workspaces.
6. **Output**: the report/artifact module reads committed DataManager state, writes non-log artifacts and reports only necessary paths/results through `CTSLOG`.

Canonical global state should include Config, Design, ClockLayout, external-service ownership, committed characterization data required across stages, stage outcomes and evaluation/QoR summaries. H-tree candidates, solver models, iteration traces, temporary FastSTA experiments and unselected objects remain module-local.

Forbidden final patterns:

- `DataManager` merely owns the old `CTSRuntime` or returns it wholesale;
- new methods only forward to old `Flow`, `Setup`, `SchemaWriter` or renamed singleton classes;
- compatibility adapters translate old Input structs to new ones while retaining both call graphs;
- files are moved while ownership, mutation authority and lifecycle remain unchanged;
- the new directory graph compiles only through broad include aliases or target forwarding.

External iDB/SDC/Liberty conversion remains legitimate because it is the real system boundary; compatibility conversion between old and new CTS architectures is not.

## Required test changes

- Replace thread-local `CTSTestRuntime` with an RAII scope that initializes, resets and destroys the one process-global DataManager around each test.
- Replace the two-live-runtime isolation test with sequential lifecycle tests: init/run/reset/init, failed-init cleanup, repeated report/query, state non-leakage and idempotent teardown.
- Keep CTest process parallelism; explicitly reject concurrent independent CTS sessions in one process.
- Verify worker callbacks, if added later, do not outlive DataManager teardown.

## Main risk

The migration is technically feasible, but a literal “use CTSDM everywhere” rewrite would make the change much riskier than the directory/logging refactor requires. Bounded facade-level access provides the global iRT DataManager requested while preserving the current algorithm isolation and unit-testability.

## Resolved product decision

The user confirmed one active CTS session per process as the supported runtime contract. The incompatible two-live-runtime isolation test is replaced by sequential lifecycle and non-leakage coverage; CTest process parallelism remains supported.
