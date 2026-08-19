# Plan 53 - 程序 - 兔子投射物碰撞与伤害恢复

## 协调

- Planner 负责人：Codex（JosephLE910 / Gavyn-side AI）。
- Executor 负责人：Codex（JosephLE910 / Gavyn-side AI）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@f2ce2a5`。
- 本地实现方式（可选，仅作交接说明）：Plan 发布后使用专属 `plan/53-rabbit-projectile-collision-damage` worktree。
- 依赖 / 阻塞：Plan49 已让兔子三球 Niagara 可见并由逻辑投射物事件驱动；本 Plan 不等待 Plan49 关闭，但最终人工验收需同时观察弹道、碰撞时机和扣血。
- Writes：本 Plan；`Design/Data/ReEchoEnemyData.xlsx` 与同批生成的 `Content/Data/enemy_abilities.csv`；`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`；`Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`；必要时最小维护 `Source/ReEcho/Public/Core/ReEchoTypes.h`；`shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；最终 `GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：`Source/ReEchoEnemies/**/ReEchoEnemyProjectileLogic.*`、`Source/ReEchoCombat/**`、`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`、`shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`。
- 影响模式：`SharedContract`（敌人能力表、EnemyHost 到 Combat 的命中接缝）；权威 XLSX/生成 CSV 为同一发布单元。
- 兼容承诺 / 下游操作：沿用现有 `FReEchoEnemyProjectileRuntimeState`、保存数组 `BossProjectiles`、投射物生命周期事件和 Combat `HitIntent`；不改变存档版本、敌我阵营规则、兔子弹道方向、Niagara 资产或其他敌方远程能力的临时零伤害状态。
- 明确排除：Boss 投射物/闪现/光束恢复伤害；世界障碍物碰撞；反弹、穿透、多目标伤害；让 Niagara 或物理回调直接扣血；为三颗可见粒子各建一套独立玩法投射物；调整玩家/怪物血量平衡。

## 锁定目标

让兔子的可见三球攻击拥有与画面覆盖范围相符的权威逻辑碰撞体，并在碰撞合法玩家目标后通过 Combat 正常造成一次伤害。恢复 `M_RABBIT_RangedBurst` 在临时安全置零前的权威伤害 `10`；其他敌方远程能力继续保持当前安全状态。

三颗 Niagara 球仍表示一个玩法投射物簇：中间球沿锁定方向飞行，逻辑载体使用一个可保存、连续扫掠的碰撞体，命中一次后结束并发布 `Ended`，避免一组三球重复扣血。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoEnemies`、`MOD-ReEcho`；`MOD-ReEchoCombat` 和 `MOD-ReEchoVFX` 仅作为稳定契约读取。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，两者均已加入 Writes。
- 设计意图：EnemyLogic 继续只产生投射物行为意图，主模块 EnemyHost 拥有世界碰撞采样，Combat 独占最终伤害，VFX 只读生命周期事件。碰撞与伤害不能迁入 Niagara，也不能在 Host 绕过 Resolver 直接扣血。
- 权威状态与依赖：
  - `ReEchoEnemyData.xlsx / EnemyAbilities` 继续是伤害和碰撞尺寸数据的唯一真源；生成 CSV 与 XLSX 同批发布。
  - `FReEchoEnemyProjectileRuntimeState` 继续拥有在途投射物位置、方向、伤害和碰撞尺寸；不新增第二个 Projectile Actor 状态真相。
  - Host 使用连续路径与目标碰撞盒做扫掠相交，命中后构造一次 `FReEchoHitIntent`；`ReEchoHitResolver` 决定实际扣血和事件。
- 决策记录：
  - 采用现有目标碰撞盒按投射物半径扩张后进行线段扫掠（等价于移动碰撞体对 Box 的连续碰撞），而不是新增依赖 Tick/Overlap 顺序的 `UBoxComponent` Actor。这样可防止高速穿透，并保持存档/恢复只有一份权威状态。
  - 兔子碰撞半径读取现有 `EnemyAbilities.RadiusCm`；当前权威值 `150 cm` 对应三球簇覆盖范围。不得继续使用怪物自身 `CollisionRadiusCm * 0.5` 作为子弹尺寸。
  - 兔子伤害恢复为历史权威值 `10`；只恢复该能力，Boss 三项远程零伤害不随本 Plan 改动。
  - 只命中当前合法目标一次；阵营、无敌、卡牌减伤和最终伤害仍由现有 Combat 契约处理。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计依赖拓扑不变。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计稳定标识与路由不变。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：更新兔子投射物碰撞尺寸来源、一次命中和远程安全状态。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：更新 EnemyHost 的世界碰撞适配职责与代码落点（若现有说明已完整则记录无需正文修改）。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`：审阅并记录无需修改；Resolver/HitIntent 公共契约不变。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：审阅并记录无需修改；Niagara 仍只读投射物事件。

## 锁定验收

- [ ] `ReEchoEnemyData.xlsx` 中 `M_RABBIT_RangedBurst.Damage == 10`，同步生成的 `enemy_abilities.csv` 字节一致；其他当前为零的 Boss 远程能力保持为零。
- [ ] 兔子投射物碰撞尺寸来自该能力的 `RadiusCm == 150`，不再由兔子本体碰撞半径推导。
- [ ] 连续扫掠能够命中路径上的玩家，造成一次大于零且符合表值/Combat 修正的伤害，随后删除逻辑投射物并发布 `Ended`。
- [ ] 路径外玩家不受伤；命中后继续推进不会重复扣血。
- [ ] Echo 嘲讽、阵营、无敌和保存/恢复不被绕过或复制；Niagara 不参与命中裁决。
- [ ] 现有兔子弹道方向、速度、三球表现、VFX 生命周期和其他敌人攻击无回归。
- [ ] `.clang-format`、聚焦自动化、Editor 构建、`python scripts/validate_project.py`、XLSX/CSV `--check` 与 `git diff --check` 通过。
- [ ] 用户 PIE 验证“被三球簇扫中会扣血、躲开不扣血、单簇只扣一次”。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f2ce2a5`；远端最大编号 52，本 Plan 使用下一空闲编号 53。
- 引擎/构建可用性：Plan49 与远端 UI/Plan52 的最终组合候选已在同一基线上完成 UE 5.8 Editor Development FullRebuild。
- 现有聚焦测试结果：`ReEcho.Enemies.Host.RabbitProjectilePipeline` 已覆盖生成/移动，但明确断言临时伤害为 0，且没有命中扣血/一次性消费覆盖；本 Plan 替换该缺口。
- 共享契约 / 难合并资源风险：`ReEchoEnemyData.xlsx` 是不可文本合并的策划权威二进制；发布前必须重新 fetch 并逐字段审计远端同路径变化，不得整块静默覆盖。最终预构建包也必须在准确组合源码上重建。
- 基线损坏时的停止条件：生产敌人工作簿不能无损导入导出、同步器不能保持非目标单元格/格式，或现有 Host→Combat 路径无法在不引入第二伤害真相的情况下复用时，停止实现并报告。

## 实现提纲

1. 用仓库同步工具与表格工作流定位并只修改 `EnemyAbilities` 中兔子能力的 Damage；保持样式、验证和其他行不变，再生成/校验 CSV。
2. EnemyHost 创建兔子逻辑投射物时从当前 Ability 复制 `RadiusCm` 为碰撞半径；保留速度、方向、保存和生命周期事件。
3. 扩展 Host 聚焦自动化：路径内命中扣血一次并结束，路径外不扣血，半径/伤害来自生产定义；保留既有生成和移动断言。
4. 更新执行记录和相关模块文档，完成 C++/数据/构建验证后交给用户 PIE。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 表格 | `python scripts/data/sync_xlsx_to_csv.py --check` 与聚焦同步器测试 | XLSX 与全部生成 CSV 一致，非目标表无漂移 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、Schema、模块边界和工作流不变量通过 |
| C++ | 对修改的 `.h/.cpp` 执行仓库 `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development` | 格式一致，UHT/UBT 退出码为 0并刷新跟踪包 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Host.RabbitProjectilePipeline`，必要时补跑 `ReEcho.Enemies` | 生成、移动、半径、路径内/外、一次伤害均通过 |
| 人工 | 用户在 PIE 中靠近/躲开兔子三球各验证一次 | 命中可读、扣血一次、躲避有效 |

## 执行记录

### 变化

### 证据

### 剩余风险

- `RadiusCm=150` 是旧锁点范围攻击遗留字段，但当前三球簇视觉也需要相近覆盖；最终手感和画面匹配由用户 PIE 决定，必要时后续由策划只调表值。
- 本 Plan 不恢复 Boss 其他不可见/未验收远程能力伤害。

### 人工验收结果/请求

- `PendingBeforeClose`：用户 PIE 验证兔子三球路径内/外和单次扣血。

### 架构文档审阅结果

- 待实现后填写。
