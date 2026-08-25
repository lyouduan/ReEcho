# Plan 112 - 程序 - 兔子双技能轮换与站定连发

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`；需要用户在 PIE 中确认兔子会交替使用移动散射与站定连发，且站定连发可辨识为连续四发。
- 本地规划 / 实现基线：`origin/main@30138a3df16227727315dad7dca6ce2121dd830a`。
- 本地实现方式：计划发布后创建 `plan/112-rabbit-dual-ability-runtime` 独立 worktree。
- 依赖 / 阻塞：依赖现有 `enemy_abilities.csv` 中 `M_RABBIT_MovingVolley`（`SequenceOrder=1`）和 `M_RABBIT_RangedBurst`（`SequenceOrder=2`）均为启用状态；实现前不修改策划平衡值。
- Writes:
  - `plans/112-rabbit-dual-ability-runtime.md`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyLogicComponent.h`
  - `Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp`
  - `Source/ReEchoEnemies/Private/Tests/ReEchoEnemyLogicTests.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `Content/Data/enemy_abilities.csv`
  - `Source/ReEcho/{Public,Private}/Data/ReEchoEnemyDefinitionCompiler.*`
  - `Source/ReEchoEnemies/{Public,Private}/Enemies/ReEchoEnemyProjectileLogic.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`
  - `shared/CODEBASE_MAP/{ARCHITECTURE,README}.md`
- 影响模式：`SharedContract`。改变 `ReEchoEnemies` 的可保存逻辑快照和主模块 EnemyHost 对远程攻击意图的消费方式；不改变 CSV Schema、Combat 最终伤害权威或 VFX 资源契约。
- 兼容承诺 / 下游操作：保持两个稳定 AbilityId、现有每球伤害、射程、散射角、移动门和投射物事件身份不变；旧存档缺少新增轮换/连发瞬时字段时采用确定性默认值，不凭空重复已完成攻击。
- 明确排除：不修改策划 XLSX/CSV 数值；不改羊 Boss 技能轮转；不让 Niagara、动画或表现回调决定技能选择、发射时机或伤害；不把两个 AbilityId 写死为运行时分支。

## 锁定目标

1. 兔子按启用能力的 `SequenceOrder` 确定性循环使用两种 `Enemy.RangedBurst`：移动散射 → 站定连发 → 移动散射；不得永久固定在第一项。
2. 一次技能从前摇开始到提交、恢复、存档恢复始终绑定同一个 `SpecialAbilityId`，不能在中途重新取能力数组第一项。
3. 移动散射仍在一次提交中产生三颗对称散射球；站定连发在能力的 `ActiveSeconds` 发射窗口内依次生成四颗同向球，而不是同帧同位置重叠生成。
4. 站定连发期间兔子不移动；移动散射保持既有施法移动语义。每颗球继续独立执行连续路径碰撞、至多命中一次并造成表中每球伤害。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoEnemies`、`AREA-Enemies`、主模块 EnemyHost 接缝与 `AREA-Tests`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`，已加入 Writes；关闭前审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoVFX.md`，只有事实变化时修改。
- 设计意图：多技能选择和技能阶段属于 EnemyLogic 权威；世界投射物的延迟生成与碰撞仍由 EnemyHost 执行。表现只消费类型化特殊动作/投射物事件，不反向影响玩法。
- 权威状态与依赖：EnemyLogic 新增可保存的普通怪特殊技能轮转游标，并以 `SpecialAbilityId` 解析活动技能；EnemyHost 保存一次已提交但尚未完成生成的连续投射物队列。Combat、Presentation、VFX 的状态所有权和模块依赖方向不变。
- 决策记录：
  1. 使用已存在的 `SequenceOrder` 作为确定性轮转顺序；不引入随机选择、权重或按 AbilityId 特判。
  2. `ProjectileCount>1 && SpreadAngleDegrees≈0 && ActiveSeconds>0` 表示直线连续发射：第一发在提交时生成，其余发在 `ActiveSeconds` 内等间隔生成；有散射角的多球仍是同帧扇形齐射。该规则复用现有字段，不新增第二份配表常量。
  3. 延迟发射队列由 Host 持有，因为 Logic 不拥有世界投射物实例；队列保存攻击身份、能力参数、下一发索引与剩余时间，使存档恢复不会丢失或重复发射。
  4. 技能开始时推进轮转游标，而不是完成时推进，保证动作被存档/恢复或目标随后丢失时不会反复选择同一项。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoVFX.md`；预计只需更新 `MOD-ReEchoEnemies.md` 的普通远程技能轮转、连续发射与保存契约。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅拓扑与依赖方向；
  - `README.md`：待审阅 AREA 路由；
  - `MOD-ReEchoEnemies.md`：待同步双技能轮转、活动技能绑定和 Host 连发队列；
  - `MOD-ReEchoVFX.md`：待审阅投射物事件消费是否无需变化。

## 锁定验收

- [ ] 生产兔子 Definition 同时保留两个启用能力，连续两次获得特殊行动许可时按 `SequenceOrder` 分别进入 `M_RABBIT_MovingVolley` 与 `M_RABBIT_RangedBurst`，第三次回到第一项。
- [ ] 改变目标、跨帧推进和快照恢复均不会把活动能力偷换为数组第一项；旧快照默认从确定性首项开始。
- [ ] 移动散射一次生成三条对称轨迹；站定连发第一发立即生成，随后三发在 `ActiveSeconds` 内按稳定索引依次生成，四发不在同帧重叠。
- [ ] 两种技能分别遵守 `bMovementDuringCast=true/false`，每球碰撞与伤害继续消费表中能力值。
- [ ] Enemies Logic/Host 聚焦自动化、项目校验、格式、Development FullRebuild、精选预构建包和 `git diff --check` 通过。
- [ ] 用户完成 PIE 人工验收后才关闭；未提交精选允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@30138a3df16227727315dad7dca6ce2121dd830a`；本地 main 与远端一致且干净。
- 引擎/构建可用性：UE 5.8 可用；当前 Unreal Editor 正在运行，计划发布使用纯文档例外，实现构建前必须等待用户保存并关闭 Editor。
- 现有聚焦测试结果：现有 `ReEcho.Enemies.Logic.RangedAndEliteBehaviors` 只配置一个兔子技能，未覆盖多技能选择；Host 测试覆盖三球散射但未覆盖跨帧连续发射。
- 共享契约 / 难合并资源风险：逻辑快照、EnemyHost 保存结构、投射物身份和精选预构建包与并行 Enemy/Save/VFX 工作存在耦合；发布实现前再次 fetch 并审计。
- 基线损坏时的停止条件：生产 Definition 未同时编译两个能力、活动技能无法从稳定 ID 恢复、延迟队列要求让 Enemies 反向依赖主模块/表现，或实现会改变羊 Boss/Combat 最终伤害语义时停止并报告。

## 实现提纲

1. 在 EnemyLogic 建立过滤后的普通特殊技能轮转和按 ID 解析入口；所有阶段统一使用活动技能，并把轮转游标纳入快照与局间瞬时重置审计。
2. 在 EnemyHost 抽取单球生成函数；齐射维持即时循环，直线连发建立可保存队列并在 Tick 中按确定性时间逐发生成。
3. 扩充 Logic 与 Host 自动化，覆盖两技能轮转、快照恢复、移动门、齐射/连发时序、球索引和每球一次碰撞。
4. 更新 Plan 执行记录和 `MOD-ReEchoEnemies.md`，格式化 C++，完成聚焦自动化、FullRebuild、项目校验、预构建与 diff 审计，交由用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 模块依赖、源码/文档和项目不变量通过 |
| Logic | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Logic` | 双技能轮转、活动 ID、移动门和快照恢复通过 |
| Host | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Host` | 三球齐射与四球跨帧连发、身份、轨迹和碰撞通过 |
| C++格式/构建 | 修改的 `.h/.cpp` 运行仓库 `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确精选包 |
| 发布包 | `python scripts/ue/prebuilt_editor.py check` | 精选 Editor 包与最终源码指纹一致 |
| 人工 | PIE 观察同一兔子至少连续完成三次特殊攻击 | 依次表现为移动三球散射、站定四发连射、再次移动三球散射 |

## 执行记录

### 变化

- 2026-08-25：只读审计确认生产 CSV 与 Definition 编译器均保留两项兔子能力；运行时 `GetSpecialAbility()` 固定返回排序后第一项，导致第二项不可达。

### 证据

- `M_RABBIT_MovingVolley` 为 `SequenceOrder=1, ProjectileCount=3, SpreadAngleDegrees=40, bMovementDuringCast=true`。
- `M_RABBIT_RangedBurst` 为 `SequenceOrder=2, ProjectileCount=4, SpreadAngleDegrees=0, bMovementDuringCast=false`。
- 当前 Host 对任意 `ProjectileCount` 都在同一次提交中循环生成，四发零散射会同帧重叠；现有自动化没有覆盖双技能或跨帧连发。

### 剩余风险

- 站定连发的主观节奏取决于当前 `ActiveSeconds=0.1`，本 Plan 不擅自修改策划数值；若用户认为四发过快，应由策划调整该字段或另行确认新增独立发射间隔字段。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现后用户在 PIE 验收两种技能的可辨识性和节奏。

### 架构文档审阅结果

- 待实现完成后逐项填写。
