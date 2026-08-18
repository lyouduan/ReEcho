# Plan 52 - 程序 - 四地图场景与 Blueprint 构图改造

## 协调

- Planner 负责人：当前程序用户。
- Executor 负责人：Codex（同一 AI 兼任规划、实现、评审与集成）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`main` / `c7ddd304f3767374ea0b80f6a2d24ea921f2c551`。
- 本地实现方式（可选，仅作交接说明）：用户已确认不采用 Planner-Executor 模式、不采用任务 worktree，直接在当前本地 `main` 工作；仅显式暂存本任务路径并保护既有未提交内容。
- 依赖 / 阻塞：UE 5.8 Editor 可用；二进制资产只能经 Unreal Editor/MCP/Editor Python 创建、修改、重命名或删除；执行旧资产删除前需先完成引用迁移与零引用审计，并按准确删除清单取得当前程序用户确认。
- Writes：`Source/ReEcho/{Public,Private}/Presentation/Scene/**`；`Source/ReEcho/Private/Tests/ReEchoArenaSceneTests.cpp`；`Content/ReEcho/Art/Scene/Map/**`；`Content/ReEcho/Scene/Profiles/**`；`Content/ReEcho/Scene/Prefabs/**`；`Content/Level00.umap`；`scripts/ue/**scene**.py`、`scripts/ue/**map**.py` 及本 Plan 新增的聚焦场景资产工具；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；必要时 `shared/CODEBASE_MAP/{README.md,ARCHITECTURE.md}`；本 Plan 执行记录；最终允许列表中的 Win64 Editor 预构建包。
- Stable Reads：`Content/ReEcho/Art/Scene/Map/sc01.png`、`sc02.png`、`sc03.png`、`sc04.png`；`Content/ReEcho/Art/Scene/Plants/**`；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/Presentation/Scene/ReEchoArenaCameraActor.*`；`plans/42-editor-authored-first-arena.md`。
- 影响模式：`Exclusive`：`Content/Level00.umap`、Arena Blueprint/Profile、地图/材质 `.uasset` 和旧二进制资产清理；`SharedContract`：`AReEchoArenaSceneActor` 的 Editor 场景配置接口与模块文档；PNG 源图只读。
- 兼容承诺 / 下游操作：保留 GameMode 对唯一 `AReEchoArenaSceneActor`、MapRoot、相机 Clamp、玩家边界、敌人出生边界、GameplayPlaneZ 与脚点排序的既有消费契约；地图/Profile/插片仅拥有表现，不改变遭遇、伤害、角色碰撞、存档或关卡流程。Level00 迁移到新通用 Arena Blueprint 后，下游仍按原生基类查找，不依赖资产路径、Label 或具体场景 ID。
- 明确排除：重绘 `sc01..04`；生成新的插片源图；改变遭遇数值、玩家/敌人边界、角色尺寸、动画、武器、伤害、存档或天气玩法；运行时随机生成无法在 Editor 审阅的装饰；让场景 Profile 决定玩法；删除 `Content/ReEcho/Art/Scene/Map` 以外且未经引用审计/人工确认的历史资产。

## 锁定目标

将 `Content/ReEcho/Art/Scene/Map/sc01.png`、`sc02.png`、`sc03.png`、`sc04.png` 导入为项目唯一保留的四张竞技场地图纹理，并建立统一母材质和每图材质实例。场景通过可在 Blueprint Details 中直接选择的类型化 Scene Profile 配置地图材质、地图表现参数、插片调色板、密度、固定种子和中央安全区；四个场景子 Blueprint 提供可直接预览和继续手工构图的默认值，Level00 使用其中一个而不改变现有玩法边界契约。

插片继续使用 `GroundDetail`、`PlantRoot`、`MidDecoration`、`Foreground`、`Atmosphere` 与 `SceneEffects` 分层；提供可重复执行的 Editor 生成/清理工具，只重建带本 Plan 专用标签的生成卡片，生成后每张卡片仍可单独编辑。完成新引用迁移和零引用审计后，按当前程序用户再次确认的准确清单，通过 Unreal Editor 删除旧 Map00/Map01/Night、旧地图材质和其他已明确废弃的地图资产；不以文件系统操作手删 `.uasset`/`.umap`。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`。场景仍是主 Runtime Module 的 Editor-authored 表现适配，不创建新 Runtime Module，也不改变 `MOD-ReEchoPresentation` 的角色/敌人动画职责。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，已加入 `Writes`；同步 Arena Scene 的权威地图路径、Scene Profile、Blueprint 编辑入口、装饰生成契约与代码/资产位置。
- 设计意图：把地图纹理、材质调色和装饰构图从路径命名及一次性工具中收敛为可在 Blueprint/数据资产中审阅的表现配置，同时保护原生 Scene Actor 对玩法边界和相机的稳定契约。
- 权威状态与依赖：`AReEchoArenaSceneActor` 继续拥有玩法边界、相机相关读取参数和分层根节点；新增 Scene Profile 只拥有地图与装饰表现默认值；关卡/BP 实例可做显式 Override，但运行时不复制第二份玩法状态。`ReEcho` 依赖 Engine 内容类型，不新增反向模块依赖。
- 决策记录：采用“通用原生契约 + Scene Profile 数据资产 + 四个薄场景子 Blueprint”，而不是在 GameMode 中用 enum/路径分支切图，也不把四图硬编码到 Construction Script。代价是新增少量资产，但可获得可预览、可复用、可审计的 Editor 工作流。装饰采用显式 Bake，不在每次 Construction 时破坏性随机重建。
- 相关文档同步范围：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 必须更新；关闭前审阅 `shared/CODEBASE_MAP/README.md`（若稳定标识/路由不变则记录无需修改）和 `ARCHITECTURE.md`（若模块拓扑/依赖不变则记录无需修改）。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待实现后填写。
  - `shared/CODEBASE_MAP/README.md`：待实现后审阅。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待实现后审阅。

## 锁定验收

- [ ] `sc01..04` 各自存在规范 Texture2D 与统一母材质派生实例，导入设置和宽高比一致，且能被 Editor 加载/Cook 引用。
- [ ] 通用 Arena Blueprint 可在 Details 中选择 Scene Profile 并即时预览地图；四个薄场景子 Blueprint 分别绑定 SC01..04，切换 Profile 不改变玩法边界、相机 Transform 或敌人出生范围。
- [ ] Scene Profile 只包含表现参数；GameMode、边界、碰撞、存档和战斗语义不读取具体 SC ID 或资源路径。
- [ ] 装饰工具支持 Preview/Bake/Clear 语义或等价可观察入口，固定种子可复现；Clear 只命中专用生成标签，手工装饰不受影响；生成卡片按根部脚点落地、朝向当前 Arena Camera 并可独立编辑。
- [ ] 默认构图满足中央战斗区无大型遮挡、地标数量受限、Ground/Mid/Foreground 分层与脚点排序明确；四张地图分别配置合适的插片白名单和视觉参数。
- [ ] Level00 使用新的 SC 场景 Blueprint/Profile，仍只有一个可由 GameMode 类型化发现的 Arena Scene，原玩法边界和相机行为保持。
- [ ] 旧地图引用迁移完成并有零引用证据；仅在当前程序用户确认准确清单后由 Unreal Editor 删除，报告恢复提交和实际删除结果。
- [ ] 场景契约聚焦测试、地图/Profile/Blueprint 资产检查、项目校验、`git diff --check` 与最终 `-FullRebuild` 通过。
- [ ] 四图 Editor/PIE 构图、角色可读性、遮挡和色调由用户人工验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`main` / `c7ddd304f3767374ea0b80f6a2d24ea921f2c551`；Plan 编号基于已 fetch 的 `origin/main` 最大编号 51。
- 引擎/构建可用性：待 Plan-only 发布前按程序发布门禁执行 UE 5.8 Development Editor `-FullRebuild`；所有 Editor/commandlet 操作使用 Git common-dir Unreal 锁。
- 现有聚焦测试结果：待执行；先验证现有 `ReEcho.Presentation.ArenaScene.Contract`，避免在损坏基线上改造。
- 共享契约 / 难合并资源风险：当前主工作区已有未提交场景/角色二进制资产；本任务保护这些改动并仅显式暂存。`Content/Level00.umap`、Arena Blueprint/Profile 和地图材质为独占二进制工作面。
- 基线损坏时的停止条件：`sc01..04` 任一源图不可读或尺寸/宽高比不一致；Level00 或 Arena Prefab 存在无法隔离的外部修改；现有 Arena Scene 契约/构建失败；必须改变玩法边界、GameMode 查找契约或覆盖未知未提交二进制工作时停止并报告。

## 实现提纲

1. 记录四张源图尺寸/色彩与现有资产引用，建立精确迁移和删除候选清单。
2. 新增只读表现 Scene Profile C++ 类型和必要的聚焦契约测试，不把具体资源路径硬编码到 GameMode。
3. 通过 Unreal Editor 工具统一导入 `T_SC01..04`、创建 `M_ArenaGround`/`MI_SC01..04` 和 `DA_ArenaScene_SC01..04`。
4. 建立通用 `BP_ArenaScene` 与四个薄子 Blueprint，迁移 Level00 并保持 MapRoot、相机和边界参数。
5. 实现可重复的装饰 Preview/Bake/Clear Editor 工具；按四图 Profile 烘焙/预览默认构图并保护手工对象。
6. 运行引用审计；向当前程序用户报告准确旧资产删除清单、影响和恢复点并请求确认，确认后只通过 Unreal Editor 删除具名资产。
7. 执行聚焦测试、资产加载检查、项目校验、完整构建与人工 Editor/PIE 验收；同步模块文档和本 Plan 执行记录。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源资产 | 聚焦脚本读取 `sc01..04` 尺寸/格式并检查同构 | 四张源图可读、尺寸与宽高比符合 Backdrop 契约 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 场景契约 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.ArenaScene` | Arena Scene 聚焦报告通过 |
| Editor 资产 | 目标化 Editor Python/MCP 加载 Texture/Material/Profile/BP/Level00 并检查引用 | 四图与配置可加载；唯一 Arena Actor；旧资产零引用 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目/源码/格式不变量通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终候选构建通过并刷新匹配预构建包 |
| 人工 | Editor/PIE 依次切换 SC01..04，检查构图、遮挡、角色可读性与色调 | 当前用户记录 `Passed` 或具名返工项 |

## 执行记录

### 变化

- 2026-08-18：基于 `origin/main` 最大编号 51 创建 Plan52；尚未进入实质实现。

### 证据

- `main` 与 `origin/main` 均为 `c7ddd304f3767374ea0b80f6a2d24ea921f2c551`；无待集成远端 main 提交。
- 已只读确认四张新 PNG 位于 `Content/ReEcho/Art/Scene/Map/`；当前仅 `sc02.uasset` 已导入，旧 Map00/Map01/Night 与材质实例仍存在。

### 剩余风险

- 当前工作区包含本任务开始前的未提交二进制与配置改动，必须持续显式暂存并避免覆盖。
- 旧资产删除具有二进制引用和恢复风险，执行前仍需准确人工确认。

### 人工验收结果/请求

- `PendingBeforeClose`：四张地图的 Editor/PIE 最终构图与角色可读性由用户验收。

### 架构文档审阅结果

- 待实现完成后填写。
