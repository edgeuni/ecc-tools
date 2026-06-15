# Design

This is a repository hygiene task, not a CTS algorithm task.

## Scope Boundary

- Source of truth for final code review: full diff from the remote merge-base.
- Source of truth for commit contents: staged diff.
- Spec policy: the branch's final diff against remote must not change
  `.trellis/spec/`.
- Asset policy: keep compact evidence reports, but leave raw experiments,
  scripts, manifests, and row-level trace data unstaged.

## Data Flow

```text
origin/cts_refactor -> full branch diff -> cleanup checklist -> staged commit scope -> validation
```

## Risk Controls

- Check full diff and staged diff separately.
- Use `--no-renames` when counting added/deleted files to avoid Git rename noise.
- Validate both code behavior and quality gate after the cleanup artifacts are
  staged.
