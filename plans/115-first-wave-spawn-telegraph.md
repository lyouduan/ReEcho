# Plan 115 - 程序 - 首波怪物出生预警时序

## 协调

- Planner 负责人：Gavyn-side Planner（当前程序用户授权的同一 AI 规划与集成）。
- Executor 负责人：Gavyn-side Executor（同一 AI 执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@9c146b5db5b8b836901a0fa6e6b0e067ef985a26`。
- 本地实现方式（可选，仅作交接说明）：独立工作树 `ReEcho-plan115`，本地分支 `plan/115-first-wave-spawn-telegraph`；Plan-only 发布后在同一工作树继续实现。
- 依赖 / 阻塞：依赖现有 Encounter WaveScheduler 的 Warning/Commit 两阶段事件、SpawnProfile 的 `WarningLeadSeconds`、GameMode 的 PendingSpawnBatch 与出生预警表现。无外部阻塞。
- Writes: 本 Plan；`Source/ReEcho/Private/Encounter/ReEchoEncounterRuntime.cpp`；必要时配对头文件；`Source/ReEcho/Private/ReEchoGameMode.cpp`；`Source/ReEcho/Private/Tests/ReEchoEncounterRuntimeTests.cpp`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`。
- Stable Reads: `Content/Data/encounter_waves.csv` 的波次触发时间；`Content/Data/spawn_profiles.csv` 的 `WarningLeadSeconds`；角色选择与 `BeginNextEncounter` 既有流程；Enemy Host/Logic/Presentation 现有装配契约。
- 影响模式：`SharedContract`，因为调整 Encounter Scheduler 对零秒首波的 Warning/Commit 时序，但不改变敌人逻辑、表现或伤害权威。
- 兼容承诺 / 下游操作：非零波次继续在配置的 `TriggerSeconds` 生成；其预警仍提前 `WarningLeadSeconds`。存档恢复继续持久化 Scheduler 游标与已准备出生批次，不生成第二套计时状态。
- 明确排除：不修改角色选择 UI；不隐藏已生成怪物来模拟预警；不让 Niagara、DebugDraw 或音频驱动生成事务；不修改怪物数量、出生位置、关卡时长、攻击逻辑或策划数值；不夹带角色基础数值提交。

## 锁定目标

玩家确认角色和武器、角色进入战场后，首波怪物不能立即存在。系统必须先在已解析的出生位置展示现有红色球框预警，并完整经过该敌人 SpawnProfile 配置的 `WarningLeadSeconds`，随后才提交 Enemy Host 生成。后续波次保持“在 TriggerSeconds 生成、提前 WarningLeadSeconds 预警”的既有语义。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Encounter`；`MOD-ReEchoEnemies` 仅消费最终 Enemy Host，不修改其公共契约。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已加入 Writes；`MOD-ReEchoEnemies.md` 只读审阅，除非实现意外改变 Host 生命周期，否则无需修改。
- 设计意图：把“预警结束后才生成玩法实体”固定在 Encounter Scheduler/Coordinator 边界。Warning 只负责锁定位置和表现；Commit 才是 Enemy Host、Roster、逻辑、碰撞和伤害进入世界的唯一入口。
- 权威状态与依赖：波次与 SpawnProfile CSV 继续分别拥有触发时间和预警时长；Scheduler 负责派生实际 Warning/Commit 时间；GameMode 只按事件准备位置并在 Commit 生成，不复制首波延迟常量。
- 决策记录：零秒首波没有可用的负时间来提前预警，因此将其 Warning 保持在遭遇 0 秒，把 Commit 延后到 `WarningLeadSeconds`。不整体平移 Encounter 时钟，也不特殊延迟角色入场；代价是首波实际生成时间晚于表中零秒，但保证预警契约完整。非零且足以容纳提前量的波次完全不变。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoEnemies.md`；预期只需在 `MOD-ReEcho.md` 明确零秒首波的时序派生规则。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：待记录首波 Warning/Commit 的时序和职责边界。
  - `MOD-ReEchoEnemies.md`：待审阅 Host 仍只在 Commit 时创建。
  - `ARCHITECTURE.md` / `README.md`：待审阅；预期模块拓扑与路由不变。

## 锁定验收

- [ ] 角色确认后，首波每个预留出生位置先出现可见红色球框，球框显示期间不存在对应 Enemy Host、Roster 条目、碰撞、AI 或伤害。
- [ ] 首波各角色类型在各自 `WarningLeadSeconds` 到期后才生成；零秒调用只返回 Warning，不同时返回 Commit。
- [ ] 10 秒、20 秒等后续波次仍在原 `TriggerSeconds` Commit，并在此前按配置提前 Warning。
- [ ] 首波预警位置与最终怪物生成位置一致，容量预留、确定性顺序、保存/恢复游标不退化。
- [ ] 聚焦 Scheduler 自动化覆盖零秒首波与普通后续波次；Editor 构建及适用 Encounter 自动化通过。
- [ ] 用户在 PIE 确认“角色入场 → 红色球框 → 怪物出生”的可读时序。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@9c146b5db5b8b836901a0fa6e6b0e067ef985a26`；当前主工作树与远端一致且干净。
- 引擎/构建可用性：最新主线含可用 UE 5.8 Editor 预构建包；最终代码候选仍须在准确集成基线上 FullRebuild。
- 现有聚焦测试结果：现有 `ReEcho.Encounter.Runtime` 测试明确断言零秒 Warning 与 Commit 同时发生，证明缺陷契约已被测试写死，必须同步更新。
- 共享契约 / 难合并资源风险：热点为 Encounter Runtime、GameMode 和 `MOD-ReEcho.md`；无 `.uasset` 或 XLSX 写入。角色数值策划分支是独立工作，不纳入本 Plan。
- 基线损坏时的停止条件：最新主线无法配置生产 Encounter/SpawnProfile、Warning 位置无法稳定解析，或远端在同一 Scheduler/出生协调路径引入不同产品语义时停止并报告。

## 实现提纲

1. 在 Scheduler 集中派生实际 Warning/Commit 时间：正常波次保持不变，无法容纳完整提前量的首波延后 Commit。
2. 更新聚焦自动化，明确零秒只发 Warning、到配置 lead 后才发 Commit，并保护 10 秒普通波次不变。
3. 核对 GameMode 在 Warning 仅准备批次/播放预警、Commit 才生成 Enemy Host；只做必要的颜色、日志或寿命修正，不创建第二计时器。
4. 更新 `MOD-ReEcho.md`，运行静态校验、构建、Encounter 聚焦自动化和最终 FullRebuild；交给用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan 结构与仓库静态规则通过 |
| 单元契约 | `ReEcho.Encounter.Runtime` | 零秒只 Warning，lead 到期 Commit；普通后续波次时序不变 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、文档和源码不变量通过 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 聚焦自动化 | Encounter Runtime、保存恢复及受影响 Spawn/Stage 测试 | Scheduler 游标、Pending Batch、容量和 Stage 行为通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合源码与精选 Editor 预构建包一致 |
| 人工 | 新游戏确认角色后观察首波 | 顺序为角色入场、红色球框完整展示、怪物生成 |

## 执行记录

### 变化

- 2026-08-26：完成只读定位。生产第一波 `TriggerSeconds=0`，Melee/Ranged/Elite/Boss 的 `WarningLeadSeconds` 为 0.8–1.0 秒；Scheduler 将 Warning 钳到 0 后仍把 Commit 放在 0，`ProcessScheduledSpawnEvents(0)` 因而同帧处理两者。

### 证据

- `FReEchoEncounterWaveSchedulerTest` 当前断言“Warning sorts before commit at the same time”，与用户期望直接相反。
- GameMode 已有 PendingSpawnBatch 两阶段链；修复无需提前生成或隐藏 Enemy Host。

### 剩余风险

- 当前出生框由 GameMode 调试绘制接口表现；本 Plan 只保证现有球框的时序与可见寿命，不扩张为新的美术资产系统。
- 若用户认为首波延迟不应计入遭遇倒计时，需要另行决定 Encounter 时钟何时开始；本 Plan 暂定角色入场即开始计时，怪物在 0.8–1.0 秒后生成。

### 人工验收结果/请求

- `PendingBeforeClose`：实现构建后由用户在 PIE 验证首波顺序与可读性。

### 架构文档审阅结果

- 待实现完成后逐项填写。
