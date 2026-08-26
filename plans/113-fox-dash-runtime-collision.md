# Plan 113 - 程序 - 狐狸分帧冲撞与路径命中

## 协调

- Planner 负责人：当前程序侧 Planner。
- Executor 负责人：独立 Executor，Plan 发布后分配。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@644a8d5f900a5a0c707b32906636eee19b1d0c8a`；Executor 必须在 Plan 发布后从最新 `origin/main` 建立新的专属 worktree。
- 本地实现方式（可选，仅作交接说明）：规划 worktree `C:\tmp\ReEcho-plan113-fox-dash-runtime`；实现不得复用规划、审计或既有 `fox-rush-vfx` worktree。
- 依赖 / 阻塞：依赖 `M_FOX_Dash` 的 `WindupSeconds/ActiveSeconds/RecoverySeconds/LengthCm/WidthCm/Damage`、EnemyLogic 固定步状态、EnemyHost swept movement、Combat HitResolver、CombatPresentationCoordinator 与现有 Fox Direction/Trail/Impact 语义。关闭前需要用户在 PIE 验收运动节拍和视觉可读性。
- Writes:
  - `plans/113-fox-dash-runtime-collision.md`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyLogicComponent.h`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyEventsComponent.h`
  - `Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp`
  - `Source/ReEchoEnemies/Private/Tests/ReEchoEnemyLogicTests.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`
  - `Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`（仅在事件生命周期接线需要修正时）
  - `Source/ReEcho/Private/Presentation/Combat/ReEchoCombatPresentationCoordinator.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatPresentationTests.cpp`（仅在阶段事件契约需要补测时）
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`（仅在 Fox Direction/Trail 生命周期需要补测时）
  - `scripts/ue/audit_fox_dash_vfx.py`（新增只读资产/组件契约审计）
  - `scripts/ue/fix_fox_direction_local_space.py`（仅在现有修复不足时扩展为具名、幂等的 Editor 适配）
  - `Content/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow.uasset`（仅经 Unreal Editor/API 修复已证明的可见性配置）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其声明的准确精选 Editor 预构建文件（最终 FullRebuild 生成）
- Stable Reads:
  - `Content/Data/enemy_abilities.csv`
  - `Design/Data/ReEchoEnemyData.xlsx`
  - `Content/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Fox.uasset`
  - `Content/VFX/Monster/Fox/Particle/NS_Fox_Rush_Charging.uasset`
  - `Content/VFX/Monster/Fox/Particle/NS_Fox_Rush_Trail.uasset`
  - `Content/VFX/Monster/Fox/Particle/NS_Fox_Rush_BeAttacked.uasset`
  - `Source/ReEcho/Private/Presentation/Combat/ReEchoCombatPresentationCoordinator.cpp`
- 影响模式：`SharedContract`（`Isolated | ReadOnly | SharedContract | Exclusive`，仅说明集成影响，不是写锁）。
- 兼容承诺 / 下游操作：保持稳定 Enemy/Ability ID、CSV Schema、配表数值、正面防御、冷却、Encounter 精英技能并发和 Combat 伤害权威不变；旧存档缺少新增冲刺瞬时字段时采用确定性安全默认值，不凭空重复伤害。冲刺继续 swept movement，遇阻即停。
- 明确排除：不修改 XLSX/CSV 数值；不让动画、Niagara、Notify 或特效碰撞决定移动或伤害；不把狐狸改为穿墙/穿人位移；不增加多段伤害；不修改兔子、羊 Boss、玩家武器或通用 Crowd Steering 语义；不在 Unreal Editor 外修改 `.uasset`。

## 锁定目标

1. `M_FOX_Dash` 前摇结束后不再单帧移动完整 `LengthCm`，而是在权威 `ActiveSeconds` 内沿 `SpecialLockedDirection` 分帧推进，并在无阻挡时累计尝试距离精确等于配表 `LengthCm`。
2. 每一步位移继续由 EnemyHost 执行 swept movement；碰到世界阻挡时保留现有停止语义，不穿透或补偿传送。
3. 冲撞路径首次接触本次锁定的有效敌对目标时，以同一个 `AttackIdentity` 经 `FReEchoHitIntent -> ReEchoHitResolver` 结算一次配表 `Damage`；未接触、已躲开、无敌或已死亡目标不扣血，同一次冲撞不得重复伤害。
4. Windup 持续播放 Charge 与可见的 Direction 箭头；箭头必须按 `SpecialLockedDirection` 对齐，在倾斜正交相机下具有有效 Renderer、Bounds、尺寸和前景排序。进入分帧 Active 时清理两者并启动 Fox Trail；Active 结束、遇阻终止、Recovery 结束、Cancelled、Death、EndPlay 和重开/恢复边界均无残留。Fox Impact 仍只消费 Combat 最终 `AppliedDamage > 0`。
5. 保存/恢复处于冲撞中的狐狸时，恢复剩余时间、剩余距离、锁定方向、AttackIdentity 和是否已结算伤害；旧存档以无重复伤害、无额外瞬移的安全默认值恢复。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoEnemies` / `AREA-Enemies`，以及主模块 `MOD-ReEcho` 的 EnemyHost、Combat 接线与 `AREA-Presentation` 下 `MOD-ReEchoVFX`。
- 对应模块文档：`MOD-ReEchoEnemies.md`、`MOD-ReEcho.md`、`MOD-ReEchoVFX.md` 均已加入 `Writes`。
- 设计意图：把冲刺轨迹和阶段计时保留在资源无关 EnemyLogic，把世界 sweep/碰撞事实和 Combat HitIntent 保留在 Host，把 Direction/Trail/Impact 保留为只读表现；视觉缺失不能改变冲刺或伤害。
- 权威状态与依赖：EnemyLogic 新增可保存的普通特殊行动 Active 状态、剩余距离、当前 AttackIdentity 和一次伤害门；Host 将每步 movement sweep 的实际 HitResult 反馈为窄命令，并仅在权威接触成立时提交 Combat。依赖方向仍为 `ReEcho EnemyHost -> ReEchoEnemies + ReEchoCombat + Presentation`，`ReEchoEnemies` 不读取世界、资产或 Combat 组件。
- 决策记录：
  1. 使用 `ActiveSeconds` 分帧积分 `LengthCm`，而非固定视觉 Delay 或插值 Actor Transform；这样暂停、固定步、保存恢复和碰撞都消费同一权威时钟。
  2. 保留 swept movement 的“遇阻即停”，不为了表现完整强制穿透或把余量瞬移到终点。
  3. 伤害以实际 sweep 接触为准，并以一次性门控防止多帧重入；废弃当前前摇结束时按长方形预测目标是否受伤的提前结算。
  4. `Committed` 表示进入 Active/冲刺开始，新增资源中立的 `RecoveryStarted` 表达 Active 已终止并立即停止 Trail，`Ended` 仍在 Recovery 完成后发布；受击、阶段转换、Encounter reset 等未完成动作使用新增 `ActionCancelled`，由 Coordinator 映射到现有 Presentation `Cancelled`。不得用 Niagara 完成回调反向驱动逻辑。
  5. Direction 与 Charging 同一 Windup 事件已证明会同时请求生成；用户实机只见 Charging，因此把剩余根因限定为箭头 Niagara 自身或其组件适配的 Renderer/Bounds/尺寸/排序/空间配置。修复必须通过 Editor/API 和只读审计证明，不允许修改玩法锁向来补偿资产。
- 相关文档同步范围：`CODEBASE_MAP/ARCHITECTURE.md` 关闭前审阅依赖拓扑；`CODEBASE_MAP/README.md` 审阅稳定标识/路线；更新上述三个模块文档的冲刺状态、世界接触与 VFX 生命周期事实。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待审阅；仅在拓扑/跨模块不变量变化时更新。
  - `shared/CODEBASE_MAP/README.md`：待审阅；仅在稳定标识或阅读路线变化时更新。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：待更新分帧冲刺、保存字段与 Host 窄反馈契约。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待更新 EnemyHost swept path 命中接线与验证路线。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：待更新 Fox Trail 的 Active/终止生命周期事实。

## 锁定验收

- [ ] 自动化证明狐狸在 `ActiveSeconds=0.15` 内经过多个固定步完成冲刺；首步不等于完整 `LengthCm`，无阻挡累计尝试距离为 `650 cm`，方向保持 Windup 锁向。
- [ ] Host 世界测试证明实际 swept path 接触玩家时准确造成一次当前配表伤害 `2`，后续 Active 步不重复；路径外、躲开、无敌、死亡或先撞世界阻挡时不造成伤害。
- [ ] 伤害只经 Combat resolver，Fox Impact 只在最终 `AppliedDamage > 0` 时出现；Niagara 缺失不会影响移动或扣血。
- [ ] 保存/恢复 Active 冲刺不重置全程、不重复伤害、不瞬移到终点；旧默认快照安全恢复。
- [ ] Direction 在 Windup 可见并按锁向旋转；资产审计证明全部启用 Emitter、至少一个有效 Renderer、非退化 Bounds、可见尺寸与前景排序契约。Trail 在可见冲刺过程中跟随，Active/Cancelled/Death/EndPlay/重开无残留。用户在 PIE 验收左右、上下和斜向冲刺、命中/未命中、撞墙、死亡/重开。
- [ ] `.clang-format`、聚焦自动化、最终 `-FullRebuild`、`python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check` 与 `git diff --check` 通过。
- [ ] 只纳入 Plan Writes、具名 Direction Niagara 适配和 FullRebuild 声明的准确精选预构建产物；不修改生产 XLSX/CSV、狐狸动画/Profile/Gameplay Blueprint 或其他 VFX `.uasset`。

## Step 0 门禁

- 基线分支/提交：`origin/main@644a8d5f900a5a0c707b32906636eee19b1d0c8a`。
- 引擎/构建可用性：UE 5.8 安装版；规划时确认 Unreal Editor 已关闭。Executor 运行命令前仍须取得 Git-common-dir Unreal 锁。
- 现有聚焦测试结果：尚未在本基线重跑。静态审计证明 `AdvanceSpecial` 当前在 Windup 到期同帧写入完整 `LengthCm`，并把 `ActiveSeconds + RecoverySeconds` 合并进 Recovery；现有 Logic 测试反而锁定单帧完整位移，需要按新契约更新。VFX Catalog/Component 已具备 Fox Direction/Trail/Impact 语义，且 Direction 与 Charging 在同一个 Windup 回调生成；用户当前 PIE 只看到 Charging、不看到 Direction，资产可见性契约必须重新审计。
- 共享契约 / 难合并资源风险：`ReEchoEnemyTypes.h` 快照与 ActionIntent、EnemyLogic/Host、CombatPresentation/VFX、模块文档和最终精选预构建包均为共享面；Plan112 最近修改普通特殊技能轮换和保存字段，Executor 必须保留其多 Ability 语义。现有 `fox-rush-vfx` worktree 已合入或属于旧候选，不得直接复用或整体 cherry-pick。
- 基线损坏时的停止条件：若本基线 Enemies/Host/CombatPresentation 聚焦测试失败，先分类并报告；若实现需要改变稳定 ID、CSV Schema、伤害次数、穿透/停止产品语义或覆盖 `.uasset`，停止越界部分并请求 Planner/用户更新 Plan。

## 实现提纲

1. 把普通特殊行动从 `None -> Windup -> Recovery` 扩为可表达狐狸 `Active` 的确定性状态；Windup 到期只提交 AttackIdentity 并初始化剩余时间/距离，随后每步产出不超过剩余预算的 movement intent。
2. Host 保留 `AddActorWorldOffset(..., sweep=true)`，记录实际起止与 HitResult；仅当 Active dash 的 swept path/阻挡 Actor 命中合法目标且一次伤害门未消费时，构造同 AttackIdentity 的 HitIntent，并通过窄命令回写已消费状态或终止原因。
3. 明确 Active 完成、世界阻挡、目标接触、取消、受击、死亡、Encounter reset 和恢复的状态转移；Recovery 不再包含 Active 秒数。
4. 扩展 Enemy special event 为 `RecoveryStarted/ActionCancelled`，让 CombatPresentationCoordinator 分别映射为资源中立的 Recovery/Cancelled，并让 VFX 按权威阶段覆盖 Direction/Trail 的开始和清理。
5. 新增只读 Direction Niagara 审计，记录启用 Emitter 的 Local Space、Renderer 数量/启用状态、Bounds、材质/网格依赖和运行时组件 Transform/排序；证据指向资产配置时，仅用幂等 Editor 脚本修复 `NS_Fox_Rush_arrow` 并保存该具名资产。
6. 更新 Logic、Host、CombatPresentation/VFX 聚焦测试与三个模块文档；不用代码方向补偿掩盖 Niagara 资产问题。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format`，再检查 diff | 仅格式化 Plan Writes，代码符合项目样式 |
| Logic | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Enemies.Logic` | 分帧距离、阶段、AttackIdentity、一次门和快照恢复通过 |
| Host/Combat | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Enemies.Host` 及新增精确过滤 | swept 接触一次伤害、路径外/无敌/阻挡不伤害通过 |
| Presentation/VFX | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.Combat` 与 `ReEcho.VFX.Catalog` 适用子集 | Direction/Trail/Impact 阶段和清理契约通过 |
| UE 资产 | 通过 UnrealEditor-Cmd 运行 `scripts/ue/audit_fox_dash_vfx.py` | Direction System 可加载；启用 Emitter、Local Space、Renderer、Bounds、尺寸/排序所需契约具名通过 |
| 构建 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UE 5.8 UHT/UBT 成功并刷新准确精选预构建包 |
| 项目 | `python scripts\validate_project.py`、`python scripts\ue\prebuilt_editor.py check`、`git diff --check` | 项目边界、源码指纹、精选包和文本检查通过 |
| 人工 PIE | `GMSpawnFox` 覆盖四向/斜向、命中/未命中、撞墙、无敌、死亡/重开 | 用户确认冲刺非闪现，Direction/Trail 可见且节拍正确，伤害一次且无残留 |

## 执行记录

### 变化

- 2026-08-26：Planner 静态复现根因：狐狸 Windup 到期同帧写入完整 `LengthCm`，`ActiveSeconds` 被并入 Recovery，导致 Host 一次 sweep 后视觉呈现闪现；当前伤害在移动前按长方形预测，而不是由冲撞路径接触触发。
- 2026-08-26：用户明确要求修复，并增加冲撞过程伤害；Plan 锁定分帧 Active、swept path 首次接触一次伤害和 Direction/Trail 生命周期。
- 2026-08-26：用户补充当前 PIE 中狐狸方向箭头完全不显示；Plan 将具名 Direction Niagara 审计与经 Editor/API 的最窄资产适配纳入 Writes 和锁定验收。
- 2026-08-26：Executor 证明现有 Windup/Committed/Ended 事件无法表达 Active 结束或未完成动作取消；Planner 批准最窄增加 `RecoveryStarted/ActionCancelled` 并补入 EnemyEvents/Coordinator Writes，不改变技能或伤害语义。

### 证据

- 待 Executor 填写。

### 剩余风险

- Direction Niagara 的最终屏幕可见性仍需用户 PIE 验收；结构审计与 Editor 适配不能替代主观大小、位置和可读性判断。

### 人工验收结果/请求

- `PendingBeforeClose`：等待用户完成锁定验收中的 PIE 矩阵。

### 架构文档审阅结果

- 待 Executor 与 Planner 关闭前填写。
