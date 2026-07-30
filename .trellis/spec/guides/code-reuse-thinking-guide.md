# Code Reuse Thinking Guide

Use before adding a helper, utility, adapter, or CMake target.

- Search the owning module, `source/toolkit/`, `source/data_manager/`, tests, and existing targets for the same responsibility.
- Keep single-use behavior local unless extraction clarifies an ownership boundary.
- Extract behavior when independent consumers need the same non-trivial contract or duplicate implementations could drift.
- Put generic infrastructure in `toolkit`; keep CTS state and domain data in `data_manager`; keep algorithm behavior in `module`.
- Reuse targets through `target_link_libraries`; do not duplicate include roots or wrap an existing target only to rename it.
- After a migration, remove superseded helpers and compatibility paths rather than preserving parallel semantics.

Authorities: [Directory Structure](../backend/directory-structure.md), [Data Manager Guidelines](../backend/database-guidelines.md), and [Quality Guidelines](../backend/quality-guidelines.md).
