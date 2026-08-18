# Plan 50 - 程序 - 统一角色与怪物资产驱动表现

## 协调

- Planner 负责人：ReEcho teammate 侧程序 Planner。
- Executor 负责人：ReEcho teammate 侧程序 AI。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@69487228edcddb5ccab754119c4837a95d757b8d`。
- 2026-08-18 模块修订：经程序用户确认，动画表现必须成为独立 Runtime Module；此前“不拆新模块”的决策作废，执行候选必须先迁移到 `ReEchoPresentation`。
- 2026-08-18 执行验收修订：经程序用户确认，本轮不新增或运行单元测试、自动化验证和机器验收；完成实现与资产配置后交由人工 Editor/PIE 验收。
- 本地实现方式（可选，仅作交接说明）：`codex/plan50-unified-presentation`，独立 worktree `C:\Users\binnanliang\Documents\ReEcho-worktrees\plan50-unified-presentation`；主工作区中现有 Editor 资产改动保持隔离，不自动带入。
- 依赖 / 阻塞：删除二进制资产前需在最终基线上通过 Unreal Editor Reference Viewer/Asset Registry 组合引用并取得用户对准确删除清单的风险确认；任何 Editor、构建或自动化命令前需用户保存并关闭交互式 Editor。权威角色表修改必须同步 `Design/Data/ReEchoData.xlsx` 与生成 CSV。
- Writes:
  - `plans/50-unified-character-enemy-presentation.md`
  - `Source/ReEchoPresentation/**`（新增 Runtime Module，拥有通用 Animation2D 类型、Profile、FSM、Catalog、Controller、Renderer 与 FrameCollision Query/Debug）
  - `Source/ReEcho/ReEcho.Build.cs`
  - `ReEcho.uproject`
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`
  - `Source/ReEcho/{Public,Private}/Player/ReEchoPlayerPawn.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEchoActor.*`
  - `Source/ReEcho/{Public,Private}/Core/ReEchoBalanceSettings.*`
  - `Source/ReEcho/{Public,Private}/Run/ReEchoRunSubsystem.*`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`
  - `Source/ReEcho/Private/Tests/**`（仅限角色、存档、录制、Enemy/Presentation 受影响测试）
  - `Design/Data/ReEchoData.xlsx`
  - `Content/Data/characters.csv`
  - `Content/Data/character_aliases.csv`
  - `scripts/data/**`（仅在权威同步/校验契约需要时）
  - `scripts/validate_project.py`
  - `scripts/ue/create_plan40_presentation_assets.py`
  - `scripts/ue/connect_player_attack_flipbooks.py`
  - `scripts/ue/**presentation*`（仅限本 Plan 的确定性资产创建、校验和清理工具）
  - `Config/DefaultGame.ini`
  - `Content/ReEcho/Animation2D/DA_PresentationCatalog.uasset`
  - `Content/ReEcho/Animation2D/DA_Character_J_{SPADE,DIAMOND,CLOVER,HEART}.uasset`
  - `Content/ReEcho/Animation2D/DA_Enemy_*.uasset`
  - `Content/ReEcho/Animation2D/SM2D_DefaultCharacter.uasset`
  - `Content/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay.uasset`
  - `Content/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_*.uasset`
  - `Content/ReEcho/Art/Animation2D/**`（仅限经引用审计确认的正式 Flipbook/Sprite/Texture 绑定和无引用旧资源清理）
  - `Content/ReEcho/Animation2D/DA_Character_J_CAT.uasset`（删除）
  - `Content/ReEcho/Animation2D/Generated/Players/J_CAT/**`（删除）
  - `Content/ReEcho/Textures/Characters/NewCast/{Player_Cat,Echo_Cat}.uasset`（删除）
  - `Content/SourceArt/Characters/NewCast/Sprites/{Player_Cat,Echo_Cat}.png`（删除）
  - `Content/ReEcho/Textures/Characters/Boss2D.uasset`（删除）
  - `Content/SourceArt/Characters/Boss2D.png`（删除）
  - `docs/2D_SEQUENCE_ANIMATION.md`
  - `docs/ART_ASSET_ORGANIZATION.md`
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`
  - `shared/CODEBASE_MAP/README.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`（新增）
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEchoEnemies/**`（怪物逻辑只发布玩法事实，不读取表现资产）
  - `Source/ReEchoCombat/**`
  - `Source/ReEchoWeapons/**`
  - `Content/Data/enemies.csv` 与权威 Enemy XLSX（读取 `PresentationId`，本 Plan 不改怪物数值、行为或稳定 EnemyId）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
- 影响模式：`SharedContract`（让现有 Character `AppearanceId` 与 Enemy `PresentationId` 成为统一表现解析入口，并新增旧 `J_CAT` 存档迁移；不改变 Combat、Weapons 或 EnemyLogic 权威）。
- 兼容承诺 / 下游操作：稳定角色 ID 只保留 `J_SPADE/J_DIAMOND/J_CLOVER/J_HEART`；旧存档、录制和 Echo 内的 `J_CAT/J01` 在加载时迁移为 `J_SPADE`，新存档不再写入 CAT。敌人稳定 ID、行为、数值、攻击、保存和录制语义不变。表现资源缺失只安全回退到 Catalog 配置的默认 Flipbook，不能改变玩法或使 Actor 隐形。
- 明确排除：不让 GameMode、EnemyActor、EnemyLogic 或动画 Controller 持有具体 Flipbook/Texture 路径；不按 `SpawnIndex`、生成顺序或 EnemyKind 随机选择外观；不让动画帧、动画通知或播放完成驱动伤害、移动、AI、死亡或存档；不重写历史提交；不静默删除当前主工作区未提交资产；不在 Unreal Editor 外修改 `.uasset/.umap`；不修改角色/怪物平衡、技能或攻击时序；不让 `ReEchoPresentation` 反向依赖 `ReEcho`、`ReEchoEnemies`、`ReEchoCombat`、GameMode、PlayerPawn 或 EnemyActor。

## 锁定目标

1. 建立唯一资产驱动链：玩家 `AppearanceId`、敌人 `PresentationId` 通过 Cook-visible Presentation Catalog 解析 Gameplay Blueprint 类与 Animation Profile；通用运行时代码不再按具体身份硬加载资源或编写分支。
2. 玩家只保留 `J_SPADE/J_DIAMOND/J_CLOVER/J_HEART` 四个生产角色；删除 CAT 生产数据、Profile、玩家/Echo 贴图和生成资产，并把所有旧存档/录制/Echo 中的 `J_CAT/J01` 确定迁移为 `J_SPADE`。
3. 删除旧 `Boss2D` 静态贴图和 Boss 特殊 Billboard 路径；`M_TimeGuard/Enemy.TimeGuard` 使用与其他怪物相同的 Blueprint + Profile + FSM 路径。没有专属 Boss Flipbook时，Profile 显式引用已批准的 Goat 动画作安全临时表现，后续替换只改资产配置。
4. 为所有生产敌人提供稳定、可在 Blueprint 调整的表现 Prefab：Grunt、Shield、Bomber、Slime、Rabbit、Fox、TimeGuard；共享完全一致的组件层级，子 Blueprint 只拥有 Collision、CharacterScale、Tint、阴影、挂点和局部 Transform，不拥有玩法状态。
5. 清理 Profile 中空/重复 Gameplay Tag、重复默认集和重复 WeaponVisualSetId；统一 Idle/Move/Attack/Hit 状态契约，Attack 单次、Idle/Move 按 Profile 循环，缺失语义显式回退 Idle。
6. 经 UE 引用审计后清理确认无引用的旧/重复 2D 资源和 Redirector；保留仍被正式 Profile、Collision Track、测试或 Cook 引用的资产。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEchoPresentation`（新增）：独占通用 Animation2D Profile/FSM/Catalog/Controller/Renderer 与逐帧 Query/Debug；只消费稳定表现 ID 和表现命令。
  - `MOD-ReEcho / AREA-Presentation`：保留 Player/Enemy Host、Gameplay Blueprint 类注册和玩法事件到表现命令的适配，不再拥有通用 Animation2D 实现。
  - `MOD-ReEcho / AREA-Player`：移除 CAT 硬引用，玩家继续只消费角色 `AppearanceId`。
  - `MOD-ReEcho / AREA-Enemies`：Enemy Host 接收 Definition 的稳定 `PresentationId`，不再传 SpawnIndex 决定外观；Presentation 只读消费逻辑事件。
  - `MOD-ReEcho / AREA-Run`：增加 CAT 到 Spade 的保存/录制/Echo 迁移并提升存档版本。
  - `MOD-ReEcho / AREA-Data`：权威角色表只发布四个角色，别名不再解析到已删除角色。
  - `MOD-ReEchoEnemies`：公共玩法事件和状态所有权保持不变，只审阅 Host 接线影响。
- 对应模块文档：新增 `MOD-ReEchoPresentation.md`，维护 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md` 和 `MOD-ReEchoEnemies.md`；审阅 `MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`，预期无需正文修改，因为表现仍是单向只读消费者。
- 设计意图：玩法域只传稳定 ID 和类型化事实；Catalog 是 Cook 可达的资产注册表；Profile 是动画策略；Gameplay Blueprint 是美术可调节点/Transform；Controller 是无身份分支的执行器。新增角色或怪物只增加/替换资产配置，不扩展 GameMode/EnemyActor switch。
- 权威状态与依赖：
  - Character/Enemy CSV 继续拥有稳定玩法身份和表现 ID。
  - `ReEchoPresentation` Catalog 只拥有 `PresentationId -> Profile` 的硬引用映射，不引用主模块 Actor/Blueprint 类，也不拥有战斗状态。
  - 主模块独立拥有 `PresentationId -> Enemy Gameplay Blueprint Class` 注册；它可以依赖 `ReEchoPresentation`，反向依赖被禁止。
  - Profile 拥有 WorldHeight、FSM、AnimationSets、Clip 与安全回退。
  - Gameplay Blueprint 拥有组件树、CharacterScale、Collision、Tint、阴影和挂点 authored defaults。
  - RunSubsystem 独占存档迁移；迁移覆盖 CurrentBuild、所有 Echo Storage、Recording 与恢复中的 Encounter Snapshot。
  - 依赖保持 `ReEcho -> ReEchoPresentation -> Engine/Paper2D/GameplayTags`；`ReEchoPresentation` 不依赖任何项目玩法模块，Enemies/Combat/Weapons 也不依赖表现模块。
- 决策记录：
  1. 采用一个通用原生 Host + 可选表现子 Blueprint，而不是为每个身份创建 C++ 类；表现 Catalog 只选择 Profile，主模块类型化 Registry 选择 Gameplay Class，GameMode 不扫描或 switch 具体身份。
  2. 玩家共享一个 `BP_PlayerGameplay`；四角色差异由 Profile 解析，避免四份组件树漂移。怪物保留按 PresentationId 的子 Blueprint，因为碰撞、阴影、比例和挂点需要美术逐类调整，但所有子类必须通过自动化证明同一组件契约。
  3. 保留当前序列化字段 `AppearanceId` 作为 Profile 内的稳定表现键，避免无收益的二进制字段重命名；Catalog Entry 对外统一称 `PresentationId`。
  4. `Enemy.TimeGuard` 在专属美术缺失时显式使用 Goat Flipbook，而不是保留 Boss2D 或返回空 Profile；临时资源选择集中在 Profile，可在 Editor 一次替换。
  5. 删除 CAT 采用 SaveVersion 迁移而非永久保留生产行；兼容路径只在加载旧数据时存在，运行时和新保存只出现四个正式角色。
  6. 不把 `TMap<GameplayTag, Clip>` 立即改为新 Schema；先用 `IsDataValid`、Editor 工具和自动化强制唯一/非空，避免不必要的资产迁移。若 UE 无法稳定序列化去重后的 Map，才在 Plan 内记录并升级为数组 Entry Schema。
- 相关文档同步范围：
  - `ARCHITECTURE.md`：更新表现 ID/Catalog/Profile/Blueprint 单向链和 Run 兼容迁移。
  - `README.md`：注册 `MOD-ReEchoPresentation` 并把 `AREA-Presentation` 通用动画路线指向新模块。
  - `MOD-ReEchoPresentation.md`：按模块模板记录存在原因、职责、公共契约、依赖方向、扩展和验证。
  - `MOD-ReEcho.md`：更新 Player/Enemy Presentation、Blueprint 参数、CAT 移除、Boss 统一路径和测试位置。
  - `MOD-ReEchoEnemies.md`：明确 PresentationId 由 Host 传递且逻辑模块不消费资产。
  - `MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`：关闭前审阅只读事件契约是否未变。
  - `docs/2D_SEQUENCE_ANIMATION.md`、`docs/ART_ASSET_ORGANIZATION.md`：删除旧 Spade-only/Boss Billboard 描述并提供资产编辑指南。
- 关闭前逐项填写审阅结果：见“架构文档审阅结果”。

## 锁定验收

- [ ] 生产角色数据、开始菜单、默认配置、Catalog 和 Profile 只包含四个角色；运行时不会创建或选择 `J_CAT`。
- [ ] 旧 SaveVersion 4-10 中 CurrentBuild、Pending/Latest/Previous/Stored Echo、活跃录制和 Encounter 快照的 `J_CAT/J01` 都迁移为 `J_SPADE`，迁移幂等且新保存使用新版本。
- [ ] 所有启用 Enemy CSV 行的 `PresentationId` 能唯一解析 Gameplay Blueprint Class 和 Profile；未知/重复/空 ID 在数据或自动化验证中失败，不按 SpawnIndex 回退。
- [ ] `ReEchoPresentation` 可独立编译且不依赖任何 ReEcho 玩法模块；`ReEcho` 仅单向依赖它，Catalog 资产不保存主模块 Actor/Blueprint 类型。
- [ ] Grunt、Shield、Bomber、Slime、Rabbit、Fox、TimeGuard 使用统一组件树；每个 Blueprint 可独立调整 CharacterScale、Collision、FootRoot、FlipbookRoot、GroundShadow、Tint 和挂点。
- [ ] Boss 不加载 `Boss2D`，不进入静态 Billboard 特例；攻击、移动、受击、死亡均通过通用表现 Controller/FSM，玩法 Boss 逻辑和数值不变。
- [ ] 四个玩家和七个生产敌人的 Profile 不含空/重复 Tag、重复默认 AnimationSet 或重复 WeaponVisualSetId；Idle 可用，Move/Attack/Hit 按约定解析并安全回退。
- [ ] `Boss2D` 和 CAT 目标资产经 Unreal Editor 引用审计后删除并修复 Redirector；Cook/Asset Registry 不含丢失引用。其他旧资源只在确认无引用后删除。
- [ ] EnemyLogic/Combat/Weapons 不读取 Profile、Blueprint、Flipbook、Sprite 或动画帧；动画结果不影响伤害、移动、AI、录制、暂停和保存。
- [ ] 修改 C++ 已格式化；权威 XLSX/CSV 同步检查、Editor Development 构建、聚焦自动化、`validate_project.py`、`git diff --check` 通过；发布候选完成 `-FullRebuild` 并刷新允许的预构建包。
- [ ] 用户在 PIE 验收四角色和全部怪物的 Idle/Move/Attack/Hit、尺寸、脚点、朝向、阴影、透明排序、Boss 可见性以及切换时无双重 Billboard/Flipbook。
- [ ] 未提交被隔离的主工作区 WIP、未审计资源删除、Redirector、日志、机器路径或精选预构建允许列表外产物。

## Step 0 门禁

- 基线分支/提交：`origin/main@69487228edcddb5ccab754119c4837a95d757b8d`；模块修订发布前和恢复实现前再次 fetch 并核验。
- 引擎/构建可用性：UE 5.8 安装版；当前交互式 Editor 曾有未保存/已保存资产修改，任何构建或命令前必须由用户保存并关闭，且使用 Git common-dir Unreal 锁。
- 现有聚焦测试结果：基线 `ReEcho.Presentation.Animation2D`、Editor Build 与 FullRebuild 曾通过，但不作为本候选证据；实现后重跑。
- 共享契约 / 难合并资源风险：`Design/Data/ReEchoData.xlsx`、Catalog/Profile/Blueprint `.uasset`、Level/预构建包均为共享或二进制表面；不得选择 ours/theirs 或从脏 main 整块复制。主工作区当前多项纹理修改独立保留，只有逐项审计并经用户选择后才组合。
- 基线损坏时的停止条件：远端新增同路径 Catalog/Profile/Blueprint/XLSX/SaveVersion/Enemy Presentation 修改；用户主工作区资产无法区分目标修改与无关重保存；删除目标仍被正式资源引用；新 Boss Profile 没有安全可见 Flipbook；此时停止相应集成或删除并报告物理、逻辑和生命周期冲突。

## 实现提纲

1. 新建 `ReEchoPresentation` Runtime Module，把通用 Animation2D 类型、Profile、FSM、Catalog、Controller、Renderer 和 FrameCollision Query/Debug 迁入；更新 `.uproject`、Build 依赖和完整模块文档，并保持它不反向依赖任何玩法模块。
2. 在主模块建立独立的 Enemy Gameplay Blueprint Class Registry；表现 Catalog 只解析 Profile。将 Enemy Definition 的 `PresentationId` 贯穿 Spawn/Restore -> EnemyActor -> EnemyPresentation，移除 SpawnIndex 外观选择、Boss 静态特殊分支和 Catalog 到主模块 Actor 类型的依赖。
3. 通过 Unreal Editor API 创建/统一七个怪物 Blueprint/Profile；TimeGuard 临时绑定批准的 Goat 动画，修复全部 Profile 的重复/空 Key 和循环策略。
4. 更新四玩家 Profile 与 Catalog，删除 CAT 的生成/导入入口；保持一个共享 Player Gameplay Blueprint。
5. 更新权威 XLSX/CSV、默认配置和 SaveVersion 迁移；改写受影响测试为四角色并增加完整存档/Echo 迁移覆盖。
6. 在最终资产引用闭包上执行 Reference Viewer/Asset Registry 检查；取得准确删除确认后通过 Unreal Editor 删除 Boss2D、CAT 和确认无引用的旧资源，修复 Redirector。
7. 更新文档、运行聚焦自动化和构建；用户 PIE 验收通过后完成最终 FullRebuild、评审、集成与发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 权威数据 | `python scripts/data/sync_xlsx_to_csv.py --check` 及相关聚焦测试 | XLSX 与 Characters/Aliases CSV 完全同步，只有四个生产角色 |
| 静态 | `python scripts/validate_project.py` | 四角色、七敌人 PresentationId、资产清单和保存版本不变量通过 |
| 格式 | 对修改的 `.h/.cpp` 执行仓库 `.clang-format`，再审查 diff | 仅目标源码格式变化，无语义漂移 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0；构建清单包含独立 `ReEchoPresentation`，且依赖方向静态审计通过 |
| 自动化 | 聚焦 `ReEcho.Presentation.Animation2D`、Enemy Definition/Host、Run Save/Recording/Echo/Loadout 测试 | ID 解析、状态回退、CAT 迁移、Boss 通用路径和相邻回归通过 |
| Blueprint/资产 | UE Editor/Commandlet 加载 Catalog、4 Player Profile、7 Enemy Profile 与全部 Gameplay BP；Reference Viewer/Asset Registry + Redirector 检查 | 资产可加载、唯一映射、统一节点树、删除目标无引用、无丢失引用 |
| Cook | 适用的 Cook/Shipping 资源存在性检查 | Catalog 硬引用闭包包含全部正式 Profile/BP/Flipbook，不含 Boss2D/CAT |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 后 `python scripts/validate_project.py`、`git diff --check` | 最终候选与预构建包指纹一致，仅含允许产物 |
| 人工 | 用户 PIE 逐项检查四玩家、七敌人和 Boss | 记录 `Passed` 或具名返工项 |

## 执行记录

### 变化

- 待实现。

### 证据

- Plan 编写前 fetch 确认远端最大正式编号为 Plan49；2026-08-18 模块修订前再次 fetch，`origin/main` 为 `6948722`。
- 程序用户指出动画表现必须执行新增 Module 规则；审计确认原 Plan50 的“不拆新模块”明确决策错误，已改为新增 `ReEchoPresentation` 并拆除 Catalog 对主模块 Actor/Blueprint 类型的反向依赖。
- 只读审计确认当前敌人表现仍以 `Archetype + SpawnIndex` 选择四个硬加载 Profile，Boss 强制使用静态 `Boss2D`；玩家 Catalog 包含 CAT，默认配置、存档测试、Echo 和生成脚本均引用 `J_CAT`。

### 剩余风险

- 当前主工作区有多项 Editor 资产修改，不能自动判断哪些是用户有意调整；实现 worktree 不带入，最终组合前逐项审计。
- 删除 CAT 会影响旧存档和 Echo，必须在所有嵌套 BuildSnapshot 完成迁移后才允许删除资产。
- TimeGuard 暂用 Goat 动画的视觉质量需要用户 PIE 验收；专属 Boss 动画以后可只替换 Profile。

### 人工验收结果/请求

- `PendingBeforeClose`：用户逐项验收四角色和全部敌人表现、Blueprint 调整能力与 Boss 临时动画。

### 架构文档审阅结果

- 待实现与关闭评审时逐项填写。
