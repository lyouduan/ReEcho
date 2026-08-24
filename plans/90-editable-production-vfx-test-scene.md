# Plan 90 - 程序 - 生产配置驱动的可编辑 VFX 测试场景

## 协调

- Planner 负责人：当前 ReEcho 程序侧 Planner。
- Executor 负责人：待 Plan 发布后分配独立 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Codex executor-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
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
- 2026-08-24：Executor 在 Plan90 独立 worktree 开始实现。新增 Editor-only `AReEchoVfxPreviewActor`、纯值解析测试、幂等 author/只读 verify 脚本和操作文档；Production 模式直接调用既有 Combat/Element Catalog 与 Weapon Profile，Sandbox 只拥有测试组件 Transform，不发布任何玩法事件。
- 2026-08-24：用户批准第二轮方案：保留单特效入口，但主工作流升级为可编辑多目标 Scenario，覆盖 SingleHit、Projectile、Conduct2Targets、ConductChain、RadiusBoundary、BurnState、Vaporize、CrowdStress。Conduct 只消费正式 `FReEchoElementReactionResolvedEvent.ReactionLinks`，由正式 `UReEchoCombatVfxComponent` 生成链路；测试工具禁止发现、排序或重算传导图。Production Simulation 只读，Visual Calibration 全部标记 `NOT APPLIED`，Pause/Step 只控制 Editor 预览时间线。
- 2026-08-24：用户扩大正式表现范围：Weapon Presentation Profile 增加 Visual-only Conduct Link 延迟，AttackIdentity 快照稳定 WeaponId；伤害与 ReactionLinks 保持立即完成，VFX adapter 仅按权威 BFS 顺序延迟播放并在实际播放时重新读取目标位置/长度/方向。测试 Rig 显示正式解析值；Calibration override 明确 `NOT APPLIED`。

### 证据

- 2026-08-24：`git fetch origin` 后确认远端最大编号为 Plan89，最新基线 `e3a8701a` 已关闭 Plan71；创建 Plan90 独立 worktree。
- 2026-08-24：架构路线确认 VFX 生产映射位于 Combat/Element Catalog 与 Weapon Presentation Profile，正式挂点位于 Player/Echo/Enemy Gameplay Blueprint，倾斜正交相机和 Editor Preview 已有稳定入口；测试场景应只读组合这些权威，不复制生产参数。
- 2026-08-24：Step0 确认专属 worktree `codex/plan90-editable-vfx-test-scene` 工作树干净，`HEAD == origin/main == 860ae397`；生产 Catalog/Profile/Gameplay Blueprint 均保持只读，未采用 Plan89 未发布候选。
- 2026-08-24：用户确认 Editor 已保存关闭后，首轮 `Build-Editor.cmd -Configuration Development -FullRebuild` 成功（95 actions），精选包刷新为 build id `55116800`、source fingerprint `06d01b7f1d4f`。随后 `ReEcho.Presentation.VFXPreview` 过滤同时匹配生产 `ReEcho.Presentation.VFX.Catalog` 与新 `VFXPreview.Values`：Preview 纯值/生产路径一致性测试通过，但生产 VFX Catalog 基线在 `ReEchoCombatVfxTests.cpp:118` 因一个空资产路径失败（日志为 `Failed to find object 'NiagaraSystem '` / `Niagara system loads: `）。该失败与 Plan89 已定位但尚未发布的 Gun Profile authored slot 一致；Plan90 不采用或修改该生产资产。按失败门禁停止，未运行 author 脚本、未创建测试 `.uasset/.umap`、未继续相邻自动化或最终门禁，共享锁已释放。
- 2026-08-24：Plan89 prerequisite slice 发布后，将 Plan90 静态候选安全重放到 `origin/main@8c805aec`；没有恢复旧预构建包，也没有覆盖最新 `MOD-ReEcho` / `MOD-ReEchoVFX`，仅在最新文档上重新应用 Preview Harness 说明。首轮最终组合 FullRebuild 成功（95 actions），精选包 source fingerprint `4e1d118603f5`；VFXPreview Values、生产 VFX Catalog、RuntimeAssetPreload Catalog/Completion 与 Arena Scene Contract 全部通过。
- 2026-08-24：author 脚本首次运行暴露 UE Python 不提供 `rerun_construction_scripts()`，第二次暴露正式 Enemy Blueprint 位于 `CharacterPrefabs` 而非 `EnemyPrefabs`；两次都只产生/修改 `Content/ReEcho/Testing/VFX/**`，未触碰生产资产。脚本修正为按 Actor Label 只补齐缺失对象，使用准确生产 Blueprint 类路径；随后成功创建单一 Preview Rig、六个无玩法 Editor Preview Host、正式 Arena Camera 参考和具名 Source/Target/Forward/标尺/遮挡参照。
- 2026-08-24：第二次成功 author 运行没有保存地图，`L_VFXAuthoring.umap` 前后 SHA-256 均为 `2D792058F921DAD5D420B65D61962C12887B2BA1621EE6A00AC815713F89689A`，证明幂等脚本保留既有布局。只读 verify 通过 Blueprint 原生父类、单一 Rig、六参考宿主、单一正式相机、Sandbox 警告和测试地图生产引用隔离；脚本用 `python -B` + AST 验证，未生成 pyc。
- 2026-08-24：最终 closed-editor FullRebuild 在 `origin/main@8c805aec` 组合候选上成功（95/95 actions），精选包为 build id `55116800`、source fingerprint `4e1d118603f5`。随后使用准确 `-Filter` 分别运行 `ReEcho.Presentation.VFXPreview`、`ReEcho.Presentation.VFX`、`ReEcho.Presentation.RuntimeAssetPreload`、`ReEcho.Presentation.ArenaScene`，四组均退出 0。期间一次遗漏 `-Filter` 的执行误启动全套 ReEcho 测试，暴露既有 Weapons 测试失败和测试辅助断言；该结果不属于 Plan90 聚焦测试，已保留日志且没有归因给本候选。
- 2026-08-24：最终 author 连续执行两次，`BP_VFXPreviewRig.uasset` SHA-256 始终为 `3763DA8854E0196ADB462E1F7847ECAE17FDB5ED71532C4B5BE827EA181CBB58`，`L_VFXAuthoring.umap` 始终为 `2D792058F921DAD5D420B65D61962C12887B2BA1621EE6A00AC815713F89689A`；verify 再次通过。资产状态审计只有这两个 `Content/ReEcho/Testing/VFX/**` 测试资产，生产 `.uasset/.umap` 变化为 0。
- 2026-08-24：`python -B` AST、`validate_project.py`、`prebuilt_editor.py check`、工作树与暂存区 `git diff --check` 全部通过。`validate_project.py` 首次受沙箱限制无法在 `Content` 创建临时 XLSX 同步目录，使用已授权权限重跑后完整通过；最终无 Unreal 进程和共享锁残留。
- 2026-08-24：Planner 复核后撤销“Authored Links 可作为 Production Event”的错误适配。Authored Links 降级为黄色 Visual Calibration / `NOT APPLIED`；Production Conduct 改为明确 PIE-only，生成真实 Enemy/Combatant，先由正式 `ApplyHitToWorld` 附着 Water，再对 Primary 应用 Lightning，由正式 CSV/位置/去重/发现顺序产生 Event 与 ReactionLinks，敌人既有 CombatVfxComponent 负责 SpawnConductLink。CallInEditor 缺少 gameplay world 与初始化 Combatant，不能伪装成 Production。范围圆和方向箭头按帧读取正式 Event/目标位置；沙盒链路绝不写入 `ResolvedReactionEvent`。
- 2026-08-24：架构修正版 FullRebuild 成功（95/95 actions，source fingerprint `c3949f8da659`）；`ReEcho.Presentation.VFXPreview`、`ReEcho.Presentation.VFX`、`ReEcho.Combat.ElementReaction` 均通过。正式 ElementReaction 自动化覆盖位置/CSV 半径改变目标集合、稳定发现顺序、去重和 bridge edge；Preview 自动化另锁定 Authored Links 在 Editor Run 后不会进入 Production Event。author 幂等复跑与多目标 verify 再次通过，生产资产变化为 0。PIE 中实际青色范围/箭头和 Conduct Niagara 仍列为人工验收。
- 2026-08-24：按用户反馈增加 PIE 主动释放和测试相机。PIE 默认只初始化真实 Combatant 与 `Ready` 状态，`bAutoReleaseOnBeginPlay=false`；视口 Slate Overlay 的“释放特效 / Release VFX”与 Space 每次销毁上一批临时宿主并重新走正式 resolver，显示 Scenario、释放次数、目标/Link 数和状态，并提供 Restart/Reset Targets/Clear。测试相机从正式倾斜正交默认值初始化为测试场景瞬时副本，WASD 平移、Q/E 旋转、滚轮缩放、Home/按钮 Reset，绝不写生产 Camera BP/config。首次构建因 UE5.8 `SVerticalBox` 头文件名错误失败，修正为 `Widgets/SBoxPanel.h` 后 FullRebuild 成功（95/95 actions，source fingerprint `2ad0b0a5233f`）；VFXPreview/VFX/ElementReaction、author/verify 均通过。
- 2026-08-24：最终日志审计发现 verify 首次使用错误 Unreal Python 属性名 `b_auto_release_on_begin_play`，虽然 commandlet 退出 0，但日志含 Python Error，因此未计为通过；改为 `auto_release_on_begin_play` 后重跑，日志明确输出 `[Plan90] verified ...` 且无 Error。最终生产 `.uasset/.umap` 变化为 0，PIE Overlay、Space Release 和相机实际手感仍需人工验收。
- 2026-08-24：author 只在测试地图补齐 `Scenario Target 1..4 - Editable`，verify 通过单 Rig、四目标顺序、六参考宿主、正式相机、生产隔离与 Sandbox 警告。`BP_VFXPreviewRig.uasset` 未变，`L_VFXAuthoring.umap` 更新为多目标布局；生产 `.uasset/.umap` 变化仍为 0。
- 2026-08-24：Conduct 表现延迟候选完成 closed-editor FullRebuild，精选包 build id `55116800`、source fingerprint `8b242d96326b`，`ReEcho.Presentation.VFX`、`VFXPreview`、`Combat`、`Weapons.Logic`、`Combat.ElementReaction` 聚焦自动化均退出 0。测试覆盖 delay=0 同帧兼容、`index * delay` 顺序、播放时几何重算、新事件替换旧 timer batch、失效目标跳过和显式清理。`validate_project.py` 首次因沙箱拒绝 Content 临时目录失败，获准重跑后通过；prebuilt、Python AST 与 diff check 通过。
- 2026-08-24：生产资产保持零写入；Weapon Profile 只增加 C++ schema 字段且默认 `0`，尚未给任何生产 Profile `.uasset` 作者化非零延迟。正式 `NS_Element_Electricity` 只声明/消费 `User.StartPosition` 与 `User.EndPosition`。用户截图显示链路漂离目标后，自动化直接读取两个正式 emitter：`Fountain002`、`Fountain007` 均为 World Space。修复移除不存在的 LinkLength/LinkDirection 输出，组件改为世界原点+identity rotation，参数直接传延迟播放瞬间读取的两端世界坐标，避免组件平移/旋转二次变换。
- 2026-08-24：最终只读 verify 在与此前一致的沙箱外 UE 5.8 commandlet 环境重跑，exit 0；日志明确记录 `[Plan90] verified Blueprint parent, single rig, multi-target scenario, test-map isolation and Sandbox warning`，且无 Python Error。此前受限环境中的 exit 1 只完成 ValidatePlatforms、没有生成新 Editor 日志；相同 LinuxArm64/VisionOS SDK 提示也出现在本次成功运行中，故不是项目失败。verify 后两测试资产 SHA 保持 `3763DA...BB58` / `EE65F3...B866`，生产资产变化仍为 0。
- 2026-08-24：用户截图确认 Conduct 电链漂离怪物。首个 Local Space 假设被新增正式资产契约测试否决：`Fountain002`、`Fountain007` 的 `bLocalSpace` 均为 false。最终修复按 World Space 契约在世界原点以 identity transform 生成组件，只填实际延迟播放时重新读取的 `User.StartPosition`/`User.EndPosition` 世界坐标；移除额外参数和组件方向旋转。最终 FullRebuild 98/98 通过，source fingerprint `126f653a8ed3`；VFX（含两 emitter 空间断言、非零原点/斜向/距离/移动端点）、VFXPreview、ElementReaction 与只读 verify 均 exit 0，生产资产变化为 0。
- 2026-08-24：用户复验 World Space 候选仍偏移，继续定位参数类型与首帧时序。静态候选将端点写入从 `SetVariableVec3` 改为 LWC-safe `SetVariablePosition`，组件 `autoActivate=false`，保证端点与排序先写入再 Activate；正式资产自动化新增精确命名空间与 Position 类型断言。VeryVerbose 日志同时记录 ActorLocation、CombatTargetLocation、写入端点和组件 Transform；测试场景方向显示增加实际 CombatTargetLocation 的绿/红端点球。因用户 Editor 仍运行，构建后的类型结论与验证暂待关闭 Editor。
- 2026-08-24：Editor 关闭后资产断言给出精确证据：`GetUserParameters` 返回 `StartPosition` / `EndPosition`，类型均为 `NiagaraPosition`，外部写入使用重定向名 `User.StartPosition` / `User.EndPosition`。原 Position 断言首次因错误地用带 `User.` 的内部名称而失败，修正后通过；测试组件在 Activate 前用 `SetVariablePosition` 写入，override 读回的两个 LWC `FVector` 与非零怪物世界坐标一致。最终 FullRebuild 98/98、fingerprint `244c6aeb99f6`；VFX、VFXPreview、ElementReaction、只读 verify、validate、prebuilt、diff check 全部通过，生产资产变化为 0。Bounds 只影响剔除/视觉包围，不参与端点参数或组件变换，未作为位置修复手段。
- 2026-08-24：Review 接受后将候选重放到 `origin/main@ef88e041`。外部提交与 Plan90 语义源码/两个测试资产零重叠；仅 prebuilt 包重叠并由最终 FullRebuild 重建，`MOD-ReEcho.md` 自动保留主线内容后追加测试场景说明。组合候选 FullRebuild 93/93，fingerprint `dc71f04fad6a`；VFX、VFXPreview、Combat、Weapons.Logic、ElementReaction、RuntimeAssetPreload、ArenaScene 七组聚焦自动化均 exit 0。author 连续两次与只读 verify 均 exit 0，两个测试资产 SHA 保持 `3763DA...BB58` / `EE65F3...B866`；validate、prebuilt、diff check 通过，生产资产变化为 0。本地集成提交不推送，远端发布保持 Pending。

### 剩余风险

- 测试地图可验证资产、空间和显示契约，不能单独证明真实 gameplay event 时序；关闭前仍需 `Level00` 真实入口对照。
- 若实际使用证明 authored 轴/偏移/缩放需要美术高频写回，应另建窄范围 Calibration Profile Plan，不能在本 Plan 暗中扩大为全 VFX 架构迁移。
- 非零蔓延延迟仍需用户指定准确 WeaponId/Profile 和数值后，通过 UE 对唯一生产 Weapon Profile `.uasset` 作者化；当前所有既有资产安全保持 `0`。
- `NS_Element_Electricity` 的长度与方向只由实际播放时重新读取并转换的 Start/End 端点控制；不要再增加或假设额外 LinkLength/LinkDirection 参数。

### 人工验收结果/请求

- `ReviewAcceptedLocalIntegration`：用户已确认 Review 完成并授权合入本地 `main`；远端发布仍为 `Pending`，本任务不推送。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：已维护测试专用 Preview Harness、Production 只读与 Sandbox 不写回边界。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已维护 Editor-only 测试地图装配与无玩法副作用边界。
- `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoWeapons.md`：已审阅；本候选不改变 Runtime Module 拓扑、稳定路由或玩法/生命周期公共契约，无需修改。
