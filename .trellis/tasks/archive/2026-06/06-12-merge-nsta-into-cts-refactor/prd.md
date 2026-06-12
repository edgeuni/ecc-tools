# PRD: merge nSTA into cts_refactor

## Requirements
- Merge `origin/nSTA` into current branch `cts_refactor`
- **Conflict resolution**: all conflicts resolved in favor of `cts_refactor` (HEAD / `--ours`)
- No commit after merge (user will review first)

## Acceptance Criteria
- [ ] `git merge origin/nSTA` completes successfully
- [ ] All conflicts resolved with `--ours` (cts_refactor version kept)
- [ ] Working tree is in merge state (not committed)
- [ ] Source files from both branches are correctly integrated
