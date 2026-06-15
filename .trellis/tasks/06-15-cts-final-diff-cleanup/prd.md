# CTS final diff cleanup

## Goal

Close the CTS commercial-alignment work against the remote branch, not only the
staged diff. The final handoff must explain and verify every remaining code
delta from `origin/cts_refactor`, including local commits that have not been
pushed and the current staged cleanup changes.

## Confirmed Facts

- Remote baseline: `origin/cts_refactor`
- Current branch: `cts_refactor`
- The previous staged-only cleanup missed one already-committed local spec edit.
- The final branch diff must not contain `.trellis/spec/` changes.
- Experimental assets, raw rerun rows, scripts, and run manifests should remain
  local-only and outside staged changes.
- Final validation must include targeted build/tests and one full
  `src/operation/iCTS` `ecc_dev_tools` check.

## Requirements

- Compare final code against the remote merge-base with:
  `git diff $(git merge-base HEAD @{u})`.
- Create a concrete cleanup checklist and mark each completed item.
- Keep algorithm code unchanged unless a checklist item exposes a real code
  defect.
- Remove or neutralize non-global spec changes from the final remote diff.
- Keep final staged assets compact and exclude raw experimental material.
- Produce a full-diff cleanup report with interface, behavior, risk, and
  validation notes.
- Re-run compile/tests and full `ecc_dev_tools` validation after cleanup.

## Acceptance Criteria

- [ ] Task artifacts contain the final-diff cleanup checklist.
- [ ] Full diff against `origin/cts_refactor` is reviewed, not just staged diff.
- [ ] Final diff against `origin/cts_refactor` contains no `.trellis/spec/`
      changes.
- [ ] Staged changes contain no raw `row_matches`, `run_manifest.json`,
      `research/`, or `scripts/` paths.
- [ ] Interface/name cleanup is summarized with any remaining risk called out.
- [ ] Targeted compile/tests pass.
- [ ] Full `python3 ./.trellis/ecc_dev_tools/check.py check --path
      src/operation/iCTS` reports zero in-scope findings.

## Out Of Scope

- No new CTS algorithm tuning.
- No new evaluation rerun.
- No commit or push in this task.
- No changes to global development specs except the staged reverse cleanup that
  restores an earlier local spec edit back to the remote version.
