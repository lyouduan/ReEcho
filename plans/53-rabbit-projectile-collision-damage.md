# Plan 53 - 程序 - 兔子投射物碰撞与伤害恢复

## 协调

- Planner 负责人：Codex（JosephLE910 / Gavyn-side AI）。
- Executor 负责人：Codex（JosephLE910 / Gavyn-side AI）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@f2ce2a5`。
- 本地实现方式（可选，仅作交接说明）：Plan 发布后使用专属 `plan/53-rabbit-projectile-collision-damage` worktree。
- 依赖 / 阻塞：Plan49 已让兔子三球 Niagara 可见并由逻辑投射物事件驱动；本 Plan 不等待 Plan49 关闭，但最终人工验收需同时观察弹道、碰撞时机和扣血。
- Writes：本 Plan；`Design/Data/ReEchoEnemyData.xlsx` 与同批生成的 `Content/Data/enemy_abilities.csv`；`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`；`Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`；必要时最小维护 `Source/ReEcho/Public/Core/ReEchoTypes.h`；`shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`；最终 `GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：`Source/ReEchoEnemies/**/ReEchoEnemyProjectileLogic.*`、`Source/ReEchoCombat/**`、`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`、`shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`。
- 影响模式：`SharedContract`（敌人能力表、EnemyHost 到 Combat 的命中接缝）；权威 XLSX/生成 CSV 为同一发布单元。
- 兼容承诺 / 下游操作：沿用现有 `FReEchoEnemyProjectileRuntimeState`、保存数组 `BossProjectiles`、投射物生命周期事件和 Combat `HitIntent`；不改变存档版本、敌我阵营规则、Niagara 资产或其他敌方远程能力的临时零伤害状态。兔子每次 Commit 仍只有一份可见三球 Niagara，但玩法侧展开为三条可保存的子弹轨迹。
- 明确排除：Boss 投射物/闪现/光束恢复伤害；世界障碍物碰撞；反弹、穿透、多目标伤害；让 Niagara 或物理回调直接扣血；调整玩家/怪物血量平衡。

## 锁定目标

让兔子的三颗可见球分别拥有权威逻辑轨迹和碰撞体；任一球碰撞合法玩家目标后均通过 Combat 按表值 `10` 正常造成伤害，并且每颗球最多结算一次。清理被新 Niagara 受击语义替代的旧 `ReEchoHitImpactActor/HitStarburst` 表现。其他敌方远程能力继续保持当前安全状态。

一次兔子 Commit 展开为三条逻辑子弹轨迹，中心方向锁定玩家，两侧按稳定扇形角展开；三条轨迹共享一次可见三球 Niagara 生命周期，视觉粒子不拥有碰撞或伤害权威。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoEnemies`、`MOD-ReEcho`；`MOD-ReEchoCombat` 和 `MOD-ReEchoVFX` 仅作为稳定契约读取。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，两者均已加入 Writes。
- 设计意图：EnemyLogic 继续只产生投射物行为意图，主模块 EnemyHost 拥有世界碰撞采样，Combat 独占最终伤害，VFX 只读生命周期事件。碰撞与伤害不能迁入 Niagara，也不能在 Host 绕过 Resolver 直接扣血。
- 权威状态与依赖：
  - `ReEchoEnemyData.xlsx / EnemyAbilities` 继续是伤害和碰撞尺寸数据的唯一真源；生成 CSV 与 XLSX 同批发布。
  - `FReEchoEnemyProjectileRuntimeState` 继续拥有在途投射物位置、方向、伤害、碰撞尺寸、子弹序号和是否已命中；不新增第二个 Projectile Actor 状态真相。
  - Host 使用每颗球的连续路径与目标碰撞盒做扫掠相交，每球首次命中构造一次 `FReEchoHitIntent`；`ReEchoHitResolver` 决定实际扣血和事件。
- 决策记录：
  - 采用现有目标碰撞盒按投射物半径扩张后进行线段扫掠（等价于移动碰撞体对 Box 的连续碰撞），而不是新增依赖 Tick/Overlap 顺序的 `UBoxComponent` Actor。这样可防止高速穿透，并保持存档/恢复只有一份权威状态。
  - `EnemyAbilities.RadiusCm` 当前权威值 `150 cm` 解释为整组三球的碰撞尺寸预算；单球半径为 `RadiusCm / 3 == 50 cm`。不得继续使用怪物自身 `CollisionRadiusCm * 0.5` 作为子弹尺寸。
  - 兔子伤害恢复为历史权威值 `10`；只恢复该能力，Boss 三项远程零伤害不随本 Plan 改动。
  - 每颗球只命中当前合法目标一次；阵营、无敌、卡牌减伤和最终伤害仍由现有 Combat 契约处理。测试区分表内原始伤害、卡牌修正后伤害和最终实际扣血，默认无减伤构筑时三者均为 `10`。
  - 删除旧 `ReEchoAttackEffects::SpawnHitImpact`、`AReEchoHitImpactActor`、`HitStarburst` 贴图与源图及其导入映射；怪物受击只保留 `EnemyHurt` Niagara，玩家受击只保留 `PlayerHurt` Niagara。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计依赖拓扑不变。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计稳定标识与路由不变。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：更新兔子投射物碰撞尺寸来源、一次命中和远程安全状态。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：更新 EnemyHost 的世界碰撞适配职责与代码落点（若现有说明已完整则记录无需正文修改）。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`：审阅并记录无需修改；Resolver/HitIntent 公共契约不变。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：更新兔子伤害从临时零值恢复为 10 的事实，并重申 Niagara 仍不拥有碰撞/伤害。

## 锁定验收

- [x] `ReEchoEnemyData.xlsx` 中 `M_RABBIT_RangedBurst.Damage == 10`，同步生成的 `enemy_abilities.csv` 字节一致；其他当前为零的 Boss 远程能力保持为零。
- [x] 一次 Commit 生成三条逻辑轨迹，中心球锁定玩家、两侧稳定展开；每球碰撞半径来自 `RadiusCm / 3 == 50`，不再由兔子本体尺寸推导。
- [x] 三颗球均能分别命中并各自按表值造成 `10` 点原始伤害；无减伤构筑下实际扣血也是 `10`，每球最多一次。
- [x] 路径外玩家不受伤；某球命中后不影响其余两球继续判定，已命中的球不会重复扣血。
- [x] Echo 嘲讽、阵营、无敌和保存/恢复不被绕过或复制；Niagara 不参与命中裁决。
- [x] 旧 `ReEchoHitImpactActor/HitStarburst` 的代码、调用、资产、源图和导入映射全部删除；新的 PlayerHurt/EnemyHurt Niagara 保持正常。
- [ ] 现有兔子中心瞄准、速度、三球表现、VFX 生命周期和其他敌人攻击无回归。
- [x] `.clang-format`、聚焦自动化、Editor 构建、`python scripts/validate_project.py`、XLSX/CSV `--check` 与 `git diff --check` 通过。
- [ ] 用户 PIE 验证“任一球命中均扣 10、三球各自最多一次、躲开不扣血、旧火焰星爆不再出现”。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f2ce2a5`；远端最大编号 52，本 Plan 使用下一空闲编号 53。
- 引擎/构建可用性：Plan49 与远端 UI/Plan52 的最终组合候选已在同一基线上完成 UE 5.8 Editor Development FullRebuild。
- 现有聚焦测试结果：`ReEcho.Enemies.Host.RabbitProjectilePipeline` 已覆盖生成/移动，但明确断言临时伤害为 0，且没有命中扣血/一次性消费覆盖；本 Plan 替换该缺口。
- 共享契约 / 难合并资源风险：`ReEchoEnemyData.xlsx` 是不可文本合并的策划权威二进制；发布前必须重新 fetch 并逐字段审计远端同路径变化，不得整块静默覆盖。最终预构建包也必须在准确组合源码上重建。
- 基线损坏时的停止条件：生产敌人工作簿不能无损导入导出、同步器不能保持非目标单元格/格式，或现有 Host→Combat 路径无法在不引入第二伤害真相的情况下复用时，停止实现并报告。

## 实现提纲

1. 用仓库同步工具与表格工作流定位并只修改 `EnemyAbilities` 中兔子能力的 Damage；保持样式、验证和其他行不变，再生成/校验 CSV。
2. EnemyHost 创建兔子逻辑投射物时展开三条扇形轨迹，单球半径取 `RadiusCm / 3`；每球独立一次命中，中心轨迹独占共享 Niagara 生命周期事件。
3. 删除旧 HitImpact 代码、调用和资产，只保留 Niagara 受击语义。
4. 扩展 Host 聚焦自动化：三球生成/方向/半径、每球分别命中扣 10、路径外不扣血、每球不重复命中；保留既有生成和移动断言。
5. 更新执行记录和相关模块文档，完成 C++/数据/构建验证后交给用户 PIE。

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

- 初始候选把三颗可见球视为一个逻辑簇；用户 PIE 指出只有一条轨迹能命中后，该决策被本节后续返修取代。
- EnemyHost 现在按共享 `ReEchoRabbitProjectilePattern` 展开 `-32.5° / 0° / +32.5°` 三条逻辑轨迹；单球半径为能力 `RadiusCm / 3`，每球首次相交各提交一次 Combat 命中，命中后只关闭该球碰撞并继续维持共享 Niagara 生命周期。
- 只有中心逻辑球发布 Spawned/Moved/Ended，因而一组三条玩法轨迹仍只生成一份已有三球 Niagara，不会错误变成九球。
- 删除旧 `ReEchoAttackEffects`、`AReEchoHitImpactActor`、`HitStarburst.uasset`、两张 SourceArt 和导入脚本映射；受击表现仅保留 VFX 模块的 PlayerHurt/EnemyHurt Niagara。
- Host 聚焦测试扩展为生产伤害、三球方向/半径、路径外不扣血、中心/两侧分别精确扣 10、每球不重复扣血及射程结束；运行时暂留 `[RabbitProjectileDamage]` 日志，用于区分表值、卡牌修正后值和最终扣血。
- 使用 `artifact-tool` 对 `EnemyAbilities!F9` 做了最小值修改和前后视觉/值检查，但其 XLSX 导出会删除既有 Sheet Protection，仓库同步器正确拒绝。无效导出当时已从实现 worktree 恢复到基线状态，之后才执行下述定向 OOXML 修改。
- 经程序用户明确确认，改用定向 OOXML 补丁只改 `EnemyAbilities!F9` 的值 `0 → 10`；补丁在写入前后验证目标旧/新值和 `sheetProtection`，用临时文件原子替换。随后由 `artifact-tool` 只读检查/渲染最终工作簿，未再通过其导出。
- 完整 `ReEcho.Enemies` 扩大测试最初暴露三个既有 Host 测试在 headless 世界产生的精确 `Animation.Idle` Presentation 错误；各测试按实际出现次数声明该已知错误，未放宽其他错误或玩法断言。
- 集成阶段把 `origin/main@8ac53ab` 合入 Plan53：远端商店/UI、Plan54 规划以及 Plan55/56 规划全部保留；源码与资产自动组合无冲突，唯一 Git 冲突是双方旧 Editor 预构建包。冲突解决时不选择旧 DLL，先采用远端占位，再由最终组合源码 `-FullRebuild` 全量覆盖。

### 证据

- 历史 `enemy_abilities.csv` 证明 `M_RABBIT_RangedBurst` 在临时安全置零前的权威伤害为 `10`；当前 `RadiusCm` 为 `150`。
- `artifact-tool` 渲染确认目标行和格式可读，区域检查确认目标值可变为 `10` 且公式错误扫描为 0；随后 `sync_xlsx_to_csv.py --sheet EnemyAbilities` 以 `Authoring sheet protection must remain enabled` 拒绝其导出。恢复基线工作簿后 `sync_xlsx_to_csv.py --check` 重新通过。
- 定向 OOXML 补丁后，`sync_xlsx_to_csv.py --sheet EnemyAbilities` 成功发布，`sync_xlsx_to_csv.py --check` 通过；CSV diff 仅为 `M_RABBIT_RangedBurst.Damage 0 → 10`，三个 Boss 远程能力仍为 0。最终 `artifact-tool` 区域检查确认 `EnemyAbilities!F9 == 10`、公式错误扫描为 0，视觉渲染保持原表布局。
- 修改的 C++ 已执行仓库 `.clang-format`；Editor Development 构建成功并刷新预构建包。`ReEcho.Enemies.Host.RabbitProjectilePipeline`、完整 `ReEcho.Enemies`、`ReEcho.Presentation.VFX` 与 `ReEcho.Run.SaveSnapshot` 均通过。
- `python scripts/validate_project.py`、`python scripts/data/sync_xlsx_to_csv.py --check` 与 `git diff --check` 通过。完整 `scripts/data/test_sync_xlsx_to_csv.py` 为 14/15：失败项 `test_invalid_workbook_cases_fail_with_location` 同样可在未修改的 `main@158287b` 复现，属于既有错误消息定位顺序问题，不由本 Plan 引入。
- `origin/main@8ac53ab + Plan53` 组合候选执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 成功，七个 Runtime Module 预构建包均由该组合源码生成；重新运行完整 `ReEcho.Enemies`、`ReEcho.Presentation.VFX.Catalog`、XLSX/CSV 同步检查、项目校验和 `git diff --check` 均通过。

### 剩余风险

- `RadiusCm=150` 是旧锁点范围攻击遗留字段；当前解释为三球碰撞预算并得到单球半径 50。最终手感和画面匹配由用户 PIE 决定，必要时后续应增加明确的单球半径/散射角表字段，而不是继续扩大隐式推导。
- 三颗球均可独立命中，理论上同一轮最多造成三次表值伤害；是否需要整轮命中上限或受击无敌帧属于后续策划规则，不在本 Plan 暗加。
- 用户上一轮 PIE 观察到实际只扣 1，但自动化无减伤链稳定为表值 10；返修候选保留临时日志，若再次出现可直接判断是卡牌修正、剩余生命上限还是运行时数据漂移。
- 本 Plan 不恢复 Boss 其他不可见/未验收远程能力伤害。

### 人工验收结果/请求

- `PendingBeforeClose`：用户 PIE 验证兔子三球路径内/外和单次扣血。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：模块拓扑与依赖方向不变。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：稳定标识和阅读路由不变。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md` 已更新：记录三球扇形轨迹、单球半径、每球一次命中和共享表现生命周期。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：明确 EnemyHost 把每条逻辑投射物连续路径与世界目标碰撞盒相交结果转换为一次 Combat `HitIntent`。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md` 已审阅、无需修改：`FReEchoHitIntent`、Resolver、阵营和最终伤害契约未变化。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md` 已更新：同步三条逻辑轨迹共享一份三球 Niagara，并记录旧 HitStarburst 体系已删除。
