# Backend Development Guidelines

Applies to `src/operation/iCTS/`. All spec documents are written in English.

## Authorities

| Document | Authority |
| --- | --- |
| [Project Constraints](../project-constraints.md) | Trellis/spec governance, repository process, file rules, approved exceptions |
| [Directory Structure](./directory-structure.md) | Layer placement, facades, and CMake ownership |
| [Data Manager Guidelines](./database-guidelines.md) | Session state, ownership, commits, and external boundaries |
| [Logging and Monitoring](./logging-guidelines.md) | `CTSLOG`, `Monitor`, tables, and report mirroring |
| [Error Handling](./error-handling.md) | Typed recovery and terminal failure |
| [Quality Guidelines](./quality-guidelines.md) | Naming, includes, dependencies, and quality gate |

## Pre-Development

- Read Project Constraints and each authority touched by the proposed change.
- For cross-layer or reuse-sensitive work, use the relevant [thinking guide](../guides/index.md).
- Resolve conflicts with current code or task assets before editing; do not silently reinterpret the spec.

## Quality Check

- Verify the change against every authority it touches.
- Run the task acceptance and final quality gate defined in Quality Guidelines.
