# Pre-refactor ICS55 baseline

## Execution identity

- Date: 2026-07-29 20:18 Asia/Shanghai
- Source HEAD: `31da2c9cfe86e63791171418fa1ff202400a1f5e`
- Branch: `cts_refactor`
- Command:

  ```bash
  cd /home/liweiguo/project/ecc-tools-dev/scripts/design/ics55_dev
  ./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl
  ```

- Exit status: `0`
- Executed binary SHA-256: `eb641ffd06d2b726ae9e59eaf43cbccc6f76dc8f8af70ebe2d655999e5f75aee`
- CTS config SHA-256: `fd2ae4099a35112ada07153833e00aafc4d271a359294b091b50fd90d476c617`
- DB config SHA-256: `85ddf16218ce73d415bb7144a446b9c0d7a9f92c1e3ecc16decb6a2804ff6537`
- Flow config SHA-256: `ccd4f9d90d54986c3af84d30ab87310c3d92e15017dd5a345308ba4ec267cf14`
- Tcl script SHA-256: `4c63a8879f9dc02e4b43b6d1316b1711f3b00f52a947e5238e3a1d193d824e79`

The executed binary resolves the Tcl script's relative design path to `/home/liweiguo/project/ecc-tools/scripts/design/ics55_dev`, even though the command is launched from `ecc-tools-dev`. The files generated at 20:18 under that actual path are the authoritative baseline. The same script and CTS config are byte-identical in both trees. Final acceptance must inspect the actual generated path reported by the command and must not compare against the stale `ecc-tools-dev/result` files.

## Primary artifacts

| Artifact | Size | SHA-256 |
|----------|-----:|---------|
| actual `result/cts.def` | 10,242,569 | `683eb80971fa353587ddd5b7d06c8cf071099474fe7e170fcbbe24b837c1252f` |
| actual `result/cts.v` | 7,392,113 | `e621a9d6f4a5a12710685a65696dff0674fa4aa99cd195d44b421be4ce5ee8d1` |
| actual `result/cts/cts.log` | 36,920 | `90b6a4cce2823a7bc7453f6cfdc23a1bdd2cdea488034dbfe010fcd9d04dbd2a` |
| actual `result/cts/cts_detail.log` | 57,498 | `64d827ba7746a4b76e452d622f7ac2252b3aad685d7d0d4b994d52678ac33b93` |

Compressed copies of DEF and Verilog are retained next to this file. Old-log hashes are recorded above, but the legacy default/detail files are not retained because their schema and dual-output behavior are intentionally not compatibility requirements.

## Functional and QoR baseline

| Measure | Baseline |
|---------|---------:|
| Design | `bp_be_top` |
| Input instances / nets | 52,092 / 48,992 |
| Final DEF components / nets | 52,440 / 49,340 |
| Final pins / special nets | 3,033 / 4 |
| Unique `cts_flow_*` names in DEF / Verilog | 696 / 696 |
| Clock count | 1 |
| Sink count | 8,751 |
| Sink-domain count | 1 |
| Selected H-tree level count / depth | 3 / 3 |
| H-tree inserted buffers | 69 |
| Final clock buffers | 348 |
| Final buffer area | 1,056.160 um^2 |
| Optimized skew | 0.0923 ns |
| Accepted sizing edits | 16 |
| Maximum clock-net wirelength | 924.436 um |
| Total clock-network wirelength | 43,405.796 um |
| Minimum / maximum clock-path buffers | 5 / 5 |
| Maximum clock-tree level | 5 |
| Flow status | finished; no failed clock; API reported success |

Runtime, timestamp, thread id, source location, and peak-memory values are nondeterministic and are not functional equality fields.

## Existing non-log artifact inventory

The baseline run did not update these files, but they are part of the existing artifact contract and their hashes are recorded for the final inventory check:

| Relative path under actual `result/cts` | SHA-256 |
|-----------------------------------------|---------|
| `output/cts_design.gds` | `610aa039761770108c3f3a7b35f13c2ad36b40ca8277fe47f5c44e819442cfd0` |
| `output/cts_design.svg` | `ed5bc097cfe604e5aa00ab83d27e19f057f1cc747eebd4a2a5c17b8971560449` |
| `output/cts_flyline.gds` | `e9e151b6a0af5d92a86c7efeca2ddf39ccd6a061fd093af12ca5dadd9c66b137` |
| `output/cts_flyline.svg` | `ffd283771579aee74c8619065c952d6de8c07115e2088980388871be148f6e63` |
| `output/cts_layers.lyp` | `09e107066d95fbee73a374324a0b81d698b0878187e1d1f5e54be30d3ecb5f78` |
| `visualization/gds/cts_design.gds` | `b79c8ddd4459ed9d560cde3593b8cc7f79dd81a9f37996f043f0eb01d5155e94` |
| `visualization/gds/cts_design.lyp` | `575a2a486ef8bf2a9862e37aeb492b6ecaac1f153576fe2d5056fb942de735cc` |
| `visualization/gds/cts_flyline.gds` | `2a12e749dd0e5929fd2baf3d40fd5319706540e3055f607f51c85ad9ac164e7b` |
| `visualization/gds/cts_flyline.lyp` | `77bf93d35fe53a51ce4a897016c1cb7694bbba3ac06cf65e343dd334901861d8` |
| `visualization/gds/cts_layers.lyp` | `6de85967cbbbd3a9390dd81edf7aed04d815c92572c066a4f02094b49b1c0d55` |
| `visualization/svg/cts_design.svg` | `7eedd80d03311e93083c8aa0be0cbc94cba2a6430918b7d02dfd13995d8be0a6` |
| `visualization/svg/cts_flyline.svg` | `e8468bd22a0ecd123bf27d96f1b41f38c4b73b592eea210b4ee6f75d27c4791a` |
| `statistics/cell_stats.rpt` | `f4155002c016a0b9dd70a1030667c989b65309db1f7896203d8802f5d8e70715` |
| `statistics/lib_cell_dist.rpt` | `2520c9b2295ad8b036e6af2656d582e450a53ee615e7f620d89d17a43ea8f9d5` |
| `statistics/wirelength.rpt` | `b25e0534d89ad04e98a6b62451b0c2fa9b689fc66046faa7af6b39a592696f0b` |

## Final comparison requirements

- Run the exact command again and require exit `0`, successful stage completion, one `cts.log`, and no terminal CTS Error.
- Compare final DEF and normalized Verilog against the retained baseline for topology, names/counts, connectivity, and placement.
- Compare every functional/QoR field above exactly unless an existing algorithm tolerance is independently documented; runtime/resource fields are excluded.
- Verify the non-log artifact inventory and content remain stable.
- Treat the approved removal of `cts_detail.log` and replacement of the old `cts.log` schema as intentional changes, not regressions.
