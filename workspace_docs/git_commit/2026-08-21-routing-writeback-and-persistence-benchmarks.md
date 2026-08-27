# Commit 记录：EnTT 路由写回差分与持久化 Benchmark

日期：2026-08-21
分支：`main`
Commit：`da095b57db4f1c89817c30d84162c005943efcc3`
提交信息：`test(database): add EnTT routing output and persistence benchmarks`

## 提交目的

本提交在 `c2c571fc2` 的 iRT/iDRC EnTT adapter 和 wrapper 差分基础上，补齐两条关键验证链路：

1. EnTTDB -> iRT -> routed output/writeback，与原有 iDB -> iRT -> routed output 做语义差分；
2. EnTTDB 的 Binary archive 导出、导入和恢复对象计数 benchmark。

它建立了“路由后输出是否等价”和“持久化后数据是否完整”的测试入口。性能优化本身不是本提交的重点，后续 `7ee063ee4` 才对这些路径做了系统优化。

## Binary archive benchmark

新增 `BinaryArchiveBenchmark.cpp` 和对应 CMake target。一次运行包含：

- LEF/DEF 文本导入并记录 `lef_text_import`、`def_text_import`；
- 分别写出 `technology.edb`、`library.edb`、`design.edb`，记录各 payload 和 total export；
- 分别恢复三个 payload，记录各阶段和 total import；
- 使用生产 archive API 计算 SHA-256、文件字节数、吞吐率；
- 对恢复后的 layer、cell、instance、net、pin、wire、via、patch 等计数做校验。

计时结果以 JSONL 输出，既能保留阶段数据，也方便后续和 DEF 文本导入/导出结果对齐。恢复计数不一致时 benchmark 失败，避免只报告“速度变快”而遗漏数据丢失。结果目录继续由 Git 忽略。

## iRT EnTT 路由输出

涉及 `RTInterface`、`RTInterfaceEnTT.cpp` 和 `IrtAdapterDifferentialTest.cpp`。

EnTT output path 增加以下写回能力：

- 将 iRT 的 track grid 和 GCell grid 写回 EnTT design；
- 将 regular wire 和 special wire 的 segment、via、patch 写回对应 net；
- 根据 iRT layer 解析到 EnTT layer；
- 对已有 route 做替换或追加，并保持输出对象与原 net 的关联。

为比较两个实现，测试把 route segment、patch、via 和关键属性转换为 canonical key 后排序。比较使用整数 DBU 坐标和长度，避免浮点格式或容器遍历顺序造成假差异。

## Routed differential test

新增参数化测试 `IspdRoutedWritebackDifferentialTest::NativeEnttMatchesLegacyIdbAfterRouting`。每个 case 同时准备 IDB 和 EnTT 两条输入路线，并行执行 routing，然后：

1. 取得两条路线的 canonical routed snapshot；
2. 比较 regular/special net 的 wire、via、patch、track/gcell 信息及 violation；
3. IDB 保存 routed DEF，EnTT 通过 `DefDesignExporter` 写出 routed DEF；
4. 重新导入写出的结果，比较 DBU wire length、via/patch 数量和路由语义；
5. 对 DRC result shape 再做一次比较，确认输出差异没有改变几何判定。

测试参数覆盖 ISPD18 sample、sample2、sample3、test1-test5，以及 ISPD19 sample、sample2、sample3、sample4、test1-test3、test5；另外以 ISPD18 test6（约 107,919 instances）作为 around-100k case。路由 worker 使用独立输入，避免两条路线共享可变数据库。

### Synthetic round-trip

`Writeback.NativeEnttMatchesLegacyIdbDefAndRoundTrips` 使用 generated-via fixture，覆盖：

- IDB 与 EnTT 路由后的输出语义一致；
- EnTT routed DEF 可以重新导入；
- 第二次 writeback 不产生额外 wire/track/gcell；
- 重复导出文本保持一致，验证写回的幂等性；
- DRC shape 仍与 legacy 路线匹配。

这是小而确定的回归用例，用来在大型 ISPD 测试之外快速发现 output API 的状态污染或重复追加问题。

## DEF 输出配套改动

本提交还为 routed output 提供确定性文本基础：

- regular Net 和 special Net 按名称排序；
- track grid 和 GCell grid 按稳定顺序输出；
- `DefWrite` 支持 fixed via master 的 RECT 序列化。

排序和 fixed-via 输出保证相同数据库重复导出时更容易做文本/语义差分。后续性能提交去除了不必要的全量 Net 排序并增加缓冲输出，因此不要把后续的 DEF 优化收益归因于本提交。

## 验证模型与限制

本提交验证的是端到端语义：route geometry、长度、via/patch、DRC shape 和 re-import 结果，而不是要求两种实现的对象地址、内部 ID 或容器顺序相同。Net 输出顺序需要在比较前 canonicalize；仅比较原始 DEF 文本会把合法的顺序差异误判为失败。

Binary benchmark 主要验证 EnTTDB archive 的阶段耗时和恢复完整性，iDB 没有对应的同格式 archive 路径，因此它不是 iDB/EnTTDB archive 格式的直接横向比较。`saveLef` 也不属于此提交的主要验证对象。

本提交引入了测试和测量能力，没有把某次机器上的耗时当作固定结论。后续 `7ee063ee4` 使用这些入口完成 Binary、DEF export 和 routing API 的性能优化，并在其 commit 文档中记录正式结果。

## 提交关系

- 前置：`c2c571fc2`，提供 EnTT iRT/iDRC adapter 与基础 wrapper 差分；
- 本提交：增加 routed output/writeback、Binary archive benchmark 和确定性 DEF 输出；
- 后续：`7ee063ee4` 优化 Binary、DEF export、routing batch API；`4642bab06` 优化 EnTT wrapper materialization。
