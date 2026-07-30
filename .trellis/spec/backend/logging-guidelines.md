# Logging and Monitoring Guidelines

Runtime logs, resource measurements, dense tables, and report mirroring for iCTS.

## Logger Contract

Use the iRT-style `CTSLOG` singleton from `source/toolkit/logger/`.

| Call | Meaning |
| --- | --- |
| `CTSLOG.info(Loc::current(), ...)` | Normal milestones, summaries, and bounded algorithm evidence |
| `CTSLOG.warn(Loc::current(), ...)` | Recoverable degradation, skip, or typed failure |
| `CTSLOG.error(Loc::current(), ...)` | Terminal invariant or infrastructure failure; logs, closes the file, and exits |

- Do not use glog `LOG_*` macros in iCTS.
- Do not add a second logger, compatibility macro, detail log, runtime log, or independent report sink for CTS runtime information.
- Outside the logger implementation, do not write runtime messages with `std::cout`, `std::cerr`, `printf`, or `fprintf`.
- Pass `Loc::current()` at the call site and include the object, state, or value needed to diagnose the message.

## Lifecycle

- `CTSAPI::init` initializes `Logger` before `DataManager`.
- Logs emitted before the file is opened are buffered.
- `DataManager::input` opens exactly `<work_dir>/cts.log` after output directories are ready; opening truncates the previous file and flushes buffered startup logs.
- Logger destruction/reset closes the file. Do not manage the stream from modules.
- The console may use colored levels; `cts.log` must remain plain text without ANSI sequences.

## Information Density

- Log stage start/completion, result summaries, resource usage, recoverable decisions, and the algorithm evidence needed to understand CTS behavior.
- Use `EmitLogTable(...)` for dense multi-field information such as config, unit/RC data, clock distribution, selected H-tree candidates, optimization evolution, committed results, and report artifacts.
- Algorithm tables must be bounded and decision-oriented. Do not emit per-net, per-sink, per-sample, or per-trial noise unless it is the bounded candidate set used to make the recorded decision.
- Do not maintain separate normal/detail semantics. `cts.log` contains the single necessary runtime record.
- Build rows near the data owner; orchestration facades decide when the table is emitted.

`LogTable` is stateless infrastructure: render with `RenderLogTable(...)`, then emit the rendered text with `EmitLogTableText(...)` or render-and-emit with `EmitLogTable(...)`. It must not own business state, file lifecycle, or another sink.

## Report Mirroring

When a report table is also runtime evidence, render one canonical table body and use the same body for the `.rpt` file and `CTSLOG`. `cts_report` therefore writes report artifacts and prints their table bodies to both the command line and `cts.log`; do not create independently formatted log/report versions.

## Monitor Contract

- Use a stack-local `Monitor` at lifecycle or stage boundaries.
- Append `monitor.getStatsInfo()` to completion or failure summaries when elapsed time, process CPU time, and peak-memory delta are useful.
- Each `getStatsInfo()` call advances that monitor's baseline; do not make `Monitor` global or use it as a metrics store.
- Failure to sample required system statistics is terminal through `CTSLOG.error`.
