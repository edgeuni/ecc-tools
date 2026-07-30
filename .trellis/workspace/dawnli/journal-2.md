# Journal - dawnli (Part 2)

> Continuation from `journal-1.md` (archived at ~2000 lines)
> Started: 2026-05-08

---



## Session 45: CTS SDC clock semantics and writeback hardening

**Date**: 2026-05-08
**Task**: CTS SDC clock semantics and writeback hardening
**Branch**: `cts_refactor`

### Summary

Implemented SDC-driven CTS clock discovery, STA clock-only SDC period extraction, explicit no-op handling, ClockDAG path metrics, rollback-safe iDB writeback, and the CtsClockReader/CtsClockIdbWriter wrapper refactor.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `1cf71a890` | (see git log) |
| `31e81ae05` | (see git log) |
| `2cd82603` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 46: iCTS H-tree complexity optimization and pruning research cleanup

**Date**: 2026-05-11
**Task**: iCTS H-tree complexity optimization and pruning research cleanup
**Branch**: `cts_refactor`

### Summary

Optimized H-tree selection/root compensation complexity, documented rejected pruning routes, restored active source scopes to the opt3 baseline, and archived the completed runtime/pruning research tasks after the final iCTS quality gate passed.

### Main Changes

- Archived `.trellis/tasks/05-09-icts-htree-dominance-pruning-research` to `.trellis/tasks/archive/2026-05/05-09-icts-htree-dominance-pruning-research`.
- Archived `.trellis/tasks/05-09-analyze-icts-htree-runtime-bottlenecks` to `.trellis/tasks/archive/2026-05/05-09-analyze-icts-htree-runtime-bottlenecks`.
- Final pushed commit: `f235e1985 perf: optimize H-tree complexity and record pruning experiments`.
- Final quality gate previously passed: `python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS` exited 0; `icts_test_flow_synthesis_htree` passed 6/6; `icts_test_module_characterization` passed 17/17; `git diff --check HEAD` had no findings.


### Git Commits

| Hash | Message |
|------|---------|
| `f235e1985` | (see git log) |

### Testing

- [OK] `python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS` exited 0.
- [OK] `./bin/icts_test_flow_synthesis_htree` passed 6/6.
- [OK] `./bin/icts_test_module_characterization` passed 17/17.
- [OK] `git diff --check HEAD` had no findings.

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 47: HTree demand-driven frontier refactor

**Date**: 2026-05-11
**Task**: HTree demand-driven frontier refactor
**Branch**: `cts_refactor`

### Summary

Refactored iCTS HTree segment frontier synthesis to use request/catalog demand planning, enabled narrowed HTree frontier construction, validated ics55_dev runtime/QoR, and passed full src/operation/iCTS ecc_dev_tools check.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `302077b5d` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 48: Fix small-fanout H-tree legality

**Date**: 2026-05-12
**Task**: Fix small-fanout H-tree legality
**Branch**: `cts_refactor`

### Summary

Enforced fanout-aware H-tree topology and pattern composition, legalized overlapping FLUTE terminals, validated max_fanout=4 and max_fanout=32 dev flows, ran ecc_dev_tools for iCTS, and documented the root-cause findings.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `2d016b246` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 49: H-tree characterization semantic alignment

**Date**: 2026-05-13
**Task**: H-tree characterization semantic alignment
**Branch**: `cts_refactor`

### Summary

Aligned native H-tree characterization boundaries with evaluation semantics, added configurable root input slew, enforced root boundary closure diagnostics, refreshed CTS Tcl config support, and validated fanout 4/32 binary QoR.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `36844c2db` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 50: Merge main into cts_refactor

**Date**: 2026-05-13
**Task**: Merge main into cts_refactor
**Branch**: `cts_refactor`

### Summary

Merged local main into cts_refactor, preserved CTS/refactor and iSTA/liberty semantics, and validated clean build plus pre/post iCTS binary equivalence.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `e18c8ce8f` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 51: Merge latest main into cts_refactor

**Date**: 2026-05-13
**Task**: Merge latest main into cts_refactor
**Branch**: `cts_refactor`

### Summary

Merged latest main into cts_refactor after review, preserving cts_refactor CTS behavior, semantically resolving iSTA changes, and confirming iCTS validation outputs match before and after merge.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `364215d55` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 52: Align non-CTS drift with main

**Date**: 2026-05-13
**Task**: Align non-CTS drift with main
**Branch**: `cts_refactor`

### Summary

Restored approved category 3/4 non-CTS drift on cts_refactor to main, kept CTS-facing/iCTS-required semantics, clean-built, and reran ics55_dev iCTS binary with matching key CTS results.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `d62a25e9f` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 53: Analytical H-tree construction

**Date**: 2026-05-15
**Task**: Analytical H-tree construction
**Branch**: `cts_refactor`

### Summary

Implemented analytical H-tree characterization and solver integration for CTS, validated with ecc_dev_tools full iCTS check, iEDA build, and focused analytical unit tests.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `3d54fec0b` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 54: Optimize CTS read_data and FastClustering

**Date**: 2026-05-15
**Task**: Optimize CTS read_data and FastClustering
**Branch**: `cts_refactor`

### Summary

Reviewed CTS changes for natural integration, optimized clock read_data bulk load materialization, improved FastClustering packing/runtime with spatial neighbor reuse and selective polish, validated builds/tests/full iCTS ecc dev check, then archived both tasks.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `bd35ef6f9` | (see git log) |
| `197f279d0` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 55: CTS SDC clock tracing

**Date**: 2026-05-15
**Task**: CTS SDC clock tracing
**Branch**: `cts_refactor`

### Summary

Added CTS-owned SDC clock parsing and tracing, ownership/unowned clock-like reports, regression coverage, and validated the huge design flow without manual use_netlist mapping.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `8fb4d2edf` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 56: CTS report log structure

**Date**: 2026-05-15
**Task**: CTS report log structure
**Branch**: `cts_refactor`

### Summary

Split CTS report logging into concise default and curated detail outputs, then fixed iSTA test build blockers needed for all-target validation.

### Main Changes

- Added CTS structured report routing so `cts.log` stays concise while curated internals go to `cts_detail.log`.
- Moved repeated HTree per-depth/helper lifecycle detail out of the default report and replaced it with compact summary tables.
- Trimmed duplicated/default-low-value characterization, source trunk, synthesis, instantiation, and report-mode fields while preserving failure/fallback diagnostics.
- Updated focused iCTS report tests for default/detail routing and report-content expectations.
- External module note: changed only `src/operation/iSTA/test` to restore all-target build compatibility with current STA/Python DB APIs and the local conda/system libffi mix.
- Validation passed: `cmake --build build -j 8`, focused iCTS tests, `./bin/iSTATest --gtest_list_tests`, and `python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS` with in-scope findings 0.
- Left `src/apps/CMakeLists.txt` uncommitted because it only redirects local app output to `scripts/design/ics55_huge_dev` and is not required for the task.


### Git Commits

| Hash | Message |
|------|---------|
| `ed18a76a9` | (see git log) |
| `2eeb4b3c1` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 57: Optimize ecc dev iCTS runtime

**Date**: 2026-05-16
**Task**: Optimize ecc dev iCTS runtime
**Branch**: `cts_refactor`

### Summary

Optimized ecc_dev_tools iCTS full-check runtime without reducing coverage: parallelized tidy header checks, combined tidy/analyzer TU execution, split header self-check subchecks, improved runtime attribution, documented experiments and rejected shortcut/runtime-contention approaches.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `657ad86c5` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 58: CTS huge optimization search

**Date**: 2026-05-18
**Task**: CTS huge optimization search
**Branch**: `cts_refactor`

### Summary

Implemented topology-aware target-window CTS optimization scoring for huge designs, validated huge 80ps runtime/QoR, and passed full iCTS ecc dev check.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `21b8652d9` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 59: CTS code structure optimization

**Date**: 2026-05-18
**Task**: CTS code structure optimization
**Branch**: `cts_refactor`

### Summary

Committed the iCTS code-structure refactor, tightened fatal runtime semantics for DBU/routing-layer/RC paths, validated the ics55 iCTS dev script, passed ecc dev, and archived completed Trellis tasks.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `a1de604f8` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 60: Refactor CTS FastSTA and STAAdapter boundaries

**Date**: 2026-05-19
**Task**: Refactor CTS FastSTA and STAAdapter boundaries
**Branch**: `cts_refactor`

### Summary

Renamed the CTS FastSTA facade, removed stale STAAdapter char-runtime APIs, unified net-cap and buffer/sink slew legality semantics, made root slew explicit, deleted the obsolete critical-branch char sizing task, and archived completed CTS FastSTA optimization/refactor tasks after validation.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `8d78fa42f` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 61: Close iCTS code structure optimization follow-ups

**Date**: 2026-05-19
**Task**: Close iCTS code structure optimization follow-ups
**Branch**: `cts_refactor`

### Summary

Closed iCTS code-structure follow-ups: kept HTree and optimization root facades minimal, moved internals into semantic subdirectories, updated CMake target wiring and tests, and validated with iCTS dev script plus ecc dev check.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `145000a24` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 62: CTS code normalization convergence

**Date**: 2026-05-20
**Task**: CTS code normalization convergence
**Branch**: `cts_refactor`

### Summary

Closed CTS code normalization convergence work, fanout-4 runtime follow-up, validation evidence, and archived all completed CTS tasks.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `597cc31b8` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 63: iCTS FastSTA reset normalization

**Date**: 2026-05-22
**Task**: iCTS FastSTA reset normalization
**Branch**: `cts_refactor`

### Summary

Committed the incremental iCTS structure port and FastSTA reset/test-isolation cleanup, resolved all in-scope ecc_dev findings for src/operation/iCTS, and archived the completed 05-20 and 05-21 Trellis tasks.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `6ddaa6c50` | (see git log) |
| `592f49841` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 64: iCTS CTS benchmark fixes

**Date**: 2026-05-22
**Task**: iCTS CTS benchmark fixes
**Branch**: `cts_refactor`

### Summary

Migrated CTS benchmark fixes onto latest cts_refactor, added reproducible ics55 benchmark tooling/results, removed use_netlist config/code, implemented structural clock precluster reuse and H-tree fixes, verified 93/93 benchmark pass and clean ecc_dev_tools check.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `2fde72076` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 65: CTS runtime boundary cleanup

**Date**: 2026-05-25
**Task**: CTS runtime boundary cleanup
**Branch**: `cts_refactor`

### Summary

Committed and archived CTS desingleton/runtime-boundary refactor; moved CTS report APIs under direct icts namespace; validated iCTS tests and deferred ecc dev per user request.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `6c437f228` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 66: CTS cleanup normalization refactor

**Date**: 2026-05-25
**Task**: CTS cleanup normalization refactor
**Branch**: `cts_refactor`

### Summary

Normalized iCTS public contracts, narrowed module facades, folded runtime and HTree contracts into owned boundaries, ran full ecc_dev_tools validation before committing, and archived the cleanup normalization task.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `e34820f21` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 67: Mathematical analytical H-tree solver

**Date**: 2026-05-26
**Task**: Mathematical analytical H-tree solver
**Branch**: `cts_refactor`

### Summary

Implemented mathematical analytical H-tree MILP solver and native H-tree memory compaction; validated dev/huge designs and passed final iCTS checker.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `a8df777d9` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 68: Analytical H-tree HiGHS integration

**Date**: 2026-05-27
**Task**: Analytical H-tree HiGHS integration
**Branch**: `cts_refactor`

### Summary

Refactored H-tree selection around discrete/analytical sibling engines, added shared finalization, removed OR-Tools/SCIP production paths, vendored HiGHS as ordinary third-party source, and validated focused build/test plus full iCTS checker.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `2d4d8f509` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 69: H-tree architecture decomposition

**Date**: 2026-05-27
**Task**: H-tree architecture decomposition
**Branch**: `cts_refactor`

### Summary

Extracted H-tree synthesis state assembly, separated discrete-only frontier synthesis from analytical selection, unified selection and finalization contracts, and validated focused builds, tests, iEDA, ics55_dev, ics55_huge_dev, and iCTS checker.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `ef4b93b9b` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 70: Clean CTS static archive CMake links

**Date**: 2026-05-27
**Task**: Clean CTS static archive CMake links
**Branch**: `cts_refactor`

### Summary

Removed explicit CTS static archive path links, replaced them with target-based dependencies, validated iEDA link, focused H-tree tests, and iCTS checker.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `55cd2834c` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 71: CTS global CMake architecture cleanup

**Date**: 2026-05-28
**Task**: CTS global CMake architecture cleanup
**Branch**: `cts_refactor`

### Summary

Refactored CTS CMake dependencies to use concrete targets, tightened visibility/include ownership, validated iEDA build, H-tree tests, ics55_dev, and ecc dev check.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `084e51e86` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 72: Merge main into cts_refactor

**Date**: 2026-05-28
**Task**: Merge main into cts_refactor
**Branch**: `cts_refactor`

### Summary

Merged origin/main into cts_refactor, preserved cts_refactor iCTS as source of truth, ported compatible CTS fixes, and validated iEDA build plus iCTS flow.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `17fbc4f42` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 73: Unbind iCTS iSTA iPA

**Date**: 2026-05-29
**Task**: Unbind iCTS iSTA iPA
**Branch**: `cts_refactor`

### Summary

Removed iCTS production dependencies on iSTA and iPA, moved RC/Liberty access into Wrapper, deleted unsupported full-design timing outputs, validated iCTS tests, ics55_dev binary run, baseline binary parity, and ecc_dev_tools.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `e919bcce0` | (see git log) |
| `308c97c4a` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 74: Optimize CTS CMake architecture

**Date**: 2026-05-29
**Task**: Optimize CTS CMake architecture
**Branch**: `cts_refactor`

### Summary

Optimized iCTS CMake target ownership, moved clock-tree realization under synthesis, removed stale conversion CMake files, and verified current/reduce builds.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `5f3beb2de` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 75: Optimize iSTA Liberty link runtime

**Date**: 2026-06-06
**Task**: Optimize iSTA Liberty link runtime
**Branch**: `cts_refactor`

### Summary

Optimized iSTA LibertyReader dispatch and scalar pin handling, reducing ics55_dev CTS read_data from 32.656s to 10.469s while preserving tracked iCTS metrics.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `0ec2a934b` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 76: Optimize Liberty read_data runtime

**Date**: 2026-06-06
**Task**: Optimize Liberty read_data runtime
**Branch**: `cts_refactor`

### Summary

Optimized iSTA Liberty read_data by using a buffered scanner and direct C++ AST replay into LibertyReader; validated iCTS metrics bitwise match and reduced read_data runtime.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `b89945a7d` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 77: Fix ics55 ECC CTS failures

**Date**: 2026-06-08
**Task**: Fix ics55 ECC CTS failures
**Branch**: `cts_refactor`

### Summary

Fixed iCTS Liberty expression traversal and SDC expression parsing, validated latest-binary rerun for six ics55 ECC CTS cases, and archived the Trellis task.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `f3517de2a` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 78: Merge main into cts_refactor

**Date**: 2026-06-08
**Task**: Merge main into cts_refactor
**Branch**: `cts_refactor`

### Summary

Merged latest main into cts_refactor, resolved conflicts by keeping current iCTS/.gitignore and main for non-iCTS conflicts, verified GCC 11 build and archived the task.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `9e2195e31` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete

---

## 2026-06-11: 对齐商业工具 CTS 性能 — 阶段1根因调研（vga_lcd）

**Date**: 2026-06-11
**Task**: 06-11-align-commercial-cts（planning，父任务）
**Branch**: `cts_refactor`

### Summary

以 vga_lcd 为基准完成 ECC vs Innovus CTS 拓扑差异深度调研。裁决了两个假设：H1 wirelength unit 失真成立（且发现更深层的 R/1000 单位 bug）；H2 NDR 缺失方向成立但当前 eval 体系下两家 clock net 均由 NanoRoute 默认规则布线，NDR 主要通过模型保真起作用。

### Key Findings

1. **R1 [P0] wire 电阻 1000x 低估**：`WrapperRc.cc:159,293`、`FastStaChar.cc:134` 将 ohm 值再除 `kMilliOhmPerOhm`；LEF 0.914 Ω/um → 模型 0.000914。所有线长相关时序代价坍缩，FastSTA 内部 skew 0.053 vs eval 0.118。
2. **R2 [P0] 表征截断**：unit 38.98um 自动推导，13 个覆盖需求被 cap 到 3 bin（ceil 量化），长段 DP 组合插额外 buffer。
3. **R3 [P1] 深度搜索失效**：`monotone_pruned_by_bottom_most_buffered_level threshold=10` 一刀切，11/10/9 深度 0 评估（run 报告 cts.log 直接证据）。
4. **R4 [P0建模] per-level 均一 vs per-branch 物理**：L3 层 std=125um/max=467um；eval 同 sink 路径 ECC 9级0.651 vs Innovus 7级0.472，每级 +8~29ps 累积 95ps skew。
5. **R5/R6**：iCTS 零 NDR 支持、C 无耦合、单层 RC；无 insertion-delay-reduction 阶段、优化器被错误模型骗成 no_op。

### Artifacts

- `prd.md`（父任务验收口径 + 子任务方向 P0-A/B, P1-C/D/E, P2-F/G）
- `research/vga-lcd-topology-gap-root-cause.md`（完整证据链）
- `research/level-spread-data.md` + `level_spread.py`（每层离散度分析）

### Status

[OK] 阶段 1 调研完成，待评审后派生子任务

### Next Steps

- 评审子任务方向，优先 P0-A（R 单位 bug 修复）+ P0-B（表征覆盖）

---

## 2026-06-12: P0-A wire 电阻 1000x 单位 bug 修复完成

**Date**: 2026-06-12
**Task**: 06-12-fix-wire-res-unit（已完成）
**Branch**: `cts_refactor`

### Summary

移除 4 处把 Wrapper 欧姆返回值再除 1000 的错误换算（WrapperRc.cc x2、FastStaChar.cc、FastStaParasitics.cc），新增 icts_test_database_io 单位契约测试（含 >0.1 Ω/um 回归哨兵，红→绿已实证），spec 增补 Physical Units Contract 节。

### Key Results (vga_lcd A/B, 同 HEAD 双二进制)

unit_resistance 914 uOhm→mOhm/um；FastSTA initial skew 0.0533→0.0576；拓扑决策几乎不变（buffer 5744→5745，WL 不变）——R1 是模型保真前提而非 vga_lcd 拓扑差距决定因素。新发现：max_length=300 未在 trunk/htree 实施（472um 裸段仍合法）；优化器仍 no_op（固定 target 0.08 太松）→ 均转后续任务。

### Git Commits

| Hash | Message |
|------|---------|
| `a9e8d7503` | fix(iCTS): correct 1000x wire resistance underestimation in RC queries |

### Testing

- [OK] icts_test_database_io 4/4（哨兵红→绿实证）
- [OK] 全量 iCTS ctest 16/16
- [OK] clang-format 干净（trellis-check PASS）

### Status

[OK] **Completed**

### Next Steps

- P0-B 表征线长覆盖修复（06-12-char-wirelength-coverage）开工

---

## 2026-06-12: P0-B 表征线长覆盖修复完成

**Date**: 2026-06-12
**Task**: 06-12-char-wirelength-coverage（已完成）
**Branch**: `cts_refactor`

### Summary

auto 模式直接表征上限与 legacy wirelength_iterations 解绑，新增 auto_direct_bins_cap（默认 6，2^bins pattern 成本上界）。设计期发现两个架构事实并勘误 PRD：pattern 枚举 2^slots（slots=length_idx）使"全覆盖"不可行；SegmentPruning 组合 join 不必然插中间 buffer（真实代价=bucket 量化误差累积+组合开销）。

### Key Results (vga_lcd, cap 扫描 6/7/8)

QoR 三档完全一致（skew 0.0562）→ 默认 6 为成本拐点：direct bins 3→6、CharBuilder 0.02→2.2s、SourceTrunk 合成 35.3→16.1s（-54%）、端到端 67.5→37.1s（-45%）、initial_skew 57.6→56.2ps。runtime_config 显式模式行为零变化（单测钉死）。

### Git Commits

| Hash | Message |
|------|---------|
| `111ac5d6d` | perf(iCTS): bound auto-grid direct characterization by auto_direct_bins_cap |

### Testing

- [OK] WirelengthGridTest 7/7 新用例
- [OK] 全量 iCTS ctest 16/16
- [OK] trellis-check PASS（1 处 clang-format 已修）

### Status

[OK] **Completed**

### Next Steps

- P1 组（htree-depth-unlock / per-branch-skew-model / latency-align）基于新基线开展
- 建议先统一跑一次 eval 管线重基线（P0-A+P0-B 合并效果的 Innovus route+STA 实测）

---

## 2026-06-12: P1-C 深度搜索解锁完成

**Date**: 2026-06-12
**Task**: 06-11-htree-depth-unlock（已完成）
**Branch**: `cts_refactor`

### Summary

边界组 split 补救替代一票否决：单个 5-load 稠密组（实证 node_912, level-11）曾经由单调阈值灭掉全部浅深度候选。SplitSinkLoadRegionGroup（确定性中位二分，合法性/嵌入共享）+ 监控（Split 列、Monotone Origin 报告）。R1 结论：单调剪枝逻辑本身正确，过约束在区域语义层（无局部补救）。

### Key Results (vga_lcd, 内部口径)

深度候选 1/4 → 4/4 可行，选中 depth 9；initial_skew 0.0562→0.0294（-48%）、max arrival -53ps、级数均一 10/10（原 9~10）；代价 buffer +4.9%（1114 split sub-buffers）、WL +1.5%。DEF 实测全部 clock net fanout ≤4。勘误：级数 ≤8 未达成（移交 P1-E）。

### Git Commits

| Hash | Message |
|------|---------|
| `2258e1cd5` | feat(iCTS): unlock H-tree depth search with boundary-group split remediation |

### Testing

- [OK] SinkLoadRegionSplitTest 6/6
- [OK] 全量 iCTS ctest 16/16；clang-format 干净
- [OK] trellis-check（格式修复已并入；一致性/单调性审查主会话补全）

### Status

[OK] **Completed**

### Next Steps

- P1-D per-branch 时序建模与 skew 修复（自适应 skew target + 优化器激活）
- P1-E latency 对齐（trunk/root/cluster 冗余级合并，级数 ≤8 目标在此）

---

## 2026-06-13: P1-D 自适应 skew target 与优化器通路验证完成

**Date**: 2026-06-13
**Task**: 06-11-per-branch-skew-model（已完成）
**Branch**: `cts_refactor`

### Summary

新配置 `skew_period_fraction`（默认 0.006，0=关闭）实现 per-clock 自适应 skew target `min(skew_bound, fraction×period)`，`ResolveClockTargetSkewNs` helper 统一解析（spec 钉契约：禁止直接读 skew_bound 当停止条件）。报告新增 Setup 三行（bound/fraction/rule）与 per-clock Clock Target 表（period/target/derivation）。R1（per-branch FastSTA）以 P1-C 证据 + 探针 late/early 分类（104/56）确认成立，无代码。

### Key Results (vga_lcd, 内部口径)

默认 run：target 0.08→0.06（period 推导，对标 Innovus 0.061），QoR 与 P1-C 基线逐位一致（skew 0.0294/buffer 6025/WL 101792.444），target_met=true 为真实达标。探针 run（fraction=0.002→target 0.02）：优化器真实求解（288 scored edits、8 batch trials 全拒）→ **sizing-only 边界实证**：均一级数+同 master 下剩余 29.4ps 来自 per-branch 线长离散，插入/蛇形挂起待统一 eval。R4 tolerance 扫描 {0.02, 0.1, 10}：**保留默认 0.1**（tol=10 skew +34.7% 仅省 WL 0.02%；tol=0.02 双输且 Pareto 收窄 649→444）——balanceTopology 挪点在 depth-9 下仍有正收益，设计预期被数据否定。

### Git Commits

| Hash | Message |
|------|---------|
| `ebbb14462` | feat(iCTS): derive per-clock skew target from clock period |

### Testing

- [OK] icts_test_flow_optimization 5/5（新目标，helper 语义）+ ConfigTest 新增 4 用例
- [OK] 全量 iCTS ctest 17/17（原 16 + 新 1）
- [OK] trellis-check PASS（clang-format 干净、check.py 触及文件 0 发现）

### Status

[OK] **Completed**

### Next Steps

- P1-E latency 对齐（trunk/root/cluster 冗余级合并，级数 ≤8 + latency_avg ≤0.52 目标）
- 全部任务完成后统一 eval 重跑（Innovus route+STA），P1-D eval 口径条款在彼时回填

---

## 2026-06-13: P1-E delay-margin 有界选型完成,级数 10→8

**Date**: 2026-06-13
**Task**: 06-11-latency-align（已完成）
**Branch**: `cts_refactor`

### Summary

新配置 `selection_delay_margin`（默认 0.07,0=完全回退旧中位）把三处 delay-power Pareto 中位选型（trunk `SelectBestSegmentEntry`、per-depth `SelectBestHTreeChar`、全局决定点 `SelectBestGlobalEntry`）统一替换为「(1+margin)×front-min delay 界内取 min-power」（共享 header-only 模板 `SelectionPolicy.hh`）。margin 经内部 Config 字段（默认 0=legacy）从 icts::Config 在 flow 装配点接入,fixture 路径零分叉。可观测性:HTree Global Selection 表 + trunk selection policy/margin/buf_count 字段。spec 钉 bounded-selection 契约。结构性合并（root/trunk、cluster/末级）经研究+数据判定不立项（research/structure-levels.md Q2/Q3、O3）。

### Key Results (vga_lcd, 内部口径, margin 扫描 {0,0.05,0.07,0.08,0.1,0.2,0.5})

**级数 10→8**（trunk 2→0:0-buffer 直驱条目在 strict 过滤中本就幸存,旧中位策略系统性放弃,margin>0 即胜出——P1-C 移交的级数 ≤8 条款达成）。默认 0.07=拐点:internal max arrival 0.5564→0.4486（**-19.4%**,外推 eval≈0.503≤0.52 目标）、skew 0.0294→**0.0279**（改善）、面积 +2.48%（0.05 要 +5.01% 同等延迟）、char power +2.4%（≤3% 上限）、WL +0%。htree 端 min-delay 仍 5 buffered levels=可行域下限（顶部 fanout 翻倍 ≤2 连续跳层+底部 split ≤max_fanout²）;全 margin 全局赢家 depth 9（split 代价盲视不影响 depth 决策）;root master BUFX20 饱和（R3 结构改造挂起待 eval 残差）。注意:最长 clock net 434.5→506.7um（trunk 直驱整段,cap/slew 合法;max_length=300 不实施为 P0-A 已记录存量缺陷）。

### Git Commits

| Hash | Message |
|------|---------|
| `64f90307c` | feat(iCTS): replace pareto-median selection with delay-bounded min-power policy |

### Testing

- [OK] SelectionPolicyTest 6/6（新,挂 icts_test_flow_synthesis_htree）+ ConfigTest 4 用例
- [OK] 全量 iCTS ctest 17/17;margin=0 与基线逐位一致（Gate-1）;无键默认 run 与显式 0.07 数值一致
- [OK] trellis-check PASS（自修 SelectionPolicy.hh 2 处 clang-tidy,修后二进制复跑确定性指标逐位一致）

### Status

[OK] **Completed**

### Next Steps

- 父任务 06-11-align-commercial-cts 全部 5 子任务完成,统一 eval 重跑（Innovus route+STA）回填 P1-D/P1-E 的 eval 口径条款
- 候选后续:多设计 margin 标定、max_length 实施、split 代价入选型、root 段结构改造（均待 eval 残差定位）

---

## Session 79: Fix GCC11 all target clean build

**Date**: 2026-06-13
**Task**: Fix GCC11 all target clean build
**Branch**: `cts_refactor`

### Summary

Removed stale iSTA/vectorization link dependencies from active CMake targets, replaced obsolete global iSTA CMake include with target-local dependencies, and verified clean GCC 11 all/ecc_bin builds.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `ea68ca2ea` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 80: CTS commercial alignment cleanup

**Date**: 2026-06-15
**Task**: CTS commercial alignment cleanup
**Branch**: `cts_refactor`

### Summary

Aligned CTS timing RC, wirelength grid, H-tree topology, sink split, and adaptive selection; created final diff cleanup task, verified targeted tests and full iCTS ecc_dev check, then archived the cleanup task.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `6f85baed3` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 81: Fix small-clock CTS HTree failure

**Date**: 2026-06-16
**Task**: Fix small-clock CTS HTree failure
**Branch**: `cts_refactor`

### Summary

Fixed depth-0 downstream HTree handling for small clustered clock trees, verified ascon and s1488 local CTS repros, and archived the Trellis task.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `f22f5cfea` | (see git log) |
| `1b7b66286` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 82: CTS capability alignment cleanup

**Date**: 2026-06-20
**Task**: CTS capability alignment cleanup
**Branch**: `cts_refactor`

### Summary

Cleaned CTS alignment task artifacts, retained algorithmic fast clustering Pareto split-axis and uniform-depth local split packing changes, staged and validated iCTS checks, then committed and archived the task.

### Main Changes

(Add details)

### Git Commits

| Hash | Message |
|------|---------|
| `c76529151` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 83: Archive unarchived Trellis tasks

**Date**: 2026-06-20
**Task**: Archive unarchived Trellis tasks
**Branch**: `cts_refactor`

### Summary

Archived all remaining unarchived task directories under .trellis/tasks/. No active tasks remain after cleanup.

### Main Changes

(Add details)

### Git Commits

(No commits - planning session)

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete


## Session 84: iCTS iRT-style systematic refactor

**Date**: 2026-07-30
**Task**: iCTS iRT-style systematic refactor
**Branch**: `cts_refactor`

### Summary

Refactored iCTS to the global DataManager and iRT-style Logger/Monitor architecture, aligned directory and CMake ownership, restored dense CTS/report tables, and minimized global specs; final iCTS tests, ICS55 binary/QoR acceptance, and ecc dev tools checks passed.

### Git Commits

| Hash | Message |
|------|---------|
| `a9d71dc3a` | (see git log) |

### Status

[OK] **Completed**
