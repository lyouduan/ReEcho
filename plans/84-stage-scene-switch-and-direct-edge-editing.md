# Plan 84 - 程序 - CSV 关卡场景切换与 SC01 插片直接编辑返工

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：独立程序 Executor，待 Plan 发布后启动。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@4cbc5f99ae99daa15bca0e7f68849fd9823ab640`。
- 本地实现方式：独立 `codex/plan84-stage-scene-switch` worktree。
- 依赖 / 阻塞：复用 Plan83 未发布候选中的 7 张源 PNG、材质和作者ing经验，但不复用其 Child Actor 结构；生产 Stage 数据必须从权威 XLSX 同步 CSV。
- Writes:
  - `plans/83-sc01-editable-edge-inserts.md`
  - `plans/84-stage-scene-switch-and-direct-edge-editing.md`
  - `Design/Data/ReEchoEncounterData.xlsx` 中 `tblStages` 与同步生成的 `Content/Data/stages.csv`
  - `Content/Data/csv_schema.csv`（将 `Stages.SceneId` 允许值更新为 `SC01/SC02/SC03/SC04`）
  - `scripts/validate_project.py`（同步 SceneId 生产数据不变量）
  - `Source/ReEcho/Public/Presentation/Scene/ReEchoArenaSceneActor.h`
  - `Source/ReEcho/Private/Presentation/Scene/ReEchoArenaSceneActor.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoStageTransitionTests.cpp`
  - `Content/ReEcho/Art/Scene/SC01/EdgeInserts/`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC01.uasset`
  - 场景切换目录/注册资产（准确路径由 Executor 在既有 Scene 目录内确定并回写）
  - `Content/Level00.umap`（仅在稳定 Scene 注册无法由现有 Arena 默认值承载时使用；必须由 Editor 修改）
  - `scripts/ue/author_plan84_scene_switch.py`
  - `scripts/ue/verify_plan84_scene_switch.py`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - 最终精选 Win64 Editor 预构建包
- Stable Reads:
  - Plan83 隔离 worktree 的未发布源 PNG、作者ing与验证脚本
  - `Source/ReEcho/{Public,Private}/Encounter/ReEchoEncounterRuntime.*`
  - `Source/ReEcho/{Public,Private}/Data/ReEchoEncounterCsvReader.*`
  - `Content/ReEcho/Scene/{Profiles,Prefabs}/`
  - `plans/52-scene-map-blueprint-authoring.md`、`plans/54-stage-encounter-transition-continuity.md`
- 影响模式：`SharedContract`（Stage CSV 的 `SceneId` 从无消费者字段提升为运行时场景选择契约，并修改 Arena/GameMode 装配）。
- 兼容承诺 / 下游操作：仍只加载 `/Game/Level00`，场景切换发生在该 World 内；不得重置 Run、Encounter、录制或同 Stage 连续性，不得让视觉加载结果改变战斗结算。
- 明确排除：不创建四张新 `.umap`，不使用 `OpenLevel` 完成 Stage 场景切换，不合并七张插片，不保留不可在 SC01 Arena Blueprint 中直接编辑的 Child Actor 包装。

## 锁定目标

1. `Stage.1/Stage.2/Stage.3/Stage.Boss` 的权威 `SceneId` 分别为 `SC01/SC02/SC03/SC04`，由 XLSX 生成 CSV；运行时必须真实消费该字段。
2. 开局进入 Encounter.1 即显示 SC01；跨 Stage 时切换到下一 Scene；同 Stage Encounter 过渡不重建场景。
3. Scene 切换同步更新地图材质、场景专属装饰、Arena Bounds/Gameplay Plane 消费和 Camera Arena 引用；旧场景视觉/碰撞不得残留或叠加。
4. SC01 七张插片成为 `BP_ArenaScene_SC01` 内直接可选中的组件；美术无需打开子 Actor、运行脚本或修改 C++ 即可逐片编辑 Transform、显隐、材质和排序。
5. 切换失败必须阻止错误 Encounter 开始并输出具名 `StageId/SceneId`，不得静默沿用上一张图冒充成功。
6. SC01 地图底图在 Arena Blueprint 中提供独立宽高编辑入口，调整后实时更新 Backdrop 尺寸，不要求修改源纹理；不得连带改变玩家、相机或出生边界，除非美术明确编辑对应 Bounds。
7. 七张插片分别提供可编辑透明排序优先级，以确定底图、角色、中景和前景之间的遮挡关系；作者ing工具重复执行不得覆盖美术已调整的 Transform、尺寸、显隐或排序值。

## 架构影响与设计决策

- 受影响：`MOD-ReEcho` / `AREA-Data`、`AREA-Encounter`、`AREA-Presentation`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；已加入 Writes。
- 设计意图：Stage CSV 是场景选择权威，GameMode 只编排，Arena Scene Actor 承担应用注册场景资产并暴露一致 Bounds 的职责。
- 权威状态与依赖：`FReEchoCsvStageRow::SceneId` 成为稳定运行时输入；不新增 Runtime Module。视觉资产不得反向决定 Stage。
- 决策记录：
  1. 在同一 `Level00` 内切换 Arena Scene，不进行 World Travel，避免破坏 Run/录制/Actor 连续性。
  2. SceneId 到 Scene 资产必须经 Editor 可配置注册表/目录解析，不在 GameMode 堆散落硬编码路径；未知或重复 ID 明确失败。
  3. SC01 插片直接进入 `BP_ArenaScene_SC01` 组件树；删除首轮 `BP_SC01EdgeInserts` 包装，解决父 Blueprint 不可逐片编辑的问题。
  4. Stage 跨界时先完成场景切换，再解析入口并启动 Encounter；同 Stage 保留现有敌人/玩家位置策略。
  5. `csv_schema.csv` 与项目校验器必须共同接受且只接受已注册的 `SC01/SC02/SC03/SC04`；不得继续以旧 `Level00` 白名单造成生产数据与运行时契约分叉。
  6. 复用 Arena 已有 `BackdropHalfExtents`/自动布局作为底图宽高编辑权威，并在 SC01 Blueprint 暴露可见默认值；不把底图显示尺寸固化到纹理导入尺寸。
  7. 插片遮挡使用每个直接组件的 `TranslucencySortPriority`，而非组件创建顺序或文件名；作者ing只为新建/缺失组件写初始 Transform 与排序，已有组件的美术值必须保留。
- 文档同步：关闭前审阅 `ARCHITECTURE.md`、`README.md`；维护 `MOD-ReEcho.md`，逐项记录结果。

## 锁定验收

- [ ] XLSX 可见 Stage 行同步生成 `SC01/SC02/SC03/SC04`，`--check` 字节一致。
- [ ] 自动化证明初始、同 Stage、跨 Stage、Boss、未知 SceneId 与重复 Scene 注册行为。
- [ ] 实际 World 中始终只有一个激活且有效的 Arena 场景；切换后 GameMode/Camera/Bounds 使用新场景。
- [ ] `BP_ArenaScene_SC01` 中七张插片均为直接组件，可逐片编辑；不再通过 Child Actor 包装。
- [ ] `BP_ArenaScene_SC01` 可独立编辑底图宽高，Backdrop 随 Construction 更新且不会暗改 Player/Camera/Enemy Bounds。
- [ ] 七张插片可独立调整 `TranslucencySortPriority` 并改变遮挡关系；作者ing第二次执行后，人工修改过的 Transform、显隐和排序保持不变。
- [ ] 作者ing重复执行无重复组件；SC02–SC04 不获得 SC01 插片。
- [ ] C++ 格式化、FullRebuild、聚焦自动化、XLSX/CSV check、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 用户 PIE 验收 Encounter 1–8 的 SC01→SC02→SC03→SC04 切换、SC01 插片编辑与视觉构图。

## Step 0 门禁

- 基线：`origin/main@4cbc5f99`；Plan 发布后重新 fetch 核验。
- Editor/构建：执行前要求关闭 ReEcho Editor并使用 common-dir 锁。
- 难合并面：XLSX/CSV 发布单元、`ReEchoGameMode.cpp`、`BP_ArenaScene_SC01.uasset` 和可能的 `Level00.umap`；远端同路径变化即停止审计。
- 契约补充：Executor Step 0 发现 `Content/Data/csv_schema.csv` 与 `scripts/validate_project.py` 仍硬限制 `Stages.SceneId=Level00`；两者已在实现前补入 Writes，必须与 XLSX/CSV/Reader/运行时测试作为一个一致候选验证。
- 既有主工作区 `DA_ArenaScene_SC01.uasset` 未提交修改不属于本任务，不得拷入或覆盖。

## 实现提纲

1. 从 Plan83 候选选择性迁移源 PNG/材质与脚本逻辑，不迁移 Child Actor 资产。
2. 修改权威 XLSX 并同步 Stage CSV；建立 SceneId→Arena Scene 的可配置、唯一注册与失败契约。
3. 把场景应用接入初始 Encounter 和跨 Stage 路径，重绑定 Camera/Bounds；保留同 Stage 连续性。
4. 通过 Editor API 把七张插片直接作者ing到 SC01 Blueprint，新增只读验证。
5. 完成自动化、构建、数据、资产链和人工 PIE 门禁。

## 验证矩阵

| 层级 | 检查 | 预期 |
|---|---|---|
| 数据 | XLSX 同步 `--check`、Stage Reader | 四个 SceneId 准确且引用合法 |
| C++ | 格式化、`Build-Editor -FullRebuild` | UHT/UBT 及最终包通过 |
| 自动化 | `ReEcho.StageTransition` + 新 Scene 切换测试 | 初始/同 Stage/跨 Stage/失败边界通过 |
| 资产 | author 两次 + verify | 直接组件、唯一注册、SC01-only 通过 |
| 静态 | `validate_project.py`、prebuilt check、diff-check | 全部通过 |
| 人工 | PIE Encounter 1–8 与 Blueprint 编辑 | 用户确认 Passed |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

`PendingBeforeClose`。

### 架构文档审阅结果

