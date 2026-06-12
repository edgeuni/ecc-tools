# Implement: P0-A 修复 wire 电阻 1000x 单位 bug

> 任务: 06-12-fix-wire-res-unit · 按序执行，每步有验证命令

## Checklist

### 1. 代码修改（原子）
- [ ] `WrapperRc.cc:159` 去除 `/ kMilliOhmPerOhm`
- [ ] `WrapperRc.cc:293` 去除 `/ kMilliOhmPerOhm`
- [ ] `WrapperRc.cc:47` 删除 `kMilliOhmPerOhm` 常量
- [ ] `FastStaChar.cc:134` 去除 `/ kMilliOhmPerOhm`；删除 `:52` 常量
- [ ] `FastStaParasitics.cc:91` 去除 `/ 1000.0`
- [ ] 全仓核查无残留补偿换算：
      `grep -rn "kMilliOhmPerOhm" src/` → 0 hits
      `grep -rn "queryWireResistance\|queryRequiredWireResistance" src/operation/iCTS --include=*.cc | grep -v WrapperRc` → 逐行确认无 /1000、无 ×1000

### 2. 单元测试
- [ ] 新增 `src/operation/iCTS/test/database/io/WrapperRcTest.cc`（设计见 design.md §4）
- [ ] 注册：仿照 `test/database/` 下既有测试的 CMake 模式（参考 ConfigTest 的注册方式），目标 `icts_wrapper_rc_test`
- [ ] 构建+运行：
      `cmake --build build -j$(nproc) --target icts_wrapper_rc_test`
      `ctest --test-dir build -R icts_wrapper_rc_test --output-on-failure`

### 3. 全量回归
- [ ] `cmake --build build -j$(nproc) --target ecc_bin && cmake --build build -j$(nproc)`（或按 ctest 列表构建 icts 测试目标）
- [ ] `ctest --test-dir build -R icts --output-on-failure`；失败用例逐一分析：仅当期望值绑定旧（错误）单位时更新期望，并在 commit message 列出
- [ ] 注意：`ICTS_BUILD_REALTECH_TESTS` 默认 OFF，保持默认范围即可

### 4. vga_lcd 集成验证（本地输出，禁写 DAC-27-CTS）
- [ ] 准备本地 run 目录（validation/ 下），克隆并改写 tcl/json 的输出路径（design.md §5.2）
- [ ] `bin/ecc_bin -script validation/icts_cts.tcl 2>&1 | tee validation/run.log`
- [ ] 提取对比表（旧基线 vs 新）：unit_resistance / initial skew / optimized skew / accepted edits / depth summary / max net WL / buffer count / runtime
- [ ] 写 `research/validation-vga-lcd.md`

### 5. 收尾
- [ ] trellis-check 质量检查
- [ ] commit（单提交，message 含失败测试期望更新清单）

## Review Gates

- Gate-1（步骤 1-2 后）：单测红→绿证明（修复前哨兵断言应失败、修复后通过——可先写测试验证其在旧代码上确实抓到 bug）
- Gate-2（步骤 4 后）：vga_lcd 行为对比表评审——QoR 变化方向必须可解释（R 变真后约束变紧），flow 不 crash

## Rollback

revert 单提交即可。
