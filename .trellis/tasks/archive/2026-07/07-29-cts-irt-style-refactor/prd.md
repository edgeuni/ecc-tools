# CTS iRT-style systematic refactor

## Goal

在不改变 CTS 算法、设计写回、QoR 和指定 iEDA 脚本结果的前提下，将 `src/operation/iCTS/` 系统化重构为 iRT 的目录和设计范式：以 CTS 输入建库、综合、优化、实例化、评估和产物输出的真实业务边界重建全局 DataManager、模块本地模型和结果提交模式；目录、Logger/Monitor 和 CMake 随业务 ownership 一并收敛，最终不存在新旧两套架构或观测语义。

## Confirmed decisions

- 只使用当前父任务完成全部工作，不创建 Trellis child/sub task；实施过程使用同一任务内的顺序检查点。
- iCTS 顶层最终只保留 `interface/`、`source/`、`test/`；`source/` 下一层只保留 `data_manager/`、`module/`、`toolkit/`，与 iRT 对齐。
- 当前 `api/`、`external_libs/`、`source/database/`、`source/flow/`、`source/utils/` 必须完成责任迁移后删除，不保留 forwarding header、alias target 或兼容目录。
- 全部运行日志使用 `CTSLOG.info/warn/error(Loc::current(), ...)`，不再使用 glog、`Log.hh` 或 SchemaWriter 日志接口。
- 只生成一个主日志 `cts.log`，它是彩色 console 的无 ANSI 纯文本镜像；不生成 `cts_detail.log` 或 `cts_runtime.log`。
- `SchemaWriter`、`ReportSink`、`StageScope`、`RuntimeMetricScope` 及其 default/detail 双 sink 语义从最终代码删除。仍有必要保留的运行信息转为 `CTSLOG`；DEF、Verilog、JSON、CSV、SVG、GDS 等非日志产物继续由各自 owner 生成。
- 以重构前主 `cts.log` 的业务语义为日志保留基线：输入建库、clock ownership、clustering、characterization、H-tree 候选与选择、optimization 过程、实例化、评估和报告摘要必须迁移到唯一 `CTSLOG`；重构前 `cts_detail.log` 的独立 sink 和逐对象调试噪声不迁移。
- iRT Logger/Monitor 的调用和输出契约严格对齐；已知安全缺陷不复制：terminal Error 使用失败状态，logger/timestamp/sink 支持并发安全，CMake 不形成环。
- 采用 iRT-style global DataManager，并明确支持“每进程仅一个活动 CTS session”：同一进程内可顺序重复 init/run/reset，CTest 等多进程并行保持支持，不再保留同进程双 runtime 能力；证据见 `research/global-data-manager-feasibility.md`。
- 目录迁移必须由业务职责、状态所有权和提交边界驱动；仅改名、搬文件、增加 adapter/translator/forwarder 维持旧调用链属于验收失败。

## Requirements

### R1 — Align the outer directory model

最终结构为：

```text
src/operation/iCTS/
├── CMakeLists.txt
├── interface/
├── source/
│   ├── data_manager/
│   ├── module/
│   └── toolkit/
└── test/
```

- `interface/` 承担外部 API、生命周期和根编排；保留现有 `CTSAPI` public method/status 行为，但 in-repo include 路径迁移到 `iCTS/interface/CTSAPI.hh`。
- `data_manager/` 成为唯一稳定运行数据 owner，接收当前 database 和 `CTSRuntime` 的职责；最终不并存 `CTSRuntime` 与另一套 DataManager 语义。
- `module/` 包含 synthesis、optimization、instantiation、evaluation、output/report 等行为 facade 及其 CTS 算法实现；输入建库由 `data_manager/` 直接承担，当前 Setup facade 与独立 `source/flow/` 均消失。
- `toolkit/` 包含 logger、monitor、utility/geometry 等无业务状态基础设施；visualization/report artifact writer 按行为归入 module，不作为第二套 logger。
- `external_libs/` 中的依赖声明下沉到真实 owner target，最终不保留独立顶层目录。
- 两层骨架严格与 iRT 一致；更深层目录以 CTS 责任命名，不机械复制 routing 专用模块名。文件继续使用仓库现行 `.hh/.cc` 规则。

### R2 — Replace logging with one iRT-style contract

- Logger 放在 `source/toolkit/logger/`，提供唯一入口 `CTSLOG.info/warn/error(Loc::current(), fragments...)`。
- Logger 在 interface/root 显式 init/open/close/destroy；文件路径由运行目录 owner 确定为 `<work_dir>/cts.log`。
- console level token 着色，`cts.log` 不含 ANSI；prefix、thread id、source location、fragment 拼接、启动缓存和逐行 flush 遵循 iRT。
- `Info` 表示正常进度和必要结果，`Warn` 表示可恢复降级，`Error` 表示不可恢复 invariant failure。
- 当前 recoverable `LOG_ERROR` 按真实控制流迁移为 Warn 加 typed status/diagnostic，不得机械升级为 terminal Error。
- 主日志保留高信息密度的业务过程：生命周期、用户可行动的诊断、关键输入输出、每阶段约束与输入规模、聚合的候选/迭代进展、选择依据、runtime profile、最终 QoR 和产物状态。逐 characterization sample、逐 optimization trial、逐 net route injection、临时对象 dump 等低层噪声不输出；候选和迭代信息不得被整体删除。
- `cts_report` 使用同一份 canonical table text 写入 `.rpt` 并逐行通过 `CTSLOG.info` 镜像到 console/`cts.log`，不得维护两套表格数据或格式语义。
- 最终不存在 glog header/link/symbol、repository `Log.hh`、`LOG_*`、SchemaWriter 日志调用或兼容 wrapper。

### R3 — Use the iRT Monitor model only

- Monitor 位于 `source/toolkit/monitor/`，是函数、阶段或有意义迭代中的局部栈对象。
- 调用遵循 `Starting...` 与 `Completed + monitor.getStatsInfo()`。
- elapsed、process CPU、peak RSS delta、字符串格式及 total/lap 语义与 iRT 一致。
- 删除基于 SchemaWriter/`ieda::Stats` 的 CTS runtime metric stack；必要 runtime summary 直接由同一个 Monitor 数据输出到 `CTSLOG`。

### R4 — Align architecture without duplicate semantics

- `CTSAPI`/interface 是唯一 external facade 和根生命周期 owner；根业务顺序保持 `DataManager input/build database -> synthesis -> optimization -> instantiation -> evaluation -> output/report`。
- 每个 module behavior 只有一个外部 facade；内部 candidate/workspace/search/cache 不暴露到 module 外。
- DataManager 是唯一稳定 CTS runtime-state owner，提供 `DataManager::initInst/getInst/destroyInst` 和 `CTSDM` 全局入口，不同时保留旧 `CTSRuntime`、Context 或 registry。
- DataManager 的 `input()` 必须吸收真实的配置、work directory、外部 clock/library/design 读取与 canonical CTS database 建立，不得只是调用旧 Setup/ClockDataRead 的转发器。
- synthesis/optimization/instantiation/evaluation/output module 必须按 `从 CTSDM 建立模块本地模型 -> 执行业务 -> 验证选中结果 -> commit/upload 到 CTSDM` 转变；candidate、solver、iteration、临时 timing state 不进入全局数据。
- global access 限制在 interface、data-manager 外部边界和顶层 module facade；低层算法继续使用业务窄 Input/Config/Output/Summary，不把 `CTSDM` 扩散成任意 helper 的 service locator。
- raw iDB/SDC/Liberty 类型限制在 data-manager adapter；行为结果完整验证后才提交，失败不得留下部分写回。
- 不引入与 iRT 同名但职责重复的 adapter、manager、logger、reporter 或 target。

### R5 — Converge CMake and integration

- 顶层 CMake 只加入 `interface`、`source`、`test`；source CMake 只加入 `data_manager`、`module`、`toolkit`。
- 每个稳定行为只有一个实现 owner target；依赖默认 PRIVATE，aggregator 不被 child 反向依赖，图中无环。
- 所有 repository 内 CTSAPI consumer 更新到最终 include/target，不保留旧路径 shim。
- 保持 CTSAPI method/status、Tcl/Python/tool-manager 行为、配置、算法和产物接口。

### R6 — Keep the final result clean and behavior-preserving

- 不修改 CTS 算法、搜索策略、时序/RC 语义、配置默认值、命名、写回规则或 QoR。
- 不保留开发流水账、迁移说明、临时代码、debug 输出、注释掉的实现、兼容旁路、旧目录或无关格式化。
- 禁止 DataManager 包裹旧 `CTSRuntime`、新 facade 转发旧 `Flow`、旧/新 Input 互转、兼容 adapter、include forwarder 或 alias target；最终调用链必须直接体现新的业务 ownership。
- 如目录/观测迁移暴露必须修改算法才能解决的问题，停止并返回规划，不夹带修复。

## In scope

- `src/operation/iCTS/` 产品代码、测试和 CMake 的完整目录及观测栈重构。
- repository 内 CTSAPI consumer 的必要 include/link 更新。
- 与新目录、Logger/Monitor、错误语义和验收规则直接相关的 Trellis spec 更新。
- 修改前基线、focused/full tests、静态扫描和指定 ICS55 端到端验收。

## Out of scope

- 全仓库或整个 iEDA 进程移除 glog；no-glog 边界止于 iCTS-owned code/targets。
- iRT 自身缺陷修复。
- CTS 算法/QoR 优化、性能调参、配置变化或顺手 bug fix。
- 除已确认的 global DataManager 和 Logger 外，为复制 iRT 而引入 module singleton、无边界 service locator、裸所有权、CMake 环或 routing 专用目录名。
- 仅为满足目标目录形状而进行的文件改名、空 facade、薄 adapter 或 target 包装。
- 继续维护旧 CTS structured/default/detail 日志兼容性；本任务明确以唯一 iRT-style `cts.log` 替代它。

## Acceptance criteria

- [x] 任务元数据没有 parent/child/subtask；整个实施在当前 task 内完成。
- [x] 最终两层目录与 R1 完全一致，旧 `api/external_libs/database/flow/utils` 路径、forwarder 和 alias target 均不存在。
- [x] `CTSAPI` public methods/status、Tcl/Python/tool-manager 行为保持，repository 内 consumer 全部构建通过。
- [x] `CTSDM` 是唯一 CTS runtime-state 全局入口；旧 `CTSRuntime`/thread-local runtime/context/registry 均不存在，重复 init/reset/teardown 不泄漏状态。
- [x] DataManager 直接承担输入建库、稳定状态和跨阶段已提交结果；Output module 从已提交状态只读生成产物，不存在无业务语义的 DataManager output 占位接口。
- [x] 每个主要 module 有真实的本地业务 model/workspace 与明确的 validate/commit 或 readonly-output 边界；失败路径不污染 CTSDM/Design/iDB。
- [x] 最终代码和 CMake 不存在只做旧新签名、路径、类型或 target 转发的 adapter/translator/forwarder/alias。
- [x] iCTS code/tests/CMake/object/archive 中没有 glog、`Log.hh`、`LOG_*` 或兼容 logging symbol。
- [x] 最终日志 sink 只有 `<work_dir>/cts.log`；不生成 `cts_detail.log`、`cts_runtime.log`，代码中不存在 SchemaWriter/default/detail sink 语义。
- [x] `cts.log` 使用 iRT-style prefix、console/file mirror、启动缓存和 flush 语义，并以 iRT-style ASCII 表格完整覆盖 R2 定义的高信息密度算法过程。
- [x] `cts_report` 的 wirelength、cell statistics、library-cell distribution 表格由同一 canonical renderer 同时写入 `.rpt` 和逐行镜像到 console/`cts.log`，且指定 ICS55 dev 验收脚本实际执行该命令。
- [x] Logger lifecycle、并发完整行、ANSI 差异、pre-open drain、reset/destroy、terminal failure status 有测试。
- [x] Monitor total/lap/nested、elapsed/CPU/RSS、格式和 sampling failure 有测试；不存在第二套 CTS runtime sampler。
- [x] target graph 与最终目录一致、无环、owner 唯一、iCTS link closure 无 glog。
- [x] 最终 diff 无临时内容、迁移注释、兼容层、死文件或无关修改，`git diff --check` 通过。
- [x] 完整质量检查通过：

  ```bash
  python3 ./.trellis/ecc_dev_tools/check.py check --path src/operation/iCTS
  ```

- [x] 修改前和最终均运行：

  ```bash
  cd /home/liweiguo/project/ecc-tools-dev/scripts/design/ics55_dev
  ./iEDA -script ./script/iCTS_script/run_iCTS_dev.tcl
  ```

- [x] 最终 exit 0 且无 terminal CTS error；与基线相比，CTS 拓扑、实例/网络名称和数量、连接、位置、DEF、normalized Verilog、配置和 QoR 不变。指定脚本新增的 `cts_report` 会有意重新生成 report/visualization artifacts，其工程数据保持等价；其余允许变化仅为已批准的单日志契约和非确定 timestamp/runtime 字段。

## Planning gate

用户确认当前 56 行文字型日志不满足原规划，并批准在同一任务内恢复主日志的 iRT-style ASCII 工程表格，同时修改符号链接实际指向的 ICS55 dev 验收脚本以执行 `cts_report`。全局 DataManager、业务模块提交边界和目录重构保持不变；不创建新 task，不修改 spec，最终仍须先通过 binary 验收再运行 ecc dev tools。
