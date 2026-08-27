# Plan 141 - 程序 - 墙体派生怪物出生安全区

## 协调

- Planner 负责人：当前对话程序 Planner（Codex）。
- Executor 负责人：独立程序 Executor，Plan 发布后启动。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@6fdb7938624031cafc6401e1b72fbc5651def6ec`。
- 本地实现方式：独立 worktree `C:\tmp\ReEcho-plan141-wall-derived-spawn-bounds`。
- 依赖 / 阻塞：四场 Arena Blueprint 中 `WallEast/WallWest/WallNorth/WallSouth` 是当前碰撞墙权威；Plan134 的事务场景切换和 Plan136 的场景资产结果保持。
- Writes:
  - `plans/141-wall-derived-enemy-spawn-bounds.md`
  - `Source/ReEcho/Public/Presentation/Scene/ReEchoArenaSceneActor.h`
  - `Source/ReEcho/Private/Presentation/Scene/ReEchoArenaSceneActor.cpp`
  - `Source/ReEcho/Public/Encounter/ReEchoEncounterRuntime.h`
  - `Source/ReEcho/Private/Encounter/ReEchoEncounterRuntime.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoArenaSceneTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoEncounterRuntimeTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoGameModeTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `Binaries/Win64/ReEchoEditor.prebuilt.json`
  - `Binaries/Win64/ReEchoEditor.target`
  - `Binaries/Win64/UnrealEditor-ReEcho*.dll`
- Stable Reads:
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC01.uasset`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC02.uasset`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC03.uasset`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC04.uasset`
  - `Content/Data/spawn_profiles.csv`
  - `Content/Data/spawn_policies.csv`
- 影响模式：`SharedContract`；修改 Arena 到 SpawnResolver 的出生边界契约，并影响正式波次、预警承诺、存档恢复后的后续出生和 `GMSpawnFox`。
- 兼容承诺 / 下游操作：不修改已预留或已保存的世界出生位置；同一输入保持确定性；预警与提交仍复用同一位置；四场 BP 墙体继续由美术直接编辑。
- 明确排除：不修改 SC01-SC04 Blueprint、插片、墙体 Transform、地图、CameraClamp/PlayerBounds、CSV/XLSX、敌人 AI、寻路、伤害或掉落；不把物理场景查询引入纯 SpawnResolver。

## 锁定目标

怪物正式波次与 `GMSpawnFox` 的出生位置必须由当前活动 Arena 四面碰撞墙的世界空间内侧区域派生，不再依赖需要与墙体手工同步的 `EnemySpawnBounds` 半尺寸，也不再假定 Arena 位于世界原点。候选必须为包含安全内缩后的合法点；越界候选应重试而不是 Clamp 到墙边，安全区无效或无可用候选时拒绝该次出生并记录明确错误，绝不把怪物生成到墙外。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`AREA-Encounter`、`AREA-Presentation`、`AREA-Tests`；GameMode 负责把活动 Arena 的世界安全区接到纯确定性 SpawnResolver。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，更新 Arena 权威状态、出生输入契约、拒绝策略和 GM 复用关系；已加入 Writes。
- 设计意图：消除墙体与 `EnemySpawnBounds` 的重复配置，让美术只移动墙体即可改变真实出生安全区，同时保持出生求解纯函数、可测试和确定性。
- 权威状态与依赖：四个 Wall 组件是出生外边界权威；Arena Scene 将四墙包围区域转换为世界 `FBox2D`，GameMode 只传值，SpawnResolver 不依赖 UObject 或碰撞查询。`EnemySpawnBounds` 保留序列化/Editor 可视兼容，但不再决定正式出生范围。
- 决策记录：
  1. 按命名语义读取墙体世界 Bounds 的内侧面：West.MaxX、East.MinX、South.MaxY、North.MinY，支持非对称位置与 Arena 整体平移；墙旋转时采用世界 AABB，结果偏保守而不会越墙。
  2. Arena 暴露可调 `EnemySpawnWallPadding`，安全区由墙内侧再统一内缩；怪物实际碰撞尺寸若当前预生成链无法无复制取得，则第一版使用覆盖现有最大普通怪碰撞的保守场景 Padding，并在 Plan 记录取值依据，不在 SpawnResolver 硬编码第二份敌人数值。
  3. `FReEchoSpawnResolveRequest` 改为显式世界 Min/Max 或 `FBox2D` 值语义；Player/Echo Anchor 按该 Bounds 处理，删除世界零点假设。
  4. 随机候选越界直接继续尝试，不再 Clamp；确定性 fallback 在安全区内部寻找合法点，并遵守玩家距离与既有出生间距，失败则 fail closed。
  5. 不以运行时 Sweep/Overlap 作为主要判断，避免物理查询破坏确定性和预警/提交一致性。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；预计模块拓扑和代码路线不变。更新 `modules/MOD-ReEcho.md` 的 Arena/Encounter 契约。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待审阅。
  - `shared/CODEBASE_MAP/README.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待更新。

## 锁定验收

- [ ] 四墙派生的世界安全区正确支持非对称墙体、非零 Arena 中心、墙体厚度和统一内缩；退化/交叉墙体配置明确失败。
- [ ] SpawnResolver 的所有成功结果严格位于安全区内部；越界随机候选不 Clamp，fallback 不违反玩家/现有出生间距。
- [ ] 正式波次预警与 Commit 继续共享同一预留位置，存档字段与旧已保存位置不迁移。
- [ ] `GMSpawnFox` 与正式波次消费同一活动 Arena 安全区，且不会刷到墙外。
- [ ] SC01-SC04 当前 Blueprint 不发生二进制变化；场景切换后使用新活动 Arena 的墙体安全区。
- [ ] 聚焦 Arena、Encounter SpawnResolver、GameMode、StageTransition 自动化及项目静态校验通过。
- [ ] 最终 `-FullRebuild` 与精选预构建包检查通过。
- [ ] 用户 PIE 验证至少 SC03 的边缘波次和 `GMSpawnFox 16 1000` 均不会在墙外生成，人工验收才可设为 `Passed`。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@6fdb7938624031cafc6401e1b72fbc5651def6ec`；相对 Plan136 发布点的传入变化涉及 GameMode/头文件和预构建包，但未修改 ArenaSceneActor、EncounterRuntime 或 SC01-SC04 Blueprint。实现基于最新主线，不回退传入命令和 UI/VFX 变化。
- 引擎/构建可用性：UE 5.8 Development FullRebuild 在 Plan136 发布候选通过；执行前仍运行 LFS 检查并遵守同克隆 Unreal 锁。
- 现有聚焦测试结果：Plan136 的 `ReEcho.GameMode` 4/4、ArenaScene 1/1、StageTransition 3/3 通过；边界契约变化后全部视为需重跑。
- 共享契约 / 难合并资源风险：`FReEchoSpawnResolveRequest` 是模块内公共值类型，调用点和测试必须原子更新；GameMode 有最新远端变化，需要保留并在最终远端审计中重新比较。
- 基线损坏时的停止条件：四墙无法形成有效轴对齐内侧矩形、现有 BP 墙体命名/类型不满足契约、必须修改 CSV Schema/存档格式/场景二进制，或无法在不破坏确定性的情况下取得所需安全间距。

## 实现提纲

1. 为 ArenaSceneActor 增加纯墙体内侧安全区计算和世界查询，覆盖非零中心、非对称、厚度、旋转 AABB、Padding 与退化失败测试。
2. 将 SpawnResolver 请求改为世界安全 Bounds；随机候选改为拒绝越界，fallback 改为安全区内确定性搜索，补齐边界/距离/间距测试。
3. GameMode 在活动 Arena 刷新时缓存或按需读取安全区，正式预留和 `GMSpawnFox` 统一消费；错误 fail closed 并打印 SceneId/Bounds。
4. 保持 SC01-SC04 Blueprint 字节不变，运行资产只读审计和 Stage/场景切换回归。
5. 更新模块文档、执行记录与验证证据，提交 Review 候选等待用户 SC03 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan141 编号、结构、范围与中文正文通过 |
| LFS | `python scripts/setup_lfs.py --check` | LFS 跟踪对象完整 |
| C++ | `scripts\\ue\\Build-Editor.cmd -Configuration Development` | UHT/UBT 通过 |
| Arena | `ReEcho.Presentation.ArenaScene` | 墙内侧安全区、非对称/平移/Padding/退化契约通过 |
| Encounter | `ReEcho.Encounter.DeterministicSpawnResolver` 及新增用例 | 所有成功候选位于 Bounds，拒绝 Clamp 与非法 fallback |
| GameMode/Stage | `ReEcho.GameMode`、`ReEcho.StageTransition` | GM 与正式切换接线回归通过 |
| 资产不变 | SC01-SC04 SHA-256 前后比对 | 四个 Arena BP 字节不变 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、数据和源码不变量通过 |
| 发布候选 | `scripts\\ue\\Build-Editor.cmd -Configuration Development -FullRebuild`、`python scripts/ue/prebuilt_editor.py check` | 最终源码与精选 Editor 包一致 |
| 人工 PIE | SC03 正式波次；`GMSpawnFox 16 1000` | 怪物均在墙内且可正常击杀 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
