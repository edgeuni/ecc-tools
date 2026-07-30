# Cross-Layer Thinking Guide

Use when a change crosses interface, data-manager, module, toolkit, or external-adapter boundaries.

For each boundary, identify:

- the exact input/output types and units;
- ownership, borrowing lifetime, and reset behavior;
- the only layer allowed to validate or mutate the data;
- recoverable status versus terminal invariant behavior;
- where necessary runtime evidence is logged.

Verify the complete path before implementation:

```text
external data -> DataManager/adapter -> stage facade -> local model
              -> validated commit -> output/report
```

Reject raw external pointers escaping adapters, `CTSDM` access below approved facades, borrowed pointers crossing reset/commit replacement, and conversions whose units or failure ownership are implicit.

Authorities: [Directory Structure](../backend/directory-structure.md), [Data Manager Guidelines](../backend/database-guidelines.md), [Logging](../backend/logging-guidelines.md), and [Error Handling](../backend/error-handling.md).
