# Data Manager Guidelines

Session ownership, state transitions, data ownership, and external adapter boundaries for iCTS.

## Global Session

`DataManager` is the single internal CTS session owner, accessed through `CTSDM` at orchestration boundaries.

- `CTS_API_INST` is the external API singleton boundary.
- `CTSDM` is allowed in `CTSAPI` and the `Synthesis`, `Optimization`, `Instantiation`, `Evaluation`, and `Output` facade implementations.
- Code below those facades receives the exact references, inputs, or local models it needs; it must not use `CTSDM` as a general service locator.
- Do not introduce another runtime/session manager, global context, reset registry, or compatibility wrapper around `DataManager`.

`DataManager` owns the active `Config`, `Design`, `Wrapper`, `FastSTA`, `ClockLayout`, and committed stage results. There is one active CTS session per process.

## Lifecycle and State

- `CTSAPI::init` resets any previous API, logger, and data-manager session before initializing a new one.
- `DataManager::initInst()` is idempotent; `DataManager::getInst()` before initialization is terminal.
- `DataManager::input()` resets session data, resolves output paths, loads config and external clock data, and reaches `CTSRunState::kInputReady` only on success.
- Reset dependent stage results before the adapters, design, and config they reference.
- Borrowed pointers and local stage models must not survive reset or committed-design replacement.

The committed state order is:

```text
kEmpty -> kInputReady -> kSynthesisCommitted
       -> kOptimizationCommitted (when optimization runs)
       -> kInstantiationCommitted -> kEvaluationCommitted
```

Input failure sets `kFailed`; invalid transitions return `DataManagerStatusCode::kInvalidState` without advancing committed state.

## Stage Work and Commits

- Synthesis and optimization operate on a cloned `Design` and local `ClockLayout`, then call `commitSynthesis(...)` or `commitOptimization(...)` only after validation succeeds.
- A failed synthesis or optimization result must destruct locally without replacing committed design state.
- Instantiation materializes through `Wrapper::writeClocksDetailed(...)`; failure must restore prior iDB clock-tree objects before `commitInstantiation(...)` is allowed.
- Evaluation is read-only over committed CTS/iDB state and publishes its derived `EvaluationState` through `commitEvaluation(...)`.
- Output reads committed state and does not create a second report/runtime state store.

Use module-qualified input, config, output, and summary types when a stable stage boundary needs them. Inputs contain execution dependencies; outputs contain committable data; summaries contain bounded diagnostics and metrics. Do not create broad snapshots that duplicate queryable `DataManager` state.

## Ownership

- Use `std::unique_ptr` for ownership and raw pointers only for borrowed topology or adapter cross-references.
- `Design` owns committed `Clock`, `Inst`, `Pin`, and `Net` objects.
- `Clock`, `Inst`, `Net`, `Pin`, and `Wrapper` cross-references are non-owning unless their type explicitly states otherwise.
- Algorithm-local results may own temporary CTS objects until a successful commit.
- `Tree` owns its `TreeNode` objects.
- Name-based `Design` lookups use maintained indexes; fix stale index ownership instead of adding full-vector scan fallbacks.

## External Boundaries

| Boundary | May access | Publishes |
| --- | --- | --- |
| `source/data_manager/io/Wrapper*` | General `idb::*` data and CTS writeback | CTS objects, narrow values, or validated iDB changes |
| `source/data_manager/adapter/sdc/` | SDC parser state and setup-time target facts | CTS clock input values and diagnostics |
| `source/data_manager/adapter/fast_sta/` | Raw Liberty data and CTS-local timing state | CTS timing values and summaries |
| `source/module/` | CTS data-manager types and narrow wrapper queries | Local results or validated stage commits |

Raw iDB, SDC-parser, or Liberty-parser pointers must not become fields of module contracts, design objects, report models, or algorithm outputs. Production iCTS must not depend on iSTA/iPA engines or reintroduce `STAAdapter`-style engine ownership.
