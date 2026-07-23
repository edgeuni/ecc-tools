# iLVS Dual-Bin Snapshots

> User confirmed the three-process, no-overwrite design on 2026-07-22.

## Goal

Temporarily remove the post-route iLVS dependency on IDB holding both the
reference Verilog view and the routed DEF view at once.  Generate each view in
an independent iEDA process, persist only the iLVS value snapshot required for
comparison, then load the two snapshots in a third process and run LVS.

## What I Already Know

* IDB currently has one current design view.  `verilog_init` replaces the DEF
  view and `def_init` replaces the Verilog view.
* The current `init_lvs` implementation extracts the current Verilog view,
  calls `idm::DataManager::readDef`, and extracts the physical view.  This
  implicit second DEF read is the behavior to replace.
* The iLVS value type `Netlist` already contains all current checker and
  reporter inputs and contains no IDB pointer/reference.
* `init_lvs` must continue to accept only `-temp_directory_path` and
  `-thread_number`.
* The existing integration script is
  `/nfs/share/home/zengzhisheng/debug_workspace/script/lvs/run_ilvs_cx55.tcl`.

## Assumptions (Temporary)

* Two independent Tcl scripts will prepare a logical snapshot from a Verilog
  view and a physical snapshot from a DEF view respectively.  Each script is
  run as a separate iEDA process and exits after writing its bin.
* No iEDA process in this flow may call both `def_init` and `verilog_init`.
  The comparison script will initialize iLVS, load two supplied snapshot
  files, run LVS, and destroy iLVS without loading either IDB design view.
* A versioned, typed streaming binary format will be implemented without a new
  dependency.  It will preserve every `Netlist` field used by the current
  checker/reporter and no IDB state.

## Requirements (Evolving)

* Preserve the two-option `init_lvs` Tcl signature.
* Enforce process isolation: the logical writer runs only
  `tech_lef_init`/`lef_init`/`verilog_init`; the physical writer runs only
  `tech_lef_init`/`lef_init`/`def_init`; the comparator runs neither
  `verilog_init` nor `def_init`.
* Add a command that extracts the current Verilog-backed IDB design and writes
  a logical iLVS snapshot (`netlist` bin).
* Add a command that extracts the current DEF-backed IDB design and writes a
  physical iLVS snapshot (`def` bin).
* Add a command that reads both bins into the iLVS database before `run_lvs`.
* Remove the implicit `readDef`/IDB-view switch from `init_lvs`.
* Reject missing, truncated, incompatible-version, wrong-kind, and swapped
  snapshot files with clear iLVS errors before comparison.
* Add focused round-trip and invalid-input tests.
* Provide the two writer Tcl scripts and update the existing CX55 comparison
  script to consume their outputs.
* Build console and RPT summaries from the same `Entity` and
  `Connectivity` tables. The console prints each table as a separate vertical
  block in the same order as RPT. The former has exactly IO(without pg),
  Instance, and Net rows with NETLIST, DEF, and Difference columns. The latter has Connectivity, Type,
  and Count columns with Routing/Open Net, Routing/Short Net, Power/Open VDD,
  and Power/Open VSS rows, with no total row or RPT supply-point section.
  Build separate sorted vectors for both views:
  compare IO and instances by name, excluding power/ground IO ports, and
  compare nets by name plus their sorted terminal/pin sets. Instances come
  from the full IDB instance list without filtering by `is_netlist()`;
  master-cell names are not compared. RPT details include DBU shape
  coordinate tables; JSON mirrors the Entity/Connectivity rows and violation
  details only.
* Persist an independent routing-shape graph for every regular DEF net in the
  physical snapshot. It must retain the `IdbNet::get_driving_pin()` terminal,
  pin-port/wire/via shapes, via pairs, and terminal-to-shape mapping. Check
  driver-to-load reachability only inside that net-local graph with an OpenMP
  parallel net loop; never infer an Open result from a whole-design connected
  component. Detect Shorts separately from whole-design components with more
  than one distinct net name. For regular DEF path `RECT` segments, translate
  the IDB delta rectangle by its preceding path point before graph insertion.
* Persist power/ground special-net membership and routing shapes in the
  physical snapshot. For lazy DEF `(* VDD)` / `(* VSS)` connections, enumerate
  matching instance pins through the IDB lookup without materializing or
  changing IDB state. From the highest-layer elongated special-net tracks,
  choose VDD/VSS supply-point midpoints at opposite outer edges, searching
  inward from one edge when both outer tracks have the same supply type. Check
  every instance VDD/VSS pin against the matching anchor's global physical
  component and report disconnected pins with coordinates.
* Keep all CX55 bins, final report, JSON, and log under the single
  `lvs_temp_cx55` output root; process temporary directories must be children
  of that root so they cannot erase the snapshots.

## Acceptance Criteria (Evolving)

* [x] A Verilog-only iEDA invocation can produce a logical `.bin` without a
      DEF being configured.
* [x] A DEF-only iEDA invocation can produce a physical `.bin` without a
      Verilog being configured.
* [x] A third iEDA invocation can run `init_lvs`, load both bins, and run
      `run_lvs` without invoking `readDef`, `def_init`, or `verilog_init`.
* [x] The loaded snapshots preserve all current comparison/report fields.
* [x] Swapping the two bin paths or supplying a corrupted bin fails clearly.
* [x] Existing iLVS unit tests and new snapshot tests pass.
* [x] Console and RPT share vertically ordered Entity/Connectivity tables,
      with exactly IO(without pg)/Instance/Net NETLIST/DEF/Difference rows and
      four grouped Routing/Power connectivity rows, with no total row,
      statistics preamble, or RPT supply-point section.
* [x] RPT details render DBU coordinate tables for physical violation shapes.
* [x] The CX55 flow leaves only `lvs_temp_cx55` as its persistent output root.
* [x] The CX55 flow compares 421 instances and 30 non-supply IOs on both
      sides with no entity difference; the four DEF-only supply IO pins are
      excluded from IO equivalence.
* [x] Each regular DEF net is checked in a net-local routing graph from the
      selected driver to all load terminals; a second net cannot bridge an
      otherwise open route.
* [x] Every whole-design physical component containing more than one distinct
      net name reports a `RoutingShort`, with its net list and DBU coordinates.
* [x] CX55 regular-route `RECT` deltas are translated to physical coordinates;
      the resulting report has no artificial origin-coordinate short.
* [x] The physical v4 snapshot expands CX55 lazy `(* VDD)` / `(* VSS)`
      membership without mutating IDB, and retains instance supply-pin and
      special-route data for comparison.
* [x] CX55 selects a VDD anchor at `(31300, 10300)` and a VSS anchor at
      `(31300, 50300)` on the highest horizontal supply layer; all 421 VDD and
      all 421 VSS instance pins reach their matching anchor.

## Definition of Done

* Tests added or updated for binary round trip and validation failures.
* Relevant build targets compile and focused tests pass.
* Tcl flows are runnable from the existing debug workspace.
* No commit or push occurs until the user explicitly requests it.

## Technical Approach

### Tcl Commands

Keep the existing commands and add three commands:

```tcl
init_lvs -temp_directory_path <directory> -thread_number <integer>
write_lvs_netlist -path <netlist.bin>
write_lvs_def -path <def.bin>
read_lvs -netlist_bin_path <netlist.bin> -def_bin_path <def.bin>
run_lvs
destroy_lvs
```

`init_lvs` only initializes iLVS configuration, output directories, logging,
and an empty database.  It will not access IDB or call `readDef`.

### Three-Process Flow

```text
writer 1: tech/LEF -> Verilog -> init_lvs -> write_lvs_netlist -> exit
writer 2: tech/LEF -> DEF     -> init_lvs -> write_lvs_def     -> exit
compare:  init_lvs -> read_lvs(two bins) -> run_lvs -> exit
```

The two writer Tcl files will be separate scripts in the existing debug
workspace.  `run_ilvs_cx55.tcl` becomes the third, comparison-only script.
The snapshot output directory is separate from every `init_lvs` temporary
directory, because `init_lvs` deliberately recreates its temporary directory.

### C++ Boundary

* Split IDB extraction into logical and physical extraction entry points so a
  writer computes only the graph appropriate for its single IDB view.
* Add `LVSSnapshotIO` as an iLVS-local, streaming binary module.  It owns
  header/payload validation and does not depend on IDB.
* Add interface methods for logical write, physical write, and atomic dual-bin
  load.  Both input files are parsed into locals before the iLVS database is
  changed.
* Track whether both snapshots were successfully loaded; `run_lvs` rejects an
  uninitialized comparison database.
* Extend the checker/reporter to compare and report top-level non-supply IO
  names, instance names, and net names plus terminal sets before the existing
  routing-connectivity check. The physical snapshot uses schema version 4 to
  carry a per-net shape graph; the checker unions same-layer intersections,
  via sides, and shapes that belong to the same terminal, then compares every
  load root to the driver root in an OpenMP-parallel net loop. It reports Open
  only from that local graph, then reports Short only from global components
  spanning distinct net names. DEF regular-route `RECT` deltas are translated
  by their path point before either graph is built. It also carries
  power/ground instance-pin membership and special-net route shapes. The
  checker derives edge supply anchors from the highest horizontal or vertical
  route tracks, then compares every VDD/VSS terminal's global component to its
  matching anchor. RPT/JSON report the VDD/VSS open counts, without exposing
  anchors or internal pin-count diagnostics.

### Validation

* Snapshot I/O unit tests use rich logical and physical graphs and verify a
  round trip retains all persisted fields.
* Negative tests cover magic/version/type mismatch, swapped logical/physical
  paths, and truncated payloads.
* The three Tcl scripts provide an integration path in which a fresh DEF-only
  process can be inspected independently of any Verilog load.  If its DEF bin
  is already incorrect, the defect is in `readDef`/DEF handling rather than an
  IDB view overwrite.

## Out of Scope

* Adding concurrent multi-view support to IDB.
* Treating the temporary `.bin` format as a long-term public interchange
  format.
* Adding comparison classes beyond top-level IO, instance, net, and the
  pre-existing physical connectivity checks.

## Technical Notes

* Current entry point: `src/operation/iLVS/interface/LVSInterface.cpp`.
* Current database bootstrap: `src/operation/iLVS/source/data_manager/DataManager.cpp`.
* Current IDB extraction: `src/operation/iLVS/source/module/netlist_extractor/NetlistExtractor.cpp`.
* Current Tcl surface: `src/interface/tcl/tcl_ilvs/`.
* Proposed data flow: `Verilog IDB -> logical Netlist -> netlist.bin` and
  `DEF IDB -> physical Netlist -> def.bin`; then
  `netlist.bin + def.bin -> Database -> LVSChecker/LVSReporter`.
* Relevant project contract: `.trellis/spec/backend/ilvs-idb-contract.md`.
* Snapshot-format decision and rationale:
  `research/snapshot-format.md`.

## Decision (ADR-lite)

**Context**: Reading DEF and Verilog sequentially in one IDB process replaces
the current design view.  Even when iLVS copies a value snapshot before the
replacement, this makes correctness depend on the single-view lifecycle and
can obscure a DEF-read defect.

**Decision**: Use three isolated iEDA invocations.  The first creates
`netlist.bin` from only a Verilog-backed IDB view.  The second creates
`def.bin` from only a DEF-backed IDB view.  The third loads the two value bins
and compares them without any IDB input command.

**Consequences**: iLVS no longer reloads DEF internally and no input view is
overwritten during a writer run.  The flow has two additional short Tcl
invocations and the snapshot format must validate that the two files belong to
their expected logical/physical roles.

## Expansion Sweep

* **Future evolution**: the checker/reporter consume only `Netlist`, so
  future dual-view IDB support can replace the snapshot writer/reader boundary
  without changing comparison logic.
* **Related scenarios**: writer scripts can run independently or in parallel
  because they write different output paths and never share an iEDA process.
* **Failure cases**: schema/type/design mismatch and partial files fail at
  `read_lvs`; a fresh DEF writer makes `readDef` problems independently
  observable.
