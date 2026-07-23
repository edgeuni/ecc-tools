# iLVS Snapshot Format Research

## Local Options Evaluated

### nlohmann JSON binary encodings

The repository bundles `src/third_party/json/json.hpp`, which exposes
`to_msgpack` and `from_msgpack`.  This would avoid a new dependency and offers
round-trip parsing, but serialization first constructs a complete JSON DOM.
For a routed DEF snapshot, the graph can contain a large shape list; retaining
both `Netlist` and a second JSON representation creates an avoidable memory
peak.

### Custom streaming binary format

All current snapshot fields are strings, fixed-width integers, vectors, maps,
and sets.  They can be written directly to a binary stream without persisting
IDB pointers or requiring a third-party ABI.  This keeps extra memory bounded
while serializing/deserializing and remains suitable for the temporary,
same-tool-build interchange needed until IDB supports two views.

## Selected Format

Use a new iLVS-local streaming `LVSSnapshotIO` component.

* File header: fixed magic, schema version, and snapshot kind
  (`logical` or `physical`).
* Payload: length-prefixed strings and containers; fixed-width integers for
  counters and coordinates.
* Map/set keys are written in sorted order for reproducible bins.
* The logical writer persists only `Netlist::net_map` and `logical_graph`;
  the physical writer persists only `net_map` and `physical_graph`.
* Reader validates the header, expected kind, version, count/string limits,
  and stream completion before replacing the target `Netlist`.
* Writer creates the output parent directory and writes a temporary sibling
  file before replacing the requested path, so a failed writer cannot leave a
  silently accepted partial bin.

## Consequences

The format is intentionally not a stable public API or an IDB serialization.
It must be removed or bypassed once IDB provides simultaneous logical and
physical views.  Focused tests must cover logical/physical round trips,
swapped files, incompatible headers, and truncated files.
