# Plan 90 - 程序 - 生产配置驱动的可编辑 VFX 测试场景

## 协调

- Planner 负责人：当前 ReEcho 程序侧 Planner。
- Executor 负责人：待 Plan 发布后分配独立 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@e3a8701a66b1abe664736993a6e6c8f798baac3e`。
- 本地实现方式：`codex/plan90-editable-vfx-test-scene` 独立 worktree；不复用 Plan89 worktree，不触碰主工作区或 Plan89 未发布候选。
- 依赖 / 阻塞：读取当前 `FReEchoCombatVfxCatalog`、`FReEchoElementReactionVfxCatalog`、Weapon Presentation Profile、正式 Gameplay Blueprint/挂点和倾斜正交相机契约；依赖美术用户完成测试场景操作性与视觉一致性验收。
- Writes:
  - `plans/90-editable-production-vfx-test-scene.md`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoVfxPreviewActor.*`
  - `Source/ReEcho/Private/Tests/ReEchoVfxPreviewTests.cpp`
  - `Content/ReEcho/Testing/VFX/L_VFXAuthoring.umap`
  - `Content/ReEcho/Testing/VFX/BP_VFXPreviewRig.uasset`
  - `Content/ReEcho/Testing/VFX/**` 中本 Plan 创建的测试专用标尺、材质或 Blueprint 资产
  - `scripts/ue/author_vfx_test_scene.py`
  - `scripts/ue/verify_vfx_test_scene.py`
  - `docs/VFX_TEST_SCENE.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终精选预构建包
- Stable Reads:
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoElementReactionVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoProjectileActor.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Scene/ReEchoArenaCameraActor.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Scene/ReEcho2DEditorPreviewActor.*`
  - `Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`
  - `Content/ReEcho/DataAsset/Weapon/Profiles/**`
  - `Content/ReEcho/Gameplay/CharacterPrefabs/**`
  - `Content/ReEcho/Gameplay/EnemyPrefabs/**`
  - `Content/VFX/**`、`Content/Mat/**`、`Content/00_Textures/**`、`Content/01_Textures/**`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
- 影响模式：`SharedContract`（`Isolated | ReadOnly | SharedContract | Exclusive`，仅说明集成影响，不是写锁）。
- 兼容承诺 / 下游操作：测试场景只读消费正式 Catalog、Weapon Profile、Gameplay Blueprint 空间参数和 Niagara 资产；测试专用状态、预览锚点与 Sandbox 参数不进入正式玩法、存档、录制、伤害、AI、关卡流程或生产资产真源。正式 `Level00` 和打包入口不引用测试地图。
- 明确排除：首版不建立统一 `UReEchoVfxPresentationProfile` 或批量迁移现有 Catalog；不把 Trigger Event、Attach Mode、Position Authority、Lifecycle Owner、Stop Conditions、Support Status 或伤害/碰撞开放给美术编辑；不提供一键无审计覆盖生产配置；不新增 Editor Utility Widget；不修改 Plan89 候选；不由测试场景生成真实攻击、伤害、AI、保存或回放；不在 Editor 外手改 `.uasset` / `.umap`。

## 锁定目标

1. 创建独立、可保存、可重复生成的 `/Game/ReEcho/Testing/VFX/L_VFXAuthoring`，美术可以在不进入完整遭遇流程的情况下检查任意 Niagara 和当前生产语义的方向、尺寸、位置、挂点、排序、Bounds、Local Space、循环与清理表现。
2. 提供 `BP_VFXPreviewRig`：通过 Details 面板选择 `RawNiagara | CombatSemantic | ElementSemantic | WeaponProfile` 模式，编辑 Source/Target、预览方向、Sandbox Offset/Rotation/Scale、排序测试值、Projectile 速度/距离和具名 User Parameters，并用 `CallInEditor` 执行 Play、Stop、Restart、Reset、FaceTarget、Swap、Four/EightDirections、ProjectilePreview 与 Clear。
3. Production 模式必须读取现有正式 Catalog、Weapon Profile、Niagara 资产和正式宿主空间参数，不保存第二套生产映射。只读展示 Trigger、Attach、Position Authority、Lifecycle、Stop Conditions 和 Support Status；Sandbox 模式醒目标记“不应用到正式游戏”。
4. 场景包含固定正式倾斜正交相机、中性深浅背景、100 cm 网格/人物高度标尺、Source/Target/Forward 标记、前景/角色/背景遮挡参照，以及 Player、Echo、Slime、Rabbit、Fox、Sheep 的无 AI 表现参考位。
5. 对安全校准参数输出可审计报告：语义、资产、当前 authored 轴、建议 Offset/Rotation/Scale、Local Space/Bounds/Renderer/排序验证及生产落点。首版不自动写回 C++ Catalog 或生产 Profile；需应用时由对应生产资产/后续 Calibration Plan 审阅执行。
6. 测试场景与正式运行时共享现有路径解析和方向函数；自动化证明 Production 模式解析结果与生产 Catalog/Weapon Profile 一致。用户仍需在 `Level00` 通过真实武器/怪物事件完成最终游戏内视觉确认。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEchoVFX` / `AREA-Presentation`：新增测试专用 Preview Harness，但不改变运行时 VFX 权威、Catalog 或生命周期。
  - `MOD-ReEcho` / `AREA-Presentation`：新增测试地图/Actor 装配和生产宿主只读预览边界。
  - `AREA-Tests`：增加生产解析一致性、测试场景资产和无玩法副作用验证。
- 对应模块文档：维护 `MOD-ReEchoVFX.md` 与 `MOD-ReEcho.md`，均已加入 `Writes`；关闭前审阅 `MOD-ReEchoPresentation.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md` 与 `MOD-ReEchoWeapons.md`，公共事实未变化时记录无需修改。
- 设计意图：让美术快速观察真实生产配置，同时保持玩法事件、载体和清理权威由现有运行时组件拥有。测试场景是可视化与校准工具，不是第二套 VFX 系统。
- 权威状态与依赖：
  - Niagara 资源内容仍由对应生产 `.uasset` 拥有。
  - Combat/Element/Weapon 的生产映射仍由现有 Catalog/Profile 拥有。
  - Attack/Hurt Root、CharacterScale、Foot/Ground/Weapon Anchor 仍由正式 Gameplay Blueprint/Presentation 契约拥有。
  - Trigger、Attach、Position、Lifecycle 和 Cleanup 仍由 Combat VFX Component、Projectile Actor 和 Presentation Coordinator 拥有。
  - Preview Actor 只拥有测试地图内可丢弃实例、锚点和 Sandbox 参数。
- 决策记录：
  1. 不先迁移全部 VFX 到统一 DataAsset；原因是测试工具不应顺带重构人物、怪物、元素、武器和投射物的生产入口。
  2. 共享解析与方向计算，不统一吞并全部 Spawn/Cleanup；Rabbit Projectile、Bow Travel、Fox Dash、Burn 和 Hurt 保留各自运行时生命周期所有者。
  3. 首版 Production 模式只读，Sandbox 可自由试验并输出报告；不提供无审计 Apply，以避免临时参数静默覆盖二进制生产资产。
  4. 可编辑字段限于表现校准；程序权威字段以只读诊断显示，防止美术修改触发和玩法语义。
  5. Details + `CallInEditor` 是唯一首版交互入口；Editor Utility Widget 只有在美术验收证明必要后才另立任务，避免两套 UI 漂移。
  6. 参考宿主使用正式 Gameplay Blueprint/现有 Editor Preview 契约，只关闭 AI、碰撞伤害和流程推进；不复制角色空间常量。
- 相关文档同步范围：关闭前审阅 `ARCHITECTURE.md` 与 `README.md`，预期不改变 Runtime Module 拓扑或 AREA 路由；更新 `MOD-ReEchoVFX.md`、`MOD-ReEcho.md`；审阅 Presentation/Combat/Enemies/Weapons 模块文档。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待审阅。
  - `shared/CODEBASE_MAP/README.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：待维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`：待审阅。

## 锁定验收

- [ ] `L_VFXAuthoring` 可通过标准 Unreal Content Browser 打开，地图不依赖 GameMode/Encounter 启动，不被 `Level00` 或生产地图引用，也不进入默认地图/打包入口。
- [ ] `BP_VFXPreviewRig` 的 Production/Sandbox 状态在 Details 中清晰可辨；Sandbox 参数不会修改正式 Catalog、Weapon Profile、Gameplay Blueprint 或 Niagara 资产。
- [ ] Raw Niagara、Combat、Element、Weapon 四类选择均可加载/播放；缺失/不支持语义具名显示 `Missing`，不借用错误角色/武器特效。
- [ ] Source、Target、Forward、网格、标尺和遮挡参照可在 Viewport 编辑；FaceTarget、Swap、四/八方向、Projectile Preview 能重复执行且不会累积旧实例。
- [ ] Player/Echo/Slime/Rabbit/Fox/Sheep 参考位读取正式 Blueprint 的整体 Scale 与 Attack/Hurt/Foot/Ground/Weapon Anchor 空间结果，无 AI、伤害、存档、录制或流程副作用。
- [ ] Production 模式对每个已支持语义解析出的资产路径、authored 轴、挂接/位置权威说明和关键校准值与现有生产 Catalog/Weapon Profile 一致。
- [ ] 有方向语义显示全部启用发射器的 Local Space 状态；显示 Renderer 数量、Bounds、当前/建议轴和旋转后方向。测试结果不能通过修改玩法方向补偿资产。
- [ ] Play/Stop/Restart/Clear 后一次性、持续、附着和 Projectile 预览没有残留组件；关闭地图或删除 Rig 不留下世界对象。
- [ ] 校准报告具名记录生产语义、资产、建议参数、验证和实际生产落点，并明确“未应用”；没有无审计自动覆盖入口。
- [ ] Editor author/verify 脚本可重复执行；第二次不会覆盖美术已手调的测试地图布局或生产资产。
- [ ] 聚焦自动化、地图/资产验证、FullRebuild、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 美术用户完成操作性验收，并在测试地图与 `Level00` 真实入口对同一组选定语义做视觉对照，确认测试结果能指导正式游戏调整。
- [ ] 未提交测试截图、日志、缓存、临时报告或精选预构建允许列表之外的生成产物。

## Step 0 门禁

- 基线分支/提交：`codex/plan90-editable-vfx-test-scene`，批准基线 `origin/main@e3a8701a66b1abe664736993a6e6c8f798baac3e`。
- 引擎/构建可用性：UE 5.8 安装版；创建/保存地图、Blueprint、材质和资产验证必须使用 Unreal Editor/API，并遵循 Git-common-dir Unreal 锁。运行需要关闭 Editor 的命令前请用户保存关闭。
- 现有聚焦测试结果：待 Executor 在准确基线上运行 `ReEcho.Presentation.VFX`、RuntimeAssetPreload 和 Arena Scene 基线；Plan90 不复用 Plan89 未发布候选的构建或人工证据。
- 共享契约 / 难合并资源风险：`.umap`、`.uasset` 为二进制独占写入面；作者脚本必须具名创建测试目录资产、保存后重载验证并列出准确路径。测试地图不得引用主工作区未提交资源或 Plan89 未发布 Bow/Gun 候选。
- 基线损坏时的停止条件：生产 Catalog/Weapon Profile 无法在 Editor 构造时稳定读取、正式 Gameplay Blueprint 无法无副作用预览、测试地图进入生产引用链、Editor API 无法幂等保存/重载，或实现需要迁移生产 Catalog/生命周期契约时，停止越界部分并由 Planner 更新 Plan 或拆出后续 Calibration Plan。

## 实现提纲

1. 建立测试专用 Preview Actor 的纯值配置、模式、方向/Transform 解析、实例清理和只读生产描述，不生成 Combat/Enemy/Weapon 玩法事件。
2. 增加聚焦 C++ 自动化，锁定 Production 解析与现有 Catalog/Weapon Profile 一致，Sandbox 不写生产资产，方向、去重、清理和无玩法副作用成立。
3. 通过幂等 UE Editor 脚本创建 `BP_VFXPreviewRig`、中性背景/标尺/遮挡参照和 `L_VFXAuthoring`；只初始化缺失资产，后续不覆盖美术布局。
4. 接入正式倾斜正交相机和正式 Gameplay Blueprint 只读参考位；关闭 AI、碰撞伤害和流程推进，验证挂点/比例来自生产配置。
5. 提供校准报告与操作文档，明确 Production/Sandbox、可编辑/只读字段、如何在 `Level00` 真实入口复验及如何提交具名生产修改。
6. 运行地图/资产验证、聚焦自动化和最终 FullRebuild；由美术完成测试场景操作性与正式游戏对照验收。
7. Planner 审阅实际 diff、生产引用边界、架构文档和二进制路径；满足人工门禁后才关闭和发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| C++ 格式/静态 | 仓库 `.clang-format`、`python scripts/validate_project.py`、`git diff --check` | Preview 不依赖玩法写入口；项目与文本边界通过 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.VFXPreview`，并回归 VFX/RuntimeAssetPreload/Arena Scene | 生产解析一致、Sandbox 隔离、方向/清理/无副作用和相邻契约通过 |
| Editor 作者ing | `scripts/ue/author_vfx_test_scene.py` 幂等执行两次 | 创建/更新准确测试资产，第二次保留人工布局，不修改生产资产 |
| Editor 验证 | `scripts/ue/verify_vfx_test_scene.py` | 地图、Blueprint、父类、组件、相机、锚点、参考宿主与生产引用隔离正确 |
| 最终构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UE 5.8 UHT/UBT 成功并刷新准确精选预构建包 |
| 预构建 | `python scripts/ue/prebuilt_editor.py check` | manifest、DLL 与最终源码指纹一致 |
| 人工测试地图 | 美术执行 Raw/Production、方向、Projectile、挂点、排序、Bounds、清理和校准报告流程 | 操作清晰、无残留、结果可复现 |
| 人工正式游戏 | 在 `Level00` 用真实武器/怪物事件复验选定语义 | 与测试场景 Production 模式的方向、尺寸、位置和排序一致；差异具名记录 |

## 执行记录

### 变化

- 2026-08-24：用户确认收敛方案。首版只建设读取真实生产配置的测试地图/Preview Rig，不批量迁移 Catalog，不开放生命周期/玩法权威，不提供无审计 Apply。

### 证据

- 2026-08-24：`git fetch origin` 后确认远端最大编号为 Plan89，最新基线 `e3a8701a` 已关闭 Plan71；创建 Plan90 独立 worktree。
- 2026-08-24：架构路线确认 VFX 生产映射位于 Combat/Element Catalog 与 Weapon Presentation Profile，正式挂点位于 Player/Echo/Enemy Gameplay Blueprint，倾斜正交相机和 Editor Preview 已有稳定入口；测试场景应只读组合这些权威，不复制生产参数。

### 剩余风险

- 测试地图可验证资产、空间和显示契约，不能单独证明真实 gameplay event 时序；关闭前仍需 `Level00` 真实入口对照。
- 若实际使用证明 authored 轴/偏移/缩放需要美术高频写回，应另建窄范围 Calibration Profile Plan，不能在本 Plan 暗中扩大为全 VFX 架构迁移。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现完成后由美术用户验收测试场景操作流程和正式游戏对照结果。

### 架构文档审阅结果

- 待实现与 Planner 关闭评审时逐项填写。
