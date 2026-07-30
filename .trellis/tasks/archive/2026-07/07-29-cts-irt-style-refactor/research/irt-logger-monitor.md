# Research: iRT logger and monitor contract for iCTS

- Query: 以当前生产调用链为准，提炼 iRT logger/monitor 的技术栈、API、生命周期、输出、级别、所有权、并发假设、构建和测试契约，并明确 iCTS 必须遵循的部分。
- Scope: internal
- Date: 2026-07-29

## Findings

### Executive summary

iRT 没有使用 glog 或仓库 `LOG_*`：它在 `source/toolkit/` 内维护独立的 `Logger` 和 `Monitor`。生产入口严格按 `init_rt -> run_rt`（或 `run_ert`）`-> destroy_rt` 执行；`Logger` 是显式生命周期的进程内单例，提供 `RTLOG.info/warn/error(Loc::current(), ...)`，同时写彩色控制台和纯文本 `rt.log`；`Monitor` 是栈上局部对象，以构造时刻或上一次 `getStatsInfo()` 为基线，输出 wall time、进程 CPU time 和进程峰值内存增量。证据见 `scripts/design/ics55/steps/route.tcl:69-81`、`src/operation/iRT/interface/RTInterface.cpp:63-88`、`src/operation/iRT/interface/RTInterface.cpp:157-180`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:24-73`、`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:26-60`。

必须区分“生产契约”和“实现偶然性”：显式 API 生命周期、三种级别、调用点位置、双 sink、启动期缓存、monitor 的局部/lap 语义是生产主路径反复使用的稳定模式；`error()` 使用 `exit(0)`、logger 缺少并发同步、CMake 循环传递依赖则是可观察或潜在缺陷，不能在未评审的情况下机械复制到 iCTS。

### Files found

| File | Role |
|---|---|
| `src/operation/iRT/source/toolkit/logger/LogLevel.hpp` | 定义 `kNone/kInfo/kWarn/kError` 四个枚举值。 |
| `src/operation/iRT/source/toolkit/logger/Logger.hpp` | logger 公共 API、`RTLOG`/`Loc`、格式化、控制台/文件 sink 和启动期缓存。 |
| `src/operation/iRT/source/toolkit/logger/Logger.cpp` | 单例创建、惰性访问、销毁和静态实例所有权。 |
| `src/operation/iRT/source/toolkit/monitor/Monitor.hpp` | 栈对象 API 和三个基线值。 |
| `src/operation/iRT/source/toolkit/monitor/Monitor.cpp` | wall/CPU/memory 采样、格式化及 lap reset。 |
| `src/operation/iRT/source/toolkit/utility/Utility.hpp` | monitor 使用的字符串、时长和两位小数格式化。 |
| `src/operation/iRT/source/data_manager/DataManager.cpp` | 从 temp dir 派生 `rt.log`、重建输出目录并打开 log stream。 |
| `src/operation/iRT/source/data_manager/advance/Config.hpp` | `temp_directory_path` 输入和 `log_file_path` 派生配置的所有者。 |
| `src/operation/iRT/interface/RTInterface.cpp` | 生产 logger 初始化/销毁以及根 flow monitor 生命周期。 |
| `src/interface/tcl/tcl_irt/src/tcl_{init,run,destroy}_rt.cpp` | Tcl 命令到 `RTInterface` 的一对一转发。 |
| `scripts/design/ics55/steps/route.tcl` | 当前 ICS55 生产脚本的 init/run/destroy 顺序。 |
| `src/operation/iRT/source/toolkit/{logger,monitor}/CMakeLists.txt` | 两个独立 concrete library target。 |
| `src/operation/iRT/source/toolkit/CMakeLists.txt` | `irt_toolkit` 聚合 logger、monitor 和 utility。 |
| `src/operation/iRT/test/test_topo_builder/test_topo_builder.cpp` | 当前唯一启用的 iRT test 手工初始化/销毁 logger。 |
| `src/operation/iRT/test/CMakeLists.txt` | 只有 `test_topo_builder` 被启用；其余 legacy tests 被注释。 |

### 1. Technology stack and dependency shape

- iRT 根 CMake 选择 C++20；logger 仍使用 `std::experimental::source_location`，并依赖标准库 filesystem/fstream/iostream/thread/stringstream/time facilities。平台采样使用 POSIX/Linux `gettimeofday(2)` 和 `getrusage(2)`，OpenMP 用于生产算法并由配置设置线程数。证据：`src/operation/iRT/CMakeLists.txt:4-7`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:19-24`、`src/operation/iRT/source/data_manager/advance/RTHeader.hpp:21-27`、`src/operation/iRT/source/data_manager/advance/RTHeader.hpp:41-55`、`src/operation/iRT/interface/RTInterface.cpp:360-365`。
- 在整个 `src/operation/iRT` 的生产 `.cpp/.hpp/CMakeLists.txt` 中未找到 glog include、初始化、链接或 `LOG_*`/`LOG(...)` 调用。生产调用统一为 `RTLOG.info/warn/error`；静态扫描得到 433 个 info、22 个 warn、289 个 error 调用。
- logger 和 monitor 各自是 concrete library (`irt_logger`, `irt_monitor`)，再由 `irt_toolkit` INTERFACE target 聚合；算法 target 通过 `irt_toolkit` 获得 API。证据：`src/operation/iRT/source/toolkit/logger/CMakeLists.txt:9-20`、`src/operation/iRT/source/toolkit/monitor/CMakeLists.txt:9-20`、`src/operation/iRT/source/toolkit/CMakeLists.txt:1-17`、`src/operation/iRT/source/module/pin_accessor/CMakeLists.txt:9-24`。
- 当前 CMake 的声明依赖不是可复制的干净层次：logger/monitor 都 `PUBLIC` 链接 `irt_data_manager`，而 data manager 又 `PUBLIC` 链接聚合了 logger/monitor 的 `irt_toolkit`；monitor 实现直接 include `Logger.hpp` 和 `Utility.hpp`，却未直接链接相应 target。证据：`src/operation/iRT/source/toolkit/logger/CMakeLists.txt:13-16`、`src/operation/iRT/source/toolkit/monitor/CMakeLists.txt:13-16`、`src/operation/iRT/source/data_manager/CMakeLists.txt:9-16`、`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:17-20`。这是传递/循环 wiring，不应被解释为 logger 应依赖数据库的设计原则。

### 2. Logger API, ownership, and lifecycle

- `using Loc = std::experimental::source_location`；宏 `RTLOG` 只展开为 `irt::Logger::getInst()`。每次调用显式把 `Loc::current()` 放在首参，消息由任意数量、支持 `operator<<` 的片段拼接，logger 不自动插入片段间空格。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:22-27`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:56-71`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:145-166`。
- `Logger` 是不可复制/移动、私有构造/析构的裸指针单例；`initInst()` 幂等创建，`getInst()` 也可惰性创建，`destroyInst()` 删除并置空。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:28-33`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:76-89`、`src/operation/iRT/source/toolkit/logger/Logger.cpp:23-48`。
- 生产路径不依赖惰性创建：`RTInterface::initRT()` 在第一条日志之前显式调用 `Logger::initInst()`；`destroyRT()` 在所有模块、DataManager 和尾部日志完成后最后调用 `Logger::destroyInst()`。证据：`src/operation/iRT/interface/RTInterface.cpp:63-80`、`src/operation/iRT/interface/RTInterface.cpp:157-180`。
- `RTInterface` 的根操作和每个主要模块遵循同一模板：栈上构造 `Monitor`，记录 `Starting...`，执行工作，最后记录 `Completed` 加 `monitor.getStatsInfo()`。证据：`src/operation/iRT/interface/RTInterface.cpp:79-88`、`src/operation/iRT/source/module/pin_accessor/PinAccessor.cpp:60-73`、`src/operation/iRT/source/module/topo_builder/TOPOBuilder.cpp:51-58`。
- logger 拥有 log path、堆分配的 `std::ofstream*` 和打开文件前的 `_temp_log_list`；析构负责关闭 stream。Config/DataManager 只拥有/派生路径，不拥有 stream。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:35-47`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:76-87`、`src/operation/iRT/source/data_manager/advance/Config.hpp:28-45`。
- 日志文件不是在 logger 创建时打开。DataManager 从绝对化后的 temp dir 派生 `<temp_directory_path>/rt.log`，先删除并重建整个 temp dir/子目录，再调用 `RTLOG.openLogFileStream()`。证据：`src/operation/iRT/source/data_manager/DataManager.cpp:693-699`、`src/operation/iRT/source/data_manager/DataManager.cpp:711-765`。
- 因此，banner、根 `Starting...` 和 DataManager 配置准备阶段的日志先立即输出到控制台并缓存为纯文本；打开文件后的下一条日志先按顺序 drain 全部缓存，再写当前日志。证据：`src/operation/iRT/interface/RTInterface.cpp:65-83`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:127-142`。`openLogFileStream()` 自身不 drain，生产路径依靠后续日志触发。
- 退出路径先完成 DataManager output/destroy，再记录根 completion、打印 log path、输出尾 banner，最后销毁 logger/关闭文件。证据：`src/operation/iRT/interface/RTInterface.cpp:157-180`。ICS55 和 Sky130 生产脚本都按 `init_rt/run_rt/destroy_rt` 顺序调用，故这是 authoritative lifecycle：`scripts/design/ics55/steps/route.tcl:69-81`、`scripts/design/sky130/steps/route.tcl:69-81`。

### 3. Log levels and failure semantics

- `LogLevel` 定义 `kNone=0`, `kInfo=1`, `kWarn=2`, `kError=3`；公共方法只有 `info`, `warn`, `error`，不存在 debug、trace、fatal、verbosity threshold、条件宏或运行时过滤。`kNone` 仅作为 `printLog` switch 的 default 标签，生产无可调用入口。证据：`src/operation/iRT/source/toolkit/logger/LogLevel.hpp:19-27`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:56-74`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:97-114`。
- `info` 用于根/阶段 Starting/Completed、配置、进度、统计和结果路径；例如 PinAccessor 根阶段 `src/operation/iRT/source/module/pin_accessor/PinAccessor.cpp:60-73`，迭代 begin/end `src/operation/iRT/source/module/pin_accessor/PinAccessor.cpp:560-580`。
- `warn` 表示明确可继续的退化或跳过，并携带对象/原因上下文；例如 Steiner legalization 失败后仍返回候选坐标并继续构造拓扑：`src/operation/iRT/source/module/topo_builder/TOPOBuilder.cpp:151-165`。
- `error` 的实际语义是终止：先写 error，关闭 log stream，然后无条件 `exit(0)`。它不是可恢复 error，也没有独立 fatal。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:68-74`。
- 部分调用点在 `RTLOG.error` 后写了返回 safe value，例如非法 topology 输入后的 return (`src/operation/iRT/source/module/topo_builder/TOPOBuilder.cpp:123-130`)；这些 return 在当前实现中不可达。相反，很多单例 getter 在 error 后直接解引用空指针，明确依赖 error 不返回：`src/operation/iRT/source/module/topo_builder/TOPOBuilder.cpp:33-39`、`src/operation/iRT/source/module/pin_accessor/PinAccessor.cpp:42-48`。因此 authoritative severity 是“warn 可继续、error 终止”，不是调用点表面上的 recoverable variant。
- `exit(0)` 令失败以成功状态退出，是实际行为但不是健康的失败契约。若 iCTS 采用 iRT 级别模型，应在 Review 中单独冻结“终止状态码”；在用户禁止夹带行为变化的前提下，不能静默决定复制 `0`、改成非零或把 error 变成 recoverable。

### 4. Exact output/report behavior

- 每行逻辑格式为：`[RT <YYYYMMDD HH:MM:SS> <base62-thread-id> <file:line> <Info|Warn|Error> <function>] <concatenated-message>\n`。timestamp 来自本地 `time/localtime/strftime`；thread id 先把 `std::this_thread::get_id()` 流化为十进制字符串、`stoul`，再压缩为 base62。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:117-128`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:168-191`。
- Info/Warn 的 file field 只保留 basename+line；Error 保留 `filesystem::absolute(file:line)`。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:117-123`。
- 控制台行仅给 level token 加 ANSI 颜色：Info blue、Warn yellow、Error red、default green；文件行完全无颜色。所有级别始终写控制台；文件打开后所有级别也写文件。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:94-115`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:127-142`。
- 文件每行都显式 `flush()`，控制台未显式 flush，但行以 `\n` 结束。`std::ofstream(path)` 使用默认 truncate/open 语义；没有追加、rotation、size limit、级别筛选或 stream-open 校验。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:35-39`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:130-142`。
- `printLogFilePath()` 只在 path 非空时通过普通 Info 输出 `The log file path is '<path>'!`。`initRT()` 在 DataManager 配置前调用一次，因此首次是 no-op；`destroyRT()` 在关闭前再次调用并实际输出。证据：`src/operation/iRT/source/toolkit/logger/Logger.hpp:49-53`、`src/operation/iRT/interface/RTInterface.cpp:75-75`、`src/operation/iRT/interface/RTInterface.cpp:168-170`。
- iRT logger 的 `rt.log` 是 console/runtime log 的纯文本镜像，不是独立 structured report/schema sink。算法 CSV/JSON/guide/GDS 等 artifacts 另走 `Utility::getOutputFileStream` 等路径；不得把这些 artifacts 和 logger sink 混为一个职责。证据：`src/operation/iRT/source/toolkit/utility/Utility.hpp:3341-3361`、`src/operation/iRT/source/module/planar_router/PlanarRouter.cpp:138-144`。

### 5. Monitor semantics and use patterns

- `Monitor` 是轻量栈对象；构造立即 `init()`，也就是采样三个初始基线。它没有单例、显式 init/destroy、析构副作用或共享结果仓库。证据：`src/operation/iRT/source/toolkit/monitor/Monitor.hpp:23-49`、`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:51-60`。
- `getElapsedTime()` = 当前 wall clock - baseline；`getCPUTime()` = 当前进程 user+system CPU - baseline；`getUsageMemory()` = 当前 `ru_maxrss/1000` - baseline。证据：`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:34-47`、`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:63-87`。
- wall clock 使用 `gettimeofday`，不是 monotonic clock；CPU/memory 使用 `getrusage(RUSAGE_SELF)`，所以 CPU 是整个进程（含 worker）的累计 CPU，memory 是 Linux 下进程最大 RSS 的增量，不是当前 RSS，也不是对象独占内存。header 的 `_init_usage_memory // GB` 注释与实现输出 MB 不一致；实现和输出字符串是 authoritative。证据：`src/operation/iRT/source/toolkit/monitor/Monitor.hpp:38-42`、`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:63-87`。
- 组合字符串精确为 ` (elapsed = HH:MM:SS, cpu = HH:MM:SS, mem = NN.NNMB) `（有首尾空格）。elapsed/CPU 秒数先 round 到整数，再以 `%02d:%02d:%02d` 格式化；memory 用 `%02.2f`。证据：`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:26-30`、`src/operation/iRT/source/toolkit/utility/Utility.hpp:3549-3576`。
- `getStatsInfo()` 在生成字符串后调用 `updateStats()`，所以第一次表示“构造至今”，后续调用表示“上一次 `getStatsInfo()` 至今”的 lap。单独调用 `getElapsedTime/getCPUTime/getUsageMemory` 不重置 baseline。证据：`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:26-47`、`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:56-60`。
- iRT 在三种粒度嵌套使用 monitor：函数/root total (`PinAccessor.cpp:60-73`)、iteration lap (`PinAccessor.cpp:560-580`) 和批次/stage progress (`SpaceRouter.cpp:448-476`, `DetailedRouter.cpp:371-404`)。局部 monitor 只负责采样/字符串，不负责直接输出；调用者把结果附加到 Info log。
- `getrusage` 失败时调用 `RTLOG.error`，因此 monitor 采样失败按当前 logger 语义终止整个进程。证据：`src/operation/iRT/source/toolkit/monitor/Monitor.cpp:71-87`。

### 6. Threading and concurrency evidence

- 生产配置在 logger 已创建后、进入算法前调用 `omp_set_num_threads`，而主要路由模块广泛使用 OpenMP。证据：`src/operation/iRT/interface/RTInterface.cpp:360-365`、`src/operation/iRT/source/module/space_router/SpaceRouter.cpp:448-473`、`src/operation/iRT/source/module/detailed_router/DetailedRouter.cpp:371-401`。
- 正常进度日志和 monitor 通常位于 parallel region 外的阶段边界；这减少了并发写。例：SpaceRouter parallel loop 是 `451-470`，进度日志在 `472-473`；DetailedRouter parallel loop 是 `374-398`，进度日志在 `400-401`。
- 但 worker 路径可以直接触发 logger。例如 PinAccessor 的 OpenMP loop 内部在 access-point 为空时调用 `RTLOG.error`：`src/operation/iRT/source/module/pin_accessor/PinAccessor.cpp:139-151`。大量 parallel body 还会调用可能 error 的公共 utility/helper。
- `Logger` 的单例创建/销毁、`_temp_log_list`、ofstream、`std::cout` 和 timestamp 的 `localtime` 都没有 mutex/atomic/`call_once`；iRT source 中也未找到包围日志的 OpenMP critical 或 C++ lock。thread id 字段证明日志希望区分线程，但不能证明实现是 thread-safe。证据：`src/operation/iRT/source/toolkit/logger/Logger.cpp:23-48`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:76-82`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:117-142`、`src/operation/iRT/source/toolkit/logger/Logger.hpp:168-179`。
- 可可靠提炼的生命周期假设是：在 API/主线程、worker 尚未启动时 init/open；在所有 worker quiescent 后 destroy。iCTS 若允许 worker logging，必须在保持格式/顺序契约的同时串行化 sink；不能宣称“照搬 iRT 即线程安全”。
- 每个 `Monitor` 通常只由创建它的外层线程读取，内部没有共享 mutable state；其 `RUSAGE_SELF` 指标有意覆盖整个进程的并行工作，而不是某个线程。

### 7. Tests and verification evidence

- iRT 没有 logger 或 monitor 单元测试目录/target。当前 `src/operation/iRT/test/CMakeLists.txt:6-14` 只启用 `test_topo_builder`；其他 legacy test targets 被注释。
- active topo-builder test 只在 `main` 中手工 `Logger::initInst()`/`destroyInst()`，从不打开 log file，也不断言 console/file 格式、level、buffer、failure 或 monitor 数值。证据：`src/operation/iRT/test/test_topo_builder/test_topo_builder.cpp:384-412`、`src/operation/iRT/test/test_topo_builder/CMakeLists.txt:1-12`。
- active test 的 invalid-input cases 会到达 `RTLOG.error` 后的 nominal fallback 路径 (`test_topo_builder.cpp:406-407`, `TOPOBuilder.cpp:123-130`)；由于 `error()` 是 `exit(0)`，test 可提前以成功状态退出，后续断言和 cleanup 不执行。这进一步说明现有 tests 不能用来判断 error 可恢复，也不能作为 iCTS logger 验收模板。
- 被 CMake 注释的 legacy tests 中存在直接 `std::cout`，但它们不是当前生产/active-test 调用规范；authoritative path 是 production source 的 `RTLOG` 和 active test 的显式 Logger lifecycle。
- iCTS 迁移至少需要补齐以下可验证项：API init/open/destroy 平衡；pre-open 日志有序 drain；console 有 ANSI 而 file 无 ANSI；prefix/Loc/message 拼接；Info/Warn 继续；Error 的已评审终止状态；monitor 首次 total、二次 lap；nested monitor；并发 sink 完整行。时间/内存测试应断言格式和单调/区间性质，避免依赖精确秒数或 RSS。

### 8. Ambiguous variants and authoritative choices

| Ambiguity | Evidence | Authoritative choice for planning |
|---|---|---|
| 显式 init 还是 `getInst()` 惰性 init | 两者都存在 (`Logger.cpp:23-35`)；生产入口显式 init (`RTInterface.cpp:63-65`) | 以 API 边界显式 init/destroy 为契约；惰性 get 只作内部防御，不可替代生命周期编排。 |
| production dual sink 还是 test console-only | DataManager 打开 `rt.log` (`DataManager.cpp:693-765`)；topo test 不打开文件 (`test_topo_builder.cpp:391-411`) | 生产必须 console + file；测试可注入临时路径或 console-only，但不能据此删除 file sink。 |
| `error` 可恢复还是 fatal | 有 error 后 return (`TOPOBuilder.cpp:123-130`)；实现无条件 exit (`Logger.hpp:68-74`)；单例 getter 要求不返回 (`TOPOBuilder.cpp:33-39`) | 级别语义以实际实现为准：Error terminal。`exit(0)` 状态码单独评审，不视为可复制的设计优点。 |
| `kNone`/Debug 是否是可用级别 | `kNone` 在 enum/switch；CMake 有 DEBUG flags，但 public API 只有 info/warn/error | 不建立 None/Debug 调用规范；CMake debug flag 只改变 build type (`logger/CMakeLists.txt:1-7`, `monitor/CMakeLists.txt:1-7`)。 |
| monitor 是总量还是区间 | 构造采样，`getStatsInfo()` 后 reset (`Monitor.cpp:26-30`, `51-60`) | 首次为 total，后续为 lap；嵌套 monitor 各自独立。 |
| memory 是当前 RSS/GB 还是 peak delta/MB | header 注释 GB (`Monitor.hpp:39-41`)；实现 `ru_maxrss/1000` 并输出 MB (`Monitor.cpp:44-47,81-87`) | 以实现为准：peak-RSS delta in MB；不要按注释改成 GB。 |
| 直接 cout 是否允许 | logger 内部用 cout (`Logger.hpp:142`)；仅 dormant tests 直接 cout | call sites 统一 Logger；console sink 属 logger 内部实现，不允许模块绕过。 |
| CMake 循环依赖是否是架构模式 | logger/monitor -> data_manager -> toolkit -> logger/monitor | target 独立和 toolkit 聚合是模式；循环/传递依赖不是，iCTS 应显式声明 acyclic direct dependencies。 |
| thread id 是否代表线程安全 | prefix 有 thread id；mutable sinks 无任何同步；worker 可触发 error | thread id 是格式契约，不是安全保证；iCTS lifecycle 在 worker 外，sink 对并发调用需保护。 |

### 9. Contract iCTS must follow

以下条目是建议在用户 Review 后冻结的 iRT -> iCTS 迁移契约；名称替换为 iCTS 语义（例如 namespace `icts`、`CTSLOG`、tag `CTS`、`cts.log`），但行为模式不变。

1. **Single stack**：iCTS runtime source 只通过本地 iRT-style logger 调用，不保留 glog include、宏、初始化、链接、adapter 或平行兼容路径；模块调用点不得直接使用 `std::cout/printf`。
2. **API shape**：提供显式 `initInst/getInst/destroyInst` 生命周期和单一 access macro；每次 `info/warn/error` 显式传 `Loc::current()`，后续参数按 stream fragments 拼接。禁止同时保留 `LOG_* <<` 与新 API。
3. **Severity**：只公开 Info/Warn/Error；Info 用于 stage/progress/summary，Warn 表示 flow 明确可继续，Error 表示 terminal invariant failure。Error 的具体退出状态/终止机制必须在实现前 Review 冻结，因为 iRT 的 `exit(0)` 与正常失败报告相冲突。
4. **Output contract**：console 始终输出、level token 着色；`cts.log` 写同一纯文本行且无 ANSI；prefix 保留 module tag、秒级本地 timestamp、compressed thread id、source file+line、level、function；消息不隐式补空格；每行换行，文件及时 flush。
5. **Startup buffering**：Logger 在 API init 的第一条日志前创建；输出目录和 `<temp>/cts.log` 路径由 runtime/config owner 建立后打开；在此之前的 banner/startup 日志立即到 console 并缓存，打开后按原顺序写入 file。
6. **Balanced ownership**：API/root flow 是 logger lifecycle 唯一 owner；logger 自己拥有 stream/buffer，config 只拥有路径；正常 reset/destroy 在所有 worker 和模块结束后写 completion/path/banner，再关闭文件和销毁 logger。不得在任意模块中各自初始化或关闭全局 logger。
7. **Monitor model**：Monitor 是函数/阶段/迭代栈对象，构造采样；调用者在 `Starting...`/`Completed + stats` 边界使用；`getStatsInfo()` 输出 elapsed/cpu/mem 并推进 baseline，从而支持 lap；Monitor 不直接写日志、不做全局注册。
8. **Metric semantics**：elapsed 为 wall interval，cpu 为 `RUSAGE_SELF` user+system interval，mem 为 `ru_maxrss` peak delta MB；格式保持 ` (elapsed = HH:MM:SS, cpu = HH:MM:SS, mem = NN.NNMB) `。若要改为 monotonic/current RSS，属于行为设计变化，须另行评审。
9. **Concurrency boundary**：init/open/destroy 只在并行工作之外发生；生产进度尽量在 parallel region 外记录。因为 worker failure 路径可写 log，iCTS sink 必须保证完整行和内部状态不发生 data race，同时保持上述外部格式。
10. **Build boundary**：Logger、Monitor（及它们真正需要的 formatter/utility）应是独立 target，由 toolkit/utility aggregate 暴露；Logger 不依赖 CTS database，Monitor 显式依赖 logger/formatter。遵循 iRT 的职责拆分，不复制其循环 CMake wiring。
11. **Reports remain separate**：`cts.log` 按 iRT 定义是 runtime log mirror；CSV/JSON/tables/artifacts 继续由 report/artifact owner 生成，不把它们塞进 Logger。是否保留现有 SchemaWriter 是另一架构决策，但不能形成第二套 console runtime logging。
12. **Tests are part of migration**：新增 logger/monitor contract tests，并用 subprocess/death-test 验证 Error；不得复用 iRT 当前会因 `exit(0)` 提前“通过”的测试形态。

### Related specs

- Task PRD 要求以 iRT 为唯一目标范式、移除 glog、冻结 logger/monitor 契约并保持用户可观察行为：`.trellis/tasks/07-29-cts-irt-style-refactor/prd.md:3-21`, `:23-32`。
- 当前 `.trellis/spec/backend/logging-guidelines.md:11-24` 要求 repository `LOG_*`、把 runtime file output 交给 structured helpers，并把 Error/Fatal 分开；`:56-67` 又禁止 `std::cout` 和 dual-write wrapper。它与 iRT 的 `RTLOG`、console+file mirror、Error-terminal 三层面直接冲突。
- `.trellis/spec/project-constraints.md:70-78` 重复了 repository `LOG_*`、structured report 和 no-`std::cout` 要求。实施前必须通过正式 spec update 对齐已批准契约；不能一边声称严格 iRT、一边保留相反规范。
- iCTS 文件后缀/命名仍受 project constraints 约束；“iRT style”应迁移职责和调用模式，不应机械复制 `.hpp/.cpp` 后缀。

### External references

- 未使用外部资料；结论全部来自当前仓库生产源码、CMake、脚本和 active tests。
- POSIX API 名称及 Linux `ru_maxrss` 语义仅用于描述源码中实际调用；若任务目标包含非 Linux 平台，需另行做 portability research。

## Caveats / Not Found

- 未找到 iRT logger/monitor 的独立单元测试、golden `rt.log`、格式规范文档、log-level 配置、rotation、async queue、signal-safe 处理或显式线程安全说明。
- 未运行 iRT flow；本研究是静态代码证据。没有仓库内 `rt.log` 样本可用于逐字节比对。
- `Logger::openLogFileStream()` 未关闭已有 stream、未验证 open 成功；`closeLogFileStream()` 删除后未置空。生产 balanced single-open/single-destroy 路径避开了多数问题，但 repeated init/open/reset 或 error 后继续 cleanup 没有可靠契约。
- `Logger::error()` 的 `exit(0)`、`localtime`/sink 并发风险、`gettimeofday` 非 monotonic、CMake 循环依赖和 monitor memory 注释错误均已明确标为 caveat；未经用户批准，不应把修复这些问题夹带进“风格重构”，也不应不加说明地复制。
- 最关键的规划阻塞是现有 iCTS specs 与用户指定的 iRT logger 模式相反。用户批准 Review 方案后，应先冻结 Error 终止状态、双 sink/structured report 边界和并发保证，再更新 spec/context，之后才能实现。
