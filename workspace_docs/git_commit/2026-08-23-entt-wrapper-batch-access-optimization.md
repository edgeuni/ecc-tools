# Commit 记录：EnTT wrapper 批量访问与 Obstacle 构造优化

日期：2026-08-23
分支：`main`
Commit：本文件随本次提交创建；最终 hash 以 `git log -1` 为准。

## 提交目的

优化 EnTTDB 到 iRT `Database` 的转换路径。此次修改保持 EnTTDB 的 component 定义、EnTT registry、GeometryPool 存储布局和持久化格式不变，只优化 iRT 适配层对已有数据的访问和目标对象物化过程。

## 代码改动

### EnTT component 访问

涉及：

- `src/operation/iRT/interface/RTInterfaceEnTT.cpp`

主要改动：

- layer lookup 从 `unordered_map` 改为按 entity index 的适配层数组，并保留 packed generation 检查；
- Net skip 判断使用 entity index cache，并在初始化阶段预计算 regular Net；
- wrapper 热循环直接读取 Design/Library registry component，避免重复经过 Storage API 的有效性检查；
- Instance Pin、Net Pin 和 IO Pin 关联数据使用只读 `std::span`，不复制已有 vector；
- geometry 矩形遍历增加 callback 路径，减少 Port/OBS 遍历中的临时 `vector<Rect>`；
- obstacle 和 regular-net 转换复用 scratch vector，避免每个 Pin/Port 重复分配临时容器。

这些改动属于 API 使用和适配层优化，没有修改 EnTT 或 EnTTDB 的底层 storage 实现。

### Obstacle 构造

涉及：

- `src/operation/iRT/source/data_manager/advance/Obstacle.hpp`

增加带有坐标和 layer 参数的直接构造函数，使 wrapper 可以直接 `emplace_back` 构造目标 Obstacle，减少默认构造、setter 调用和中间对象移动。

## 未纳入的实验

曾尝试根据 Instance/IO Pin 数量为 routing/cut obstacle vector 预留容量，但在实际 test10 上造成回归：运行时间变慢，同时 allocator 占用显著增加。因此 reserve 代码已经移除，没有进入本次提交。

本次也没有修改：

- EnTT 第三方库；
- `src/database/refactor` component 定义和 registry storage；
- GeometryPool 的底层数组布局；
- iRT 的 obstacle/net 数据结构语义；
- DEF/LEF/Binary 文件格式。

## 性能测试

输入设计为 ISPD2019 test10，所有结果统计相同的 `irt_database_wrap` 阶段，LEF/DEF 解析时间不计入。

| 输入 | EnTTDB | iDB | iDB 相对速度 |
|---|---:|---:|---:|
| `ispd19_test10.input.def`，134.6 MiB | 7.97～8.04 s | 5.78 s | 约 1.38x |
| `12.t10.def`，2.64 GB routed DEF | 8.52 s | 6.76 s | 约 1.26x |

两条路线的 materialized shape 数量一致：`61,572,883`。wrap 阶段增量 RSS 均约 `2.79 GiB`；EnTTDB 的 allocator 增量约 `3.38 GiB`，iDB 约 `2.80 GiB`。iDB 的总 RSS 更高，主要来自其在转换前保留的传统数据库对象图，而不是 iRT 目标数据库本身。

2GB routed DEF 和 134 MiB 输入的转换计数相同，说明当前 benchmark 主要衡量数据库到 iRT 数据的物化，不代表完整 routed wire geometry 的转换成本；DEF 解析阶段在计时区间之外。

## Profile 依据

在 2GB 输入上的 gperftools 采样显示，优化前主要热点为：

- `appendObstacle`：约 24.3% flat；
- `entt::basic_sparse_set::index`：约 13.9%；
- allocator/operator new、`collectPortGeometry` 和 `analyzeNetSkip` 也占较大比例。

本次修改针对对象构造、临时 geometry 和重复 component lookup，profile 临时代码已全部移除，仓库中不保留 profiler 依赖。

## 验证

- `cmake --build build/full-gcc --target irt_interface irt_input_benchmark -j 128` 通过；
- `cmake --build build/full-gcc --target irt_adapter_differential_test -j 128` 通过；
- `Wrap.Ispd19Sample` 通过；
- `Wrap.Ispd19Test1` 通过；
- `Wrap.Ispd19Test2` 通过；
- 三个 wrapper 差分测试的对象计数和几何结果保持一致；
- `git diff --check` 通过。

## 后续方向

下一步应在 `src/database/refactor` 的 Storage 层增加基于 EnTT view 的只读批量 API，例如 `forEachInstance`、`forEachNet` 和 Library hierarchy view。该方向仍然可以保持底层 component storage 不变，再通过 benchmark 验证是否能进一步减少 sparse-set lookup 和临时 ID vector。

full-owning group、component pool 排序和全局 geometry pool 改造暂不与本次 wrapper 优化混合，待批量 API 完成并重新 profile 后单独评估。
