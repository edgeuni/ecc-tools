# iLVS: Layout Versus Schematic

## Background

iLVS is the post-route LVS operation in iEDA. It compares a reference
Verilog-derived netlist snapshot with a routed DEF-derived snapshot after
routing and DRC. Until IDB can retain both views concurrently, the flow uses
three independent processes: one writes the netlist snapshot, one writes the
DEF snapshot, and one reads both snapshots and reports LVS results.

## Software Structure

### API: iLVS Tcl and C++ interfaces

The interface owns the `init_lvs`, snapshot write/read, `run_lvs`, and
`destroy_lvs` lifecycle. It initializes the data manager and iLVS modules,
then connects the snapshot, checking, and reporting stages.

### Data Manager: Top-level data manager

The data manager owns configuration, the two value snapshots, the check
result, report output paths, and the per-module temporary directories.

### Module: Main LVS modules

- NetlistExtractor: Extracts a logical or physical value snapshot from the
  current single IDB design view.
- LVSSnapshotIO: Writes and validates versioned logical and physical binary
  snapshots.
- LVSChecker: Compares entities and checks routed-net and power connectivity.
- LVSReporter: Produces the console, RPT, and JSON LVS reports.

### Utility: Tool modules

- Logger: Log module.
- Monitor: Runtime status monitor.
- Utility: Configuration, filesystem, and table helpers.
