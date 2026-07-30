# Error Handling

Recoverable status propagation and terminal failure rules for `src/operation/iCTS/`.

## Decision Contract

| Condition | Required behavior |
| --- | --- |
| Normal or no-op result | Return the appropriate success/no-op status; use `CTSLOG.info` when it is operationally relevant |
| Recoverable skip, degradation, or stage failure | Use `CTSLOG.warn` and return the owning typed status/summary |
| Invalid `DataManager` transition or non-committable local result | Return `DataManagerStatus`; do not mutate committed state |
| Missing invariant where continuation is unsafe or misleading | Use `CTSLOG.error`; it is terminal |

`CTSLOG.error` is `[[noreturn]]`. Do not use it for a path whose caller is expected to recover.

Use the existing boundary type that owns the failure:

- `CTSStatus` / `CTSStatusCode` for public API results.
- `DataManagerStatus` / `DataManagerStatusCode` for input, state, and commit results.
- The module's existing outcome or summary type for local stage results.

Do not replace typed failures with sentinel data, a success-shaped empty object, or a log-only failure. A warning supplies diagnostic context; the returned type carries control flow.

## Algorithm Preconditions

Topology, RC, timing, sizing, legality, and QoR decisions require valid DBU, routing-layer, adapter, and Liberty state. Validate these at the first boundary that owns the requirement. Do not mask missing infrastructure with zero RC, `routing_layer = 0`, `dbu = 0`, or denominator clamping.

Fallible query helpers may return an unavailable result for report/probe callers; an algorithm caller must convert that result to its typed stage failure or a terminal invariant before using it.

## Exceptions and Termination

- Do not use exception-based control flow in iCTS.
- The existing `source/data_manager/config/Config.cc` JSON conversion/parsing catches may remain; do not copy that exception pattern elsewhere.
- Do not call `exit`, `_Exit`, `abort`, or `terminate` outside the `Logger` implementation.
- Do not use `assert` for user-visible runtime validation.
- Log a failure once at the boundary that decides its disposition; downstream callers propagate the status without repeating the same invariant message.
