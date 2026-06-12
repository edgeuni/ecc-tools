#!/usr/bin/env python3
"""Per-level driver->load Manhattan distance spread for clock tree graphs.

Usage:
  1. Generate graph JSON via DAC-27-CTS/.trellis/tasks/06-11-def-clock-graph-parser/def_clock_graph.py:
       python3 def_clock_graph.py <rt.def> --output /tmp/graph.json
  2. python3 level_spread.py /tmp/graph_a.json [LABEL_A] /tmp/graph_b.json [LABEL_B] ...

Quantifies within-level branch length dispersion (mean/std/min/max of
driver->load Manhattan distance grouped by driver level). Used in task
06-11-align-commercial-cts to show ECC's per-level-uniform-length model
mismatches physical embedding (vga_lcd: htree top level std=125um, max=467um).
"""
import json
import statistics
import sys


def analyze(path: str, label: str) -> None:
    g = json.load(open(path))
    nodes = {}
    for b in g["buffers"]:
        nodes[b["id"]] = b
    for s in g["sinks"]:
        nodes[s["id"]] = s
    src = g["source"]
    nodes[src["id"]] = src
    per_level, fanout_per_level = {}, {}
    for drv, loads in g["graph"].items():
        d = nodes.get(drv)
        if d is None or d.get("x_um") is None:
            continue
        lvl = d.get("level")
        dists = []
        for ld in loads:
            n = nodes.get(ld["node"])
            if n is None or n.get("x_um") is None:
                continue
            dists.append(abs(n["x_um"] - d["x_um"]) + abs(n["y_um"] - d["y_um"]))
        if not dists:
            continue
        per_level.setdefault(lvl, []).extend(dists)
        fanout_per_level.setdefault(lvl, []).append(len(loads))
    print(f"\n===== {label} =====")
    print(f"{'lvl':>4} {'drvs':>5} {'edges':>6} {'avg_fo':>6} {'mean_um':>8} {'std':>7} {'min':>7} {'max':>8}")
    for lvl in sorted(per_level, key=lambda x: (x is None, x)):
        ds = per_level[lvl]
        fo = fanout_per_level[lvl]
        print(f"{str(lvl):>4} {len(fo):>5} {len(ds):>6} {statistics.mean(fo):>6.2f} "
              f"{statistics.mean(ds):>8.2f} {statistics.pstdev(ds):>7.2f} {min(ds):>7.2f} {max(ds):>8.2f}")


if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(1)
    i = 0
    while i < len(args):
        path = args[i]
        label = args[i + 1] if i + 1 < len(args) and not args[i + 1].endswith(".json") else path
        analyze(path, label)
        i += 2 if label != path else 1
