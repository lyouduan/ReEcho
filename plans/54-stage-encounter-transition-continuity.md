# Plan 54 - 程序 - Stage 内跨 Encounter 连续性

## 协调

- Planner 负责人：Gavyn（JosephLE910）/ Codex。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`Passed`（2026-08-19 用户确认 Plan54 完成并授权合入、推送远端 main）。
- 本地规划发布提交为 `598f6a0`，实现基线为当时的 `origin/main@598f6a0`；本地规划 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan54-stage-transition`，分支 `plan/54-stage-transition-continuity`。发布前已将包含 Plan53 的 `origin/main@d9c4064` 合入并完成组合验证。
- 本地实现方式：延续本对话已确认的一任务一 worktree；不得在主工作区直接实现。
- 依赖 / 阻塞：依赖 Plan48 已发布的 Stage/Encounter Catalog、WaveScheduler、SpawnResolver 和 Roster 语义；与尚未关闭的 Plan53 兔子投射物碰撞可本地并行，但若两者都修改 `ReEchoEnemyActor.*`、`ReEchoGameMode.*` 或最终预构建包，进入 `main` 前必须组合审查并重跑最终构建。
- Writes：本 Plan；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；必要时新增或维护 `Source/ReEcho/{Public,Private}/Encounter/ReEchoEncounterTransition.*`；同 Stage 局间冻结需要时修改 `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`；新增或维护聚焦自动化 `Source/ReEcho/Private/Tests/ReEchoStageTransitionTests.cpp`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；若模块契约真实变化则维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`；关闭前审阅 `shared/CODEBASE_MAP/{ARCHITECTURE.md,README.md}`；最终 `shared/GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：`Design/Data/ReEchoEncounterData.xlsx` 及生成的 `Content/Data/{stages,encounters,encounter_waves}.csv`；`Source/ReEchoEnemies/**` 的 Roster/EnemyLogic 公共契约；Run、Cards、UI、Recording、Echo、Arena Scene/Camera 和现有保存契约。
- 影响模式：`SharedContract`（GameMode 的 Encounter/局间 UI 生命周期与 Enemy Host/Roster 连续性）；不改变跨机器写权限。
- 兼容承诺 / 下游操作：继续以 Stage 表的 `PreserveEnemiesBetweenEncounters` / `ClearEnemiesOnEnter` 为敌人生命周期真源；不新增第二份 Stage 常量。保持八场、三波、刷怪种子、活跃单位上限、保存版本、Echo 回放、卡牌/商店顺序和 Arena 边界不变。玩家生命值/属性在新 Encounter 开始时的既有刷新语义本 Plan 不改，只修正位置连续性。
- 明确排除：不调整 Stage/Encounter/Wave 数值，不制作新场景或局间 UI，不改变怪物 AI、伤害、掉落、玩家回血规则或跨 Stage 出生点产品设计；不把保存版本升级纳入本 Plan；不以销毁后从快照重建 Actor 冒充“保留”。

## 锁定目标

修复同一 Stage 内相邻 Encounter 的世界连续性。进入抽卡、商店和 Echo 管理等局间流程时，存活怪物不得被销毁；下一 Encounter 开始后必须继续使用原有 Enemy Host Actor，保持稳定 EnemyId、SpawnIndex、世界位置、当前生命和持久逻辑状态。新 Encounter 的配表波次仍按 0/10/20 秒正常追加生成，并将保留怪物计入 `ActiveUnitLimit`，不能用新生成的一批替换旧 Roster。

玩家在同一 Stage 内进入下一 Encounter 时保持上一 Encounter 结束位置，不再被传送到世界原点；局间开始时停止残余移动，局间 UI 期间不得发生玩家或怪物的移动、攻击、投射物命中、冷却/状态推进。进入不同 Stage 时继续按 Stage 策略清理旧 Roster，并把玩家放到当前 Arena 的 Stage 入口兼容位置；当前只有 `Level00`，继续使用既有原点/玩法平面入口，不新增隐藏坐标常量。

Encounter 自身仍是独立录制和回放单元：旧 Encounter 的 Echo、Recorder、Director、WaveScheduler、预警和瞬时攻击载体按原有边界结束或重置；保留的是同 Stage 的玩家位置与存活怪物，不把上一场 Echo 或波次游标带入下一场。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Encounter`、`MOD-ReEcho / AREA-Presentation`（Enemy Host 生命周期）、`MOD-ReEcho / AREA-Run`（只读当前/下一 Encounter 阶段）；`MOD-ReEchoEnemies` 的 Roster/EnemyLogic 仅消费局间冻结契约，除非实现证明必须新增模块公共 API，否则不转移状态所有权。
- 对应模块文档：必须维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，把 Stage 过渡决策、同 Stage Actor/Transform 连续性和局间冻结写入 Encounter 主流程。若修改 `Source/ReEchoEnemies` 或其公共类型，必须同时维护 `MOD-ReEchoEnemies.md`；否则关闭时记录“已审阅、无需修改：Roster 状态所有权未变化”。
- 设计意图：Stage 是多个 Encounter 的连续玩法空间，Encounter 结束只终止本场时钟、波次、录制和 Echo，不等于重建整片战场。所有局间入口和下一场启动必须消费同一个类型化过渡决策，不能由抽卡、商店或异常回退路径各自决定是否清场/传送。
- 权威状态与依赖：
  - Encounter Catalog 的 `StageId` 与下一 Stage 行继续决定是否同 Stage以及敌人清理策略。
  - `UReEchoEnemyRosterComponent` 继续独占当前活动敌人集合；保留通过原 Actor/Host 引用完成，不创建第二份可写敌人数组或用 Transform 快照重生。
  - Player Actor 的当前世界 Transform 是同 Stage 位置权威；跨 Stage 入口由 Arena/GameMode 的既有入口解析负责。
  - `AReEchoEncounterDirector`、WaveScheduler、Recorder 和 Echo 仍是一 Encounter 一生命周期，不因保留 Actor 而跨场复用。
- 决策记录：
  - 新增单一、纯值的 Stage 过渡判定（具体类型名由实现确定），一次计算并明确给出“是否同 Stage、是否保留 Roster、是否保留玩家位置、是否重置 Encounter 瞬时状态”。`ShowTraitCardChoice()`、局间异常回退和 `BeginNextEncounter()` 必须复用该结果。
  - 不接受“结束时销毁、下一场按相同 EnemyId/位置重新 Spawn”的方案，因为它会丢失 Actor 身份、SpawnIndex、生命、逻辑状态和下游表现绑定，并继续制造用户观察到的重刷感。
  - 局间流程不能只依赖 `bEncounterTransitioning` 阻止 GameMode；Enemy Host 当前有独立 Tick，因此保留敌人时必须有显式的局间模拟冻结/恢复入口。冻结期间世界可以继续服务 UMG，但战斗对象不得推进。恢复时不能把 UI 停留的现实时间计入敌人玩法冷却或状态时长。
  - 进入局间时立即结束旧 Recorder、Director、Echo 和瞬时攻击载体；同 Stage 保留 Enemy Host 的身份、Transform、HP 与非瞬时状态。若某个攻击 phase 无法安全冻结，必须在具名边界上取消该瞬时动作，而不是销毁 Host 或重置整个 EnemyLogic。
  - 同 Stage 开始下一 Encounter 时不调用玩家原点传送；跨 Stage 或新 Run 才解析入口位置。玩家速度在局间开始时归零，避免 UI 打开后惯性漂移。
  - 现有 `ShowTraitCardChoice()` 的无条件 `ClearCombatants()` 与 `BeginNextEncounter()` 的无条件 `SetActorLocation(FVector(0,0,112))` 是本次已确认根因；修复必须移除这两个绕过 Stage 策略的独立决策点。
- 相关文档同步范围：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 必改；`MOD-ReEchoEnemies.md` 按公共契约是否变化决定正文修改；`ARCHITECTURE.md` 审阅 Stage/Encounter 跨模块不变量，若只落实既有 Plan48 拓扑则无需修改正文；`README.md` 审阅路由标识，预计无需修改。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md` 已更新：记录 Stage/Encounter 连续性、纯过渡决策、冻结入口与代码位置；
  - `MOD-ReEchoEnemies.md` 已更新：记录新增的 `ResetEncounterTransientState` 窄命令及持久/瞬时状态边界；
  - `ARCHITECTURE.md` 已审阅、无需修改：本 Plan 只落实既有 `ReEcho → ReEchoEnemies` Host/Logic 拓扑，没有新增 Runtime Module 或反向依赖；
  - `README.md` 已审阅、无需修改：稳定模块标识和阅读路由未变化。

## 锁定验收

- [x] 配表矩阵正确：1→2、3→4、4→5、6→7为同 Stage 连续；2→3、5→6、7→Boss 为跨 Stage 重置。非法/缺失 Stage 引用必须安全失败并记录诊断，不能默认为保留。
- [x] 同 Stage 局间前后，每个存活怪物的 Enemy Host 对象身份、EnemyId、SpawnIndex、世界位置、当前生命值和应保留逻辑状态一致；没有 Destroy/重新 Spawn 伪连续。
- [x] 同 Stage 下一 Encounter 的 0 秒波次正常新增；原 Roster 仍在且被计入 `ActiveUnitLimit`，不会因保留数量导致超上限或重复注册。
- [x] 跨 Stage 时旧 Roster 只清理一次且没有悬空弱引用；下一 Stage 只包含其合法新增敌人。
- [x] 玩家在同 Stage 前后保持非原点结束位置；跨 Stage 和新 Run 仍使用合法 Arena 入口。局间开始时速度归零，Camera/ViewTarget 不跳回错误对象。
- [x] 局间 UI 打开期间，保留怪物和玩家不移动、不攻击、不造成/受到战斗伤害，投射物不推进，玩法冷却/状态不消耗 UI 停留时间；恢复后不会立即结算局间积累的动作。
- [x] 旧 Encounter 的 Echo、Recorder、WaveScheduler、预警和瞬时攻击载体不会泄漏进下一 Encounter；下一场录制和 Echo 仍从各自时间零点开始。
- [x] Trait→Shop→Echo 管理的正常路径，以及报价/界面失败后直接进入下一 Encounter 的回退路径，都使用相同过渡策略。
- [x] Plan54 不修改权威 XLSX/CSV、保存版本、玩家回血/属性刷新、八场顺序或敌人平衡数值；保存、商店、音频和 Echo 回归通过，Traits 扩大回归仅保留已记录的远端基线默认武器期望错配。
- [x] 新增聚焦自动化覆盖纯过渡矩阵和实际 World Actor 连续性；Editor Development 构建、项目校验和 `git diff --check` 通过。
- [x] 用户在主文件夹 PIE 验收：战斗1末尾把玩家与一只受伤怪物留在易辨识位置，经抽卡/商店进入战斗2后两者位置正确且怪物不是重刷；战斗2→3确认旧怪物清理、玩家回到 Stage 入口。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：实现前 fetch；批准基线为已核验的 `origin/main@598f6a0`。实现期间远端前进到 `8ac53ab`；只读审计见执行记录，尚待用户决定是否现在重放到新基线。
- 引擎/构建可用性：UE 5.8 安装版；构建前确认交互式 Editor 已关闭并取得 Git common-dir Unreal 锁。
- 现有聚焦测试结果：实现前记录 `ReEcho.Encounter`、Roster/Enemy Host 与 Save 聚焦基线；当前人工失败证据为同 Stage 敌人重刷感和玩家被传送回原点。
- 共享契约 / 难合并资源风险：主要热点是 `ReEchoGameMode.*`、`ReEchoEnemyActor.*`、模块文档和最终预构建包；Plan53 若尚未集成，不能整分支覆盖，需在最新 main 上组合其兔子投射物差异。
- 基线损坏时的停止条件：Stage/Encounter 外键无效、主线本身无法构建、现有 Roster 注册失效，或发现局间 UI 必须由未决定的新产品规则暂停整个 World 时，停止扩大实现并报告。

## 实现提纲

1. 在 Encounter 领域增加纯 Stage 过渡判定，并以 1→2、2→3、3→4、4→5、5→6、6→7、7→8 的生产矩阵建立无 World 自动化。
2. 把 GameMode 中“结束旧 Encounter”“进入局间”“启动下一 Encounter”拆成具名小步骤：结束本场瞬时对象、冻结/恢复保留 Host、按过渡策略清理 Roster、解析玩家入口/保留位置、初始化新场时钟和波次。
3. 移除 `ShowTraitCardChoice()` 无条件清场；所有 Trait/Shop/Echo 和失败回退路径复用同一缓存或可重算的过渡决策，防止 UI 分支改变战场生命周期。
4. 为 Enemy Host 建立最窄的局间冻结契约；保留对象身份、Transform、HP 和持久逻辑，阻止 Actor Tick、投射物、攻击与 WorldTime 截止状态在 UI 期间偷跑。恢复时处理冻结时长，不把 UI 时间算入玩法。
5. 修改 `BeginNextEncounter()`：同 Stage 不传送玩家、不重建保留敌人；跨 Stage 清理旧 Roster并使用 Arena 入口；随后配置新 Encounter 并让新波次在现有 Roster 容量基础上追加。
6. 新增 World 集成测试，使用受伤敌人、非原点玩家位置和稳定对象引用验证同 Stage连续、跨 Stage清理、局间冻结及 0 秒波次容量行为。
7. 运行聚焦自动化、受影响回归、格式化、Editor Development 构建和静态校验；更新 Plan 执行记录与相关 `CODEBASE_MAP` 文档，交给用户 PIE。
8. 用户人工通过后，在最新 `origin/main` 上组合其他提交，执行 `-FullRebuild`、提交精选预构建包、发布 `main` 并安全清理 worktree。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目、文档、路径和预构建声明无静态错误 |
| C++ 格式/构建 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新跟踪包 |
| 过渡策略 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.StageTransition` | 生产 Stage 矩阵、非法输入和统一决策通过 |
| World 连续性 | `ReEcho.StageTransition.WorldContinuity` | 同 Stage Actor/位置/HP/冻结保持；跨 Stage清理与玩家入口正确 |
| 相邻回归 | `ReEcho.Encounter`、Enemy Host/Roster、Run Save、Recording/Echo、UI Flow 聚焦集合 | 波次、保存、局间 UI、Echo 与敌人生命周期无回归 |
| 人工 PIE | 用户完成战斗1→2和2→3具名场景 | 同 Stage不重刷/不传送，跨 Stage按规则清理/复位 |
| 发布 | 最新候选 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；最终 fetch审计 | 精选 Editor包与最终源码一致，无未审计远端差异 |

## 执行记录

### 变化

- 在 `ReEchoEncounterRuntime` 新增纯值 `FReEchoStageTransitionDecision` / `ReEchoStageTransition::Resolve`，由生产 Stage/Encounter Catalog 唯一决定 Roster 和玩家位置策略，非法引用失败关闭。
- `AReEchoGameMode` 拆分 Enemy Roster/Echo 清理，移除 Trait 屏无条件清场和下一场隐藏坐标传送；所有正常/失败 UI 路径复用同一过渡解析。跨 Stage 入口从 Arena Scene 中心、玩法平面与玩家碰撞半高解析。
- 同 Stage 局间保留原 Enemy Host，显式停止玩家、阻断能力并冻结 Host；恢复下一场前解除冻结。旧 Echo、敌方逻辑投射物、特殊/Boss/Fuse/受击位移等瞬时状态在具名边界清理，普通攻击冷却、序号、身份、Transform 和生命保留。
- 新增 `ReEcho.StageTransition.PolicyMatrix` 与 `ReEcho.StageTransition.WorldContinuity`，覆盖生产矩阵、最终边界失败和实际 World 中对象身份/EnemyId/SpawnIndex/位置/生命/冷却连续性。

### 证据

- 用户 PIE：同 Stage怪物位置表现为重刷，玩家在跨小关时回到错误入口。
- Editor Development 增量构建通过并刷新 Editor 预构建包；`python scripts/validate_project.py` 与 `git diff --check` 通过。
- 自动化通过：`ReEcho.StageTransition`、`ReEcho.Encounter`、`ReEcho.Enemies.Logic`、`ReEcho.Run.SaveSnapshot`、`ReEcho.Recording`、`ReEcho.Run.Echo`、`ReEcho.UI.IntermissionContexts`。
- `ReEcho.Enemies.Host` 三项旧测试仍因裸 Host 缺少 Presentation Catalog 而报告既有 `Animation2D semantic 'Animation.Idle' could not resolve` 错误；本 Plan 新测试用精确 expected-error 隔离该既有表现测试夹具缺口，没有把它误判成连续性失败。
- 远端差异审计：`598f6a0..8ac53ab` 的已实现源码只修改 `ReEchoInventoryShopWidget.cpp`，与 Plan54 没有文本冲突；预构建包有必然二进制冲突，必须在组合源码上重建。Plan55 尚为规划但未来写 `AReEchoEnemyActor`，Plan56 尚为规划但未来写 `ReEchoGameMode.cpp`，两者与 Plan54 存在后续逻辑/热点耦合，不能整文件覆盖。
- 最终组合审计：将 `origin/main@d9c4064`（含 Plan53）合入后，源码自动组合且仅旧预构建包产生 Git 冲突；冲突包未选任一旧版本，而由组合源码 FullRebuild 覆盖。Plan53 三球逻辑与 Plan54 冻结清理的唯一逻辑耦合已修正为只让中心球发布共享 Niagara `Ended` 事件，三条玩法碰撞轨迹仍全部清理。
- 最终发布候选执行 Development `-FullRebuild` 成功；`ReEcho.StageTransition`、完整 `ReEcho.Enemies`、`ReEcho.Encounter`、`ReEcho.UI.IntermissionContexts`、`ReEcho.Run.SaveSnapshot`、`ReEcho.Run.Echo`、XLSX/CSV 检查、项目校验和 `git diff --check` 全部通过。
- 扩大回归 `ReEcho.Shop`、`ReEcho.Audio` 通过；`ReEcho.Traits` 五项中四项通过，唯一失败为 `CsvEffectsApply` 仍期待默认武器 `W_J_02`、而当前权威数据返回 `W_J_01`。Plan54 相对 `origin/main` 在 Trait 测试、角色/武器 CSV 与主策划工作簿均无差异，因此这是已存在的基线期望错配，不由本 Plan 引入，也不在跨 Encounter 连续性任务内暗改产品默认值。

### 剩余风险

- 自动化已证明 Enemy Host 在 World 继续 Tick 一秒时不移动、不消耗普通攻击冷却，并取消旧瞬时行动；玩家位置、Trait→Shop→下一场完整视觉路径和跨 Stage Arena 入口仍需用户 PIE。
- 玩家生命值/属性的新 Encounter 刷新语义保持现状；若产品希望同 Stage同时保留当前 HP，需要用户另行明确后新开 Plan。
- Plan55/56 目前仍只有规划；未来实现若修改 EnemyActor/GameMode，必须从本 Plan 已发布的新 main 开始，不能用旧分支整文件覆盖。
- `ReEcho.Traits.CsvEffectsApply` 的默认武器期望需由后续对应任务确认是更新测试到 `W_J_01`，还是恢复产品默认值为 `W_J_02`。

### 人工验收结果/请求

- `Passed`：2026-08-19 用户确认 Plan54 完成并要求直接合入、推送远端 main；视觉验收由用户完成，未交给执行者消耗图像 Token。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已补 Stage/Encounter 连续性、过渡策略、Host 冻结和测试入口。
- `MOD-ReEchoEnemies.md` 已补 `ResetEncounterTransientState` 公共命令与瞬时/持久状态边界。
- `ARCHITECTURE.md` 已审阅：Runtime Module 和依赖方向未改变，无需修改。
- `README.md` 已审阅：模块稳定标识与导航未改变，无需修改。
