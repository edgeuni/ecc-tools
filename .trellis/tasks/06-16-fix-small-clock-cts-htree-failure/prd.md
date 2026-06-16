# 修复小规模时钟 CTS HTree 失败

## Goal

Fix ECC/iCTS so the 2026-06-16 benchmark cases `ascon` and `s1488` can complete CTS instead of aborting during downstream HTree topology construction.

## Confirmed Facts

- The failing benchmark assets live under `/nfs/share/home/liweiguo/auto_run_benchmark/output/20260616/`.
- Original case inputs are shared assets under `/nfs/share/home/liweiguo/innovus_place_result/{case}/`.
- Asset and benchmark directories must be treated as read-only for debugging. Any repro output, generated config, or temporary artifact must stay inside `/home/liweiguo/project/ecc-tools`.
- Both failing ECC CTS runs exit with code `134`.
- Both runs stop before producing `cts.def` or `cts.v`, causing `icts_convert` and `eval_icts` to be skipped.
- Both `cts_detail.log` files end at `Topology Build downstream HTree` with `status = failed` and `reason = unknown_h_tree_failure`.
- `ascon` has clock `clk`, clock net `clk`, and `sink_count = 5`.
- `s1488` has clock `CK`, clock net `CK`, and `sink_count = 6`.
- Innovus CTS succeeds for both cases, so the inputs are expected to be CTS-runnable.

## Requirements

- Reproduce or otherwise isolate the ECC/iCTS failure using project-local output paths.
- Preserve the input asset directories and benchmark output directory as read-only.
- Fix the CTS topology behavior so low-sink-count HTree cases do not abort with `unknown_h_tree_failure`.
- Keep the fix scoped to the CTS implementation and directly related tests/configuration.
- Preserve existing CTS behavior for normal larger clock trees unless the current behavior is demonstrably wrong.
- Record enough validation evidence for `ascon` and `s1488` to show the fix addresses the reported benchmark failure.

## Acceptance Criteria

- [x] A project-local repro for `ascon` no longer exits `134` at downstream HTree construction.
- [x] A project-local repro for `s1488` no longer exits `134` at downstream HTree construction.
- [x] Repro output for both cases includes generated CTS output expected by the benchmark flow, or the remaining failure is a different documented downstream issue.
- [x] The fix is covered by a focused automated test, or by a documented reason why the available test harness cannot cover it directly.
- [x] Relevant existing CTS tests or build checks pass.
- [x] No files under `/nfs/share/home/liweiguo/innovus_place_result` or `/nfs/share/home/liweiguo/auto_run_benchmark/output/20260616` are modified.

## Validation Evidence

- `bin/icts_test_flow_synthesis_htree --gtest_filter=HTreeTest.DegenerateTopologyBuildsDirectRootLoads:HTreeTest.SingleLoadBuildsTrivialTopology:HTreeTest.EmptyLoadsReturnsEmptyResult:HTreeTest.MissingRootDriverStopsBeforeTopology` passed 4 tests.
- `bin/icts_test_flow_synthesis_htree` passed 28 tests.
- `ctest --test-dir build -R '^icts_test_flow_synthesis_htree$' --output-on-failure` passed.
- `python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS` passed with 0 in-scope findings.
- Local `ascon` run under `tmp/cts-debug/06-16-fix-small-clock-cts-htree-failure/ascon` exited 0 and generated `cts.def` and `cts.v`.
- Local `s1488` run under `tmp/cts-debug/06-16-fix-small-clock-cts-htree-failure/s1488` exited 0 and generated `cts.def` and `cts.v`.
- Both local repro logs show downstream HTree finished with `selected_depth=0`, `inserted_insts=0`, and `inserted_nets=0`; stdout records `reason=direct_root_loads`.

## Out of Scope

- Changing benchmark asset inputs.
- Tuning CTS quality metrics beyond what is needed to complete these cases.
- Modifying Innovus evaluation outputs.
- Re-running the full 88-case benchmark matrix unless needed for regression confidence.

## Open Questions

- None blocking at task creation. Repository inspection should determine the exact failing code path before any user-facing scope decision is needed.
