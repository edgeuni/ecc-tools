# Implementation Plan

## Checklist

- [x] Load relevant backend specs before editing.
- [x] Inspect benchmark logs and generated configs for `ascon` and `s1488`.
- [x] Locate the `unknown_h_tree_failure` source and downstream HTree builder.
- [x] Build or identify the runnable ECC binary in this checkout.
- [x] Create project-local repro configs and output directories.
- [x] Reproduce the failure or capture enough stack/log evidence to identify the defect.
- [x] Patch the HTree/topology code with the narrowest compatible behavior.
- [x] Add or update focused tests where feasible.
- [x] Re-run local repros for `ascon` and `s1488`.
- [x] Run relevant build/test/quality checks.
- [x] Confirm read-only asset directories are not modified.

## Outcome

The failing cases were caused by treating a depth-0 HTree topology as an error. For `ascon` and `s1488`, sink clustering reduces the downstream HTree input to two local buffers. With max fanout 4, `TopologyGen` correctly produces a single-node direct-root-load topology. `AssembleHTreeSynthesisState` now marks that topology as a completed CTS result with `selected_depth=0` and no inserted objects instead of returning `no_h_tree_levels`.

## Candidate Commands

```bash
python3 ./.trellis/scripts/task.py start .trellis/tasks/06-16-fix-small-clock-cts-htree-failure
rg "unknown_h_tree_failure|downstream HTree|HTree|h_tree" -n .
cmake --build <build-dir> --target ecc_bin
```

Exact build and test commands should be selected after inspecting the repository build layout.

## Risk Points

- The benchmark binary path points to `/nfs/.../tools/ecc-tools/bin/ecc_bin`; validation should prefer this checkout's binary once rebuilt.
- The failure may occur only with full technology assets, so pure unit tests may need to be supplemented with local integration repros.
- HTree fallback behavior must not silently weaken normal large-clock CTS behavior.

## Review Gate Before Start

- `prd.md`, `design.md`, and this implementation plan exist.
- The user already requested task creation and debugging, so implementation can start after this planning gate.
