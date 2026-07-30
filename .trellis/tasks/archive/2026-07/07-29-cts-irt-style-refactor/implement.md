# iCTS iRT-style systematic refactor implementation plan

## Gate and task model

The work remains one Trellis task without child tasks. The approved global DataManager and one-active-session contract is implemented. A corrective logging iteration is active because the first implementation over-pruned the pre-refactor primary-log business content.

## Checkpoint 0 — Freeze business contracts and baseline

- [x] Record the confirmed global DataManager contract: one active CTS session per process, sequential re-init supported, process-level parallelism preserved.
- [x] Update Trellis directory, data ownership, logging, error and quality specs before product code.
- [x] Write an ownership matrix for every stable object and cross-stage result: current owner, final owner, read/write stages, reset order and commit authority.
- [x] Write a business call map for input/build database, synthesis, optimization, instantiation, evaluation and output; identify old forwarding paths that must disappear.
- [x] Inventory all external API consumers, 827 `LOG_*` calls, 113 reporter-related files, targets and external dependency owners.
- [x] Record HEAD/config/binary identity and run the exact pre-edit baseline:

  ```bash
  cd /home/liweiguo/project/ecc-tools-dev/scripts/design/ics55_dev
  ./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl
  ```

- [x] Persist exit status, topology, names, connectivity, placement, DEF, normalized Verilog, non-log artifacts and QoR. Capture old logs only to classify necessary messages.

Gate: every target responsibility and commit boundary is defined from business behavior, and the functional baseline is valid before product edits.

## Checkpoint 1 — Establish the final toolkit contract

- [x] Implement final `source/toolkit/logger` and `source/toolkit/monitor` leaf targets with iRT call/lifecycle/format/metric semantics.
- [x] Add lifecycle, startup-buffer, ANSI split, complete-line concurrency, failure-status, total/lap/nested and sampling-failure tests.
- [x] Add root-controlled Logger lifecycle without introducing a compatibility logging macro or dual-write path.

Gate: Logger/Monitor contract tests pass independently. The old observability stack may remain only in not-yet-migrated code and is not a checkpoint/final acceptance state.

## Checkpoint 2 — Build the global business DataManager

- [x] Implement `DataManager::initInst/getInst/destroyInst`, `CTSDM`, an explicit run-state machine and deterministic reset lifecycle in `source/data_manager`.
- [x] Make DataManager directly own validated Config, canonical Design/ClockLayout, external object bindings, FastSTA service state and committed cross-stage results.
- [x] Refactor config/work-directory/external clock/library/design ingestion into DataManager input builders; absorb the real Setup and ClockDataRead responsibilities.
- [x] Define stage-specific canonical queries and commit operations for synthesis, optimization, instantiation and evaluation.
- [x] Keep candidates, solver models, iteration state, temporary timing trials and incomplete conversion objects outside DataManager.
- [x] Remove `CTSRuntime` after its real responsibilities are migrated; do not store, wrap, alias or expose it through DataManager.
- [x] Replace thread-local test runtime with an RAII global DataManager lifecycle fixture.
- [x] Replace two-live-runtime isolation coverage with sequential init/reset/re-init, failed-input cleanup, repeated query/report and teardown-idempotence tests.

Gate: DataManager input produces the same canonical CTS input state; reset prevents cross-run leakage; no old runtime/context/registry path remains; the exact script preserves the baseline.

## Checkpoint 3 — Transform each CTS business module

For each module, change ownership, call pattern, physical placement and CMake owner together. Do not create an adapter between old and new module APIs.

### Synthesis

- [x] Build a local synthesis model from CTSDM canonical input.
- [x] Keep characterization/search/HTree candidates and workspaces local.
- [x] Validate the selected topology and commit only the selected result plus stable characterization data and summary.
- [x] Remove the old `SynthesisInput` pointer bundle where it only represented runtime plumbing.

### Optimization

- [x] Initialize from the committed synthesis result and shared timing facts.
- [x] Keep trial edits and timing experiments local; commit only accepted edits and final timing summary.
- [x] Prove failed optimization leaves committed topology unchanged.

### Instantiation

- [x] Build a transactional logical-to-Design/iDB projection from committed state.
- [x] Validate names, connectivity and placement before one writeback commit.
- [x] Prove failed conversion leaves Design/iDB unchanged.

### Evaluation and output

- [x] Evaluate committed state read-only and commit only stable QoR/evaluation results.
- [x] Generate non-log artifacts from committed DataManager state.
- [x] Emit only necessary result/path lines through `CTSLOG`; create no report-writer abstraction.

### Interface orchestration

- [x] Make interface own `DataManager input -> synthesis -> optimization -> instantiation -> evaluation`, readonly report/output and typed public status mapping.
- [x] Delete old `Flow`, Setup facade and stage forwarding call graph after direct business orchestration is complete.

Gate after each business slice: facade/local-model/commit tests pass, no compatibility bridge remains for that slice, affected targets build, and design/QoR stay baseline-equivalent.

## Checkpoint 4 — Complete observability, directories and CMake

- [x] Migrate all remaining runtime calls to `CTSLOG` and local Monitor; classify recoverable versus terminal behavior from control flow.
- [x] Delete SchemaWriter, ReportSink, StageScope, RuntimeMetricScope, glog, `Log.hh`, `cts_detail.log`, any `cts_runtime.log` path and reporter parameters.
- [x] Ensure `<work_dir>/cts.log` is the only CTS log.
- [x] Complete the ownership-driven tree: top-level interface/source/test; source data_manager/module/toolkit.
- [x] Delete old api/external_libs/database/flow/utils roots, forwarding includes, alias targets, old debug options and stale variables.
- [x] Assign one target owner per business behavior, distribute external links to their true owner, default visibility to PRIVATE and remove all cycles.
- [x] Build CTSAPI, feature, Tcl, Python, tool-manager and full iEDA consumers with final paths.

Gate: the physical tree and target graph directly match the new business ownership; no old architecture or observability semantic is reachable.

## Checkpoint 5 — Anti-surface-refactor review and final acceptance

- [x] Inspect every DataManager/module implementation: reject old-object wrapping, old-facade forwarding, old/new type translators, generic compatibility adapters and broad include/target tunneling.
- [x] Verify every major module owns a real local model and has a validated commit or readonly-output boundary.
- [x] Review every changed file for task ownership; remove migration narration, temporary code, debug output, shims, dead code and unrelated formatting.
- [x] Run `git diff --check` and all final scans from `design.md`.
- [x] Run Logger/Monitor, DataManager lifecycle/input, module commit/failure, interface and external-consumer tests.
- [x] Run:

  ```bash
  python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS
  ```

- [x] Run exactly:

  ```bash
  cd /home/liweiguo/project/ecc-tools-dev/scripts/design/ics55_dev
  ./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl
  ```

- [x] Require exit 0, no terminal CTS error, one `cts.log`, and baseline-equivalent topology, names, connectivity, placement, DEF, normalized Verilog, non-log artifacts and QoR.
- [x] Confirm that only the approved logging contract and nondeterministic runtime fields changed.

## Checkpoint 6 — Restore primary-log observability

- [x] Inventory the pre-refactor primary `cts.log` sections and map each high-value business summary to its final data-manager or module owner.
- [x] Restore input/config/clock ownership and distribution summaries through `CTSLOG` without SchemaWriter.
- [x] Restore bounded clustering, characterization, H-tree candidate/selection and synthesis summaries from module-local typed data.
- [x] Restore optimization setup, candidate/iteration evolution, accepted transition, timing and runtime profile summaries without per-trial spam.
- [x] Restore instantiation, evaluation, final QoR and artifact-status summaries.
- [x] Render `cts_report` tables once and mirror the same canonical lines to `.rpt` and `CTSLOG`.
- [x] Verify the only log file remains `cts.log`, with no default/detail routing or compatibility reporter.
- [x] Run focused build/tests, then binary acceptance and functional/QoR comparison.
- [x] Only after binary acceptance passes, run `python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS`.

## Final validation evidence

- The final iCTS CTest suite passed 22/22 after the table-rendering correction and checker fixes.
- The exact ICS55 command exited 0 with no terminal CTS error. Its output contains one 499-line ANSI-free `cts.log`, no detail/runtime log, and 37 owner-stage engineering tables.
- The primary log covers runtime paths/configuration, routing and wire RC, clock tracing/ownership, instance classification, clock distribution, clustering, characterization, bounded H-tree candidates and selection, source trunk, synthesis, optimization evolution/runtime/selection, instantiation, evaluation, key results and report artifacts.
- `HTree Depth Candidate Summary` occurs once and contains four bounded candidate rows. The former `HTree Depth Candidate:`, `CTS Clustering Summary:` and `CTS result:` sentence forms are absent.
- The exact script executes `cts_report`. Wirelength, cell-statistics and library-cell-distribution tables each occur once in `cts.log`; after removing logger prefixes, every table body is byte-identical and contiguous with its `.rpt` counterpart.
- The final DEF is byte-identical to the baseline with SHA-256 `683eb80971fa353587ddd5b7d06c8cf071099474fe7e170fcbbe24b837c1252f`. Verilog is identical after excluding its generated timestamp line. DEF retains 52,440 components and 49,340 nets; DEF and Verilog each retain 696 unique `cts_flow_*` names.
- Final QoR matches the baseline: 348 buffers, 1,056.160 um^2 area, 0.0923373 ns optimized skew, 16 accepted edits, five of five path buffers, clock level 5, 924.436 um maximum wirelength and 43,405.796 um routed total wirelength.
- The final DataManager access scan leaves `CTSDM` only in the synthesis, optimization, instantiation, evaluation and output facades. `QorEvaluation` consumes a facade-built local model, and SVG/GDS writers consume one readonly `Drawing` built by Output; nested algorithms and artifact writers no longer query global state.
- The designated script now intentionally regenerates report/visualization artifacts through `cts_report`; their engineering data is equivalent while generated timestamp fields are not byte-stability requirements.
- The post-binary ecc dev tools check reports zero in-scope findings for format, clang-tidy, headers, CMake and IWYU. Its 3,758 diagnostics are explicitly classified as out-of-scope findings in existing database/Liberty dependencies.
- Final structural and stale-symbol scans pass, and `git diff --check` reports no whitespace errors.

## Checkpoint 7 — Restore iRT-style engineering tables and exercise report in the exact script

- [x] Add one stateless canonical table renderer under toolkit ownership. It formats titles/headers/rows and emits complete lines only through `CTSLOG`; it owns no file, sink, lifecycle, routing policy or business state.
- [x] Reuse the same renderer for `QorReport` so `.rpt`, console and `cts.log` consume one canonical table text.
- [x] Replace the current long text summaries with owner-stage tables for runtime configuration and paths, Runtime Routing / Wire RC, clock trace/ownership, inst classification and clock distribution.
- [x] Restore bounded clustering, HTree build scope, characterization grid/setup/results, one multi-row HTree depth-candidate table, selected HTree/synthesis and source-trunk information.
- [x] Restore optimization setup/evolution, accepted master transitions and runtime profile as bounded tables; retain no per-trial output.
- [x] Restore instantiation, evaluation, wirelength, key-results and artifact/report tables from committed business state.
- [x] Keep lifecycle and actionable diagnostics as sentences; do not retain sentence and table forms of the same business result.
- [x] Add `cts_report -path $CTS_WORK_DIR` to the approved symlink target `/home/liweiguo/project/ecc-tools/scripts/design/ics55_dev/script/iCTS_script/run_iCTS_dev.tcl`.
- [x] Add tests that assert canonical table shape/titles, one HTree candidate table with bounded rows, no duplicate sentence semantics, and `cts_report` table identity.
- [x] Build and run all iCTS tests, then run the exact ICS55 binary command and verify table inventory, one log file, report artifacts, baseline-equivalent DEF/normalized Verilog/QoR and no terminal CTS error.
- [x] Only after the corrected binary acceptance passes, run `python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS` and resolve all in-scope findings.

## Rollback boundaries

- Toolkit rollback removes the isolated Logger/Monitor implementation and root wiring.
- DataManager rollback covers the canonical state/input/reset contract as one unit; a wrapper around old Runtime is not a fallback.
- Each module rollback restores that whole business slice; do not leave adapters that preserve both call graphs.
- Directory/CMake rollback follows the owning business slice, never a cosmetic path-only patch.
- Final acceptance introduces no new product architecture; failures return to the owning checkpoint.

## Highest risks

- global DataManager lifetime and the one-active-session compatibility decision;
- current Flow-held ClockLayout, characterization, synthesis/optimization/instantiation/evaluation state migration;
- test harness migration from 441 thread-local `CurrentRuntime()` calls;
- 474 fatal guards and recoverable `LOG_ERROR` paths;
- removal of SchemaWriter/report plumbing from 100+ code files;
- transactional Design/iDB writeback and rollback;
- repository consumers and CMake dependency redistribution;
- final ICS55 DEF/Verilog/artifact/QoR comparison.
