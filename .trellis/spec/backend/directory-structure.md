# Directory Structure

Code placement and CMake boundaries for `src/operation/iCTS/`.

## Layers

| Layer | Directory | Responsibility |
| --- | --- | --- |
| Interface | `interface/` | External `CTSAPI` entry points and public status/value types |
| Source | `source/` | Data ownership, algorithms, orchestration facades, and shared infrastructure |
| Test | `test/` | Tests organized by the source responsibility they verify |

`interface` depends on `source`; `source` must not depend on `interface`. Tests may depend on both.

## Source Categories

Only these peer categories belong directly under `source/`:

| Category | Responsibility |
| --- | --- |
| `data_manager/` | Global CTS session, config, owned design data, stage results, and external adapters |
| `module/` | CTS algorithms and the synthesis, optimization, instantiation, evaluation, and output facades |
| `toolkit/` | Reusable infrastructure without CTS session or stage ownership |

Do not reintroduce `api/`, `database/`, `flow/`, or `utils/` as parallel top-level architectures.

## Execution Boundaries

The public flow is:

```text
CTSAPI::init -> DataManager::input
CTSAPI::runCTS -> Synthesis -> Optimization -> Instantiation -> Evaluation
CTSAPI::report -> Output
```

- `CTSAPI` coordinates public lifecycle and stage order; it does not implement algorithms.
- `DataManager` owns session state and validated stage commits; it does not implement stage algorithms.
- Stage facades live under their corresponding `source/module/<stage>/` directory.
- Lower-level algorithm code receives explicit inputs or local models from its facade.
- Report generation and visualization live under `source/module/output/`.

Moving files alone does not satisfy a layer change. Ownership, call direction, target dependencies, and public contracts must move with the business responsibility. Do not retain compatibility forwarding layers or parallel old/new abstractions after a completed refactor.

## Placement

- Put external entry points and public API status/value types in `interface/`.
- Put session-owned or cross-stage data in the narrowest `source/data_manager/` subdirectory.
- Put behavior and algorithms in the narrowest `source/module/` subdirectory.
- Put stateless or lifecycle-independent infrastructure in `source/toolkit/`.
- Put tests under `test/data_manager/`, `test/module/`, or `test/toolkit/` according to the responsibility under test.
- Keep raw iDB, SDC, and Liberty access inside `source/data_manager/io/` or `source/data_manager/adapter/`.

Behavior directories expose their intended facade at the directory root; implementation helpers stay in responsibility subdirectories. Callers outside the behavior directory include the facade, not internal helper headers. Data-model directories may expose multiple domain-object headers.

## CMake

- Target names follow the directory hierarchy: `icts_interface`, `icts_source`, and `icts_source_<category>[_<module>]`.
- Every directory with buildable children owns their `add_subdirectory()` wiring.
- Use a real library for `.cc` implementation and `INTERFACE` only for header-only or aggregation targets.
- Express dependencies with `target_link_libraries`; do not duplicate include paths to bypass target ownership.
- Default dependencies to `PRIVATE`; use `PUBLIC` or `INTERFACE` only when the public header contract requires propagation.
