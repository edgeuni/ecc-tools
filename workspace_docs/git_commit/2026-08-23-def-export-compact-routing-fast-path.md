# Commit 记录：DEF Export 紧凑 Routing 快速路径

日期：2026-08-23
分支：`main`
Commit：本文件随本次提交创建；最终 hash 以 `git log -1` 为准。

## 提交目的

优化 EnTTDB 在大型已布线设计上的 DEF 文本导出。上一阶段已经完成 64 KiB 文本缓冲、整数 `std::to_chars()` 格式化、取消 Net 无条件排序以及 `wireIds()` / `forEachPath()` 批量遍历。本次继续减少 routing 热路径中的语义对象重建、名称查询和碎片化格式化开销：

```text
DesignRoutingPool 连续记录
  -> DesignRoutingCompactPathView（只读 span）
  -> 顺序关联 point/via/rectangle/extra
  -> 栈上组合 routing token
  -> DefTextOutput 缓冲写出
```

本次不修改 DEF Import 或 Si2 parser，不修改 RoutingPool 的底层数组和磁盘格式，也不修改 BinaryArchive 编解码实现。

## 修改范围

### Routing 紧凑只读视图

涉及：

- `src/database/refactor/design/routing/pool/DesignRoutingPool.h`
- `src/database/refactor/design/routing/storage/DesignRoutingStorage.h`

新增 `DesignRoutingCompactPathView` 和 `forEachCompactPath()`。视图直接提供现有 design-wide 数组中的只读切片：

- `DesignRoutingPointRecord`；
- `DesignRoutingViaRecord`；
- `DesignWireRectangle`；
- path、point、via 的稀疏 extra；
- point/via 在全局数组中的起始下标。

调用方可以通过全局起始下标将稀疏 extra 与局部记录关联，不必为每条 Path 重建 `DesignWirePointRange` 和 `DesignWireViaRange` 等语义包装。

原有 `forEachPath()` API 保留，并在内部基于紧凑视图构造原来的 `DesignWirePathView`，因此现有调用方不需要改动。Storage 层只提供转发接口，exporter 不直接访问 Pool 私有数组。

该改动没有改变：

- RoutingPool 数组布局；
- Path/Point/Via/Rectangle 记录大小；
- entity 生命周期；
- BinaryArchive schema。

### DEF routing 输出热路径

涉及：

- `src/database/refactor/export/def/DefDesignExporter.cpp`

主要实现如下。

#### 1. Routing token 栈上合并

新增 128 字节 `SmallBuffer`，使用 `std::to_chars()` 格式化坐标和计数。Point、RECT、VIA MASK 和 VIA ARRAY 的多个小字段先在栈上组合，再一次追加到 `DefTextOutput`，减少每个 Shape 上的多次输出函数调用。

该缓冲区只用于有明确长度上界的 routing 基本 token；原有 64 KiB `DefTextOutput` 仍负责面向文件的批量写入。

#### 2. Layer/Via 名称缓存

新增只在一次导出期间存活的 `DefNameCache`：

- Tech Layer 和 Tech Via 使用 entity index 对应的稠密 vector；
- Design Via 使用 packed entity 到名称指针的 hash 表；
- 稠密表同时记录 packed entity，检查 entity generation，避免把失效 ID 映射到复用后的槽位。

导出过程中数据库不发生修改，因此缓存保存的只读名称指针在本次 `write()` 生命周期内有效。该缓存保存通用数据库对象名称，不保存 iRT 专用结构。

#### 3. 直接消费紧凑 Path

`writeNetSection()` 改用 `forEachCompactPath()`。`writePath()` 直接读取 Point/Via/Rectangle 记录，并用前向 cursor 关联稀疏 point/via extra。

对于按 `point_index` 排序的 VIA 和 RECTANGLE，使用单调 cursor 完成线性合并；若输入不是有序数据，保留扫描或 `lower_bound()` 回退，避免快速路径改变兼容语义。

#### 4. 正确性检查保持

快速路径仍检查：

- regular Net 与 Special Net 的 WIDTH 规则；
- path layer 和 Via 引用有效性；
- VIA mask/array 对应的 extra 是否存在；
- Tech/Design Via 类型和 entity generation；
- point extension、virtual point、RECT 和 Via 与 anchor point 的关系。

### Benchmark 入口

涉及：

- `src/database/refactor/benchmark/BinaryArchiveBenchmark.cpp`
- `src/database/refactor/benchmark/CMakeLists.txt`

现有 BinaryArchive benchmark 增加互斥输出模式：

```text
--archive-dir DIR
--def-export FILE
```

`--def-export` 复用同一套 LEF/DEF 或 BinaryArchive 输入、数据库计数和 JSONL 计时框架，只执行 `DefDesignExporter::write()` 并记录 `def_text_export` 和输出字节数。文件名仍为 BinaryArchiveBenchmark，但本次没有在其中增加或修改 Archive codec。

### 回归测试

涉及：

- `src/database/refactor/design/test/DesignRoutingConstraintStorageTest.cpp`

扩展 `StoresMultiplePathsInDesignWideTypedPools`，验证紧凑视图中的：

- Path 数量与 WIDTH/SHAPE extra；
- Point/Via span 和全局起点；
- 稀疏 point/via extra 的全局下标；
- Rectangle 的局部 anchor point 下标。

## Test10 性能结果

数据：

```text
LEF: reference/ispd2019/ispd19_test10/ispd19_test10.input.lef
DEF: reference/ispd2019/test10/def/12.t10.def
DEF size: 2,641,862,207 bytes，约 2.46 GiB
Routing shapes: 93,956,838
```

每个阶段运行三次，下表使用中位数：

| 阶段 | DEF Export 中位数 | 相对前一阶段 |
| --- | ---: | ---: |
| 本次改动前基线（`3b5161ab0`）：64 KiB 缓冲与通用 `forEachPath()` | 15.475 s | 基线 |
| Routing token 专用格式化 | 11.709 s | 降低 24.3% |
| 紧凑 Path API 与名称缓存 | 9.196 s | 再降低 21.5% |
| 当前提交：增加顺序 cursor | 9.428 s | 回升约 2.5% |

当前提交相对改动前基线的总体结果：

```text
15.475 s -> 9.428 s
耗时降低约 39.1%
约 1.64x 加速
约 9.97 M routing shapes/s
```

顺序 cursor 本身没有带来额外收益。最终保留它主要是因为它明确表达有序 VIA/RECTANGLE 与 Point 的线性关联，同时保留无序回退；本轮主要收益来自 routing token 合并、紧凑 Path 读取和名称缓存。后续性能分析不能把全部收益归因于 cursor。

以上数字来自已存在的 benchmark 结果文件；结果目录被 `.gitignore` 排除，不随提交进入 Git。

## 构建与验证

仅使用已有 `build/full-gcc` 配置构建相关目标，没有全量构建 ecc-tools：

```bash
cmake --build build/full-gcc \
  --target \
    idb_refactor_design_test \
    idb_refactor_def_design_roundtrip_test \
    idb_refactor_binary_archive_benchmark \
  -j
```

本次提交前验证结果：

- Ninja 报告三个目标均为最新，无需重新编译；
- `idb_refactor_design_test`：19/19 通过；
- `idb_refactor_def_design_roundtrip_test`：8/8 通过；
- `idb_refactor_binary_archive_benchmark --help` 正确显示 `--def-export`；
- `git diff --check` 通过。

本次没有重新运行耗时较长的 2.64 GiB test10 导出，也没有重新运行完整 DEF corpus 差分；性能表复用本轮开发过程中保存的三次实测结果。

## 后续方向

- 若继续压缩单线程 DEF Export，下一步应 profile 当前 9 秒版本，区分名称/连通读取、文本格式化和系统写入占比；
- cursor 阶段已经显示边际收益为负，不应继续围绕相同循环做小幅改写；
- 若目标继续接近 5 秒，需要评估并行生成分段文本后按 Net 顺序合并，而不是修改 RoutingPool 底层布局；
- DEF Import/Si2 callback adapter 的实验已撤回，不属于本提交。
