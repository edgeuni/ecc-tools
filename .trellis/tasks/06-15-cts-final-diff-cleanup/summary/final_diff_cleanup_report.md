# Final Diff Cleanup Report

## Baseline

| Item | Value |
| --- | --- |
| Remote baseline | `origin/cts_refactor` |
| Merge base | `d080c76a85f2d03f22010df6e6d695040e2e4739` |
| Current local HEAD | `9822169b39e90b8084208aa0427634e609047d61` |
| Current task | `06-15-cts-final-diff-cleanup` |

## Final Branch Diff

The final review scope is the full branch diff from the remote merge-base, not
only staged changes.

| Scope | Files | Insertions | Deletions |
| --- | ---: | ---: | ---: |
| Code: `src/operation/iCTS` + `cts_default_config.json` | 60 | 2493 | 343 |

## Cleanup Results

| Item | Result |
| --- | --- |
| `.trellis/spec/` final branch diff | Empty |
| Staged files | 124 |
| Staged bytes | 1,150,542 |
| Raw `row_matches` staged | No |
| `run_manifest.json` staged | No |
| `research/` staged | No |
| `scripts/` staged | No |
| Unstaged tracked files | None |

## Interface And Behavior Summary

| Area | Final state |
| --- | --- |
| FastSTA RC/timing | Native ground/coupling/driver parasitic fields are retained; RC resistance unit bug is fixed; driver effective PI and physical PI stay separate. |
| Wrapper RC query | `WireCapacitanceProfile` replaces vague `Breakdown` naming; profile APIs expose area, edge, ground, coupling, total, and timing-effective cap. |
| Config surface | User-facing hardcode knobs are removed; old names remain deprecated-only warnings. |
| Optimization skew | Target skew resolves from `skew_bound`; unused `Clock*` argument is removed. |
| Wirelength grid | Direct characterization lengths and coverage lengths are handled separately to reduce DBU/grid distortion. |
| H-tree topology | Topology generation supports bounded k-ary branching from max leaf load count instead of assuming binary shape everywhere. |
| Sink-load split | Local split remediation has one legality plan and one materialization path. |
| Global selection | Adaptive timing/physical selection records `GlobalSelectionDecision` and accounts for split buffers and physical depth. |
| Deleted legacy interfaces | Old FastSTA report adapter and old delay-margin selection helper are removed from the final code surface. |

## Generalized Review Rules

- Always compare against `git merge-base HEAD @{u}` during final cleanup; staged-only review can miss local committed changes.
- Review with `--no-renames` when classifying file additions/deletions; Git rename detection can hide delete+add cleanup.
- Treat electrical model constants as named model assumptions, not user-facing tuning parameters.
- Keep legality evaluation and materialized CTS objects driven by the same data shape.
- Any code assuming binary H-tree multiplicity must be rechecked after k-ary topology support.

## Validation

Completed validation:

- targeted compile/tests passed
- full `python3 ./.trellis/ecc_dev_tools/check.py check --path
  src/operation/iCTS` passed with 0 in-scope findings

Residual note:

- `ecc_dev_tools` reports out-of-scope diagnostics from external database/parser
  headers. They are not in the `src/operation/iCTS` in-scope result and were not
  changed for this cleanup task.
