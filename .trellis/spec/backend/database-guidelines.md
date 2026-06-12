# Database Guidelines

Runtime ownership, dependency boundaries, and data-model rules for iCTS.

## Scope

This document covers runtime ownership, lifetime, singleton exceptions, database-layer placement, and external adapter boundaries.
Naming and generic accessor style live in `quality-guidelines.md`.

## Rules

### Runtime Ownership

Keep singleton use at the external API boundary:

| Macro | Role |
|-------|------|
| `CTS_API_INST` | External API entry point |

Rules:
- External callers enter through `CTS_API_INST`.
- `Config`, `Design`, `Wrapper`, `FastSTA`, and `SchemaWriter` are runtime-owned dependencies passed or bound at API/flow boundaries.
- Modules and algorithms should receive the exact objects or interfaces they need through parameters, narrow input structs, or constructor binding.
- Do not pass a global service locator or `CTSRuntime&` deep into algorithms as a replacement singleton.
- Do not introduce new singleton boundaries without a clear external-boundary need.

### Flow Data Shapes

Use explicit `{Name}Input`, `{Name}Config`, `{Name}Output`, and `{Name}Summary` types when a stage needs a stable boundary contract:
- `{Name}Config` contains only behavior-changing knobs: search space, constraints, heuristics, or enable/disable choices.
- `{Name}Input` contains design references, adapters, DBU/runtime facts, paths, reporter references, libraries, caches, and other execution context.
- `{Name}Output` contains design data that the caller will consume or commit.
- `{Name}Summary` contains metrics, logs, diagnostics, and report rows.

### Ownership

- Use `std::unique_ptr` for ownership.
- Use raw pointers only for non-owning cross-references and topology edges.
- `Design` owns final CTS `Clock`, `Inst`, `Pin`, and `Net` objects.
- `Clock` owns no final `Inst`, `Pin`, or `Net` objects. It stores only borrowed pointers for per-clock anchors and final membership views, such as the clock source pin, clock source net, original sinks, clock insts, and clock nets.
- `Inst` owns no pins. Pin accessors expose only borrowed pointer views and ordering/query helpers.
- Algorithm-local result objects may own temporary `Inst`, `Pin`, and `Net` objects while an operation is in progress. Commit temporary objects into `Design` only after success; failed temporary results must destruct without changing final `Design` or `Clock` state.
- `Net` driver/load pointers and `Pin` inst/net pointers are non-owning topology edges. Do not turn them into ownership links.
- `Wrapper` may keep cross-reference maps between iDB objects and CTS objects, but those maps borrow CTS pointers; `Wrapper` does not own per-clock topology objects.
- `Tree` owns `TreeNode` objects.
- Borrowed pointers must not outlive the owner.
- Do not cache borrowed pointers across owner reset boundaries.

### Placement

Put new types in the narrowest database subdirectory that matches their role:
- config types -> `source/database/config/`
- design objects -> `source/database/design/`
- iDB adapter code -> `source/database/io/`
- CTS-local SDC/FastSTA adapter code -> `source/database/adapter/`
- spatial types -> `source/database/spatial/`
- routing DB types -> `source/database/routing/`
- timing DB types -> `source/database/timing/`

If a type is shared across modules and is part of the stable data model, prefer `source/database/` over `source/module/`.

### Access Boundaries

- Validate runtime-owned dependencies at API, setup, or flow-stage entry boundaries.
- Avoid scattering the same null-check pattern across modules.
- Treat raw external database/tool types as ingress or projection details, not as stable CTS contracts:

  | Boundary | Raw external access allowed | Publishes to callers |
  |----------|-----------------------------|----------------------|
  | `source/database/io/Wrapper.*` | General `idb::*` read/write and iDB-backed geometry/RC lookup | CTS objects, narrow `Wrapper*` value types, or committed iDB changes |
  | `source/database/adapter/sdc/**` | SDC parser state and the minimum `idb::*` facts needed to resolve setup-time clock targets | `SdcClock*` values, diagnostics, and CTS clock/read-data inputs |
  | `source/database/adapter/fast_sta/**` | Raw Liberty parser/data objects and CTS-local FastSTA state | `FastStaClock*` CTS value types and summaries |
  | `source/flow/**`, `source/module/**`, evaluation, report, visualization | No raw external database/tool pointers in contracts | `Design`, `Clock`, `Inst`, `Pin`, `Net`, `ClockLayout`, `FastSTA`, and narrow wrapper queries |

- Keep CTS-required routing-layer RC, iDB geometry, and Liberty lookup access inside the adapter boundary above; do not add separate RC, Liberty, or TimingProvider service classes for iCTS.

### Physical Units Contract

- iDB stores LEF values verbatim: `IdbLayerRouting::get_resistance()` is LEF `RESISTANCE RPERSQ` in ohm/sq, `get_capacitance()`/`get_edge_capacitance()` are LEF pF-based values. iDB applies no unit scaling on read.
- `Wrapper` RC queries publish CTS-facing units directly: resistance in ohm for the queried length, capacitance in pF, lengths in um, DBU conversion only via `queryDbUnit`.
- Consumers must use `Wrapper` RC results unscaled. Never insert milli/kilo conversion factors at call sites (a historical `/1000` at four call sites underestimated wire resistance 1000x and silently zeroed all length-dependent timing terms; fixed in task 06-12-fix-wire-res-unit). If a different unit is genuinely needed, convert inside `Wrapper` and encode the unit in the accessor or field name (`*_ohm`, `*_pf`, `*_um`).
- `icts_test_database_io` (WrapperRcTest) pins this contract with a regression sentinel; extend it when adding new RC query surfaces.
- Do not reintroduce production iCTS dependencies on iSTA/iPA engines, including `ista::TimingEngine`, `api/TimingEngine.hh`, `api/TimingIDBAdapter.hh`, `api/Power.hh`, `STAAdapter`, `ista-engine`, or `power`.
- Liberty parser/data types that still use historical `ista` namespace names may be consumed only as raw Liberty data sources; they must not imply iSTA timing-engine initialization or full-design timing behavior.
- Module code should operate on CTS types, not external-tool types.
- Only synthesis/instantiation boundaries may commit CTS-created topology into `Design` or project final CTS objects through `Wrapper`/iDB.
- Evaluation, report, and visualization are readonly consumers of committed CTS results.
- Report-only data should be narrow and typed. Do not add broad snapshots that duplicate data already available from `Design`, `Clock`, `Inst`, `Net`, report metadata, or narrow `Wrapper` queries.
- Raw `idb::*`, SDC parser, or Liberty parser pointers must not escape their owning adapter as fields in flow inputs, module configs, `Design` objects, report models, or algorithm outputs.

Wrong:

```cpp
struct ClockDistributionInput {
  idb::IdbDesign* idb_design = nullptr;
};
```

Correct:

```cpp
struct ClockDistributionInput {
  Design* design = nullptr;
  Wrapper* wrapper = nullptr;
};
```

### Config Semantics Contracts

- `skew_bound` is a ceiling, not the effective optimization target. The per-clock effective skew target is `min(max(0.0, skew_bound), skew_period_fraction × clock_period_ns)`, falling back to `skew_bound` when the clock period is unknown or `skew_period_fraction` is 0 (added in task 06-11-per-branch-skew-model).
- Optimization or evaluation code that needs a skew target must resolve it per clock through `clock_sizing_optimization::ResolveClockTargetSkewNs(config, clock)`; do not read `config.get_skew_bound()` directly as a stop condition, which silently bypasses period-derived tightening.
- `icts_test_flow_optimization` pins the resolution semantics; extend it when the rule gains new terms.

### Scalable Query Paths

- Name-based `Design` lookups such as `findInst`, `findNet`, and full-name `findPin` must use maintained indexes as the authoritative query path. Do not add vector-scan fallback logic to these hot lookups; it hides indexing bugs and turns report/evaluation paths into O(N) or worse behavior on million-instance designs.
- When object names can change after insertion, maintain a reverse index or equivalent targeted removal path so re-indexing does not scan the entire object map.
- `Wrapper` queries over iDB objects should use iDB-provided maps/search helpers, such as `find_instance`, when available. Avoid linear scans over all iDB instances from report, evaluation, or visualization code.
- If a required index is missing or stale, fix the index ownership/update path rather than compensating at each query call site.

### Output Directories

- Flow/session code derives report roots from the runtime work directory.
- Visualization output is rooted at `visualization_dir`; statistics output is rooted at `statistics_dir`.
- Format-specific subdirectories may live below those roots.
- Remove legacy or unused output-path config fields once they are confirmed unused.

### Adding New Data Classes

When adding a new database-layer type:
1. Place it under the correct `source/database/` subdirectory.
2. Use `enum class` for enums.
3. Initialize members with sensible defaults.
4. Use an `INTERFACE` target if the type is header-only.
5. Add a real library target only when `.cc` implementation is needed.
6. Document any non-trivial ownership rule.

### Singleton Exception

`CTS_API_INST` is the only allowed iCTS singleton boundary. It exists for external callers and must not be used by source-layer code as an internal dependency path.

Do not add new singleton macros, `getInst()` accessors, service locators, global contexts, or reset registries. When a dependency needs shared lifetime, make the owner explicit at the API or flow boundary and pass a narrower reference or input contract to lower layers.

## Checklist

Before handoff, verify:

- [ ] Ownership is explicit and minimal
- [ ] Borrowed pointers do not outlive their owners
- [ ] New data types live in the correct database subdirectory
- [ ] External-tool access stays inside adapter layers
- [ ] Hot name-based queries use maintained indexes, not fallback full-object scans
- [ ] Evaluation/report code is readonly with respect to iDB projection
- [ ] Report-only data is narrow, typed, and not a broad snapshot of database state
- [ ] Header-only database types use `INTERFACE` targets when appropriate
- [ ] No new singleton, service-locator, or global-context access was introduced

## Related Docs

- `directory-structure.md`
- `quality-guidelines.md`
- `../guides/cross-layer-thinking-guide.md`
