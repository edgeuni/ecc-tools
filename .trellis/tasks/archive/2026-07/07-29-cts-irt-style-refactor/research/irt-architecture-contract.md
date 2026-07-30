# iRT architecture contract for implementation context

This injection-sized contract combines repository evidence with the user's latest review decisions. The full research remains authoritative for evidence; `prd.md` and `design.md` are authoritative for the chosen iCTS target.

## Production iRT pattern

```text
external callers
  -> interface/RTInterface
  -> source/data_manager
  -> source/module/<behavior>
  -> source/toolkit/{logger,monitor,utility}
```

Evidence anchors:

- external lifecycle and ordered orchestration: `src/operation/iRT/interface/RTInterface.hpp:69-86`, `RTInterface.cpp:63-180`
- central Config/Database owner: `src/operation/iRT/source/data_manager/DataManager.hpp:29-91`
- stage-local model and selected-result upload: `src/operation/iRT/source/module/planar_router/PlanarRouter.cpp:95-221`, `:1866-1871`
- one behavior facade: `src/operation/iRT/source/module/planar_router/PlanarRouter.hpp:28-61`
- tier layout and aggregation: `src/operation/iRT/source/CMakeLists.txt:1-18`

## User-selected iCTS mapping

Final two-level skeleton must match iRT:

```text
iCTS/{interface,source,test}
iCTS/source/{data_manager,module,toolkit}
```

| Current iCTS | Final owner |
|---|---|
| `api` | `interface` |
| `CTSRuntime` + `database` stable state | one global `DataManager` with interface-owned lifecycle |
| root `Flow` ordering | `interface` |
| `flow/<stage>` + current reusable behaviors | `module/<behavior>` |
| `utils` stateless infrastructure | `toolkit` |
| report/visualization behavior | owning module/artifact writer |
| `external_libs` | real target owner CMake |

Old roots, old types that duplicate final ownership, forwarding headers and alias targets do not remain.

## Logging decision

- Logger/Monitor live under `source/toolkit` and follow production iRT call/lifecycle/metric contracts.
- `cts.log` is the only CTS log and is the plain mirror of console output.
- glog, `Log.hh`, SchemaWriter, default/detail sinks, `cts_detail.log`, `cts_runtime.log` and parallel runtime metric scopes are removed.
- DEF/Verilog/JSON/CSV/SVG/GDS remain artifact-specific outputs, not alternate runtime loggers.

## Frozen architectural rules

1. Interface alone owns external API, global DataManager/module lifecycle and `input -> synthesis -> optimization -> instantiation -> evaluation -> output` ordering.
2. Global DataManager is the only stable CTS runtime-state owner. It directly builds canonical input data and owns committed cross-stage results; it does not wrap `CTSRuntime` or forward old Flow/Setup calls.
3. Each behavior exposes one facade, builds a real local business model, owns temporary state locally, validates the complete result and commits once through a stage-specific DataManager operation.
4. Raw external types stay in data-manager adapters; evaluation/report/artifact export reads committed state.
5. Logger and global DataManager are the approved internal singleton accessors. Top-level module facades may use `CTSDM`; nested algorithms do not use it as an unrestricted service locator.
6. Each behavior has one implementation owner target; dependencies default PRIVATE; the graph is acyclic.
7. Tests use final paths and production facades; no old-path or old-log compatibility tests remain.
8. Keep CTS algorithms, config, topology, naming, writeback, artifacts and QoR unchanged.
9. Keep `.hh/.cc` and CTS-domain inner names; exact alignment is required for the top two directory levels, not routing-specific names.
10. Final code contains no old-object wrapper, old-facade forwarder, old/new type translator, compatibility adapter, migration narration, temporary shim, commented debug implementation or unrelated cleanup.

## iRT mechanics not copied

- `exit(0)` for terminal Error;
- unsynchronized logger/timestamp/file state;
- raw ownership;
- per-module singleton state and unrestricted low-level `CTSDM` access;
- source-to-interface callbacks;
- cyclic or child-to-aggregator CMake links;
- routing-specific abbreviations, `.hpp/.cpp` mass churn or giant facade files.
