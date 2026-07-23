# iLVS Dual-Bin Snapshot Contract

## 1. Scope / Trigger

Post-route iLVS compares a reference Verilog netlist with a routed DEF after
routing and DRC. The first comparison surface is exactly three design-object
classes: top-module IO pins, instances, and nets. IDB currently keeps one
current design view: loading Verilog replaces the DEF view, and loading DEF
replaces the Verilog view.

Until IDB supports simultaneous views, iLVS uses three separate iEDA
processes. The first serializes a logical value snapshot, the second
serializes a physical value snapshot, and the third reads both snapshots and
compares them. No process in this flow may load both `def_init` and
`verilog_init`.

## 2. Signatures

```tcl
# Process 1: logical writer
tech_lef_init -path <technology.lef>
lef_init -path <cell.lef>...
verilog_init -path <reference.v> -top <top>
init_lvs ?-temp_directory_path <directory>? ?-thread_number <integer>?
write_lvs_netlist -path <netlist.bin>
destroy_lvs

# Process 2: physical writer
tech_lef_init -path <technology.lef>
lef_init -path <cell.lef>...
def_init -path <routed.def>
init_lvs ?-temp_directory_path <directory>? ?-thread_number <integer>?
write_lvs_def -path <def.bin>
destroy_lvs

# Process 3: comparator
init_lvs ?-temp_directory_path <directory>? ?-thread_number <integer>?
read_lvs -netlist_bin_path <netlist.bin> -def_bin_path <def.bin>
run_lvs
destroy_lvs
```

`init_lvs` accepts only `-temp_directory_path` and `-thread_number`; defaults
are `./lvs_temp_directory` and `128`. It initializes iLVS configuration,
directories, logging, and an empty database. It does not access IDB or call
`readDef`.

```cpp
void LVSInterface::writeNetlist(const std::string& file_path);
void LVSInterface::writeDef(const std::string& file_path);
void LVSInterface::readSnapshots(const std::string& netlist_file_path,
                                 const std::string& def_file_path);
```

The C++ names describe the value boundary. The Tcl command names remain
`write_lvs_netlist`, `write_lvs_def`, and `read_lvs` for script compatibility.

## 3. Contracts

- `write_lvs_netlist` requires a current Verilog-backed IDB design and a
  nonempty IDB Verilog path. It extracts `Netlist::net_map` and
  `logical_graph` only.
- `write_lvs_def` requires a current DEF-backed IDB design and a nonempty IDB
  DEF path. It extracts `Netlist::net_map` and `physical_graph` only. The
  physical graph contains regular/special routed metal, via top/bottom
  shapes, routing-layer pin-port shapes, same-layer overlap edges, and via
  edges.
- A snapshot is an iLVS value object, never an IDB object graph. It has magic
  `ILVSBIN\0`, schema version, logical/physical kind, a length-prefixed
  payload, and a 64-bit FNV-1a payload checksum. Map and set keys are written
  in sorted order.
- The logical and physical payloads both retain `design_name` and `net_map`.
  Snapshot schema version 2 added a named top-module IO-pin list and an
  instance map on each side. An instance record contains its instance name
  and master-cell name; the logical record also retains its connected pin
  names. Snapshot schema version 3 additionally retains a local routing graph
  for every regular DEF net. Snapshot schema version 4 additionally retains
  power/ground instance-pin membership and special-net route shapes needed by
  the power-connectivity check.
- Extract IO pins only from `IdbDesign::get_io_pin_list()`. They are encoded
  as `PIN/<port>` and represent top-module ports, not every endpoint on a net.
  A terminal is broader: it is either a top-module IO pin (`PIN/<port>`) or
  an instance pin (`<instance>/<pin>`).
- Extract instances from the complete `IdbDesign::get_instance_list()` on
  both sides. Do not filter the DEF list with `IdbInstance::is_netlist()`:
  DEF `SOURCE` classification can mark components as `DIST`, while the same
  components are present in the reference Verilog. Compare only sorted
  instance-name vectors; master-cell names remain snapshot metadata and are
  not an LVS equivalence criterion.
- Build separate sorted, duplicate-free vectors before every entity
  comparison. IO and instance equivalence is their name-vector equality.
  IO comparison removes `PIN/<port>` entries whose port belongs to the
  physical power or ground net set, so supply ports do not produce logical
  versus DEF differences. Net equivalence first compares sorted net-name
  vectors, then compares the sorted, duplicate-free terminal vectors for
  every common net. A common net with unequal terminal vectors is a
  `NetPinMismatch`, even when its endpoint counts happen to match.
- The physical payload retains the existing whole-design graph metrics,
  component mapping, terminal mapping, shape locations, and power/ground set
  for future checks. It additionally retains
  `net_routing_graph_map[net_name]`: the DEF-selected driver terminal,
  routing-layer pin-port/wire/rectangle shapes, via bottom/top shape pairs,
  and a terminal-to-shape-index map. This local graph is the sole input for a
  per-net Open conclusion. Do not use whole-design components to decide that a
  driver reaches a load: a physical short on another net could otherwise make
  an open net appear connected. Conversely, a whole-design component is the
  sole input for a Short conclusion: its sorted, duplicate-free net-name list
  having more than one entry produces one `RoutingShort` violation. A
  net-local graph cannot detect a bridge whose shapes belong to another net.
- Schema version 4 physical payload additionally retains
  power_instance_pin_net_map and ground_instance_pin_net_map, keyed by
  non-IO terminal name (<instance>/<pin>) with the owning special-net name as
  value. It also retains supply_route_shape_list; each entry has its
  power/ground special-net name, whole-design component ID, routing-layer ID
  and routing-layer order, and absolute DBU rectangle. Populate the maps from
  explicit special-net pin references and from lazy DEF wildcard connections
  such as (* VDD) / (* VSS). Use IdbDesign::findSpecialNetForInstancePin(pin)
  while extracting the value snapshot; do not call
  materializeSpecialNetWildcardPins or otherwise mutate the shared IDB design.
- For an `IdbRegularWireSegment` with `is_rect()`,
  `get_segment_rect()` is the DEF path `RECT` delta, relative to the preceding
  path point. iLVS must translate that rect by `get_point_start()` before it
  enters either local or whole-design physical graph. Leaving the delta at the
  origin makes unrelated patch wires overlap and creates false shorts.
- Writers create the snapshot parent directory, write a temporary sibling,
  and rename it only after the complete payload has been written. Snapshot
  output must be outside `init_lvs`'s temporary directory because iLVS
  recreates that directory during initialization.
- `read_lvs` parses both files into local `Netlist` values, verifies their
  kinds and matching nonempty design names, then stores both values in the
  iLVS database. The comparator does not need technology, LEF, DEF, or
  Verilog initialization.
- `run_lvs` requires that both snapshot values were loaded successfully by
  `read_lvs`. It reports missing/unexpected IO pins, missing/unexpected
  instances, missing/unexpected nets, and `NetPinMismatch` terminal-set
  differences. It then checks every physical regular net independently: the
  driver captured from `IdbNet::get_driving_pin()` must reach every remaining
  terminal through the local routing graph. Same-layer intersecting
  rectangles, the two sides of each via, and multiple port shapes of one
  terminal are connected. A net with zero or one terminal is trivially
  connected. A missing usable driver produces `RoutingDriverMissing`; an
  unreachable load produces `RoutingOpen` with the load terminal and its
  disconnected local shapes. The per-net loop uses
  `#pragma omp parallel for schedule(dynamic)` and each iteration writes only
  its own result slot before deterministic sequential aggregation. After that
  loop, `run_lvs` scans `component_net_map` in ascending component-ID order;
  every component containing more than one sorted, unique net name produces a
  `RoutingShort` violation with that component ID and net-name list.
- After Open/Short checking, run_lvs performs power connectivity from the
  physical snapshot only. Among power/ground special-route shapes, select the
  largest routing-layer order. Infer horizontal versus vertical from the
  aggregate span of elongated shapes on that layer, merge fragments on the
  same centerline/net/component into tracks, and sort tracks bottom-to-top
  (horizontal) or left-to-right (vertical). The first outer track is one
  supply anchor; scan from the opposite outer edge inward until a track of the
  other supply type is found. Each anchor is the midpoint of its merged track.
  This handles a same-type pair at both die edges. Every VDD/VSS instance pin
  is connected only when its whole-design terminal component equals the VDD or
  VSS anchor component. A missing terminal component is disconnected. Missing
  anchors produce PowerSupplyPointMissing/GroundSupplyPointMissing; wrong
  components produce PowerDisconnected/GroundDisconnected grouped by net and
  component. A VDD/VSS short remains a separate RoutingShort result.
- `LVSReporter::getSummaryTableList` is the single source for both console
  output and the human-readable `ilvs.rpt`. It returns `Entity` and
  `Connectivity` libfort tables. `Entity` has exactly `IO(without pg)`,
  `Instance`, and `Net` rows with `NETLIST`, `DEF`, and `Difference` columns.
  The IO counts are the sorted vectors after power/ground port exclusion; the
  other counts are the corresponding sorted vectors used by comparison.
  `Difference` is the number of entity differences (for nets it includes
  pin-set mismatches). The table has no total row because the columns describe
  the same design from different views. `Connectivity` has `Connectivity`,
  `Type`, and `Count` columns, with exactly four rows: `Routing/Open Net`,
  `Routing/Short Net`,
  `Power/Open VDD`, and `Power/Open VSS`. The second Routing/Power row has
  an empty first cell for visual grouping. Open Net is
  `routing_open_net_num + routing_missing_driver_num`, so a missing driver is
  not hidden by this compact report. Short Net is the short-component violation
  count; Open VDD/VSS are disconnected VDD/VSS instance-pin counts. It has no
  total row. `run_lvs` logs each table as an independent block in that order;
  it must not horizontally concatenate corresponding table rows.
- `ilvs.rpt` starts with `iLVS Report`, followed directly by the `Entity` and
  `Connectivity` blocks in the same order as the console, then
  `[Violation Details]`. It does not render a `[Statistics]` heading, an
  excluded-power/ground-IO explanation, or a power-supply-point section. Each
  detail includes a `Violation`/`Value` table, component and terminal lists, then a
  `Coordinates (DBU)` table with `Component`, `Layer`, `LLX`, `LLY`, `URX`,
  and `URY` for every physical shape. A `RoutingOpen` detail also records its
  driver and disconnected local shapes, whose component cell is `-`. A
  `RoutingShort` detail records its global component, the sorted list of nets
  in that component, and all component-shape coordinates. A
  `NetPinMismatch` detail prefixes unmatched terminals with `NETLIST/` or
  `DEF/`. `ilvs.json` contains only the same report-level information as the
  RPT: ordered `entity` records with `entity`, `netlist`, `def`, and
  `difference` fields for `IO(without pg)`, `Instance`, and `Net`; ordered
  `connectivity` records with `connectivity`, `type`, and `count` fields for
  the four Routing/Power rows; and an always-present `violations` array. An
  empty `violations` array is the JSON form of RPT `None`. It omits the legacy
  `summary`, `physical_graph`, and power-supply-point diagnostics.
- The CX55 scripts use the `lvs_temp_cx55` directory under
  `/nfs/share/home/zengzhisheng/debug_workspace/output` as their only
  persistent output root. They store snapshots in `lvs_snapshot/`; each
  writer and comparator uses a separate child temp directory so `init_lvs`
  cannot delete the snapshots. The comparison script promotes
  `lvs_reporter/` and `lvs.log` to the root and removes its child temp
  directory and the legacy split output directories.

### iRT-Aligned iLVS Structure

- Treat iRT as the implementation convention, not an edit target. iLVS owns
  its own `interface/`, `source/{data_manager,module,toolkit}/`, and `test/`
  hierarchy; changes for this feature must not modify `src/operation/iRT`.
- iLVS module objects follow the iRT singleton lifecycle:
  `initInst()`, `getInst()`, `destroyInst()`, a module macro, a private static
  instance pointer, deleted copy/move operations, and `// public` / `// private`
  source partitions. `LVSInterface::initLVS` initializes DataManager before
  the iLVS modules; `destroyLVS` destroys modules in reverse order before
  DataManager and Logger.
- Keep module/API names qualified by LVS where they describe a tool surface
  (`LVSInterface`, `LVSChecker`, `LVSReporter`, `LVSSnapshotIO`). Keep values
  inside `namespace ilvs` as domain names without a redundant LVS prefix:
  `Database`, `Netlist`, `Net`, `Instance`, `LogicalGraph`, `PhysicalGraph`,
  `NetRoutingGraph`, `CheckResult`, and `Violation`. `Database` owns exactly
  the netlist snapshot, DEF snapshot, and current check result; use
  `netlist`/`def` terminology rather than `expected`/`physical` aliases.
- Use functional `#if 1  // <area>` groups only where a public or private
  subsystem has a meaningful boundary (for example `snapshot`, `check`,
  `report`, `build`, or `destroy`). Do not add empty compatibility blocks.
- Tests live at
  `src/operation/iLVS/test/test_<module>/test_<module>.cpp`. The directory,
  CMake executable target, and CTest name must all be the same
  `test_<module>` value, mirroring iRT. Module CMake files own libraries;
  the top-level iLVS CMake owns the `interface`, `source`, and `test`
  subdirectories in the same order as iRT.

## 4. Validation & Error Matrix

| Condition | Required behavior |
| --- | --- |
| `write_lvs_netlist` without `verilog_init` | iLVS logs a Verilog-init error and terminates before writing a logical snapshot. |
| `write_lvs_def` without `def_init` | iLVS logs a DEF-init error and terminates before writing a physical snapshot. |
| Missing snapshot file | `read_lvs` logs the file path and read failure before comparison. |
| Truncated payload, invalid magic, or checksum mismatch | Snapshot reader rejects the file without changing its output value. |
| Unsupported schema version | Snapshot reader rejects the file as incompatible. |
| Schema-1, schema-2, or schema-3 snapshot | Snapshot reader rejects it; regenerate both writer snapshots with schema version 4. |
| Logical/physical paths swapped | Snapshot kind validation rejects the unexpected type. |
| Empty or mismatched design names | `read_lvs` rejects the pair before it changes the database. |
| `run_lvs` before a successful `read_lvs` | iLVS logs that both snapshots must be loaded first. |
| Existing iLVS temporary directory | `init_lvs` intentionally removes and recreates it; do not place snapshots there. |
| No elongated top-layer power/ground route of one supply type | The corresponding supply point is absent; every mapped instance pin of that type reports one supply-point-missing violation. |

## 5. Good / Base / Bad Cases

- Good: run a Verilog-only writer process, a DEF-only writer process, then a
  comparator process. The writers may run independently or in parallel when
  they use different snapshot paths.
- Base: omit `-thread_number` to use `128`. Omit the temporary directory only
  when the default directory is safe for iLVS to remove.
- Bad: put `def_init` and `verilog_init` in one iLVS script, then expect
  `init_lvs` to preserve or reconstruct both IDB views.
- Bad: put `netlist.bin` or `def.bin` under `-temp_directory_path`; the next
  `init_lvs` removes it.
- Bad: pass the persistent CX55 output root directly as
  `-temp_directory_path`; it would remove `lvs_snapshot/` before comparison.
- Bad: call `run_lvs` after only one snapshot has been written or after a
  `read_lvs` type/version/checksum failure.
- Bad: use `IdbInstance::is_netlist()` to reduce the DEF instance map before
  comparing it to Verilog. DEF `SOURCE DIST` is not proof that the instance
  is absent from the reference design.
- Bad: treat an empty `IdbSpecialNet::get_instance_pin_list()` as proof that a
  DEF power net has no cell pins. A DEF `(* VDD)` / `(* VSS)` connection is
  lazy by default in IDB and must be queried during extraction.

## 6. Tests Required

- Tcl surface test: `init_lvs` exposes only `-temp_directory_path` and
  `-thread_number`; `write_lvs_netlist`, `write_lvs_def`, and `read_lvs`
  require their documented path options.
- Snapshot unit test: round-trip rich logical and physical `Netlist`
  values, including IO lists, instance names, and master names; assert all
  retained fields survive, and assert the irrelevant graph type is absent
  from each payload.
- Snapshot negative tests: missing file, invalid magic, unsupported version,
  swapped kind, truncated payload, and payload checksum corruption must fail
  while the previously loaded value remains unchanged.
- Checker and reporter unit tests: cover sorted-vector missing/unexpected IO
  and instance comparison, power/ground IO exclusion, missing and unexpected
  nets, same-name net terminal-set mismatches, same-layer routing connection,
  via connection, multi-layer port connection, disconnected load reporting,
  missing-driver reporting, a global component with two net names reporting
  `RoutingShort`, and the fact that another net cannot bridge a local open.
  Assert the shared console/RPT tables have exactly the `IO(without pg)`/
  Instance/Net NETLIST/DEF/Difference rows and the four Connectivity rows,
  with no total row. Assert that console integration output emits the whole
  Entity block before the Connectivity block and does not put both table rows
  on one log line. Assert that Open Net includes missing-driver failures,
  that VDD/VSS rows count disconnected instance pins, and that the RPT starts
  with `iLVS Report` followed by the same table order, has no `[Statistics]`,
  excluded-IO explanation, or Power Supply Points section, and retains the
  driver field, short-net list, and DBU coordinate-table fields. Assert JSON
  has only the matching Entity/Connectivity rows and `violations`, omits the
  legacy diagnostics, and emits an empty `violations` array for RPT `None`.
- Netlist-extractor unit test: create lazy `(* VDD)` and `(* VSS)` special-net
  membership with empty explicit special-net pin lists; assert the physical
  snapshot maps the two instance terminals and leaves both the special-net
  lists and `IdbPin::get_special_net()` unchanged.
- Power-connectivity checker/reporter tests: cover horizontal and vertical
  anchor selection, the same-type-at-both-edges inward scan, missing anchors,
  a terminal with no physical component, an incorrect component, the four
  VDD/VSS summary rows, and the absence of anchor diagnostics from JSON.
  Snapshot round-trip must
  retain the v4 maps and supply-route-shape list.
- Three-process integration test: run the two writer Tcl scripts in fresh
  iEDA processes and the comparison Tcl script in a third. Assert the
  comparator has no `def_init` or `verilog_init`, both snapshot counts load,
  physical graph nodes and per-net routing graphs are nonzero, the only
  persistent CX55 output root contains snapshots/RPT/JSON/log, and RPT/JSON
  agree on every Entity and Connectivity row. A routed DEF with regular-path
  `RECT` entries must not create artificial origin-coordinate shorts.

## 7. Wrong vs Correct

### Wrong

```tcl
tech_lef_init -path $TECH_LEF_PATH
lef_init -path $LEF_PATH
def_init -path $DEF_PATH
verilog_init -path $VRLG_PATH -top $VRLG_TOP
init_lvs
run_lvs
```

This relies on one IDB process holding two incompatible current views. It
also hides a DEF-read defect behind the later Verilog load.

### Wrong Instance Filter

```cpp
if (!instance->is_netlist()) {
  continue;
}
```

DEF `SOURCE` labels are not a logical/physical equivalence filter. In the
CX55 integration case this would reduce 421 DEF instances to 136 while the
reference Verilog still has 421.

### Correct Instance Extraction

```cpp
for (idb::IdbInstance* instance : design->get_instance_list()->get_instance_list()) {
  if (instance == nullptr) {
    continue;
  }
  instance_map[instance->get_name()] = {instance->get_name(), {}, instance->get_cell_master()->get_name()};
}
```

This preserves the same full instance population on both snapshot sides;
the checker then compares only their sorted instance-name vectors.

### Wrong Global Connectivity Graph for Open

```cpp
// A component can contain geometry from more than one DEF net after a short.
const uint64_t driver_component = physical_graph.terminal_component_map.at(driver);
const uint64_t load_component = physical_graph.terminal_component_map.at(load);
return driver_component == load_component;
```

This can classify an open as connected when another net physically bridges the
two disconnected shapes.

### Correct Local Routing Graph

```cpp
const NetRoutingGraph& routing_graph = physical_graph.net_routing_graph_map.at(net_name);
// Union only this net's same-layer intersections, via pairs, and port shapes.
return local_driver_root == local_load_root;
```

The local graph is also safe to construct independently for each net in the
OpenMP loop.

### Correct Global Component Scan for Short

```cpp
const std::vector<std::string> net_name_list = getSortedUniqueStringList(component_net_map.at(component_id));
if (net_name_list.size() > 1) {
  reportRoutingShort(component_id, net_name_list);
}
```

This deliberately uses the whole-design graph: a short is precisely the case
where geometry from distinct DEF nets belongs to one physical component.

### Wrong DEF Path `RECT` Coordinates

```cpp
// For a regular DEF route this is a delta from the current path point.
add_shape(net_name, segment->get_layer(), segment->get_segment_rect());
```

Multiple `RECT (0 0 ...)` patch wires then collapse onto the origin in the
extracted graph and can falsely report a short.

### Correct DEF Path `RECT` Translation

```cpp
idb::IdbRect rect = segment->get_segment_rect();
if (segment->is_rect()) {
  if (idb::IdbCoordinate<int32_t>* point = segment->get_point_start(); point != nullptr) {
    rect.moveByStep(point->get_x(), point->get_y());
  }
}
add_shape(net_name, segment->get_layer(), rect);
```

### Correct

```tcl
# Separate process: Verilog only.
verilog_init -path $VRLG_PATH -top $VRLG_TOP
init_lvs
write_lvs_netlist -path $netlist_bin_path

# Separate process: DEF only.
def_init -path $DEF_PATH
init_lvs
write_lvs_def -path $def_bin_path

# Separate process: no IDB input.
init_lvs
read_lvs -netlist_bin_path $netlist_bin_path -def_bin_path $def_bin_path
run_lvs
```

The comparison boundary is now two validated value snapshots, so loading one
IDB view cannot overwrite the other before LVS consumes it.

### Wrong Lazy Special-Net Handling

~~~cpp
for (idb::IdbPin* pin : special_net->get_instance_pin_list()->get_pin_list()) {
  add_pin(net_name, pin, true, false);
}
~~~

For a DEF declaration such as - VDD ( * VDD ), this list is empty unless an
optional IDB materialization mode was enabled. The report would falsely show
zero VDD pins and pass no connectivity check.

### Correct Lazy Special-Net Extraction

~~~cpp
if (special_net->has_wildcard_instance_pins()) {
  for (idb::IdbInstance* instance : design->get_instance_list()->get_instance_list()) {
    for (idb::IdbPin* pin : instance->get_pin_list()->get_pin_list()) {
      if (design->findSpecialNetForInstancePin(pin) == special_net) {
        add_pin(net_name, pin, is_power_net, is_ground_net);
      }
    }
  }
}
~~~

This uses the same IDB lookup semantics without changing the design view that
the isolated DEF writer owns.
