# Commit 记录：EnTT Operation Adapter 与 Benchmark 基础设施

日期：2026-08-21
分支：`main`
Commit：`c2c571fc2c8e470f66e74d43c17d42409a156ccc`
提交信息：`feat(database): add EnTT operation adapters and benchmarks`

## 提交目的

这是 EnTTDB 接入 operation 层的基础提交。它把 EnTTDB 的 Tech/Library/Design 数据转换为 iRT 和 iDRC 可以直接消费的数据库对象，同时建立 wrapper 差分测试和第一版性能 benchmark。提交的重点是适配层和验证入口，不是修改 EnTTDB 的底层 component storage，也不是实现路由写回。

## 主要改动

### iRT EnTT adapter

涉及 `src/operation/iRT/interface/RTInterfaceEnTT.cpp` 及 `RTInterface` 的 source dispatch。

`wrapDatabaseFromEnTT()` 按 TechDatabase、LibraryDatabase、DesignDatabase 的边界建立 iRT `Database`，依次转换：

- technology layer、via master 和 routing rule；
- library cell、port、pin geometry；
- die、row/floorplan、obstacle；
- net、instance pin、IO pin、wire geometry 和特殊网信息。

`RTInterface` 增加 EnTT source 的设置和分派逻辑。没有设置 EnTT source 时仍走原有 iDB wrapper，因此两条路线可以在同一个测试入口下比较。

### iDRC EnTT adapter

涉及 `src/operation/iDRC/interface/DRCInterfaceEnTT.cpp` 及 `DRCInterface`。

iDRC 现在可以从 EnTT source 生成规则数据库、design shape 和 result shape 列表，并在 iRT 初始化 iDRC 时沿用相同的 source 选择。规则、层、via、矩形和 violation 结果都通过语义字段比较，而不是依赖对象地址。

### Storage 的只读批量访问 API

`DesignNetlistStorage` 将 instance pin、net pin 和 IO pin 的只读查询改为 `std::span`，并增加 `forEachInstancePin`、`forEachIoPin` callback 入口。这样调用者可以直接遍历 registry 中已有的连续 pin ID 序列，避免为了查询关系复制一个临时 `vector`。

这次没有改变 component 类型、registry storage、GeometryPool 或持久化格式。span 只在底层容器不发生结构性修改时有效；需要拥有数据的旧 API 仍可继续使用。

### DEF importer 的配套修正

为使 wrapper 差分建立在稳定的 netlist 上，提交同时修复了几处直接导入问题：

- GROUP 的 region 引用延迟到全部 region 解析完成后再解析；
- 不在 NETS callback 中过早解析 PINS 的 net membership，保持 iDB NETS 顺序；
- parser session 显式调用 `defrInit()`；
- 增加 DEF round-trip 覆盖这些场景。

这些修正属于导入正确性和测试稳定性，不是本提交的性能目标。

## 差分测试设计

### iRT

`IrtAdapterDifferentialTest.cpp` 新增 `Wrap` 测试组，覆盖 ISPD18 和 ISPD19 的 sample、test1-test10（由 `IDB_REFACTOR_ISPD18_ROOT`、`IDB_REFACTOR_ISPD19_ROOT` 环境变量控制）。比较内容包括 layer、via、net、pin、obstacle、geometry 和连接关系，并对需要稳定顺序的集合做 canonicalize。

另有 synthetic generated-via case，确认 DEF 中生成的 via 能在特殊网的对应点上 materialize。快照辅助测试用于定位差异，但不是跨实现的最终判定。

### iDRC

`IdrcAdapterDifferentialTest.cpp` 增加规则、design shape、result shape 和 violation 的比较；synthetic case 覆盖 generated via 与 regular wire 的 shape 一致性。iDRC 和 iRT 都使用同一套输入数据，使差异可以定位到具体 adapter，而不是输入数据库不一致。

本提交尚未包含“路由后 EnTT 写回 DEF，再与 iDB 输出做差分”的完整流程；该流程在后续 `da095b57d` 中加入。

## Benchmark

新增 `src/database/refactor/benchmark`，包含 `idb_refactor_benchmark` 和 `irt_input_benchmark`：

- `idb_refactor_benchmark` 可用 `--source entt|idb|both` 测量 LEF/DEF import、net-pin 正向查询、pin-net 反向查询、placement geometry、routing geometry、LEF/DEF export 和 routing batch append；
- `irt_input_benchmark` 把 LEF/DEF 解析放在计时区间外，只测 `RTInterface::wrapDatabase()`，并输出 materialized shape count、耗时和内存信息；
- 结果按 JSONL 写入 `src/database/refactor/benchmark/results/`，目录加入 `.gitignore`，避免大文件和机器相关结果进入版本库。

benchmark README 同时记录输入文件、操作定义和指标含义，为后续 iDB/EnTTDB 同并行度比较提供统一入口。此次建立的是测量框架，正式的 Binary archive 和 routed writeback benchmark 在下一提交扩展。

## 验证与边界

提交新增了对应 CMake target、wrapper test target 和 DEF round-trip test target；ISPD 测试在缺少数据集时会按环境变量跳过。当前仓库后续已确认的 targeted wrapper cases（`Wrap.Ispd19Sample`、`Wrap.Ispd19Test1`、`Wrap.Ispd19Test2`）通过，说明 adapter 的基本路径和对象计数可用。

本提交的边界是：

- 不改变 EnTTDB 底层存储布局；
- 不声称 iRT/iDRC 算法等价，只比较 adapter 输出的语义数据；
- 不包含 routed output、持久化恢复和最终 IDB/EnTTDB writeback 差分。

## 后续关系

`da095b57d` 在本基础上加入 Binary archive benchmark、iRT EnTT 路由输出/写回和参数化 routed differential test；`7ee063ee4` 再对 Binary、DEF export 和 routing 批量访问做性能优化。
