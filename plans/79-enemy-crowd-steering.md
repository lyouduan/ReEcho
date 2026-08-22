# Plan 79 - 程序 - 怪物群体移动与防堆积优化

## 协调

- Planner 负责人：当前对话程序 AI。
- Executor 负责人：当前对话程序 AI。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@cd962b9297aef9eb773d44b5a12a50319f271105`。
- 本地实现方式：`codex/enemy-crowd-steering`，独立 worktree `ReEcho-worktrees/enemy-crowd-steering`。
- 依赖 / 阻塞：依赖现有 `EnemyLogic -> MovementDelta -> EnemyHost swept movement` 与 Encounter `EnemyRoster`；无外部资产依赖。
- Writes:
  - `plans/79-enemy-crowd-steering.md`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyCrowdSteering.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`
  - `Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `Binaries/Win64/` 下 `shared/GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEchoEnemies/{Public,Private}/Enemies/ReEchoEnemyLogicComponent.*`
  - `Source/ReEchoEnemies/{Public,Private}/Enemies/ReEchoEnemyRosterComponent.*`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`
- 影响模式：`SharedContract`。保持 EnemyLogic 行为权威不变，在主模块 Host 世界移动层增加群体移动解算，并让同一 Roster 内普通怪物互相忽略硬 Sweep。
- 兼容承诺 / 下游操作：攻击冷却、伤害、射程、Boss 技能、击退、保存状态和表现事件不变；旧存档无需迁移。
- 明确排除：不引入 NavMesh、Detour Crowd、Mass Entity 或物理推挤；不改变玩家碰撞；不允许怪物穿墙；不修改怪物数值工作簿；不处理场景本身的封闭死角。

## 锁定目标

- 多只普通怪物从相同或不同方向追击同一目标时，不再因为彼此 Pawn Sweep 阻挡形成静止队列。
- 近战怪物围绕目标形成稳定、可读的分布；远程怪物保留自身行为逻辑，但能避开邻居密集方向。
- 群体移动具有确定性，不使用逐帧随机数；同一 SpawnIndex、位置和邻居快照得到相同输出。
- 场景、玩家和 Boss 的硬碰撞安全不降低，Combat 命中与攻击节拍不改变。

## 架构影响与设计决策

- 受影响架构标识：`AREA-Enemies`、`MOD-ReEcho`、`MOD-ReEchoEnemies`。
- 对应模块文档：维护 `MOD-ReEcho.md` 的 EnemyHost 世界移动职责；维护 `MOD-ReEchoEnemies.md` 的 Roster/Host 群体协调契约，均已加入 `Writes`。
- 设计意图：EnemyLogic 继续只产生资源无关追踪/攻击意图；新的纯值 Crowd Steering 在 Host 应用 Transform 前，把追踪位移与攻击槽位、邻居 Separation、切向绕行及受阻恢复合成最终世界位移。
- 权威状态与依赖：不改变权威状态所有者或模块依赖。短时受阻计时由 EnemyHost 持有，不进入运行存档；Roster 仍是稳定排序的存活怪物入口。
- 决策记录：
  - 不直接关闭全部 Pawn 碰撞；仅对同一 Roster 内普通敌人建立双向 MoveIgnore，玩家/场景仍硬阻挡。
  - 不把 Crowd 查询下沉进 EnemyLogic；Host 从已注入 Roster 读取稳定邻居快照，避免 World Scan 和 `ReEchoEnemies -> ReEcho` 反向依赖。
  - 第一版使用稳定排序的 O(n²) 邻居采样，目标规模 20～50 只；保留升级二维空间哈希的接口，不提前增加复杂度。
  - 完全重叠和左右等价时以 SpawnIndex 生成稳定方向，禁止随机抖动。
- 相关文档同步范围：审阅 `ARCHITECTURE.md` 与 `README.md`；预计依赖拓扑和路由标识不变，无事实变化则只在执行记录说明。更新 `MOD-ReEcho.md`、`MOD-ReEchoEnemies.md`。

## 锁定验收

- [ ] 20～50 只怪物同向追击时持续产生有效实际位移，不形成永久静止队列。
- [ ] 多方向追击能形成分散包围，怪物之间不会完全重叠或高频左右抖动。
- [ ] 槽位、Separation、切向选择和完全重叠退让均有纯值自动化，结果按 SpawnIndex 确定。
- [ ] 普通怪物互相不做硬 Sweep；玩家、场景、Boss 仍保留具名硬碰撞策略。
- [ ] 攻击、受击击退、狐狸突进、兔子投射物、Boss 传送/技能以及保存恢复回归通过。
- [ ] 通过格式化、Enemy Logic/Host 聚焦自动化、Development FullRebuild、`validate_project.py`、`git diff --check` 和预构建包一致性检查。
- [ ] 用户在 PIE 验收密集追击、包围可读性、远程怪不堵塞和 Boss 场景。

## Step 0 门禁

- 基线分支/提交：`origin/main@cd962b9297aef9eb773d44b5a12a50319f271105`，创建 Plan 前 fetch 无外部提交。
- 引擎/构建可用性：UE 5.8 标准构建入口可用；Plan 发布前执行程序 FullRebuild 门禁。
- 现有聚焦测试结果：实现前不声称新 Crowd 行为基线；现有 Enemy Logic/Host 测试作为回归基准。
- 共享契约 / 难合并资源风险：仅 C++、Markdown 与精选预构建包；不修改 `.uasset`。
- 基线损坏时的停止条件：若现有 Enemy Logic/Host 聚焦测试在未改实现前出现非已知失败，记录并隔离，不把基线失败误归因于本任务。

## 实现提纲

1. 新增纯值 `FReEchoEnemyCrowdSteering`，输入自身 SpawnIndex/位置/半径、目标、原始位移和稳定邻居数组，输出槽位化、分离及切向修正后的位移。
2. EnemyHost 从 Roster 构建邻居样本，普通怪互设 MoveIgnore；Boss 不参与普通互穿策略并拥有最高优先级。
3. Host 记录期望/实际位移和受阻时间，在持续受阻时稳定翻转绕行侧并提高分离强度；攻击、击退和特殊位移绕过普通槽位修正。
4. 增加纯值与 Host 组合测试，维护模块文档和执行记录。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Crowd 纯值 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Crowd` | 分离、槽位、切向、确定性与速度上限通过 |
| Enemy 回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies` | Logic 与 Host 现有行为无回归 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`git diff --check`、`prebuilt_editor.py check` | 项目与发布包一致 |
| 人工 | PIE：50 敌同向/四向、近远程混编、Boss | 无永久堆积、无穿墙、攻击与特殊动作正常 |

## 执行记录

### 变化

- 待执行。

### 证据

- 待执行。

### 剩余风险

- 待执行。

### 人工验收结果/请求

- `PendingBeforeClose`。

### 架构文档审阅结果

- 待执行。
