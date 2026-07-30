# Research: iCTS current structure and iRT-style refactor gap

> Review update (2026-07-29): current-state evidence and risk counts remain valid. The earlier recommendation to preserve SchemaWriter/default/detail report compatibility has been superseded by the user's explicit product decision: the final system has one iRT-style `cts.log` and removes SchemaWriter, `cts_detail.log`, and any parallel runtime log semantic.

- Query: Inventory current iCTS architecture, lifecycle, logging/reporting/monitoring, glog dependency closure, build/test impact, and migration gaps against iRT patterns and frozen Trellis backend specifications.
- Scope: internal
- Date: 2026-07-29

## Findings

### Executive conclusion

The current iCTS is not an unstructured legacy module waiting to be replaced wholesale. It already implements the principal architecture frozen by recent Trellis work: one public `CTSAPI` singleton, an API-owned `CTSRuntime`, explicit flow dependencies, typed status propagation, facade-oriented behavior directories, and no `source -> api` dependency. Its main verified gap against the requested iRT model is the runtime observability stack: iCTS still reaches glog through repository `Log.hh`/`log`, has no iRT-style `Monitor`, and couples its structured `SchemaWriter` helpers to runtime `LOG_*` calls.

A literal copy of iRT is incompatible with current project contracts. iRT uses `interface/data_manager/toolkit`, `.hpp/.cpp`, many module singletons/service macros, broad `PUBLIC` dependencies, and a logger whose `error()` calls `exit(0)`. Trellis currently mandates `api/source/test`, `.hh/.cc`, `database/utils/module/flow`, a single production singleton, explicit reporter ownership, typed recoverable failures, and narrow dependency visibility. The review therefore must explicitly decide whether “strictly follow iRT” means semantic observability parity or literal structural parity. Implementation cannot honestly satisfy both without first changing the relevant Trellis specs.

Recommended interpretation for review: port the useful iRT contracts—an iCTS-owned logger independent of glog, source-location-aware call sites, deterministic init/open/close/reset, early-message handling, and stack-scoped phase monitors—while preserving the newer iCTS ownership/dependency architecture and observable CTS behavior. Do not mechanically import iRT's singleton proliferation, dependency cycles, exit semantics, extensions, or folder names.

### Representative files found

| Path | Role |
| --- | --- |
| `.trellis/spec/backend/directory-structure.md` | Frozen iCTS tiers, source categories, facade and CMake target rules. |
| `.trellis/spec/backend/logging-guidelines.md` | Current runtime-log versus structured-report contract; conflicts with the new no-glog requirement. |
| `.trellis/spec/backend/database-guidelines.md` | Runtime ownership, adapter boundaries, and singleton prohibition. |
| `.trellis/spec/backend/error-handling.md` | Recoverable/fatal failure and typed-status rules. |
| `.trellis/spec/project-constraints.md` | Required `.hh/.cc`, dependency, logging, and validation conventions. |
| `src/operation/iCTS/api/CTSAPI.hh` / `CTSAPI.cc` | Sole public singleton and runtime/flow lifecycle owner. |
| `src/operation/iCTS/source/flow/Flow.hh` / `Flow.cc` | `CTSRuntime`, explicit stage pipeline, report/metric lifecycle, and reset. |
| `src/operation/iCTS/source/flow/setup/Setup.cc` | Opens the per-run `cts.log` and initializes wrapper/config dependencies. |
| `src/operation/iCTS/source/utils/logger/Schema.hh` / `Schema.cc` | Structured report writer, report scopes, file lifecycle, and residual `LOG_*` coupling. |
| `src/operation/iCTS/source/utils/logger/SchemaScope.cc` | Runtime/stage metrics built on repository `Stats`; emits runtime logs and report tables. |
| `src/utility/log/Log.hh` / `Log.cc` | Repository glog wrapper and global glog lifecycle currently consumed by iCTS. |
| `src/utility/usage/usage.hh` / `usage.cc` | Current elapsed-time/memory sampler used by iCTS report scopes. |
| `src/operation/iRT/source/toolkit/logger/Logger.hpp` / `Logger.cpp` | iRT logger API, formatting, early-message buffer, and singleton lifecycle. |
| `src/operation/iRT/source/toolkit/monitor/Monitor.hpp` / `Monitor.cpp` | iRT stack monitor and elapsed/CPU/memory delta semantics. |
| `src/operation/iRT/interface/RTInterface.cpp` | iRT logger/data/module initialization and per-phase monitor calling pattern. |
| `src/operation/iCTS/test/main.cc` / `common/runtime/CTSTestRuntime.cc` | Per-test reporter lifecycle, output capture, and test runtime isolation. |
| `scripts/design/ics55_dev/script/iCTS_script/run_iCTS_dev.tcl` | Required end-to-end acceptance entry and output artifacts. |

### Quantified current-state gap

Counts are repository-snapshot scans under `src/operation/iCTS` on 2026-07-29; generated/build output was excluded.

| Measure | Current iCTS result | Refactor implication |
| --- | ---: | --- |
| Total files | 626 | A top-level mechanical move would have a very large review and merge surface. |
| Code files | 250 `.cc`, 220 `.hh` | Existing extensions already match the project constraint and differ from iRT. |
| CMake files | 135 | Build restructuring is a first-class migration concern, not clerical cleanup. |
| Direct `#include <glog/logging.h>` | 107 files: 100 source, 7 test | “No glog” requires direct include elimination, not only replacing one target. |
| `Log.hh` consumers | 112 files | The repository wrapper is the principal call-site seam. |
| `LOG_*` invocations | 827 | 474 `LOG_FATAL_IF`, 134 warning, 123 error, 70 info, 24 fatal, and 2 conditional warning/error calls; severity semantics must be mapped deliberately. |
| CMake/.cmake files with a direct `log` target token | 65 | Target-link closure must be cleaned after source migration. |
| CMake files mentioning `icts_source_utils_logger` | 47 | Structured reporting is already a widely shared infrastructure dependency. |
| Code files mentioning `SchemaWriter` or `reporter` | 113 | Reporter removal/replacement would be a large behavior change; decoupling it from runtime logging is the safer seam. |
| Test registration call sites | 23 | Several are option-gated real-tech targets; validation must distinguish configured tests from declared sites. |

The direct glog includes are concentrated in behavior-critical code: synthesis (29), routing (19), topology (9), reporting (9), adapter code (8), optimization (7), characterization (6), database I/O (5), evaluation (3), and smaller setup/instantiation/config/design/timing/test groups. This makes a single giant search-and-replace high risk: most fatal guards sit directly in algorithm and data-boundary code.

The no-glog acceptance gate must cover four different edges:

1. No direct glog headers inside iCTS production or tests.
2. No iCTS include of repository `Log.hh` if the requirement means full independence from the glog-backed wrapper.
3. No residual iCTS link to the `log` target, including the indirect API edge in `external_libs/icts_api_external_libs.cmake:1-8`.
4. No new iCTS logger/report target whose transitive link closure reintroduces `log`, glog, gflags, or unwind.

### Current architecture and lifecycle anchors

The current top-level layout is `api`, `external_libs`, `source`, and `test`; `source` is divided into `database`, `flow`, `module`, and `utils` (`src/operation/iCTS/CMakeLists.txt:36-47`, `src/operation/iCTS/source/CMakeLists.txt:1-21`). This directly matches the Trellis taxonomy in `.trellis/spec/backend/directory-structure.md:13-55` except for the explicit external dependency shim.

The public lifecycle is intentionally narrow:

- `CTSAPI` is a Meyers singleton and owns both `std::unique_ptr<CTSRuntime>` and `Flow` (`src/operation/iCTS/api/CTSAPI.hh:37-92`).
- API initialization constructs runtime/flow; run/report return typed statuses; reset tears down flow/runtime state (`src/operation/iCTS/api/CTSAPI.cc:89-180`).
- `CTSRuntime` owns `Config`, `Design`, `Wrapper`, `FastSTA`, and `SchemaWriter`, while `Flow` binds the runtime explicitly (`src/operation/iCTS/source/flow/Flow.hh:77-139`).
- Flow runs setup, synthesis, optimization, instantiation, evaluation, and report with explicit input structures rather than deep runtime/service-locator access (`src/operation/iCTS/source/flow/Flow.cc:94-303`).
- `Setup` validates explicit config/design/wrapper/reporter inputs, creates output directories, opens `cts.log`, and initializes wrapper state (`src/operation/iCTS/source/flow/setup/Setup.hh:35-65`, `Setup.cc:53-126`).
- `Report` receives reporter and data dependencies explicitly and records evaluation/QoR/visualization results (`src/operation/iCTS/source/flow/report/Report.hh:37-64`, `Report.cc:44-108`).

Targeted dependency scans found no production `source` include of `api/CTSAPI` and no source target link to `icts_api`. Scans of `api`, `flow`, and `module` also found no raw `idb::`, timing-engine, or STA-adapter types crossing those boundaries. The only production singleton pattern found in iCTS itself is `CTSAPI`; the test harness has a separate thread-local `CurrentRuntime()` convenience used by 36 test files and is not a production service locator (`src/operation/iCTS/test/common/runtime/CTSTestRuntime.hh:38-56`, `CTSTestRuntime.cc:32-68`).

These are invariants to preserve, not gaps to “fix” by importing iRT module singletons.

### Logger and structured-report gap

Current iCTS has two intentionally different output channels:

- Runtime diagnostics use repository `LOG_*` aliases. `src/utility/log/Log.hh:27-80` includes glog and maps the aliases to glog; `Log.cc:66-121` owns global glog initialization/shutdown.
- `SchemaWriter` owns deterministic structured artifacts such as `cts.log` and `cts_detail.log`. It exposes RAII runtime/stage scopes, tables, diagnostics, artifacts, and summaries (`src/operation/iCTS/source/utils/logger/Schema.hh:43-173`). It has its own mutex and stream state (`Schema.hh:175-207`).

`SchemaWriter` is explicit runtime state, but its implementation still calls `LOG_WARNING` on file failures (`Schema.cc:189-213`, `Schema.cc:287-305`), and its helper functions emit both runtime and structured output (`Schema.cc:483-519`). `SchemaWriter::open()` supports nested writer suspension and truncates/opens both streams; `close()` restores a suspended writer; `reset()` closes streams and clears paths, suspended writers, and metrics (`Schema.cc:189-285`). Those lifecycle semantics are used by nested/per-test reporting and must be retained unless the review explicitly changes them.

iRT's logger contributes useful patterns: module-owned formatting, source-location capture, pre-open buffering, deterministic init/open/close/destroy, timestamp/thread/file/function context, and an iRT-specific call surface (`Logger.hpp:24-143`, `Logger.cpp:23-48`). It also contains patterns that cannot be copied blindly:

- `Logger::error()` closes the file and calls `exit(0)` (`Logger.hpp:49-74`), which would turn failures into a successful process exit and bypass current typed status/reset behavior.
- It is a raw-pointer singleton (`Logger.hpp:76-89`), conflicting with the frozen “only `CTS_API_INST`” rule.
- Its output stream/state is not protected by a mutex, unlike `SchemaWriter`.
- Its logger CMake target publicly depends on iRT data manager, contributing to cyclic architecture rather than a leaf infrastructure target.

The clean migration seam is therefore runtime logging, not deletion of reporting: introduce/finalize an iCTS-owned runtime logger contract, migrate severity call sites in coherent dependency slices, remove glog/link closure, and keep `SchemaWriter` as explicit structured-report state. Runtime logger and report writer must not be merged into an implicit global “current report,” because that recreates the service-locator design removed in May.

Before implementation, the review must freeze all severity mappings. In particular, 474 current `LOG_FATAL_IF` guards cannot all be mechanically mapped to iRT `error()` without changing termination, cleanup, exit-code, and test behavior. Recoverable boundary failures should continue returning typed status; invariant violations may terminate only under the existing severity policy.

### Monitor gap

iCTS does not have a standalone iRT-style `Monitor`. Its `SchemaWriter::RuntimeMetricScope`/`StageScope` use repository `ieda::Stats` to report wall elapsed time and peak virtual memory (`src/operation/iCTS/source/utils/logger/SchemaScope.cc:92-232`; `src/utility/usage/usage.hh:31-48`, `usage.cc:42-129`). There is no equivalent CPU-time field.

iRT creates a stack `Monitor` at interface/module phase boundaries. `getStatsInfo()` reports elapsed, CPU, and memory deltas and then advances the baseline (`src/operation/iRT/source/toolkit/monitor/Monitor.hpp:23-49`, `Monitor.cpp:26-88`). `RTInterface` surrounds initialization, run phases, and destruction with local monitors (`src/operation/iRT/interface/RTInterface.cpp:63-180`).

Adopting that pattern changes observable metrics unless explicitly normalized:

- iRT uses `gettimeofday` for wall time, `getrusage(RUSAGE_SELF)` for CPU and `ru_maxrss`, while iCTS uses `CLOCK_MONOTONIC` plus `/proc` memory sampling.
- iRT monitor getters mutate their baseline, so call ordering affects subsequent values.
- Current report scopes emit named/staged summary tables; replacing them with text-only monitor messages would regress `cts.log` schema.

The lowest-risk seam is a stack-scoped iCTS monitor used at the same stage boundaries already owned by `Flow`/module facades, with a frozen unit, clock, baseline-update, CPU, and peak-memory contract. It can feed both the new runtime logger and the existing explicit structured reporter without making the monitor or reporter global. Metric golden tests are required because “same function” includes stable report meaning, not necessarily byte-identical timing values.

### Directory, facade, and build gap

iRT's literal layout is `interface/source/test`, with `source/data_manager/module/toolkit`, `.hpp/.cpp`, module-root singleton facades, and many broad public dependencies. Current Trellis rules require:

- `api/source/test` and `source/database/utils/module/flow` (`directory-structure.md:13-55`).
- One facade at a strict behavior root, helpers below responsibility subfolders, and no broad public implementation include roots (`directory-structure.md:69-85`).
- Targets named `icts_{tier}_{category}_{module}` (`directory-structure.md:87-118`).
- `.hh/.cc`, PascalCase types, and narrow/default-`PRIVATE` dependencies (`project-constraints.md:28-34`; `quality-guidelines.md:13-29`, `quality-guidelines.md:65-86`).

Current behavior roots such as `BSTRouter`, `FastClustering`, and `Characterization` already expose narrow facades while their CMake files aggregate implementation subfolders with mostly private internal dependencies. Flow roots also consistently expose one public facade pair. A possible local exception is `source/module/analytical_characterization`, whose root exposes three public pairs; it should be evaluated only if that module is touched.

There remains legitimate CMake cleanup opportunity: iCTS has 123 `add_library` sites, 134 `PUBLIC` visibility lines, 115 `PRIVATE` lines, and several source-category aggregators expose broad include roots (`source/database/CMakeLists.txt:38-50`, `source/flow/CMakeLists.txt:37-42`, `source/utils/CMakeLists.txt:20-27`). However, copying iRT would move in the wrong direction: iRT itself is dominated by broad `PUBLIC` linkage and cross-layer target relationships. Architecture work should narrow real dependency leaks and maintain facade roots, not rename every directory or transplant iRT's target graph.

### Spec conflicts that require review resolution

| Requested/literal iRT behavior | Current Trellis contract | Required review decision |
| --- | --- | --- |
| iRT-owned logger instead of glog | Logging spec currently mandates repository `LOG_*` for runtime diagnostics (`logging-guidelines.md:11-16`, `:32-47`). | Approve a spec update before implementation; otherwise code and normative guidance will contradict. |
| Logger singleton/global `RTLOG` | Only `CTS_API_INST` may be a production singleton; reporter/config/runtime are explicit (`database-guidelines.md:12-25`, `:125-130`). | Choose an explicit/runtime-owned logger or approve and document one infrastructure exception. |
| iRT `error()` exits with code 0 | Recoverable failures return status; fatal means termination under severity matrix (`error-handling.md:11-25`, `:56-79`). | Preserve typed status and nonzero/fatal semantics; do not copy `exit(0)`. |
| `interface/data_manager/toolkit`, `.hpp/.cpp` | `api/database/utils`, `.hh/.cc` are frozen (`directory-structure.md:13-55`; `project-constraints.md:28-34`). | Prefer semantic style transfer; literal rename requires an explicit broader spec migration. |
| Per-module singleton facade | Deep dependencies are explicit and only API is singleton. | Retain current facades/ownership; do not import service locators. |
| Broad public/cyclic target graph | Default `PRIVATE`, explicit layer direction, no broad impl roots. | Rebuild logger/monitor as leaf/cohesive targets and narrow dependencies. |
| Text monitor messages | `cts.log` has a structured schema and explicit writer lifecycle. | Add monitor data without erasing structured report contracts. |

### Migration seams and reviewable delivery slices

1. **Baseline and contract freeze.** Capture the exact development script result, generated artifacts, API statuses, `cts.log`/detail sections, severity behavior, and representative QoR metrics before changing files. Resolve all spec conflicts above.
2. **Leaf observability targets.** Define iCTS logger and monitor contracts with no glog/data-manager dependency. Freeze thread safety, source location, early logging, file ownership, clock/memory units, and reset behavior in unit tests.
3. **Structured reporter decoupling.** Replace `Schema.cc`/`SchemaScope.cc` runtime `LOG_*` edges while preserving explicit `SchemaWriter`, nested writer suspension, table schema, and lifecycle. Do not introduce a global current writer or a new dual-write wrapper.
4. **Lifecycle integration.** Integrate logger init/open/close/reset with the API/setup/flow teardown order and per-test runtime. Ensure repeated init-run-report-reset and failed setup leave no stale stream or monitor state.
5. **Call-site migration by dependency slice.** Migrate database/utils first, then modules, flows, API, and tests. For every slice, classify each current info/warning/error/fatal guard; avoid a macro-only textual conversion.
6. **CMake closure cleanup.** Remove direct and transitive `log` edges, keep new targets leaf-like, retain target naming rules, and reduce broad public include/link exposure where the touched dependency graph proves it safe.
7. **Selective facade/directory cleanup.** Change only roots shown to violate one-facade or ownership rules. Do not combine mass path renames with logger semantic changes.
8. **Regression and acceptance.** Run focused logger/monitor/report/runtime tests, all configured iCTS tests, build checks, static no-glog/dependency scans, then the mandated end-to-end script and artifact comparison.

This ordering keeps each change reviewable and preserves a rollback seam. A one-commit, whole-tree rewrite would mix severity changes, output-format changes, target-graph changes, path moves, and algorithm regressions in the same diff.

### API, test, and external integration impact

The public callers that must remain source-compatible include tool-manager iCTS I/O, Tcl, Python, and feature builder. Direct `CTSAPI` use was found in `src/platform/tool_manager/tool_api/icts_io/icts_io.cpp:44-70`, `src/interface/python/py_icts/py_icts.cpp:40`, and `src/feature/builder/feature_builder_tool.cpp:55`. Tcl exposes `run_cts`, `cts_report`, save-tree, and config commands (`src/interface/tcl/commands/cts/tcl_register_cts.h:34-38`). Logger refactoring must not alter those API results or command sequencing.

The test harness begins a thread-local runtime per test, prepares output, opens a per-test `cts.log`, emits context/result, captures stdout/stderr, closes the reporter, and ends the runtime (`src/operation/iCTS/test/main.cc:39-153`). Thirty-six test files use `CurrentRuntime()` with 447 call sites. It is an important test migration seam: new logger state must be isolated/reset here without legitimizing a production service locator.

The exact acceptance path also enters platform code that independently includes glog and owns a `CtsIO` singleton (`src/platform/tool_manager/tool_api/icts_io/icts_io.cpp:19-75`, `icts_io.h:27-65`). That code is outside `src/operation/iCTS`. The review must define “iCTS no longer uses glog” as either:

- **Module boundary (recommended):** no glog/`Log.hh`/`log` dependency in iCTS targets; unrelated platform and other iEDA modules may still use the repository logger.
- **End-to-end command closure:** also migrate platform/Tcl wrappers used by `run_cts`; this is a material scope expansion and cannot imply removing glog from the whole iEDA process.

### Validation baseline and risks

The mandated command is:

```bash
cd /home/liweiguo/project/ecc-tools-dev/scripts/design/ics55_dev
./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl
```

The script initializes flow/database/LEF/DEF, calls `run_cts`, saves `result/cts.def` and `result/cts.v`, and exits (`run_iCTS_dev.tcl:1-73`). Its exit code alone is insufficient: a literal iRT `error()->exit(0)` port could report success after a fatal path. Acceptance should compare at least:

- command exit and absence of fatal/error markers;
- presence and parseability of `result/cts.def` and `result/cts.v`;
- expected CTS work directory and structured `cts.log`/detail sections;
- API/flow final status and stage completion sequence;
- stable clock-tree topology/counts and agreed QoR/timing metrics, with tolerances only for runtime/memory measurements;
- repeated-run/reset behavior in one process;
- configured unit/real-tech test results;
- static scans proving zero in-scope glog headers, `Log.hh`, `LOG_*`, direct `log` links, and transitive glog closure.

Baseline must be captured before implementation. This research did not execute the full flow, so it does not establish the current golden artifacts or numeric QoR values.

Highest-risk hotspots are the 474 fatal guards, setup/open failure paths, `SchemaWriter` nested/per-test stream restoration, multithreaded logging, reset after partial initialization, and report/monitor metric semantics. Mass file moves and logger conversion should not occur in the same slice.

### In scope, conditional scope, and unrelated legacy

**Core in scope**

- All direct and indirect glog/`Log.hh`/`log` dependencies owned by `src/operation/iCTS`.
- An iCTS logger/monitor design and its lifecycle/calling conventions.
- Runtime severity mapping, source-location format, thread safety, early-message policy, and reset behavior.
- Decoupling `SchemaWriter`/stage scopes from glog while preserving structured reporting.
- iCTS CMake target/link/include changes required for the new observability stack.
- iCTS tests and the exact `ics55_dev` regression.
- Architecture/folder/facade changes proven necessary by the agreed semantic iRT model and current specs.

**Conditional; require explicit review approval**

- A logger singleton exception to the single-API-singleton rule.
- Literal top-directory renames, extension migration, or whole-tree target renaming.
- Platform/Tcl/Python wrapper logger migration outside `src/operation/iCTS`.
- Changes to public CTS API signatures, command names, report schema, or error semantics.

**Out of scope / unrelated legacy**

- Removing glog from the repository or unrelated iEDA modules.
- Copying iRT module service locators, cyclic CMake links, raw singleton ownership, or `exit(0)` failure behavior.
- CTS algorithm/QoR changes, opportunistic performance tuning, or unrelated cleanup/comments.
- Generated results, temporary diagnostics, migration shims, commented-out code, and benchmark artifacts in the final product diff.
- Third-party/external-library restructuring unless required to remove an actual iCTS transitive glog edge.

### Related task-history constraints

Recent Trellis history deliberately moved iCTS away from the literal iRT-style global architecture:

- `archive/2026-05/05-24-cts-reporter-config-explicit` removed global reporter/config access and froze explicit `SchemaWriter&`/config dependencies at flow boundaries; its design explicitly forbids a global current reporter.
- `archive/2026-05/05-24-cts-runtime-flow-desingleton` and `05-25-cts-runtime-boundary-cleanup` consolidated runtime ownership under Flow/API and removed subsystem singletons.
- `archive/2026-06/06-01-optimize-cts-architecture` froze only `CTS_API_INST`, typed API status, narrow contracts, facade cleanup, and no `source -> api` dependency; it explicitly rejected mechanical taxonomy churn and deep `CTSRuntime` propagation.
- `archive/2026-05/05-15-cts-report-log-structure` established the distinction between runtime diagnostics and structured `cts.log`, plus concise/default versus detail report sinks. The new no-glog requirement supersedes its runtime backend choice, not its structured-report contract by implication.
- CMake-focused May tasks established file/target granularity and narrow dependency goals; they are evidence against importing iRT's broad public/cyclic target graph.

These are not incidental preferences: current code embodies them. Reversing them would be a separate architecture decision with a larger regression surface.

### External references

No external documentation was needed for this internal comparison. iRT and iCTS source at the repository snapshot dated 2026-07-29 are the comparison basis. No assumption was made that iRT's observable bugs or build cycles are normative requirements.

### Related specs

- `.trellis/spec/backend/directory-structure.md`
- `.trellis/spec/backend/logging-guidelines.md`
- `.trellis/spec/backend/error-handling.md`
- `.trellis/spec/backend/quality-guidelines.md`
- `.trellis/spec/backend/database-guidelines.md`
- `.trellis/spec/backend/guides/cross-layer.md`
- `.trellis/spec/backend/guides/code-reuse.md`
- `.trellis/spec/project-constraints.md`
- `.trellis/tasks/07-29-cts-irt-style-refactor/prd.md`

## Caveats / Not Found

- No end-to-end baseline execution, build, unit test, or artifact comparison was run during this read-only research phase; current functional/QoR goldens remain to be captured before implementation.
- Counts are static text-scan results for the 2026-07-29 workspace snapshot. Conditional CMake targets and transitive links require confirmation in the actual configured build graph.
- “Strictly follow iRT” is presently ambiguous and conflicts with frozen Trellis specs in logger ownership, failure semantics, directory names, extensions, and singleton policy. This is the critical review blocker; implementation should not begin until the intended interpretation is recorded.
- The exact end-to-end command passes through non-iCTS platform code that still uses glog. Module-level and command-closure acceptance scopes must be distinguished explicitly.
- No authoritative historical artifact was found that authorizes undoing the May/June explicit-runtime and de-singleton decisions; the available task history supports preserving them.
