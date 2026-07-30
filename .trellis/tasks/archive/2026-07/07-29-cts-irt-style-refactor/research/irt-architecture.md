# Research: iRT architecture and design rules for an iCTS refactor

> Review update (2026-07-29): repository evidence in this document remains valid, but its original conservative iCTS mapping has been superseded by the user's explicit decision to align the top two directory levels with iRT and remove old directory compatibility. The selected target is authoritative in `../prd.md`, `../design.md`, and `irt-architecture-contract.md`.

- Query: Analyze the production iRT module's directory hierarchy, layering, facades, contracts, naming, ownership/dependency access, CMake organization, tests, and orchestration/data-flow boundaries; separate stable principles from routing-specific or legacy mechanics; propose rules suitable for later iRT-to-iCTS mapping.
- Scope: internal
- Date: 2026-07-29

## Findings

### Executive conclusion

iRT does not have the literal `api/database/flow/utils` structure used by iCTS. Its actual architecture is:

```text
Tcl/Python/other callers
          |
          v
interface/RTInterface
  lifecycle + top-level routing orchestration + external-tool projection
          |
          v
source/data_manager
  Config + Database + shared indexes/results/summaries
          |
          v
source/module/<routing-stage>
  one root facade + stage-local <prefix>_data_manager types
          |
          v
source/toolkit/{logger,monitor,utility}
```

The reusable architectural pattern is **one visible facade per behavior, a centrally ordered lifecycle, a stage-local model/workspace, and an explicit upload/summary boundary**. The literal implementation mechanism is not uniformly reusable: production iRT also contains module-level singleton macros, manual `new`/`delete`, source-to-interface callbacks, large translation units, broad public include roots, and circular CMake relationships. Those mechanics must not be copied into iCTS.

For iCTS, “follow iRT” should therefore mean preserving iRT's semantic boundaries while using the already-established iCTS contracts: `CTSAPI` is the sole external singleton boundary, `Flow` owns CTS lifecycle order, runtime dependencies are explicitly owned and passed through narrow stage inputs, external types stay behind adapters, and each behavior directory exposes only its facade. These constraints are already codified in `.trellis/spec/backend/directory-structure.md:13-35`, `.trellis/spec/backend/database-guidelines.md:12-33`, and `.trellis/spec/backend/database-guidelines.md:125-129`.

### Files found

| File | Role / relevance |
|---|---|
| `src/operation/iRT/README.md` | Declares the intended API, Data Manager, Module, Solver, and Utility concepts (`README.md:9-43`). |
| `src/operation/iRT/CMakeLists.txt` | Defines iRT's C++20 setting, tier paths, and interface/source/test roots (`CMakeLists.txt:6`, `CMakeLists.txt:43-53`). |
| `src/operation/iRT/interface/RTInterface.hpp` | The external facade and also the public declarations for iDB/iDRC/iSTA/notification projection (`RTInterface.hpp:69-170`). |
| `src/operation/iRT/interface/RTInterface.cpp` | Owns iRT initialization, routing-stage order, teardown, data wrapping, and writeback (`RTInterface.cpp:63-180`, `RTInterface.cpp:354-387`, `RTInterface.cpp:1345-1470`). |
| `src/operation/iRT/source/data_manager/DataManager.hpp` | Central owner of `Config` and `Database`, plus shared update/query facade (`DataManager.hpp:29-84`). |
| `src/operation/iRT/source/data_manager/DataManager.cpp` | Implements input/build/output lifecycle and central shared-map mutation (`DataManager.cpp:28-74`, `DataManager.cpp:693-782`). |
| `src/operation/iRT/source/data_manager/advance/Config.hpp` | Single runtime configuration value object, including derived per-stage output paths (`Config.hpp:23-67`). |
| `src/operation/iRT/source/data_manager/advance/Database.hpp` | Passive, centrally owned database value object (`Database.hpp:35-105`). |
| `src/operation/iRT/source/data_manager/advance/Summary.hpp` | Stage-qualified summary data retained by the database (`Summary.hpp:23-152`). |
| `src/operation/iRT/source/data_manager/advance/Net.hpp` | Example stable iRT domain object that owns pins by value (`Net.hpp:28-53`). |
| `src/operation/iRT/source/module/planar_router/PlanarRouter.hpp` | Representative behavior facade: one public action and private orchestration/algorithm helpers (`PlanarRouter.hpp:28-61`). |
| `src/operation/iRT/source/module/planar_router/PlanarRouter.cpp` | Representative stage pipeline, local model construction, shared-result upload, summary, and artifacts (`PlanarRouter.cpp:95-145`, `PlanarRouter.cpp:151-221`, `PlanarRouter.cpp:1866-1871`, `PlanarRouter.cpp:1963-2201`). |
| `src/operation/iRT/source/module/planar_router/pr_data_manager/PRModel.hpp` | Representative stage-local workspace with owned value collections and borrowed task pointers (`PRModel.hpp:60-105`). |
| `src/operation/iRT/source/module/planar_router/pr_data_manager/PRNet.hpp` | Representative stage projection that borrows its origin `Net` while owning stage-local pin values (`PRNet.hpp:24-53`). |
| `src/operation/iRT/source/module/topo_builder/TOPOBuilder.hpp` | Reusable solver/builder facade with a narrow task/result contract (`TOPOBuilder.hpp:27-65`). |
| `src/operation/iRT/source/module/topo_builder/tb_data_manager/TBTask.hpp` | Example module-qualified task contract (`TBTask.hpp:25-52`). |
| `src/operation/iRT/source/toolkit/utility/Utility.hpp` | Broad iRT utility facade; useful as evidence of shared primitives but also of over-broad coupling (`Utility.hpp:34-78`). |
| `src/operation/iRT/source/CMakeLists.txt` | Aggregates Data Manager, Module, and Toolkit into `irt_source` (`source/CMakeLists.txt:1-18`). |
| `src/operation/iRT/source/module/CMakeLists.txt` | Creates one subdirectory/target per routing behavior and an aggregate target (`module/CMakeLists.txt:1-29`). |
| `src/operation/iRT/source/module/planar_router/CMakeLists.txt` | Representative module target, source ownership, dependencies, and include exposure (`planar_router/CMakeLists.txt:9-24`). |
| `src/operation/iRT/source/toolkit/CMakeLists.txt` | Aggregates logger, monitor, and utility; also exposes a build-cycle caveat (`toolkit/CMakeLists.txt:1-17`). |
| `src/operation/iRT/test/CMakeLists.txt` | Shows that only the topology-builder test is enabled in the production test tree (`test/CMakeLists.txt:6-14`). |
| `src/operation/iRT/test/test_topo_builder/test_topo_builder.cpp` | Facade-level deterministic test and explicit lifecycle setup/teardown (`test_topo_builder.cpp:106-123`, `test_topo_builder.cpp:384-412`). |
| `src/operation/iCTS/api/CTSAPI.hh` | Current iCTS external facade and sole singleton boundary (`CTSAPI.hh:37-92`). |
| `src/operation/iCTS/source/flow/Flow.hh` | Current iCTS explicit runtime owner and flow facade (`Flow.hh:77-139`). |
| `src/operation/iCTS/source/flow/Flow.cc` | Current CTS lifecycle and narrow input-contract calls (`Flow.cc:94-152`, `Flow.cc:155-283`). |
| `.trellis/spec/backend/directory-structure.md` | Authoritative iCTS layer, behavior-directory, flow, and target rules (`directory-structure.md:13-118`). |
| `.trellis/spec/backend/database-guidelines.md` | Authoritative iCTS ownership, stage-data, adapter, and singleton rules (`database-guidelines.md:12-82`, `database-guidelines.md:125-129`). |
| `.trellis/spec/backend/quality-guidelines.md` | Authoritative iCTS naming, include, and dependency visibility rules (`quality-guidelines.md:13-29`, `quality-guidelines.md:65-95`). |

### 1. Actual iRT layer model

#### 1.1 External interface tier

- External Tcl and Python bindings include only `RTInterface.hpp` and call the `RTI` facade. For example, Tcl converts options into a configuration map and calls `RTI.initRT(...)` (`src/interface/tcl/tcl_irt/src/tcl_init_rt.cpp:45-52`), calls `RTI.runRT()` (`src/interface/tcl/tcl_irt/src/tcl_run_rt.cpp:31-37`), and calls `RTI.destroyRT()` (`src/interface/tcl/tcl_irt/src/tcl_destroy_rt.cpp:27-33`). Python follows the same facade (`src/interface/python/py_irt/py_irt.cpp:29-66`).
- `RTInterface` publicly exposes the stable lifecycle verbs `initRT`, `runERT`, `runRT`, and `destroyRT` (`src/operation/iRT/interface/RTInterface.hpp:77-86`). This is the strongest public-facade principle to carry to iCTS.
- The same header also publicly exposes dozens of wrapping, output, conversion, DRC, timing, and notification methods (`RTInterface.hpp:90-158`). Those methods are implementation adapters, not evidence that every method should be part of iCTS's public contract.
- iRT's `interface/` target compiles a single facade implementation and publishes only the interface include root (`interface/CMakeLists.txt:9-24`). External Tcl links `irt_interface` privately (`src/interface/tcl/tcl_irt/CMakeLists.txt:10-20`).

#### 1.2 Source tier: Data Manager, Module, Toolkit

- `source/CMakeLists.txt` has three direct children: `data_manager`, `module`, and `toolkit`, then aggregates them into `irt_source` (`source/CMakeLists.txt:1-18`). There is no standalone iRT `flow/` directory.
- `data_manager/basic/` contains reusable coordinates, rectangles, grids, queues, trees, segments, directions, and orientations. `data_manager/advance/` contains routing-domain objects such as `Config`, `Database`, `Net`, `Pin`, layers, obstacles, violations, and summaries. The `irt_data_manager` target publishes both roots (`source/data_manager/CMakeLists.txt:9-22`).
- `module/` is partitioned by behavior: pin access, supply analysis, planar routing, layer assignment, space routing, track assignment, detailed routing, DRC, topology building, violation reporting, plotting, and early routing. The module aggregator enumerates each child explicitly (`source/module/CMakeLists.txt:1-29`).
- `toolkit/` contains logger, monitor, and general utility targets (`source/toolkit/CMakeLists.txt:1-12`). Architecturally these are cross-cutting facilities; the exact logger/monitor call contract is covered by the separate logger/monitor research.

#### 1.3 Test tier

- iRT has a separate `test/` root, but its production CMake enables only `test_topo_builder`; the other test/process directories are commented out (`test/CMakeLists.txt:6-14`).
- The enabled test links the topology facade target and toolkit, not a private implementation translation unit (`test/test_topo_builder/CMakeLists.txt:1-10`). It creates a `TBTask`, invokes only `RTTB.getPlanarTopoList(...)`, and checks returned values (`test_topo_builder.cpp:106-123`). This facade-level testing direction is reusable.
- The test manually initializes/destroys Logger and TOPOBuilder (`test_topo_builder.cpp:384-412`). This reflects iRT's singleton lifecycle, not the preferred iCTS test fixture model.

### 2. Where iRT flow orchestration really lives

iRT splits top-level lifecycle across `RTInterface` and `DataManager`, rather than a distinct flow tier:

1. `RTInterface::initRT` initializes Logger, creates DataManager, asks DataManager to ingest input, and initializes persistent helper facades (`RTInterface.cpp:63-88`).
2. `DataManager::input` calls back into `RTInterface::input`, then normalizes/builds configuration and database state and emits initial artifacts (`DataManager.cpp:53-64`).
3. `RTInterface::input` wraps configuration and external database facts (`RTInterface.cpp:354-387`).
4. `RTInterface::runRT` deterministically invokes PinAccessor, SupplyAnalyzer, PlanarRouter, LayerAssigner, SpaceRouter, TrackAssigner, DetailedRouter, and ViolationReporter, creating and destroying each stage singleton around one action (`RTInterface.cpp:109-154`).
5. `RTInterface::destroyRT` asks DataManager to project results outward, then destroys DataManager and Logger (`RTInterface.cpp:157-180`).
6. `DataManager::output` calls `RTInterface::output` and releases its GCell map (`DataManager.cpp:67-74`). `RTInterface::output` writes track grids, GCell grids, net results, and summaries (`RTInterface.cpp:1345-1351`); net writeback converts iRT segments and patches into iDB wire objects (`RTInterface.cpp:1423-1470`).

Logical data flow:

```text
config map + iDB
      |
      v
RTInterface wrap/convert boundary
      |
      v
DataManager-owned Config + Database
      |
      v
stage-local Model / Net / Pin / Node / Task values
      |
      v
DataManager update/query boundary + Database Summary
      |
      v
RTInterface writeback / feature projection / notification
```

This flow contains a useful semantic separation—external projection, normalized shared data, stage-local work, explicit upload—but its code dependency direction is cyclic because DataManager and modules call back into the external interface. iCTS should preserve the semantic stages while making the dependency graph one-way.

### 3. Module facade and local-model pattern

#### 3.1 Single visible behavior facade

- A typical iRT module directory has `CMakeLists.txt`, one `{Behavior}.hpp/.cpp` pair, and one prefixed local-data directory. For example, `planar_router/` exposes `PlanarRouter.hpp/.cpp`, while `pr_data_manager/` holds `PRModel`, `PRNet`, `PRPin`, `PRNode`, parameters, candidates, and iteration data.
- `PlanarRouter` exposes singleton lifecycle plus one business verb, `generate()`; initialization, conversion, scheduling, graph construction, routing, upload, summary, report, and debug helpers are private (`PlanarRouter.hpp:28-61`).
- `TOPOBuilder` similarly exposes lifecycle plus narrow topology functions accepting a `TBTask` and returning segments/statistics, with legalization helpers private (`TOPOBuilder.hpp:27-65`).
- This “root facade plus internal responsibility types” pattern is stable and matches the iCTS behavior-directory contract in `.trellis/spec/backend/directory-structure.md:69-85`.

#### 3.2 Repeated stage lifecycle

Across production modules, the root action follows the same high-level shape:

```text
start monitor/log
  -> construct stage-local model
  -> bind stage parameters
  -> build tasks/schedules/graph/workspace
  -> run algorithm/iterations
  -> upload selected result
  -> update summary / emit optional artifacts
  -> complete monitor/log
```

Evidence:

- Pin access constructs `PAModel`, initializes candidate access points, routes, and uploads selected access points/results/patches (`PinAccessor.cpp:60-73`).
- Planar routing constructs `PRModel`, creates parameters/tasks/graph supply, runs generation, then updates summary and optional artifacts (`PlanarRouter.cpp:95-145`).
- Layer assignment uses the same model/parameter/task/graph/process/summary/artifact order (`LayerAssigner.cpp:53-73`).
- Track assignment constructs `TAModel`, builds panels/schedule, assigns, then summarizes and emits artifacts (`TrackAssigner.cpp:55-72`).
- Detailed routing constructs `DRModel`, runs an explicit iteration pipeline, uploads result/patch/violations, and retains the best result (`DetailedRouter.cpp:59-65`, `DetailedRouter.cpp:103-145`).
- Violation reporting constructs `VRModel`, refreshes violation state, updates summary, and emits report artifacts (`ViolationReporter.cpp:53-65`).

The stage order is reusable. The routing-specific model contents, graph structures, GCell/box/panel scheduling, cost units, and routing-stage names are not.

#### 3.3 Stage-local projection and commit boundary

- `PlanarRouter::initPRModel` creates a local value object and converts the shared database's `Net` list into stage-qualified `PRNet` values (`PlanarRouter.cpp:151-184`).
- Each `PRNet` owns its stage-local `PRPin` list but borrows an `origin_net` pointer (`PRNet.hpp:24-53`).
- `PRModel` owns the `PRNet` list and graph/workspace values; its task list and current task are non-owning pointers into that owned list (`PRModel.hpp:60-105`). `initPRTaskList` explicitly populates and sorts those pointer views (`PlanarRouter.cpp:212-221`).
- The selected result is uploaded through the central DataManager boundary (`PlanarRouter.cpp:1866-1871`), after which stage summary is written into the shared database summary (`PlanarRouter.cpp:1963-2033`).

This is the most important data-design principle to transfer: **stage-local temporary state must not become the canonical database implicitly; only the owning stage boundary commits selected results**.

### 4. Ownership and dependency access

#### 4.1 What iRT actually does

- `DataManager` is a manually managed singleton and owns `Config` and `Database` by value (`DataManager.hpp:29-36`, `DataManager.hpp:77-91`).
- `Database` in turn owns most stable collections and summary values directly, including layer lists, obstacles, macros, nets, GCell map, and summary (`Database.hpp:77-105`). `Net` owns `Pin` objects by value (`Net.hpp:28-53`).
- Stage models are normally automatic local values, so their workspace lifetime is naturally scoped to one stage call (`PlanarRouter.cpp:95-100`, `PinAccessor.cpp:60-65`, `DetailedRouter.cpp:59-64`).
- Stage projections often use raw pointers as borrowed views into value-owned collections (`PRModel.hpp:92-104`, `PRNet.hpp:47-52`).
- Committed routing results are frequently allocated with `new` and transferred implicitly to DataManager's shared maps; DataManager deletes them on removal (`PlanarRouter.cpp:1866-1871`, `DataManager.cpp:145-181`). This ownership transfer is convention-based rather than type-enforced.
- There is no conventional dependency injection in iRT. `RTI`, `RTDM`, `RTPR`, `RTTB`, `RTDE`, `RTGP`, and other macros are global singleton access paths; for example, `PlanarRouter` declares `RTPR` (`PlanarRouter.hpp:28-35`) and directly reads `RTDM` during local-model creation (`PlanarRouter.cpp:151-160`).

#### 4.2 What the iCTS mapping should do

iCTS should transfer the ownership **roles**, not iRT's raw mechanisms:

- Keep the one external singleton boundary already represented by `CTS_API_INST` (`src/operation/iCTS/api/CTSAPI.hh:37-62`). Do not create `CTS_DM_INST`, per-stage singletons, or source-layer service-locator macros.
- Keep runtime ownership explicit. Current `CTSAPI` owns `CTSRuntime` and `Flow` with `std::unique_ptr` (`CTSAPI.hh:81-91`), while `CTSRuntime` owns Config, Design, Wrapper, FastSTA, and SchemaWriter by value (`Flow.hh:77-93`). This is the safe equivalent of iRT's centrally owned Config/Database.
- Pass exact dependencies through constructor binding or `{Stage}Input` structures. Current Flow already does this for clock-data read, synthesis, optimization, instantiation, evaluation, and report (`Flow.cc:169-174`, `Flow.cc:192-200`, `Flow.cc:218-224`, `Flow.cc:244-248`, `Flow.cc:262-283`).
- Use `std::unique_ptr` for ownership and raw pointers only for borrowed cross-references, as required by `.trellis/spec/backend/database-guidelines.md:35-47`.
- A stage-local result may own temporary CTS objects, but it must commit into `Design` only on success; a failed stage must destruct without mutating final design membership (`database-guidelines.md:39-47`).

The proposed dependency-injection rule is therefore an iCTS-safe translation of iRT's central ownership boundary, not a claim that production iRT itself uses DI.

### 5. Public versus internal contracts

#### Stable public contract

- External callers should see one module-level API facade. iRT demonstrates this through `RTInterface`; current iCTS already demonstrates it through `CTSAPI`, whose public methods are run/report/init/reset/status/feature queries while runtime and flow access stay private (`CTSAPI.hh:55-92`).
- External platform code currently depends on `CTS_API_INST.init`, `runCTS`, and `report` (`src/platform/tool_manager/tool_api/icts_io/icts_io.cpp:33-75`), and Python consumes a typed timing projection (`src/interface/python/py_icts/py_icts.cpp:36-61`). These signatures and observable results are compatibility boundaries for the refactor.

#### Stable internal behavior contract

- A flow stage or algorithm behavior should have one root header with a small, domain-named action surface. Callers outside the directory include that facade, not its workspace/helper headers.
- When the boundary carries more than trivial arguments, use module-qualified `{Name}Input`, `{Name}Config`, `{Name}Output`, and `{Name}Summary` types, consistent with `.trellis/spec/backend/database-guidelines.md:27-33` and `.trellis/spec/backend/quality-guidelines.md:46-49`.
- Stable shared CTS objects belong in `source/database/`; stage-only models, tasks, candidate state, or search workspaces stay inside their owning behavior directory.

#### iRT contracts that must remain internal in iCTS

- External database/tool conversions: iRT makes wrap/output/timing/DRC methods public on `RTInterface` (`RTInterface.hpp:90-158`) and modules call `RTI.updateTiming` or `RTI.sendNotification` directly, for example `PlanarRouter.cpp:2031` and `PlanarRouter.cpp:2200`. iCTS should instead keep iDB, SDC, Liberty, and FastSTA details behind `Wrapper` or dedicated adapters (`database-guidelines.md:62-82`).
- Report/visualization writers: iRT stage implementations calculate summary, print tables, emit CSV/JSON, send notifications, and call GDSPlotter from the algorithm translation unit (`PlanarRouter.cpp:1963-2201`). iCTS report/evaluation/visualization must remain readonly consumers after commit (`database-guidelines.md:79-81`).
- Internal orchestration helpers: iRT declares many private helpers in the facade header and places thousands of lines in one `.cpp` (for example `PinAccessor.cpp` ends at line 4413 and `DetailedRouter.cpp` at line 4029). iCTS should keep one visible facade but split implementation slices under semantic responsibility subfolders, per `directory-structure.md:73-82`.

### 6. CMake and target organization

#### Reusable organization

- iRT creates one real library per behavior (`irt_planar_router`, `irt_detailed_router`, `irt_topo_builder`, and so on), and creates tier aggregators (`irt_module`, `irt_toolkit`, `irt_source`) (`source/module/CMakeLists.txt:1-29`, `source/toolkit/CMakeLists.txt:1-12`, `source/CMakeLists.txt:7-18`).
- Each real library owns its implementation source, and third-party implementation dependencies can be private; for example, `irt_topo_builder` privately links FLUTE (`source/module/topo_builder/CMakeLists.txt:9-20`).
- The external facade target privately consumes the source aggregate and publishes only its external include root (`interface/CMakeLists.txt:9-24`).
- Tests link a production facade target rather than recompiling its source (`test/test_topo_builder/CMakeLists.txt:1-10`).

#### CMake mechanics that must not be copied

The observed production target graph is cyclic:

- `irt_interface -> irt_source` (`interface/CMakeLists.txt:13-20`).
- `irt_source -> irt_data_manager + irt_module + irt_toolkit` (`source/CMakeLists.txt:11-18`).
- `irt_toolkit -> irt_interface` (`source/toolkit/CMakeLists.txt:5-13`).
- `irt_data_manager -> irt_toolkit` (`source/data_manager/CMakeLists.txt:13-16`).
- Each module target links `irt_module` while `irt_module` links every module target; the planar-router instance is visible at `source/module/planar_router/CMakeLists.txt:14-19` and `source/module/CMakeLists.txt:14-29`.

The module targets also publish both facade and internal data-manager include roots broadly (`planar_router/CMakeLists.txt:21-24`), and individual subdirectories set global `CMAKE_BUILD_TYPE` (`planar_router/CMakeLists.txt:1-7`). These are historical build mechanics, not architectural requirements.

The iCTS target rule should be:

```text
external binding
  -> icts_api
      -> icts_source_flow
          -> stage facade targets
              -> algorithm targets
                  -> stable database/value targets + leaf utility targets
          -> adapter targets -> external libraries
          -> report target (readonly)
```

No source target may link back to `icts_api`; no child target may link its own aggregator; no algorithm target may depend on flow/report/visualization; and no target may use an `INTERFACE` or `OBJECT` library to hide a real architecture cycle. This agrees with `directory-structure.md:21-24`, `directory-structure.md:110-118`, and `quality-guidelines.md:79-86`.

### 7. Naming and file/directory style

#### Patterns worth retaining semantically

- Directories are lowercase `snake_case`: `data_manager`, `planar_router`, `track_assigner`, `pr_data_manager`.
- Types and files use PascalCase: `PlanarRouter`, `PRModel`, `TBTask`, `DataManager`, `RoutingLayer`.
- Stage-local types are qualified by their owning behavior: `PRModel`, `PRNet`, `PRNode`, `PRComParam`; `DRModel`, `DRTask`, `DRIterParam`; `TBTask`. This prevents generic `Model`, `Task`, or `Param` contracts from leaking across modules.
- Members use a leading underscore with lowercase words, trivial getters/setters use snake case, and enums are `enum class` with `k`-prefixed values (`PRComParam.hpp:21-53`, `ChangeType.hpp:23-28`).
- Behavior functions use action-oriented names such as `buildPRNodeMap`, `generatePRModel`, `uploadNetResult`, `updateSummary`, and `getPlanarTopoList` (`PlanarRouter.hpp:50-69`, `TOPOBuilder.hpp:43-46`).

#### Mechanical iRT style that does not override iCTS rules

- iRT uses `.hpp/.cpp`, but iCTS must retain `.hh/.cc` (`.trellis/spec/project-constraints.md:28-34`).
- iRT's local `.clang-format` declares Google/C++20 with a 160-column limit (`src/operation/iRT/.clang-format:18-51`), but iCTS must use the repository `.clang-format` (`project-constraints.md:70-78`).
- iCTS should use readable CTS-qualified names such as `HTreeInput`, `ClockWritebackPlan`, or `SynthesisTraceSummary`, not mechanically invent two-letter prefixes for every class. Acronyms should be reserved for established domain names; the semantic qualification rule is more important than matching iRT's abbreviations.
- `RTHeader.hpp` is a broad umbrella header containing a large standard/system/third-party include surface (`source/data_manager/advance/RTHeader.hpp:19-64`). iCTS headers must instead be self-contained and include only what they use (`quality-guidelines.md:65-77`).

### 8. Stable principles versus iRT-specific or legacy details

| Stable principle to transfer | iRT-specific / legacy detail not to copy |
|---|---|
| One external facade is the integration boundary. | A public `RTInterface` that also exposes all adapter internals. |
| One root facade per behavior directory. | One 2,000-4,000 line implementation file per complex behavior. |
| Central, deterministic lifecycle orchestration. | Routing-specific order `PA -> SA -> PR -> LA -> SR -> TA -> DR -> VR`. |
| Stage-local workspace/model built for one operation. | GCell, box, panel, layer-grid, A*, DRC, supply, and routing-cost data structures. |
| Explicit selected-result upload/commit boundary. | Raw-pointer ownership transfer through `new` and DataManager maps. |
| Shared stable domain data separated from stage search state. | A generic `basic/advance` split applied verbatim to CTS. |
| Module-qualified input/config/output/summary names. | Opaque two-letter prefixes where no established CTS acronym exists. |
| Cross-cutting logger/monitor at operation boundaries. | Logger/monitor/DataManager/interface target cycles. |
| Real target per behavior plus tier aggregators. | Child-to-aggregator links, broad `PUBLIC` includes, and subdirectory `CMAKE_BUILD_TYPE`. |
| Tests call facade contracts and assert deterministic outputs. | Only one enabled production iRT test and manual singleton lifecycle setup. |
| Concise comments for algorithm rationale and ownership. | `#if 1` section scaffolding, commented debug calls, `framwork.txt`, and development-process narration. |
| Optional artifacts are separated from the selected design result. | Algorithm TUs directly emitting CSV/JSON/GDS and notifications. |

Routing-specific algorithm details must not change during this refactor. In particular, iRT's stage conditions and cost constants (for example the detailed-router iteration schedule at `DetailedRouter.cpp:103-145`) are not templates for CTS synthesis, optimization, or timing policy.

### 9. Proposed iRT-to-iCTS architectural mapping

| iRT concept | Actual responsibility | iCTS target location / equivalent | Mapping rule |
|---|---|---|---|
| `interface/RTInterface` external verbs | Tool entry and lifecycle | `api/CTSAPI.hh/.cc` | Keep `init/run/report/reset/status/feature` compatibility; API remains thin. |
| `RTInterface::runRT` | Root stage orchestration | `source/flow/Flow.hh/.cc` | Keep CTS order `setup -> synthesis -> optimization -> instantiation -> evaluation -> report`, not routing order. |
| `RTInterface` wrap/output methods | External iDB/tool projection | `source/database/io/Wrapper.*`, `source/database/adapter/**`, setup/instantiation boundaries | Raw external types never enter module or report contracts. |
| `DataManager` lifetime | Central shared runtime ownership | API-owned `CTSRuntime` bound into `Flow` | Retain explicit ownership; do not add a DataManager singleton/service locator. |
| `Config` | Runtime configuration plus derived paths | `source/database/config/` | Normalize once at setup; algorithms receive narrow config values. |
| `Database` stable data | Canonical design/routing/timing state | `source/database/{design,routing,spatial,timing,characterization,qor}` | Place each stable type in the narrowest semantic package; avoid a monolithic database class. |
| `data_manager/basic` | Geometry/container primitives | `source/database/spatial/`, `source/utils/geometry/`, or owning module | Place by semantic ownership, not by “basic” level. |
| `module/<stage>/<prefix>_data_manager` | Stage-local workspace, tasks, candidates, parameters | `source/flow/<stage>/<responsibility>/` or `source/module/<algorithm>/<responsibility>/` | Root exposes facade only; local state stays private; stable shared types move to database. |
| `TOPOBuilder` / reusable module facade | Reusable solver/algorithm | `source/module/routing/**`, `source/module/topology/**`, or the closest algorithm package | Expose a narrow task/input and output/summary contract; no runtime singleton. |
| `Summary` and stage `updateSummary` | Cross-stage metrics | typed `{Stage}Summary`, `source/database/qor/`, `SchemaWriter` | Data owner builds fields; report is readonly and does not recompute ownership state. |
| `GDSPlotter` and stage artifact methods | Debug/final visualization | `source/flow/report/visualization/` | Visualization consumes committed state; algorithm modules do not link it. |
| `toolkit/logger` / `monitor` | Cross-cutting diagnostics/runtime metrics | logger/monitor locations frozen by the dedicated logger/monitor design | Follow iRT call contract strictly, but keep target dependencies acyclic. |
| `toolkit/utility` | Shared functions | narrow `source/utils/**` or private module helpers | No general `Utility` service locator or umbrella header. |
| `test/` | Facade validation | mirrored `test/{database,flow,module,utils}/` | Test each public behavior contract and full flow; do not reproduce iRT's sparse enablement. |

Current iCTS already has the core target shape: `source/` declares database, utils, module, and flow categories (`src/operation/iCTS/source/CMakeLists.txt:1-19`), while `Flow` owns the stage sequence and passes narrow dependency sets (`Flow.cc:94-152`, `Flow.cc:186-283`). The refactor should therefore be a systematic convergence and cleanup, not a rename of the existing iCTS tree to `interface/data_manager/toolkit`.

### 10. Proposed architectural rules to freeze before implementation

#### AR-01 — External facade

`CTSAPI` is the only externally visible iCTS facade and the only allowed singleton access point. Existing platform/Tcl/Python-visible behavior and typed status/feature outputs are frozen unless separately reviewed.

#### AR-02 — Flow ownership

`Flow` owns lifecycle and cross-stage state. The canonical sequence remains:

```text
setup -> synthesis -> optimization -> instantiation -> evaluation -> report
```

`CTSAPI` delegates; it does not implement synthesis lifecycle details. Algorithms never call back into `CTSAPI`.

#### AR-03 — Explicit runtime ownership

The API boundary owns `Config`, `Design`, `Wrapper`, FastSTA state, and the structured reporter. Lower layers receive exact references/pointers through constructors or module-qualified input structs. No new `getInst`, `_INST`, global runtime, reset registry, or whole-runtime parameter is allowed below API/Flow.

#### AR-04 — One facade per behavior

Every operation, adapter, solver, builder, router, or flow step has one intended root `.hh/.cc` facade. External callers of that directory include the facade, never internal workspace/helper headers.

#### AR-05 — Semantic internal slices

Complex facade implementations are split under domain-responsibility directories such as `builder/`, `selection/`, `tree/`, `geometry/`, `clock_trace/`, or an equally precise CTS term. Do not introduce generic `internal/`, `support/`, `types/`, or `<prefix>_data_manager/` buckets merely to resemble iRT.

#### AR-06 — Stable stage contracts

Non-trivial stage boundaries use `{Name}Input`, `{Name}Config`, `{Name}Output`, and `{Name}Summary` as needed. Config contains only behavior-changing knobs; Input carries execution context; Output carries data to commit/consume; Summary carries metrics and diagnostics.

#### AR-07 — Temporary-versus-committed state

Stage workspaces own temporary objects and die with the operation. Borrowed pointers are permitted only as non-owning views into a live owner. A stage selects and commits final objects only after success; failure cannot leave partial Design/Clock/iDB membership.

#### AR-08 — Canonical data placement

Shared stable CTS types live in the narrowest `source/database/` package. Algorithm-specific tasks, candidates, search nodes, caches, and iteration state stay inside the algorithm/flow behavior directory. Reusable pure geometry belongs in `database/spatial` or `utils/geometry` according to ownership; one-use helpers remain private.

#### AR-09 — External adapter containment

Raw iDB, SDC-parser, Liberty-parser, or external-tool pointers stay inside `Wrapper`/adapter implementation boundaries. Module, flow-stage, evaluation, report, and visualization contracts use CTS types and narrow adapter queries only.

#### AR-10 — Write authority and readonly consumers

Only synthesis/instantiation-owned boundaries may commit CTS-created topology into canonical Design/iDB state. Evaluation, report, summary export, and visualization consume committed state readonly. Algorithms do not link to visualization or notification implementations.

#### AR-11 — Logger/monitor as leaf infrastructure

Logger/monitor initialization and operation scopes follow the separately frozen iRT contract. Their targets may depend on low-level platform facilities, but DataManager/runtime/module/API targets must not form a cycle through logger or monitor. There is no parallel glog path.

#### AR-12 — Acyclic target graph

Targets use the `icts_{tier}_{category}_{module}` hierarchy. A real `.cc` owner is a real library; header-only data may be `INTERFACE`. Dependencies default to `PRIVATE`; `PUBLIC` is used only when a public header requires it. Aggregators depend on children, never the reverse. Internal subfolders are not broad public include roots.

#### AR-13 — Naming translation

Retain PascalCase files/types, lowercase namespaces, `_lower_case` members, `enum class` with `k` values, snake-case trivial accessors, and camelBack behavior. Use full CTS-domain qualification rather than inventing routing-style abbreviations. Retain mandatory iCTS `.hh/.cc` extensions and repository formatting.

#### AR-14 — Test architecture

Tests mirror source structure, link the public facade target under test, and construct explicit input/runtime fixtures. Each migrated behavior needs contract-level tests for success, empty/no-op, invalid-boundary, and failure/commit atomicity where applicable. Full-flow tests freeze API behavior, report schema, key counts/metrics, and writeback effects.

#### AR-15 — Comment and artifact hygiene

Production code contains only stable design intent, domain rationale, units, ownership, or non-obvious constraints. Do not add `#if 1` section scaffolding, commented-out debug calls, `framwork.txt`, migration notes, temporary adapters, duplicate compatibility branches, or development chronology. Debug visualization is opt-in and isolated from algorithms.

#### AR-16 — Behavior-preserving migration gate

Structural moves, target rewiring, ownership changes, and logger/monitor migration must be reviewed separately from algorithm changes. Each step must build and run focused tests. The final acceptance command compares against a pre-refactor baseline:

```bash
cd /home/liweiguo/project/ecc-tools-dev/scripts/design/ics55_dev && ./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl
```

Any required behavior or QoR change discovered during the refactor returns to planning rather than being bundled into the architectural change.

### 11. Suggested review decisions before coding

The review should explicitly confirm all of the following so “match iRT” is unambiguous:

1. **Semantic rather than literal layer mapping:** retain `api/source/{database,flow,module,utils}/test`; do not rename iCTS to `interface/data_manager/toolkit`.
2. **No module-level singleton replication:** retain only `CTS_API_INST`; translate iRT's central lifetime into API-owned runtime plus explicit stage inputs.
3. **No CMake graph replication:** adopt one target per behavior and aggregators, but require an acyclic target DAG and narrow visibility.
4. **No routing algorithm import:** preserve CTS stage order, algorithms, configuration, timing, QoR, report semantics, and writeback behavior.
5. **Facade contract as the unit of migration:** move/split internals behind a stable facade, then build/test before the next behavior.
6. **Logger/monitor contract handled as a cross-cutting prerequisite:** exact initialization, call grammar, levels, sinks, and monitor scopes come from the dedicated logger/monitor research and must be frozen before module migration.
7. **Current iCTS structure is an asset:** existing `CTSRuntime`, `Flow`, stage input/summary types, adapters, and behavior directories should be audited and normalized, not discarded solely because their names differ from iRT.

### External references / versions

- No external web references were used. The task requests repository-backed iRT behavior, and all conclusions above are derived from the checked-in production code and CMake.
- The iRT subtree requests C++20 (`src/operation/iRT/CMakeLists.txt:6`) and its local formatter also declares C++20 (`src/operation/iRT/.clang-format:51`). This is descriptive evidence only; build-standard or formatting changes for iCTS must follow repository-wide configuration and are not implied by this research.

### Related specs

- `.trellis/spec/project-constraints.md:28-78` — mandatory iCTS suffixes, file naming, header form, formatting, logging, and CMake-first rules.
- `.trellis/spec/backend/directory-structure.md:13-118` — iCTS layers, CTS lifecycle, behavior facade structure, target naming, and library rules.
- `.trellis/spec/backend/database-guidelines.md:12-82` — runtime ownership, stage data shapes, object lifetime, data placement, and adapter containment.
- `.trellis/spec/backend/database-guidelines.md:125-129` — `CTS_API_INST` is the sole singleton boundary.
- `.trellis/spec/backend/quality-guidelines.md:13-95` — names, namespaces, include hygiene, dependency visibility, and forbidden generic/snapshot patterns.
- `.trellis/spec/guides/cross-layer-thinking-guide.md` — type, unit, ownership, validation, and logging questions for the API/database/module/adapter path.
- `.trellis/spec/guides/code-reuse-thinking-guide.md` — search/extraction and target-reuse checks before creating new helpers or build wiring.

## Caveats / Not Found

- iRT's checked-in README is high level and partially stale: it mentions a `Global_router`, `Solver`, `Report`, and `Plotter` arrangement (`README.md:17-43`) that does not exactly match the present source tree. Source and CMake were treated as authoritative.
- Production iRT does not demonstrate conventional constructor/input-based dependency injection; it demonstrates singleton/service-locator access. The proposed iCTS DI rules are the safe semantic translation required by current iCTS specs, not a literal copy.
- Production iRT does not provide a clean acyclic target graph. Its target-per-module organization is useful, but its dependency visibility and aggregator links must not be used as templates without correction.
- iRT's enabled production tests do not cover every routing stage. The topology-builder test demonstrates facade testing, but it is insufficient evidence for a full iCTS validation strategy.
- This document intentionally does not restate the detailed Logger/Monitor API, formatting, sink, severity, or lifecycle contract; that is a separate research topic in this task. It only identifies their architectural placement and target-boundary implications.
- No product code, CMake file, spec, task PRD, or other task artifact was modified by this research.
