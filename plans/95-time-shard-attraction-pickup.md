# Plan 95 - 程序 - 时间碎片范围吸附与跨关拾取修复

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`Passed`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@36ef1e415ca931e9914b2d0f8cb658e5ec332095`。
- 本地实现方式：一任务一 worktree；发布本 Plan 后从最新 `origin/main` 创建 `ReEcho-plan95-time-shard-attraction`。
- 依赖 / 阻塞：依赖 Plan92 的通用 `BP_TimeShardPickup`、`AReEchoTimeShardPickupActor`、玩家 `AReEchoPlayerPawn` 与 Run 的 `GrantTimeShards` 窄事务；依赖当前活动 `AReEchoArenaSceneActor` 的 GameplayPlane。
- Writes:
  - `plans/95-time-shard-attraction-pickup.md`
  - `Source/ReEcho/Public/Graybox/ReEchoTimeShardPickupActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoTimeShardPickupActor.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoEnemyShardDropTests.cpp`
  - `scripts/ue/author_time_shard_pickup_blueprint.py`（仅在需要补充可复现默认值时）
  - `Content/ReEcho/Gameplay/Pickups/BP_TimeShardPickup.uasset`（仅通过 Unreal Editor 保存碰撞半径或默认表现值）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终精选预构建包
- Stable Reads:
  - `plans/92-enemy-time-shard-drops.md`
  - `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`
  - `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`
  - `Source/ReEcho/Public/Presentation/Scene/ReEchoArenaSceneActor.h`
  - `Source/ReEcho/Private/Presentation/Scene/ReEchoArenaSceneActor.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
- 影响模式：`SharedContract`（修改世界拾取物与 GameMode 活动场景只读接缝，不改变 Run 货币、敌人死亡或卡牌经济契约）。
- 兼容承诺 / 下游操作：敌人和武器符文继续共用同一时间碎片 Actor；掉落数额、幂等死亡结算、余额入账、存档和表现材质不变；现有 BP 尺寸、阴影和视觉 Transform 由用户调整结果继续保留。
- 明确排除：不调整逐关掉落表、商店价格、玩家移动速度、自动拾取音效、碎片跨存档持久化或敌人死亡生成规则；不让 Player、Enemy、Combat 或 Niagara 直接写货币。

## 锁定目标

1. 修复第三关及以后时间碎片经常无法拾取的问题；碎片贴地必须使用当前 GameMode 持有的活动 Arena，而不是遍历世界时可能命中的上一 Stage 待销毁 Arena。
2. `BP_TimeShardPickup` 继承组件树中的 Sphere Collision 是唯一摄取范围权威；策划/美术可在 Blueprint 组件详情中直接调整半径，不在 C++ 另写一份范围常量。
3. 玩家进入该范围时，碎片锁定玩家并沿地面快速吸向玩家；一旦开始吸附，即使玩家继续移动也不取消。接近玩家的可调捕获距离后才调用一次 `GrantTimeShards`，然后沿用现有上升淡出表现。
4. 吸附速度每帧至少为“玩家当前平面速度 + Blueprint 可调速度优势”，并同时受 Blueprint 可调最小吸附速度保护，保证正常移动时碎片追速高于玩家。
5. Overlap 与距离兜底只负责进入吸附态，不能直接在大范围边缘入账；重复 Overlap/Tick/捕获不得重复增加余额。
6. 时间碎片是当前 Encounter 的临时世界奖励：关卡结束时，所有尚未入账的碎片（包括正在吸附的碎片）立即销毁，不进入同 Stage 下一小关、下一 Stage、商店或存档恢复。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Run`、`AREA-Player`、`AREA-Scene`、`AREA-Tests`；不改变 Runtime Module 拓扑。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，已加入 `Writes`；具名审阅 `MOD-ReEchoEnemies.md`，敌人仍只发布死亡事实，预计无需正文修改。
- 设计意图：时间碎片 Actor 自己拥有“等待 → 吸附 → 已拾取表现”的局部生命周期；Player 只提供位置/当前速度，GameMode 只提供活动 Arena 地面高度，Run 仍是余额权威。用一个 BP Sphere 同时表达可视化摄取范围与运行时触发，避免碰撞半径和 C++ 距离常量分叉。
- 权威状态与依赖：
  - `BP_TimeShardPickup/Collision.SphereRadius` 是吸附触发范围权威；原生构造值仅作为资产缺失时的安全默认。
  - Pickup Actor 拥有吸附目标、吸附速度参数、捕获半径和一次性状态；这些是世界瞬时状态，不进入 SaveGame。
  - `AReEchoGameMode::ArenaScene` 是当前活动 Arena 权威，并通过窄只读接口提供 GameplayPlane Z；Pickup 不再扫描世界选择 Arena。
  - `UReEchoRunSubsystem::GrantTimeShards` 保持唯一入账事务；移动与表现完成回调不决定奖励数额。
- 决策记录：
  1. 保留现有原生组件名 `Collision`，避免重命名破坏 `BP_TimeShardPickup` 的继承组件覆盖；其语义在文档和 Category 中明确为吸附范围。
  2. 进入范围后持续追踪，不要求玩家留在范围内，否则高速玩家会在边界反复启停，造成拾取不稳定。
  3. 吸附移动保持在活动 GameplayPlane 的 XY 平面；只有现有“拾取上升淡出”移动视觉根，避免吸附过程中穿地或提前升空。
  4. 速度用 `max(MinAttractionSpeed, PlayerVelocity2D + AttractionSpeedAdvantage)` 每帧重算，兼容角色能力与临时移速变化，不复制玩家基础移速常量。
  5. 第三关后故障优先修复活动 Arena 选择。当前 `SnapToArenaGroundPlane()` 取 `TActorIterator` 的首个 Arena，在 Stage 切换帧可能同时看到旧 Arena 与新 Arena；这会把碎片贴到错误高度，并被 `CollectionHeightToleranceCm` 拒绝。
  6. Encounter 收尾由 GameMode 集中清理所有 `AReEchoTimeShardPickupActor`；清理发生在结束事务入口，`BeginNextEncounter` 和战场总清理仅作幂等兜底，避免 UI/商店期间残留或绕过标准收尾路径。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计模块拓扑不变。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计稳定路由不变。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：更新世界拾取物的活动 Arena、BP 范围与吸附生命周期说明。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：关闭前审阅；预计死亡事实边界不变。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅。
  - `README.md`：待审阅。
  - `MOD-ReEcho.md`：待维护。
  - `MOD-ReEchoEnemies.md`：待审阅。

## 锁定验收

- [x] 第 1–8 关生成的时间碎片均贴到当前活动 Arena 地面，并能被玩家稳定摄取；第三关 Stage 切换后不再出现错误高度或无法拾取。
- [x] 修改 `BP_TimeShardPickup` 的 Sphere Radius 会直接改变摄取范围；代码不存在第二份生产范围参数。
- [x] 玩家进入范围后碎片吸向玩家，速度始终高于玩家当前平面速度；玩家继续移动不会令吸附取消或永久追不上。
- [x] 进入大范围不会立即加钱；碎片到达可调捕获半径时余额只增加一次，并继续播放现有上升淡出表现。
- [x] 敌人基础掉落与武器符文碎片入口行为一致，掉落数值、寿命策略和 Run 存档语义无回归。
- [x] 关卡结束会清除全部未入账时间碎片，下一小关、下一 Stage 和商店中均无残留；已经拾取入账的余额不受影响。
- [x] 聚焦自动化、Development FullRebuild、项目校验、预构建检查与 `git diff --check` 通过。
- [x] 用户在 PIE 验收第三关以后、不同 Sphere Radius、静止/移动/高速移动下的摄取范围、追速和观感后，人工验收才可设为 `Passed`。
- [x] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：Plan-only 基线 `origin/main@36ef1e415ca931e9914b2d0f8cb658e5ec332095`；实现从包含本 Plan 的最新 `origin/main` 创建专属 worktree。
- 引擎/构建可用性：UE 5.8 安装版；修改 C++ 后最终候选必须关闭 Editor 并执行 Development `-FullRebuild`。
- 现有聚焦测试结果：Plan92 数据与 Run 掉落自动化已覆盖数额/幂等/资产存在，但没有 Stage 切换活动 Arena 和吸附世界生命周期覆盖；执行前记录当前 `ReEcho.Run.EnemyShardDrops` 基线。
- 共享契约 / 难合并资源风险：`ReEchoGameMode.*`、`MOD-ReEcho.md` 和精选预构建包是远端近期 Plan94/VFX 热点；`BP_TimeShardPickup.uasset` 不可文本合并，若远端出现同路径修改必须停止并由用户选择。
- 基线损坏时的停止条件：问题实际来自掉落 Actor 未生成、玩家 Pawn 在第三关被替换为非 `AReEchoPlayerPawn`、当前活动 Arena 无稳定读取接口，或实现需要改变货币/存档/逐关配置时，停止扩张并回报。

## 实现提纲

1. 在最新独立 worktree 记录第三关前后 Player、Pickup、活动 Arena 与 GameplayPlane 的聚焦基线，验证首个 Arena 遍历导致的高度选择风险。
2. 为 GameMode 增加当前活动 GameplayPlane 的窄只读查询；Pickup 贴地和吸附移动只消费该接口，不扫描世界 Arena。
3. 将 Sphere Collision 作为吸附触发范围；增加等待/吸附/捕获状态、最小追速、速度优势与捕获半径等 BP 可编辑参数。
4. Overlap 和 2D 距离兜底统一进入吸附态；每帧按玩家速度重算追速并沿地面移动，到捕获半径后复用一次性 `TryCollect`。
5. 扩充时间碎片自动化，覆盖 BP Sphere、速度不变量、进入范围不立即入账、捕获幂等和活动 Arena 契约；回归 Run 掉落与武器符文入口。
6. 维护 `MOD-ReEcho.md` 和 Plan 执行记录，完成格式化、聚焦测试、FullRebuild、项目/预构建/diff 检查后交由用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 聚焦基线 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Run.EnemyShardDrops` | Plan92 数额与资产契约保持通过 |
| 吸附逻辑 | 新增纯值/世界自动化 | BP Sphere 为唯一范围；追速大于玩家；大范围触发、近距离捕获、幂等入账正确 |
| 关卡接缝 | 活动 Arena 查询与 Stage 切换聚焦检查 | 第三关后 Pickup 使用 GameMode 当前 Arena，而非世界遍历顺序 |
| 武器回归 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Weapons.Runes.DynamicHitHandlers` | 符文碎片仍共用生产拾取入口 |
| 构建 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 静态 | `python scripts\validate_project.py`、`python scripts\ue\prebuilt_editor.py check`、`git diff --check` | 项目、预构建和文本门禁通过 |
| 人工 PIE | 第 1–4 关至少覆盖一次 Stage 切换，并测试静止/移动/高速移动与不同 Sphere Radius | 用户确认摄取范围、追速、入账和表现符合预期 |

## 执行记录

### 变化

- 2026-08-24：用户报告时间碎片尤其在第三关及以后无法正常拾取，并锁定改为 BP Sphere 范围摄取、进入范围后吸向玩家、吸附速度必须快于主角。
- 2026-08-24：Plan95 已发布到 `origin/main@76366797`，并从该提交创建独立实现 worktree；任务进入 `InProgress`。
- 2026-08-24：Pickup 生命周期改为“等待范围触发 → 锁定玩家沿活动地面吸附 → 进入捕获半径后一次性入账 → 上升淡出”。Overlap 和 Tick 距离兜底只启动吸附，不再在摄取范围边缘直接加钱。
- 2026-08-24：原生 Sphere 默认半径由 48 cm 调为 300 cm，生产 `BP_TimeShardPickup` 继承值经资产自动化读取为 300 cm；实际权威始终是 Blueprint 继承组件的 Sphere Radius，可在组件详情覆盖。
- 2026-08-24：GameMode 新增活动 Arena GameplayPlane 窄只读接口；Pickup 去掉 `TActorIterator` 首项选择，第三关 Stage 切换不再可能贴到待销毁旧 Arena 的地面高度。
- 2026-08-25：用户锁定未拾取碎片不跨关。Plan95 回到 `InProgress`；GameMode 在 Encounter 结束入口集中销毁全部世界时间碎片，并在下一关入口与 `ClearCombatants` 做幂等兜底。
- 2026-08-25：用户完成 PIE 验收并反馈“感觉没问题”；人工验收设为 `Passed`，任务关闭。
- 2026-08-25：发布前将 `origin/main@c3342804` 合入候选。远端仅在精选预构建包与 Plan95 发生生成物冲突，源码和资产无同路径冲突、无逻辑冲突；在组合源码上重新生成全部精选预构建包。

### 证据

- 规划审计确认现有 `Tick` 在 Sphere 半径内直接 `TryCollect`，没有吸附阶段；Overlap 同样直接入账。
- 规划审计确认 `SnapToArenaGroundPlane()` 使用 `TActorIterator<AReEchoArenaSceneActor>` 的第一个结果，而 Stage 切换在新 Arena 成为权威后才延迟销毁旧 Arena，存在第三关起选择错误地面高度的生命周期风险。
- 当前 `BP_TimeShardPickup` 已继承原生 Sphere Collision，可直接在 Blueprint 组件详情编辑半径；现有 C++ 48 cm 仅作为原生回退默认，实施后不得再以独立距离常量覆盖 BP 半径。
- 零实现基线 `ReEcho.Run.EnemyShardDrops` 2/2 通过；现有测试只覆盖数额、资产和 Run 幂等，不覆盖活动 Arena 与吸附生命周期。
- 新增 `ReEcho.Run.EnemyShardDrops.AttractionPolicy`，锁定静止玩家时至少 900 cm/s、高速玩家时始终为当前平面速度 + 300 cm/s，并验证负配置不会令碎片反向。
- 最终 `ReEcho.Run.EnemyShardDrops` 3/3、完整 `ReEcho.Run` 17/17、`ReEcho.StageTransition` 3/3、`ReEcho.Weapons.Runes.DynamicHitHandlers` 1/1 通过。
- Development `-FullRebuild` 成功（96 actions）；精选预构建包 build id `55116800`、source `7f8ffc66856b`。`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- 本机未提供 `clang-format` 可执行文件；修改文件已由编译器和人工 diff 审阅，未声称执行了格式化工具。
- 跨关清理补充后 `ReEcho.StageTransition` 3/3 通过；世界测试生成两个碎片，第一次清理精确销毁 2 个、第二次清理为 0，并证明保留敌人未被误删。最终 Development `-FullRebuild` 成功（95 actions），精选预构建包 source 刷新为 `08716e76f717`。
- 最新远端整合候选 Development `-FullRebuild` 成功（96 actions）；精选预构建包 build id `55116800`、source `a1e7cfbfb8a1`。整合后 `ReEcho.Run.EnemyShardDrops`、`ReEcho.StageTransition`、`ReEcho.Weapons.Runes.DynamicHitHandlers` 均通过，`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- 远端新增的 `ReEcho.Presentation.VFX.Catalog` 在整合候选与未合并的 `origin/main@c3342804` 上均以相同原因失败：武器 Niagara emitters `Fountain001`、`Fountain002` 未启用 local space。该问题为远端既有 VFX 门禁问题，与 Plan95 源码和资产无耦合，不阻断本任务关闭。

### 剩余风险

- 远端既有 VFX Catalog 测试存在两个武器 Niagara emitter local-space 失败；应由对应 VFX 任务修复，不属于 Plan95 范围。

### 人工验收结果/请求

- `Passed`：2026-08-25 用户完成 PIE 验收并反馈“感觉没问题”。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅、无需修改；Runtime Module 拓扑和依赖方向不变。
- `shared/CODEBASE_MAP/README.md`：已审阅、无需修改；未新增稳定模块或 AREA 路由。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已更新 BP Sphere 范围权威、活动 Arena 地面查询、吸附追速/捕获与一次性入账生命周期。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：已审阅、无需修改；Enemy 仍只发布死亡事实，不拥有 Pickup、吸附或货币。
