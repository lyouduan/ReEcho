# Plan 84 - 程序 - CSV 关卡场景切换与 SC01 插片直接编辑返工

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：独立程序 Executor `plan84_executor`。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`OpenAI Codex`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@4cbc5f99ae99daa15bca0e7f68849fd9823ab640`。
- 本地实现方式：独立 `codex/plan84-stage-scene-switch` worktree。
- 依赖 / 阻塞：复用 Plan83 未发布候选中的 7 张源 PNG、材质和作者ing经验，但不复用其 Child Actor 结构；生产 Stage 数据必须从权威 XLSX 同步 CSV。
- Writes:
  - `plans/83-sc01-editable-edge-inserts.md`
  - `plans/84-stage-scene-switch-and-direct-edge-editing.md`
  - `Design/Data/ReEchoEncounterData.xlsx` 的 `tblStages` 与同步生成的 `Content/Data/stages.csv`
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
7. 七张插片分别提供可编辑透明排序优先级；默认 Backdrop 最低、角色/怪物在其上、上方和左右 Mid 插片高于角色、下方 Foreground 最高。蓝框只作为战斗区参考，不裁切插片。
8. 作者ing工具重复执行不得覆盖美术已调整的 Transform、缩放、旋转、显隐、材质或 `TranslucencySortPriority`；只为新建/缺失组件写入默认值。
9. 四个 Scene Arena Blueprint 默认关闭 `bAutoLayoutCollision`；关闭前保留既有 Floor/Wall Transform，之后 Construction 与 author 重跑不得把美术移动过的墙体回弹，也不得改动三组 Bounds。
10. SC01-SC04 的 Backdrop 组件模板直接绑定各自 SceneProfile 的 `MI_SC01-04`，使 Blueprint Editor 可预览正确底图；运行时仍由 `ApplySceneProfile` 创建动态材质并应用参数。Floor 不承担底图显示。
11. 相机在地图边缘进入视野时按轴锁定，角色仍可继续移动至 Player Bounds；相机 Actor 暴露左、右、下、上四个独立边缘内缩参数，且不得用 Player Bounds 代替 Camera Clamp Bounds。

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
  7. 插片遮挡使用每个直接组件的 `TranslucencySortPriority`，而非组件创建顺序或文件名；初始 Mid 沿用 `55-59`、Foreground 沿用 `90+`，高于角色深度排序上界 `50`。
  8. 作者ing仅初始化新增组件；既有组件只继续强制无碰撞/无阴影安全契约，不重写任何美术布局、显隐、材质或排序字段。
  9. Floor/Wall 作者ing从自动布局切为四 Scene Blueprint 模板权威；关闭 `bAutoLayoutCollision` 时只切换开关，不重新计算或覆盖现有 Transform。
  10. Backdrop 模板材质与 SceneProfile `MapMaterial` 保持同源，解决 Blueprint Editor 预览；运行时动态参数化路径保持不变，不把视觉材质转移到 Floor。
  11. 地图 Blueprint 的 `CameraClampHalfExtents` 提供基础镜头安全区；共享相机的四边 inset 只作方向独立微调。镜头按当前正交视锥在玩法平面的 footprint 扣除可移动范围，玩家仍由独立 `PlayerHalfExtents` 与墙体限制。
- 文档同步：关闭前审阅 `ARCHITECTURE.md`、`README.md`；维护 `MOD-ReEcho.md`，逐项记录结果。

## 锁定验收

- [ ] XLSX 可见 Stage 行同步生成 `SC01/SC02/SC03/SC04`，`--check` 字节一致。
- [ ] 自动化证明初始、同 Stage、跨 Stage、Boss、未知 SceneId 与重复 Scene 注册行为。
- [ ] 实际 World 中始终只有一个激活且有效的 Arena 场景；切换后 GameMode/Camera/Bounds 使用新场景。
- [ ] `BP_ArenaScene_SC01` 中七张插片均为直接组件，可逐片编辑；不再通过 Child Actor 包装。
- [ ] `BP_ArenaScene_SC01` 可独立编辑底图宽高，Backdrop 随 Construction 更新且不会暗改 Player/Camera/Enemy Bounds。
- [ ] 默认排序满足 Backdrop < 角色/怪物 < Mid 边缘插片 < Foreground；每片排序仍可独立编辑。
- [ ] 对已有插片写入非默认 Transform、缩放、旋转、显隐、材质与排序后重跑 author，全部值保持；空白 Arena 缺失的插片组件会按默认值补建。
- [ ] SC01-SC04 默认关闭碰撞自动布局；手动移动 Wall 后 Construction/author 不回弹，Floor/其余 Wall Transform 和 Camera/Player/Enemy Bounds 不变。
- [ ] 四场景 `SceneProfile → MI_SC01-04 → Texture` 引用链正确，Backdrop 模板材质与 Profile 一致，Floor 不使用地图材质。
- [ ] 相机左、右、下、上四个限制参数可在相机 Details 独立配置；四边和四角锁定后不露出地图安全区外内容，角色仍可继续移动到 Player Bounds，跨 Scene 后立即消费新 Arena 的 Camera Clamp Bounds。
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

- 权威 Encounter XLSX 与生成 CSV 已把四个 Stage 映射为 `SC01/SC02/SC03/SC04`；Schema 和项目校验器同步只接受这四个注册 ID。
- `AReEchoArenaSceneActor` 新增 Editor 可配置且唯一的 SceneId→Arena Blueprint 注册契约；`AReEchoGameMode` 在初始 Encounter、跨 Stage 和保存恢复入口先切换 Arena，再启动 Encounter，同 Stage 不重建。
- Arena 替换后统一重绑 Player Bounds、Camera Arena、Enemy Spawn Bounds 尺寸与存量 Enemy Gameplay Plane；未知 SceneId、重复注册、错误 Blueprint SceneId 或无效 Arena 均显式失败并阻止 Encounter 开始。
- 7 张源 PNG 原样导入；七个稳定命名的 StaticMeshComponent 直接作者ing到 `BP_ArenaScene_SC01`，不创建/引用 `BP_SC01EdgeInserts` Child Actor，SC02-SC04 无插片组件。
- SC01 继续使用独立 `BackdropHalfExtents` 控制底图宽高；作者ing改为只初始化新增插片，既有插片的 Transform、缩放、旋转、显隐、材质和透明排序全部保留。
- SC01-SC04 Blueprint 默认关闭 `bAutoLayoutCollision`；关闭前保留既有 Floor/四墙 Transform，后续 construction/author 重跑不再把美术手调墙体回弹，也不联动 Camera/Player/Enemy Bounds。
- 四个 Backdrop 组件模板直接绑定各自 `SceneProfile.MapMaterial`（`MI_SC01..04`），因此 Blueprint Editor 可直接预览；运行时仍由 `ApplySceneProfile` 建立动态材质参数，Floor 不承载底图材质。
- Arena Camera 默认启用边缘 Clamp，基础范围改为读取当前场景的 `CameraClampHalfExtents`，不再错误复用 `PlayerHalfExtents`；相机 Details 新增 `LeftEdgeInset`、`RightEdgeInset`、`BottomEdgeInset`、`TopEdgeInset` 四个独立参数，单边到界时只锁定对应轴。
- 默认遮挡顺序经资产验证固定为 Backdrop `-100` < 角色/怪物上界 `50` < Mid `55-59` < Foreground `90-93`；每片 priority 仍可由美术单独编辑。
- 新增幂等作者ing与只读资产校验脚本；维护 `MOD-ReEcho` 的 Scene 权威状态和运行流程。

### 证据

- `Build-Editor.cmd -Configuration Development -FullRebuild`：96/96，成功；精选 Editor 包 7 模块刷新并通过 `prebuilt_editor.py check`，最终 source fingerprint `b61e1b3a15c5`。
- 相机锁边增量 `Build-Editor.cmd -Configuration Development -FullRebuild`：97/97，成功；精选 Editor 包 7 模块刷新，source fingerprint `7cf38befe4dc`。
- `Run-Automation.cmd -Filter ReEcho.Presentation.ArenaScene.Contract`：发现 1 项并 Success；覆盖原有中心、四边、四角、小地图居中，以及新增非对称四边 inset。
- `Run-Automation.cmd -Filter ReEcho.StageTransition`：聚焦发现 3 项，`PolicyMatrix`、`SceneRegistry`、`WorldContinuity` 全部 Success。
- Editor 作者ing连续执行两次；`verify_plan84_scene_switch.py` 通过直接组件、唯一注册、SC01-only 和无 Child Actor 契约。
- 隔离作者ing探针从空白 Arena Blueprint 补建全部 7 个缺失组件；随后把现有 TopLeaves 写成非默认位置、旋转、缩放、隐藏、替代材质和 priority `777`，第二次 author 后逐项保持；临时资产已删除。
- 同一隔离探针修改 `BackdropHalfExtents` 后，Camera/Player/Enemy 三组 Bounds 均保持原值；SC01 `bAutoLayoutBackdrop` 保持启用。
- 四个 Scene Blueprint 均验证 `bAutoLayoutCollision=false`；隔离探针把 `WallNorth` 移到非默认位置后执行 construction/author，墙体 Transform 保持且 Camera/Player/Enemy Bounds 不变。
- 四条 `SceneProfile -> MI_SC01..04 -> sc01..04 Texture2D -> Backdrop template` 链一致，Floor 均未错误绑定视觉底图材质；运行时动态参数路径保持不变。
- 新建组件默认排序探针通过 Backdrop `<` 角色/怪物上界 `50` `<` Mid `55-59` `<` Foreground `90-93`。正式 SC01 中既有 TopLeaves priority `86` 属于保留的美术调整值，author 未覆盖；其最终视觉关系留给 PIE 人工验收。
- `sync_xlsx_to_csv.py --check`、`validate_project.py`、三个 Python 文件语法检查与 `git diff --check` 通过。
- 初次误以位置参数调用 Automation 时实际运行了全套 ReEcho 基线；Plan84 三项先通过，随后无关既有 `ReEcho.Traits.CsvEffectsApply` 数据期望失败并在 `ReEchoWeaponRuntimeTests.cpp:178` 断言退出。改用 `-Filter` 后聚焦套件干净通过，不把该无关基线记为 Plan84 回归。

### 剩余风险

- 尚无用户 PIE 视觉证据；需要人工跑 Encounter 1-8 确认 SC01→SC02→SC03→SC04、四边构图、中央可读性、默认遮挡顺序和切换瞬间无残留，并在 Blueprint Details 手动确认底图宽高与逐片排序编辑体验。
- 已在 `origin/main@d5f843a5` 上完成组合适配：保留远端 Plan68 敌人/Boss 数据与重开进入 Loadout 流程，仅在远端权威 `ReEchoEncounterData.xlsx` 的 `tblStages.SceneId` 四行应用 `SC01/SC02/SC03/SC04`；重开后的新 Run 仍经 `BeginNextEncounter` 解析 Stage.1 并切换 SC01。
- 最终组合候选 FullRebuild 为 92/92，精选 Editor 包 fingerprint `2ede4d73179a`；`ReEcho.StageTransition` 3/3、`ReEcho.Presentation.ArenaScene.Contract` 1/1 Success，XLSX 全包同步、项目校验、预构建检查与 diff-check 通过。
- `ARCHITECTURE.md` 已审阅：仍为单 Runtime Module、单 World 内表现 Actor 替换，无拓扑/模块依赖变化，无需修改。
- `CODEBASE_MAP/README.md` 已审阅：既有 AREA-Data/Encounter 路由仍准确，无稳定标识/路由变化，无需修改。

### 人工验收结果/请求

`PendingBeforeClose`。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：增加 Stage CSV SceneId/Arena 注册权威状态、初始/恢复/跨 Stage 应用与消费者重绑流程。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：模块拓扑和跨模块依赖未变化。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：受影响区域仍由现有 `AREA-Data`、`AREA-Encounter`、`AREA-Presentation` 路由覆盖。
