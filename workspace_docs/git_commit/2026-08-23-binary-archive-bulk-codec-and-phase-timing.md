# Commit 记录：BinaryArchive 批量编解码与分阶段计时

日期：2026-08-23
分支：`main`
Commit：本文件随本次提交创建；最终 hash 以 `git log -1` 为准。

## 提交目的

优化 EnTTDB BinaryArchive 的完整 Design 导入和导出路径，并把 Archive 数据处理与 SHA-256 校验的时间拆开统计：

```text
EnTTDB Design
  -> Binary payload encode/decode
  -> SHA-256 dependency/payload validation
  -> design.edb
```

本次不改变现有磁盘格式，不改变 EnTT component、registry storage 和 RoutingPool 的内存布局。新实现生成的 `technology.edb`、`library.edb` 和 `design.edb` 与旧实现逐字节一致，旧 Archive 也可由新实现读取。

## 代码改动

### BinaryArchive 连续数据快速路径

涉及：

- `src/database/refactor/persistence/binary/BinaryArchive.h`
- `src/database/refactor/persistence/binary/BinaryPayload.cpp`

新增 `ByteCompatibleArchiveValue` 约束以及连续序列批量读写 API。只有满足以下条件的类型才允许直接复制对象表示：

- trivially copyable；
- `std::has_unique_object_representations_v<T>`；
- 固定记录的大小、对齐和字段偏移均由 `static_assert` 固定；
- little-endian 主机走批量字节路径，big-endian 保留逐字段回退。

当前 RoutingPool 中的 `paths`、`points`、`point_extras`、`vias`、`via_extras` 和 `rectangles` 使用该路径。Archive 内部的大块 I/O 直接绕过 64 KiB 中间缓冲；整数标量在 little-endian 主机上使用 `memcpy` 快速路径。

这些改动减少了数亿次逐字段函数调用，但没有改变任何字段顺序或二进制表示。

### RoutingPool 恢复校验线性化

涉及：

- `src/database/refactor/design/routing/storage/DesignRoutingStorage.cpp`

恢复后的 RoutingPool 校验从逐 Path 建立语义视图、重复执行 sparse lookup，改为直接线性扫描原始记录。有效 routing layer ordinal 和 Tech Via entity 在进入主循环前一次性缓存。

校验语义保持不变，仍覆盖：

- Path width 与 layer 引用；
- Point/Via/Rectangle 的范围及关联；
- Via reference 和 anchor；
- regular/special Net 的路径归属。

### Archive 与 SHA-256 分项计时

涉及：

- `src/database/refactor/persistence/binary/BinaryFormat.h`
- `src/database/refactor/persistence/binary/BinaryFormat.cpp`
- `src/database/refactor/export/binary/BinaryDatabaseExporter.h`
- `src/database/refactor/export/binary/BinaryDatabaseExporter.cpp`
- `src/database/refactor/import/binary/BinaryDatabaseImporter.h`
- `src/database/refactor/import/binary/BinaryDatabaseImporter.cpp`
- `src/database/refactor/benchmark/BinaryArchiveBenchmark.cpp`

新增可选的 `BinaryArchiveTiming` 输出参数。默认传入空指针，不要求普通调用方采集计时。

benchmark 现在分别输出：

```text
*_archive
*_checksum_sha256
*_total
```

其中 `archive` 是编解码、文件读写、对象恢复和校验时间，不包含 SHA-256；`checksum_sha256` 包含依赖摘要和 payload 摘要；`total` 为二者之和。

benchmark 还新增 `--source-archive-dir`，可以直接复用已经生成的 Archive 测试导入和再导出，避免每次先解析 2.64 GB DEF。

### 正确性测试

涉及：

- `src/database/refactor/export/binary/test/BinaryDatabaseArchiveTest.cpp`
- `src/database/refactor/export/binary/test/DesignBinaryDatabaseArchiveTest.cpp`

新增测试覆盖：

- 批量兼容编码与原逐字段编码逐字节一致；
- 超过 64 KiB 缓冲区的 Point 序列；
- Design Archive 可选计时接口；
- 原有 round-trip、corruption detection 和 fixed-point 输出。

## Test10 性能结果

输入 Archive 来源：

```text
LEF: reference/ispd2019/ispd19_test10/ispd19_test10.input.lef
DEF: reference/ispd2019/test10/def/12.t10.def
DEF size: 2,641,862,207 bytes，约 2.46 GiB
```

`design.edb` 大小为 2,226,016,381 bytes；三类 Archive 总大小为 2,226,213,760 bytes。

优化前后使用相同旧 Archive 与相同数据规模：

| 操作 | 优化前 | 优化后稳定值 | 保守加速比 |
| --- | ---: | ---: | ---: |
| Binary export 总时间 | 6.308 s | 约 4.04 s | 1.56x |
| Binary import 总时间 | 11.017 s | 约 4.23 s | 2.61x |

一次完整分项实测如下：

| 操作 | Archive | SHA-256 | Total | Archive 吞吐 |
| --- | ---: | ---: | ---: | ---: |
| Export | 2.991 s，70.9% | 1.227 s，29.1% | 4.218 s | 709.8 MiB/s |
| Import | 2.809 s，66.4% | 1.421 s，33.6% | 4.230 s | 755.6 MiB/s |

这说明此前的 Binary export/import 数字包含校验时间。当前 Archive 主体已经接近 3 秒，SHA-256 单独占总时间约三分之一，下一阶段不能继续把全部时间归因于序列化。

## Test10 数据规模

| 数据 | 数量 |
| --- | ---: |
| Instances | 899,404 |
| Instance pins | 5,753,641 |
| Nets | 895,255 |
| Wires | 895,077 |
| Paths | 46,978,419 |
| Points | 74,768,813 |
| Vias | 14,697,732 |
| Rectangles | 4,490,293 |

RoutingPool 四个主数组的最小数据量约 1,597.19 MiB，占 `design.edb` 约 75.2%。其中 Path 约 716.83 MiB、Point 约 570.44 MiB、Via 约 224.27 MiB、Rectangle 约 85.65 MiB；该数字尚未包含 extras 和 layers。

## Profiling 结论

优化前：

- export 的整数逐字段写入是主路径；
- import 同时受逐字段读取、RoutingPool 恢复校验和 registry restore 影响。

优化后，在一次包含旧 Archive 导入、再导出和新 Archive 导入的组合 profile 中：

| 热点 | 组合 workload 占比 |
| --- | ---: |
| SHA-256 | 36.7% |
| `readRegistrySnapshot` | 14.9% |
| `writeRegistrySnapshot` | 11.1% |
| RoutingPool 恢复校验 | 6.4% |
| `readInteger` cumulative | 3.1% |

逐整数开销已明显下降。后续热点转向 SHA-256、EnTT component 的逐类型恢复，以及 RoutingPool 的分段/并行处理。

## 正确性与构建验证

只构建和运行相关目标，没有全量构建 ecc-tools：

```text
idb_refactor_design_test
idb_refactor_binary_database_archive_test
idb_refactor_design_binary_database_archive_test
idb_refactor_binary_archive_benchmark
```

验证结果：

- Release `DesignStorageTest`：19/19 通过；
- Release `BinaryDatabaseArchiveTest`：4/4 通过，包含 Sky130/IHP130 fixed-point；
- Release `DesignBinaryDatabaseArchiveTest`：2/2 常规测试通过，大数据环境变量测试按预期跳过；
- ASan + UBSan 相关 Binary Archive 测试通过；
- 新旧 `technology.edb`、`library.edb`、`design.edb` 使用 `cmp` 验证逐字节一致；
- `git diff --check` 通过。

当前环境没有安装 `clang-format`，因此未执行独立格式化命令。

## 本次未实现的内容

- 没有把 SHA-256 替换成 CRC；
- 没有修改单文件 Archive 格式；
- 没有实现一个 Component 一个文件；
- 没有并行修改 EnTT registry；
- 没有修改 RoutingPool 或 component 的底层存储布局。

多文件、分片、CRC32C 与并行加载方案记录在：

```text
workspace_docs/daily/2026年8月23日/02-binary-archive-multifile-and-snapshot-plan.md
```
