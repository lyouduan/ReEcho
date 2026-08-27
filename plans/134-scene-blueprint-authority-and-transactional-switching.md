# Plan 134 - 程序 - 场景 Blueprint 单一权威与事务式切换

## 协调

- Planner 负责人：当前对话程序 Planner（Codex）。
- Executor 负责人：独立程序 Executor，待 Plan 发布后启动。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`OpenAI Codex`。
- 任务状态：`Review`（SC01 非视觉程序契约统一与玩法高度解耦已通过技术门禁；等待 Blueprint/PIE 重新人工验收，不发布远端）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@41da5187ba694d2629b6e2c0a6bbb9f5b3fb6c54`。
- 本地实现方式：规划 worktree `C:\tmp\ReEcho-plan134-scene-blueprint-authority-plan`；实现使用独立 Plan134 worktree，不复用含未提交 `BP_ArenaScene_SC01.uasset` 的审查 worktree。
- 依赖 / 阻塞：以当前 `BP_ArenaScene_SC01` 组件树和已发布 Plan84/Plan52 场景契约为迁移参考；现有审查 worktree 中用户/Editor 修改的 SC01 二进制资产只读保留，未经明确交接不得覆盖。Plan133 同样修改 Stage1→Stage2 编排和 `ReEchoGameMode.*`，实现与集成时必须组合其最新权威候选，不能整文件覆盖。
- Writes:
  - `plans/134-scene-blueprint-authority-and-transactional-switching.md`
  - `Source/ReEcho/Public/Presentation/Scene/ReEchoArenaSceneActor.h`
  - `Source/ReEcho/Private/Presentation/Scene/ReEchoArenaSceneActor.cpp`
  - 新增或调整场景 Catalog 类型及其测试（位于 `Source/ReEcho/{Public,Private}/Presentation/Scene/`）
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoArenaSceneTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoStageTransitionTests.cpp`
  - `Content/Level00.umap`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene.uasset`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC01.uasset` 至 `BP_ArenaScene_SC04.uasset`
  - 单一场景 Catalog 资产（预期 `Content/ReEcho/Scene/DA_ArenaSceneCatalog.uasset`，Executor 可在不改变契约的前提下确定准确路径）
  - 仅为迁移/只读审计所需的 `scripts/ue/*scene*.py`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - 必要精选 Editor 预构建包
- Stable Reads:
  - `Content/Data/stages.csv`、权威 XLSX 中的 Stage 配置及其读取器
  - `Content/ReEcho/Art/Scene/Map/**`、现有地图材质实例
  - `plans/42-*`、`plans/52-*`、`plans/84-*`、`plans/133-*`
  - `AReEchoArenaCameraActor`、Player/Enemy 对 Arena 边界和玩法平面的只读消费接口
- 影响模式：`Exclusive`，因为迁移 Level00 与四个场景 Blueprint 二进制资产，并改变 Arena 注册、地图配置和 Stage 切换公共契约。
- 兼容承诺 / 下游操作：`stages.csv` 继续只负责 `StageId→SceneId`；仍在持久 `/Game/Level00` 内切换，不进行 World Travel，不重置 Run、录制或存档语义。现有 SC01 插片的 Transform、缩放、旋转、显隐、材质和透明排序必须原样保留。
- 明确排除：不由程序生成、补齐、删除、定位或重排插片；不规定插片数量和名称；不修改地图源图；不替美术决定构图、色调或遮挡效果；不把 SceneId 写回平衡数据之外的第二套 Stage 配置；不在人工 PIE 验收前关闭 Plan。

## 锁定目标

以 `BP_ArenaScene_SC01` 为结构参考，将 `BP_ArenaScene_SC01..04` 建立为各自地图、插片、装饰、视觉层和玩法边界的完整 Editor 权威。美术在对应 BP 中直接选择 Backdrop 材质并手工放置所有插片/装饰；Construction、Python 工具和运行时不得覆盖这些表现配置。

运行时只从 Stage 配置取得 `SceneId`，再从单一 Catalog 解析目标 Arena Blueprint。场景切换先生成并验证候选，再原子重绑消费者和清理旧场景/局内对象；候选失败时保留旧场景及切换前局内状态。Level00 不再固定承载某个正式 SC 的地图或独立场景装饰。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Encounter / AREA-Presentation / AREA-Tests`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，已加入 `Writes`。
- 设计意图：消除 Stage CSV、每个 Arena BP 重复 Registry、SceneProfile MapMaterial、Actor MapMaterial、Backdrop 材质、Level00 实例覆盖和生成脚本之间的多重权威；让运行时只编排场景，让场景 BP 自己拥有可视内容。
- 权威状态与依赖：
  - Stage XLSX/CSV：唯一拥有 `StageId→SceneId`。
  - 单一 Arena Catalog：唯一拥有 `SceneId→Arena Blueprint Class`。
  - `BP_ArenaScene_SCxx`：唯一拥有 Backdrop 材质、视觉组件、插片/装饰、MapRoot 和可视化玩法边界。
  - GameMode：只拥有当前 Active Arena、切换事务和消费者重绑，不拥有地图路径或美术参数。
- 决策记录：
  1. Backdrop 组件 Material Slot 0 是地图材质权威；移除或停用会覆盖它的 `SceneProfile.MapMaterial`/Actor `MapMaterial` 路径。地图调色保存在 BP 指定的材质实例中。
  2. 插片和装饰是场景 BP 内的美术组件；程序脚本只允许导入源资产或只读验证，不再作者ing组件。
  3. 单一 Catalog 取代每个 SC Blueprint 中重复保存完整 SceneRegistry；运行时不扫描 `/Game/ReEcho/Scene/Prefabs` 目录，也不硬编码分散路径。
  4. CameraClamp、PlayerBounds、EnemySpawnBounds 的组件实际 Extent/Transform 成为玩法范围权威，逐步删除与组件重复的 HalfExtents 字段；若碰撞墙仍需自动布局，只能消费这些玩法组件，不能修改视觉层。
  5. Level00 只保留全局 Camera、GameMode 所需 Actor 和场景 Spawn Anchor；启动时直接按 Encounter 1 的 Stage 配置生成 SC01，不再先加载 SC02 再替换。
  6. 切换采用 prepare/commit：候选 BP 成功加载、SceneId 对应关系和必需组件验证通过后，才清理 Echo/敌人并替换 Active Arena。失败销毁候选且不改变旧场景。
  7. Plan52 的 Level00 独立 tagged decoration 不再作为生产场景内容；迁移前先列出并验证其是否已由美术在某个 BP 中接收，不能静默删除唯一作品。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md` 和 `modules/MOD-ReEcho.md`；预计模块拓扑不变，若索引不变则在执行记录中说明无需修改。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：更新场景 BP、Catalog、Level00 和事务式切换权威。
  - `ARCHITECTURE.md`：审阅持久 World 与场景编排不变量。
  - `README.md`：审阅场景阅读路由是否需要新增稳定标识。

## 锁定验收

- [ ] `BP_ArenaScene_SC01..04` 可在 Blueprint Editor 直接选择 Backdrop 材质，Construction/运行时不覆盖；保存、重开 Editor 后仍保持。
- [ ] SC01 现有全部插片保持原 Transform、缩放、旋转、显隐、材质和 `TranslucencySortPriority`；程序不再要求固定插片名称或数量。
- [ ] 美术可在四个 SC BP 的 GroundDetail、MidDecoration、Foreground、Atmosphere、SceneEffects 中直接增删和调整组件，无需运行 Python 或修改 C++。
- [ ] `stages.csv` 的 `SceneId` 经单一 Catalog 解析 SC01-SC04；不存在每个场景 BP 各存一份完整 Registry 的重复配置。
- [ ] Level00 不固定放置 SC02，也不保存属于单个场景的生产装饰；新 Run 直接生成 Stage.1/SC01。
- [ ] Encounter 2→3、5→6、7→8 分别切换 SC02、SC03、SC04；同 SceneId 不重建；存档恢复按保存 Encounter 的 Stage 应用正确场景。
- [ ] 切换成功后 Camera、Player Bounds、Enemy Spawn Bounds、Gameplay Plane 和存量敌人消费者全部重绑；旧 Arena 及其场景内容完整销毁。
- [ ] 目标 SceneId 未注册、BP 加载失败或配置无效时，旧 Arena、玩家、敌人和 Echo 保持切换前状态，下一 Encounter 不启动并输出具名诊断。
- [ ] Plan133 的 Stage1→Stage2 CG/下一 Encounter 编排在组合后保持其锁定顺序，不被旧 GameMode 流程覆盖。
- [ ] `.clang-format`、Editor Development 构建、场景/Stage 聚焦自动化、Editor 资产只读验证、项目校验、`git diff --check`、最终 FullRebuild 和预构建检查通过。
- [ ] 用户在 Blueprint Editor/PIE 确认地图可切换、SC01 插片未变化、四场场景内容正确、跨关切换无闪烁/残留后，人工验收才可设为 `Passed`。
- [ ] 未提交精选预构建允许列表之外的 UE 生成物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@41da5187ba694d2629b6e2c0a6bbb9f5b3fb6c54`；远端最大编号 Plan133，本 Plan 使用 Plan134。
- 引擎/构建可用性：规划前最新主线通过项目静态校验；执行阶段需确认 Unreal Editor 已保存并关闭后再运行构建或资产迁移。
- 现有聚焦测试结果：当前仅有历史 `ReEcho.StageTransition` 与 Plan84 Editor 验证记录；本 Plan 不复用旧结果证明新 Catalog、事务回滚或 BP 权威。
- 共享契约 / 难合并资源风险：Plan133 写 `ReEchoGameMode.*` 和 Stage1→Stage2 时序；当前审查 worktree 中 `BP_ArenaScene_SC01.uasset` 有未提交外部修改。Executor 启动时必须获取该资产的明确交接基线或保持只读，不能从 main 旧版覆盖。
- 基线损坏时的停止条件：SC01 当前美术修改来源无法确认；Level00 tagged decorations 是尚未迁入 BP 的唯一美术作品；Plan133 改变下一 Encounter 入口；现有 BP 组件无法无损迁移；Editor/资产锁被占用；需要改变 Stage 数据或产品构图。

## 实现提纲

1. 发布并核验 Plan134；Executor 读取最小规则、Plan、`MOD-ReEcho` 场景段、Arena/GameMode 公私接口和场景相关测试。
2. 审计最新 main、Plan133 候选与 SC01 外部修改，冻结四个 BP、Level00、GameMode 和场景工具的准确迁移输入；先生成只读资产清单和属性快照。
3. 新增单一 Arena Catalog 契约并调整 GameMode 初始化，使 Level00 可在没有正式 Arena 实例时根据下一 Encounter 直接生成目标 BP。
4. 将地图权威收敛到 BP Backdrop 材质，将玩法边界收敛到可视组件；删除 Construction 对美术表现的覆盖路径，同时保持旧 BP 迁移兼容直到资产完成转换。
5. 把场景切换拆为候选准备与提交；验证成功后才执行消费者重绑、旧场景销毁和关次清理，失败保持旧状态。
6. 使用 Unreal Editor 自动化进行一次性迁移：以 SC01 为结构参考更新 SC02-SC04，保留 SC01 全部插片属性；把确认属于各 SceneId 的 Level00 独立装饰迁入对应 BP，未确认归属的内容保留并停止删除。
7. 删除/降级程序化插片作者ing入口为只读验证；校验不再锁定插片数量/名称，只验证无碰撞、有效材质和组件属于场景 BP。
8. 增加纯策略、World 切换和 Editor 资产测试；更新模块文档和 Plan 执行记录，完成构建、静态检查及用户 Blueprint/PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan134 编号、结构、范围和中文正文通过 |
| C++ | `.clang-format`、`scripts\ue\Build-Editor.cmd -Configuration Development` | Catalog、Arena 和 GameMode 通过 UHT/UBT |
| 策略/World | `scripts\ue\Run-Automation.cmd -Filter ReEcho.StageTransition` 及场景聚焦过滤器 | 首次生成、同场景复用、跨场景提交、失败回滚、恢复路径通过 |
| Editor 资产 | 只读场景审计脚本 | 四个 BP 可加载；地图材质、必需层/边界、Catalog 映射和 SC01 属性快照一致 |
| Level00 | Editor 只读验证 | 无固定生产 Arena/独立 Scene 装饰；Camera 与 Spawn Anchor 唯一有效 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、数据和文档不变量通过 |
| 发布 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`、`python scripts/ue/prebuilt_editor.py check` | 最终组合候选和精选预构建包一致 |
| 人工 Blueprint/PIE | SC01 编辑保存、四场切换、跨关/读档/失败场景 | 用户确认地图切换、插片保留、场景无闪烁残留且构图正确 |

## 执行记录

### 变化

- 新增 `UReEchoArenaSceneCatalog` 与 `/Game/ReEcho/Scene/DA_ArenaSceneCatalog`，SC01-SC04 映射只保存一份；GameMode 不再从放置 Arena 的重复 Registry 初始化。
- GameMode 场景切换拆为 `PrepareArenaSceneForStage`/`ApplyArenaSceneForStage`：候选以隐藏、无碰撞状态生成并验证；`PrepareEncounterIntermission`、新 Encounter 与读档恢复均在清理 Combatant/Echo/Shard 前准备候选，准备失败保留原 Arena 与局内对象。
- Arena 范围查询改读三个 BoxComponent 的实际缩放 Extent，Backdrop Material Slot 0 成为唯一地图材质校验。SC01-SC04 已把旧 HalfExtents 烘入组件并启用 `bUseEditorAuthoredSceneLayout`，同时清空 SceneProfile、Actor MapMaterial 和重复 SceneRegistry；Construction 不再覆盖美术组件。
- Level00 删除固定 SC02 Arena，新增唯一无表现 `AReEchoArenaSceneSpawnAnchor`。权威 `origin/main` Level00 在迁移前实际已是 0 个 Plan52 tagged decoration；迁移器只接受 0 或预期旧值 18，拒绝任意其他数量，最终只读审计确认仍为 0。
- `author_plan84_scene_switch.py`、`author_plan52_decorations.py`、`build_plan52_scene_assets.py`、`migrate_plan52_level00.py` 的写入入口均退休；新增一次性 Plan134 迁移器、Catalog-only 作者ing脚本与不写资产的审计脚本，审计不锁定插片名称或数量。
- `ResumeSavedEncounter` 在 Arena prepare 成功后才调用 `ConsumePendingEncounterResume`，目标 Scene 缺失/无效时 pending resume 保留可重试。
- SC01 迁移严格使用本 Executor 从 `origin/main@b7e15000` 建立的资产；迁移前后内存快照一致，保留 8 个非核心模板（7 个具名插片及 1 个无 Mesh/Material 的空模板）。review 脏 worktree 始终未访问、未复制、未修改。
- 首次截图人工验收发现旧 `UpdateEditorLayout` 的 Backdrop Transform 未随 Bounds 一起烘入。修复器现按原公式写入四 BP：Location `(0,0,-0.5)`、Rotation `(0,90,0)`、Scale `(BackdropHalfExtents.Y*2/100, BackdropHalfExtents.X*2/100,1)`，保持 `bUseEditorAuthoredSceneLayout=true`，不恢复 Construction 覆盖。修复前后每个 BP 的非核心美术组件快照一致。
- `HasValidConfiguration` 与 Editor 只读审计新增真实覆盖契约：读取 Backdrop StaticMesh local bounds 和 CameraClamp Box local bounds，分别将八角经实际 Component World Transform 投影为 XY AABB，再以 1uu 数值容差验证 Backdrop 覆盖 CameraClamp；旋转轴向由真实 Transform 决定，不由 Scale 推测。
- 以当前 SC01 为非视觉程序契约基准，将 SC02-SC04 的 GameplayPlaneZ、GameplayRoot、三组 Bounds 模板、DepthSort、接触阴影和视差参数，以及 Floor/墙的 Z 高度与垂直厚度统一；不增加 SC02 或角色专用 offset。SC01 的 SceneRoot 视觉缩放为 1.6，而其余场景为 1.0，因此保留各场景 SceneRoot/MapRoot 的视觉 scale 与 Floor/墙 XY 布局，避免改变独立 Backdrop 尺寸和构图。
- `GetGameplayPlaneWorldZ` 改为只由 Arena Actor Transform 与 GameplayPlaneZ 计算，不再读取 MapRoot/SceneRoot 视觉 Transform；地图或视觉节点的 Z/scale 调整不会改变角色玩法高度，现有世界 XY Bounds 消费契约不变。

### 证据

- 远端规则/基线：`git fetch origin main` 后 Executor 与 `origin/main@b7e15000968b79f433368f0b69815588ad7ebe87` 一致，无额外外部提交。
- `.clang-format` 已执行；最终 `Build-Editor.cmd -Configuration Development -FullRebuild` 98/98 actions 通过，精选预构建包 `modules=7 build_id=55116800 source=4b7aef5986c1`，随后 `prebuilt_editor.py check` 通过。
- FullRebuild 后重跑 `ReEcho.StageTransition` 三项均为 `Success`：`PolicyMatrix`、Catalog `SceneRegistry`、`WorldContinuity`；`ReEcho.Presentation.ArenaScene.Contract` 为 `Success`。
- 完整 Editor 只读审计通过：Catalog 唯一解析 SC01-SC04；四 BP 的 Backdrop 材质、必需视觉层、组件 Bounds、Editor-authored 门、空 Profile/MapMaterial/Registry 及美术组件无碰撞均有效；Level00 为 0 Arena、1 SpawnAnchor、1 Camera、0 tagged decoration。
- 一次性迁移日志：`Migrated 4 Arena Blueprints, preserved 8 SC01 artist components, deleted 0 tagged decorations, and replaced the fixed Arena with one spawn anchor`。0 是当前权威 main 的实测基线，不伪造为删除 18。
- `python scripts/validate_project.py`、`git diff --check` 与全部相关场景 Python 脚本 AST 解析通过；未产生精选预构建允许列表之外的已跟踪生成物。
- Backdrop 修复后最终 FullRebuild 97/97 actions 通过，精选预构建包仍为 `modules=7 build_id=55116800 source=10a260fd4759` 且检查通过；FullRebuild 后重跑 Arena Contract 1/1、StageTransition 3/3 均为 `Success`。
- 修复脚本日志明确记录 `Repaired Backdrop transforms for SC01-SC04 without changing artist inserts`；独立复开 Editor-Cmd 的最终只读审计退出码 0，并确认四个实际 Backdrop mesh XY footprint 覆盖各自 CameraClampBounds，Level00 权威仍通过。
- SC01 核心契约统一后的最终 FullRebuild 97/97 actions 通过，精选预构建包 `modules=7 build_id=55116800 source=ef53ea35f774` 且检查通过；FullRebuild 后 Arena Contract 1/1、StageTransition 3/3 均为 `Success`。
- 最终 Editor 只读审计明确输出 `SC01=0.000`、`SC02=0.000`、`SC03=0.000`、`SC04=0.000` GameplayPlaneWorldZ，并逐项比较程序参数、Root pose、GameplayRoot、Bounds 和碰撞高度契约；完整 Catalog、Backdrop 覆盖、视觉层及 Level00 审计通过。迁移器的逐场视觉快照前后相等，SC01、Backdrop、插片和构图未写入本次资产差异。

### 剩余风险

- Plan133 与本 Plan 共享 `ReEchoGameMode.*` 和 Stage1→Stage2 进入下一 Encounter 的时序，最终证据必须来自组合后的候选。
- 静态/自动化不能证明美术构图与遮挡质量，四场景仍需人工 PIE 验收。
- Plan133 当前远端只有 Plan、没有可组合实现提交；本候选只对 `BeginNextEncounter` 做局部 prepare/commit 调整，未来组合其 CG 门时必须保留“CG 完成/失败→现有 BeginNextEncounter 2”顺序并重跑 Stage/Encounter 证据。
- 当前提交只供本地人工验收；用户明确要求确认正确前不得向任何远端提交或推送。

### 人工验收结果/请求

- `FailedThenPendingRecheck`：首次截图确认实际 Backdrop 仅显示为红框小矩形，定位为迁移只烘入 Bounds、遗漏旧 `UpdateEditorLayout` Backdrop Transform。技术修复与真实 bounds 审计已完成；仍需用户重新打开 Blueprint/PIE，确认四场地图尺寸、轴向、构图和切换表现。

### 架构文档审阅结果

- `MOD-ReEcho.md`：已更新单一 Catalog、prepare/commit、组件范围权威、Level00 SpawnAnchor 及已退休作者ing边界。
- `ARCHITECTURE.md`：已审阅；持久 World 与现有模块拓扑未改变，当前阶段无需修改。
- `README.md`：已审阅；`MOD-ReEcho / AREA-Encounter / AREA-Presentation / AREA-Tests` 路由未改变，当前阶段无需新增稳定标识。
