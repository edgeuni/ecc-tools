# Project Constraints

Mandatory project-wide rules for `src/operation/iCTS/`.

## Trellis Asset Governance

- After a task's definition assets—including its PRD, design, implementation plan, context manifests, reviewed research/decision records, and applicable specs—have been reviewed and the task is active, treat them as frozen development inputs.
- Do not edit approved Trellis assets during implementation to track progress, match the implementation, broaden scope, or justify a decision after the fact.
- If an approved asset must change, first present the exact files and clauses, reason, and impact. Edit only after explicit user review and approval.
- Update generated task status and runtime state only through the Trellis lifecycle commands; do not hand-edit them as task-definition content.

## Spec Governance

- Specs contain only stable, reusable, project-wide, actionable development contracts.
- Allowed content: architecture and dependency boundaries; public or cross-layer contracts; ownership, lifecycle, error, naming, build, test, and quality rules; explicitly approved narrow exceptions.
- Excluded content: task plans or status, migration narration, implementation history, temporary diagnostics, validation results, one-off design choices, and source inventories or behavior evident from direct code inspection.
- Examples and signatures must match current code unless explicitly marked as an approved normative contract.
- Before changing `.trellis/spec/**`, present the exact scope, reason, and impact and obtain explicit user approval. Editing first and notifying afterward is not approval.
- Make the smallest sufficient change. Keep each rule in one authority document; indexes and guides link to it instead of duplicating it.

## Repository Process

- AI agents must not run `git push`.
- Git mutations require explicit user authorization; use read-only Git commands otherwise.
- Every exception must name the exact path or pattern, relaxed rule, and allowed scope.

Approved exceptions:

| Path | Relaxed rule | Allowed scope |
| --- | --- | --- |
| `src/operation/iCTS/test/main.cc` | PascalCase file name | Existing GoogleTest entry point only |
| `src/operation/iCTS/source/data_manager/config/Config.cc` | No-exception policy | Existing JSON conversion/parsing catches only |

## Files and Naming

- Use `.hh` for headers and `.cc` for sources; do not add `.h`, `.hpp`, `.cpp`, `.cxx`, or `.c` files in iCTS.
- File names use PascalCase; acronyms remain uppercase, such as `CTSAPI.hh`, `FLUTE.cc`, and `CBS.cc`.
- Every header uses `#pragma once`.

Every new `.hh` and `.cc` file starts with:

```cpp
// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
```

Immediately follow it with:

```cpp
/**
 * @file FileName.hh
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date YYYY-MM-DD
 * @brief One-line description of this file's responsibility.
 */
```

## Scope and Terminology

- Format touched code with the repository `.clang-format`.
- Keep changes to external modules such as iSTA or iPA minimal and free of unrelated cleanup; report those diffs for user review.
- Use established iCTS terms: `inst`, `net`, `pin` (except real top-level IO), `cell_master`, `dbu`, `loads`, `clock_source`, `inserted_insts`, and `inserted_nets`.
