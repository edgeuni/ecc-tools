# Commit 记录：iRT EnTTDB Wrapper 并行物化与 Test10 全量差分

日期：2026-08-23
分支：`main`
Commit：本文件随本次提交创建；最终 hash 以 `git log -1` 为准。

## 提交目的

优化 EnTTDB 到 iRT `Database` 的输入转换路径：

```text
EnTTDB Tech/Library/Design
  -> RTInterface::wrapDatabaseFromEnTT()
  -> RTDM::Database
```

本次提交在上一阶段批量访问优化的基础上，引入只读 cache、精确容量计算和 OpenMP 并行物化。底层 EnTT component、registry storage、GeometryPool 布局和 Binary schema 均保持不变。

## 代码改动

### Storage 批量遍历 API

涉及：

- `src/database/refactor/design/netlist/storage/DesignNetlistStorage.h`
- `src/database/refactor/design/routing/storage/DesignRoutingStorage.h`
- `src/database/refactor/design/test/DesignNetlistStorageTest.cpp`
- `src/database/refactor/design/test/DesignRoutingConstraintStorageTest.cpp`

新增或改造：

- `forEachInstance()`：通过 EnTT dense view 同时读取 `DesignInstance` 和 `DesignInstancePins`，Pin ID 以只读 `std::span` 暴露；
- `forEachInstancePin()`、`forEachIoPin()`：使用 `view.each()` 直接把 component 引用交给 callback；
- `forEachRegularNet()`、`forEachSpecialNet()`：在 view 内完成 regular/special 过滤，不创建中间 ID vector；
- `forEachWire()`：遍历 Net 已有的 `wireIds()` span，不复制 wire ID 列表。

这些接口保持 registry dense order，不执行名称排序。callback 执行期间不能修改对应 storage，返回的 span 也不能跨越 storage 修改继续持有。

### Wrapper 本地索引与几何缓存

涉及：

- `src/operation/iRT/interface/RTInterfaceEnTT.cpp`

新增 `EntityPointerTable` 和 `WrapperEntityTables`，为 Instance、InstancePin、IoPin、Net、CellMaster、MasterObs、MasterTerm、MasterPort 建立只在本次 wrapper 生命周期内有效的 entity-indexed 指针表。热循环由 EnTT sparse-set lookup 转为连续数组边界检查和指针读取。

layer 映射同样使用 entity-indexed table，`LayeredRect` 可以缓存已解析的 layer index，避免每个 Shape 重复查层。

Master、MasterTerm 与 orientation 的几何处理结果被缓存。polygon 分解、方向变换、layer resolve 和 Via routing bounding box 合并只对每种组合执行一次；实例热循环只计算 origin offset。

这些 cache 都在进入 OpenMP 区域前串行构建，并在并行区域内保持只读，不改变底层 Storage 的 ownership 或失效规则。

### Instance Obstacle 固定区间并行

Instance Obstacle 使用两阶段算法：

```text
串行 count/cache
  -> 得到每个 Instance 的 routing/cut 数量
  -> 计算 prefix offset 和总数
  -> 精确 reserve/resize 目标 vector

OpenMP schedule(static)
  -> 每个 Instance 写自己的固定 subspan(offset, count)
```

`FixedObstacleSink` 只写预先分配的独占 `std::span<Obstacle>`，不在多个线程中调用共享 vector 的 `push_back()`，因此不需要锁，也不会改变 Instance 输出顺序。SpecialNet 与 IO Pin 产生的尾部 Obstacle 保持串行追加和原有顺序。

精确容量同时避免约 4966 万个 Obstacle 在 vector 扩容时发生大块内存迁移。test10 上 allocator 增量减少约 603.5 MiB，即约 17.4%。

### Net/Pin/Shape 并行

regular Net 先按 dense order 建立目标列表，每个 Net 对应一个固定的输出下标。OpenMP 使用：

```cpp
#pragma omp for schedule(guided, 256)
```

Net 的工作量由 Pin 数量、Shape 数量、Via 和小对象分配共同决定，差异明显；test10 还存在 2762 Pin 的大 Net。`guided` 相比连续区间的 `static` 能减少尾部线程等待。调度顺序不会改变结果顺序，因为每次迭代只写预先确定的 `net_list[index]`。

每个线程复用 routing shape、cut shape 和 Via bounding-box scratch vector，线程之间不共享 scratch，不引入锁。

### Benchmark 与差分入口

涉及：

- `src/database/refactor/benchmark/IrtInputBenchmark.cpp`
- `src/operation/iRT/interface/test/IrtAdapterDifferentialTest.cpp`

benchmark JSONL 新增 `threads` 字段，使用 `omp_get_max_threads()` 记录真实 OpenMP 最大线程数。

差分测试新增 `Wrap.ExternalLefDef`，通过以下环境变量读取指定的大型 LEF/DEF：

```text
IDB_REFACTOR_IRT_WRAP_LEF
IDB_REFACTOR_IRT_WRAP_DEF
```

该入口用于直接验证 benchmark 所用的原始 routed DEF，而不是只使用已有的小型 fixture。

## 性能结果

输入：

```text
LEF: reference/ispd2019/ispd19_test10/ispd19_test10.input.lef
     326,014 bytes
DEF: reference/ispd2019/test10/def/12.t10.def
     2,641,862,207 bytes，约 2.46 GiB
```

LEF/DEF 解析发生在计时区间外，以下只统计 `RTI.wrapDatabase()`。两条路线都生成 895,075 Nets、3,957,321 Pins 和 61,572,883 个 materialized shapes。

| 条件 | EnTTDB | iDB | EnTTDB 相对 iDB |
| --- | ---: | ---: | ---: |
| 1 线程 | 5.891 s | 9.864 s | 快 1.67x |
| 8 线程 | 2.420 s | 7.575 s | 快 3.13x |
| 128 线程 | 2.481 s | 6.735 s | 快 2.71x |

EnTTDB 从 1 到 8 线程为 2.43x 加速，8 线程是当前已测最优点。128 线程受串行准备、约 4966 万个 Obstacle 的 vector 初始化、小对象分配、内存带宽和 NUMA 开销限制，没有进一步收益。

同一数据集的内存结果：

| 数据源 | 线程 | Wrapper 前 RSS | Wrapper 增量 RSS | Wrapper 后 RSS |
| --- | ---: | ---: | ---: | ---: |
| EnTTDB | 1 | 2.740 GiB | 2.790 GiB | 5.530 GiB |
| EnTTDB | 8 | 2.740 GiB | 2.791 GiB | 5.531 GiB |
| EnTTDB | 128 | 2.740 GiB | 2.798 GiB | 5.538 GiB |
| iDB | 1 | 20.905 GiB | 2.790 GiB | 23.695 GiB |
| iDB | 8 | 20.905 GiB | 2.790 GiB | 23.695 GiB |
| iDB | 128 | 20.905 GiB | 2.796 GiB | 23.701 GiB |

两条路线的 wrapper 增量接近，因为最终物化的是同一套 iRT 对象；主要内存差异来自 wrapper 开始前保留的源数据库。

## 正确性验证

原始 2.64 GB routed DEF 已完成 iDB wrapper 与 EnTTDB wrapper 的完整内容差分：

```text
[       OK ] Wrap.ExternalLefDef (214985 ms)
[  PASSED  ] 1 test
wall time: 3:36.74
maximum resident set size: 33,268,036 KiB
swaps: 0
exit status: 0
```

比较范围包括 design 基本信息、routing/cut layer、track、spacing table、ViaMaster、Obstacle、Net、Pin 及其 routing/cut shapes。无语义顺序的 ViaMaster、Net、Pin 和 Shape 在比较前按稳定键 canonicalize，因此并行执行次序不会掩盖内容差异。

提交前的验证范围：

- targeted build：`idb_refactor_design_test`、`irt_input_benchmark`、`irt_adapter_differential_test`；
- `DesignStorageTest.*`：19/19 通过；
- `Wrap.Ispd19Sample`、`Wrap.Ispd19Test1`、`Wrap.Ispd19Test2`：3/3 通过；
- `Wrap.ExternalLefDef`：原始 2.64 GB Test10 完整差分通过；
- `git diff --check` 通过。

大型 Test10 没有运行 ASan/UBSan。Release 差分峰值已经约 31.73 GiB，sanitizer 会显著扩大内存需求；本次只构建和运行相关目标，没有全量构建 ecc-tools。

## 本次明确未修改的范围

- EnTT 第三方库和 EnTTDB component 定义；
- registry storage 类型与 GeometryPool 底层布局；
- Binary Archive 格式；
- iRT `Database` 的对象语义；
- iDB wrapper 实现；
- benchmark `results/` 目录和其他生成文件。

## 复现命令

```bash
cmake --build build/full-gcc \
  --target idb_refactor_design_test irt_input_benchmark irt_adapter_differential_test \
  -j128

OMP_NUM_THREADS=8 \
OMP_DYNAMIC=FALSE \
OMP_PROC_BIND=spread \
OMP_PLACES=cores \
IDB_REFACTOR_RT_THREAD_NUMBER=8 \
bin/irt_input_benchmark \
  --lef reference/ispd2019/ispd19_test10/ispd19_test10.input.lef \
  --def reference/ispd2019/test10/def/12.t10.def \
  --source entt \
  --output /tmp/12t10-entt-t8.jsonl

OMP_NUM_THREADS=128 \
OMP_DYNAMIC=FALSE \
OMP_PROC_BIND=spread \
OMP_PLACES=cores \
IDB_REFACTOR_RT_THREAD_NUMBER=128 \
IDB_REFACTOR_IRT_WRAP_LEF=$PWD/reference/ispd2019/ispd19_test10/ispd19_test10.input.lef \
IDB_REFACTOR_IRT_WRAP_DEF=$PWD/reference/ispd2019/test10/def/12.t10.def \
bin/irt_adapter_differential_test \
  --gtest_filter='Wrap.ExternalLefDef'
```

## 后续工作

下一步不应继续优化 8 线程下已经约 59 ms 的 Instance Obstacle 填充。优先分析串行 setup、cache/count、Obstacle vector 初始化以及每个 Pin 的名称和小 vector 分配。Binary Archive 的批量 Codec 与并行读写应作为独立提交，避免与本次 wrapper 改动混合。
