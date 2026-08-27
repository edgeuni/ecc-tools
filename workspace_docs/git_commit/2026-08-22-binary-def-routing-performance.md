# Commit 记录：Binary、DEF Export 与 Routing API 性能优化

日期：2026-08-22
分支：`main`
Commit：本文件随本次提交创建；最终 hash 以 `git log -1` 为准。

## 提交目的

提交当前已经完成并验证的数据库性能优化，范围包括：

- Binary Archive 的批量 I/O 和单遍 SHA-256 写入；
- DEF 文本导出的缓冲输出和整数格式化；
- routing path 的批量只读访问 API；
- 对应 benchmark 和回归测试。

本次提交不包含 DEF parser 内存池尝试。该尝试已经用 Git 恢复，当前没有第三方 LEF/DEF parser 或 `DefDesignImporter.cpp` 的未提交修改。

## 代码改动

### Binary Archive

涉及：

- `src/database/refactor/persistence/binary/BinaryArchive.h`
- `src/database/refactor/persistence/binary/BinaryFormat.cpp`
- `src/database/refactor/persistence/binary/BinaryPayload.cpp`

主要改动：

- Binary 输入和输出 archive 增加 64 KiB 缓冲区；
- 整数序列化先在栈上完成 little-endian 编码，再批量写入；
- 输入使用 `streambuf::sgetn()` 批量读取；
- payload 写入过程中同步计算 SHA-256；
- 删除 payload 写完后关闭文件、重新打开并完整扫描计算 hash 的二次 I/O；
- Tech、Library、Design payload 边界显式 flush。

### DEF Export

涉及：

- `src/database/refactor/export/def/DefDesignExporter.cpp`
- `src/database/refactor/import/def/test/DefDesignRoundTripTest.cpp`

主要改动：

- 增加 64 KiB `DefTextOutput` 文本缓冲；
- 整数使用 `std::to_chars()`，减少 `ostream` facet 和 locale 格式化开销；
- 去除无条件的 Net 名称排序；
- 共线 routing point 使用 DEF 的相对坐标 `*` 表示；
- DEF 输出遍历改用 `wireIds()` 和顺序 `forEachPath()`；
- 增加 exporter 输出失败、`exportText()` 一致性和相对坐标的测试。

### Routing 批量读取 API

涉及：

- `src/database/refactor/design/routing/pool/DesignRoutingPool.h`
- `src/database/refactor/design/routing/storage/DesignRoutingStorage.h`
- `src/database/refactor/design/routing/storage/DesignRoutingStorage.cpp`
- `src/database/refactor/benchmark/RefactorBenchmark.cpp`
- `src/database/refactor/design/test/DesignRoutingConstraintStorageTest.cpp`

主要改动：

- 增加 `wireIds()`，返回只读 `std::span`，避免按 Net 构造临时 vector；
- 增加 `forEachPath()` 顺序 cursor，一次建立 path/point/via/rectangle 的范围关系；
- 保留原 `wires()` API，兼容需要拥有 vector 的调用者；
- benchmark 改用批量读取接口；
- 测试覆盖 virtual point、point extension、via orientation、via mask 和 cursor 遍历。

## 新增代码的实现逻辑

### `BinaryArchive` 的缓冲读写

`BinaryOutputArchive::writeBytes()` 不再每次直接调用 `ostream::write()`，而是把数据先复制到 64 KiB 的内部 buffer。buffer 满时才一次性写到底层 stream，`flush()` 负责处理剩余数据并检查写入状态。

整数序列化仍然保持原有 little-endian 格式，但先把一个整数的全部字节写入栈上的 `std::array`，再作为一个连续块进入 buffer。这样既没有改变 binary schema，也避免了一个整数触发多次 stream 调用。

`BinaryInputArchive::readBytes()` 使用 `rdbuf()->sgetn()` 批量取数据，再从内部 buffer 拆分给字段读取。字段跨越 buffer 边界时由循环处理，因此调用方不需要感知缓冲区存在。

### Binary payload 的单遍 hash

`Sha256OutputStreamBuffer` 位于 payload writer 和真实文件 stream 之间。payload 的每次实际写入同时完成三件事：

1. 写入目标文件；
2. 更新 SHA-256 状态；
3. 累计 payload 字节数。

payload 完成后直接得到大小和 digest，随后 seek 到文件开头写回 header。这样不再关闭文件后重新打开并扫描整个 payload，文件格式和校验规则保持不变。

### `DefTextOutput` 的文本输出

DEF exporter 的所有字符串、字符和整数都经过 `DefTextOutput`。字符串和字符直接追加到 64 KiB buffer，整数使用 `std::to_chars()` 写入栈上临时数组，再追加到 buffer。只有浮点数仍然先 flush buffer，再使用原有 `ostream` 浮点格式化路径，因此没有改变浮点输出规则。

exporter 结束时显式 flush，并检查原始 `ostream`，所以底层短写或 flush 失败会报告异常，而不会把不完整的 DEF 当作成功结果。

共线点的输出逻辑只比较当前点和前一个点：

- x 坐标相同，输出 `(* y)`；
- y 坐标相同，输出 `(x *)`；
- 其他情况输出完整 `(x y)`。

DEF importer 会按前一个点恢复 `*` 坐标，因此这是文本表示的压缩，不是几何数据变化。

### Net 和 routing 的批量遍历

`wireIds(net)` 直接把 `_wires_by_net` 中的 vector 暴露为只读 `std::span`。旧的 `wires(net)` 保留不变，只在兼容调用者需要拥有副本时构造 vector。

`DesignRoutingPool::forEachPath()` 首先校验 routing handle，然后计算当前 wire 在全局 path、point、via 和 rectangle pool 中的起点。对于稀疏的 path/point/via extra 数组，它只在开始时用 `lower_bound()` 定位一次，之后通过 cursor 顺序向前推进；每个 path 最终以多个 `span` 组成的 `DesignWirePathView` 传给回调。

因此 benchmark 和 exporter 不再对每个 path 重复执行 `path()` 的边界检查、全局区间定位和临时 vector 构造，但底层 pool 的存储格式没有改变。

### 测试代码的作用

- `DesignRoutingConstraintStorageTest` 使用 extension point、virtual point、via orientation 和 via mask，确认 sparse extra 数据在 pool 和 view 之间正确还原；
- `DefDesignRoundTripTest` 检查相对坐标、`exportText()` 与 stream 输出一致，并使用拒绝写入的 `streambuf` 验证最终 flush 错误会抛出异常；
- benchmark 只改变访问方式，不改变统计对象和 sink，保证前后测量仍然遍历相同的 routing 数据。

## 性能记录

正式大数据使用 ISPD2019 test10：

- LEF：`reference/ispd2019/ispd19_test10/ispd19_test10.input.lef`
- routed DEF：`reference/ispd2019/test10/def/12.t10.def`
- DEF 大小：约 2.64 GB

代表性结果：

| 操作 | 优化前 | 优化后 | 结果 |
|---|---:|---:|---:|
| Binary export | 28.905 s | 6.279 s | 约 4.60x |
| Binary import | 34.825 s | 10.724 s | 约 3.25x |
| DEF export，中位数 | 20.792 s | 15.475 s | 约 25.6% 降低 |

DEF export 六次输出文件大小一致；Binary archive 的 test1 和 routed test10 输出已做逐字节比较。

## 验证

- `git diff --check` 通过；
- DEF round-trip Release：8/8 通过；
- DEF round-trip ASan/UBSan：8/8 通过；
- DEF full corpus semantic differential：6/6 通过；
- Binary archive 测试和恢复后的对象/路由数量检查通过；
- benchmark 输出目录 `src/database/refactor/benchmark/results/` 被 Git 忽略，没有作为提交内容纳入。

## 后续方向

本提交没有修改 DEF import parser。后续若继续优化 DEF import，优先考虑 importer 热路径的 `string_view`、临时 Via/Layer 索引、InstancePin 快速查找和 trusted connect；独立重写 DEF parser 应作为单独实验分支和单独 commit。
