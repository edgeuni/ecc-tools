# Quality Guidelines

Naming, C++ structure, dependency visibility, and validation rules for `src/operation/iCTS/`.

## Naming

- Classes use `PascalCase`; scoped-enum values use `kPrefix`; members use `_lower_case`; namespaces use lowercase.
- Use `snake_case` for direct accessors and simple predicates; use `camelBack` for computed or multi-step behavior.
- Name types and operations after their CTS or physical-design responsibility. Avoid generic standalone names such as `Internal`, `Support`, `Request`, `Response`, `Types`, or `Session`.
- Stable stage contracts use module-qualified names such as `{Stage}Input`, `{Stage}Config`, `{Stage}Output`, and `{Stage}Summary`; narrow private helpers may use shorter names.
- Do not infer CTS behavior from object-name substrings.

## C++ and Includes

- Define iCTS code in `namespace icts` or a responsibility-qualified child namespace; keep anonymous namespaces inside the active named namespace.
- Do not use whole-namespace imports. Prefer explicit qualification or narrow symbol-level `using` declarations.
- Keep every header self-contained. Prefer forward declarations for pointer/reference-only dependencies and keep implementation-only includes in `.cc` files.
- Do not use `../` include traversal; fix the owning target's include interface and use rooted includes.
- In `.cc` files, include the corresponding header first, then standard-library headers, then project/third-party headers, with blank lines between groups.
- Keep declarations and definitions aligned when changing an interface.

## CMake Dependencies

- Express dependencies with `target_link_libraries`; do not recreate an existing target's include path.
- Default links to `PRIVATE`. Use `PUBLIC` when a public header exposes the dependency and `INTERFACE` for header-only or aggregation targets.
- Avoid architectural cycles; an object library may solve archive composition but must not conceal a source-level cycle.
- When splitting a translation unit, give each new `.cc` only the includes and links required by its own symbols.

## Quality Gate

- Build and run the tests relevant to the changed responsibility.
- Run any task-defined functional or binary acceptance before the final static-quality pass.
- At final handoff, run one full-module check:

```bash
python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS
```

- Do not run concurrent checker instances that share a build directory.
- Do not add `NOLINTNEXTLINE`, broad suppressions, or include-path workarounds to silence findings; fix the owning code or target.
- Classify out-of-scope findings without modifying unrelated code. Rerun the full-module check until in-scope findings are clean.
