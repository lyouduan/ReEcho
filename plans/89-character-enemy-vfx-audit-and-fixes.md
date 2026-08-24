# Plan 89 - 程序 - 人物与怪物特效全链路审计和修复

## 协调

- Planner 负责人：当前 ReEcho 程序侧 Planner。
- Executor 负责人：当前 ReEcho 程序侧 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Codex executor-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingFollowUp`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）；本次只发布 Plan90 前置最小 slice，Plan89 全量审计未关闭。
- 本地规划 / 实现基线：`origin/main@85004addd96372c084a8aca2c4b89c4eeb5728b6`。
- 本地实现方式（可选，仅作交接说明）：`codex/plan89-vfx-audit-fixes` 独立 worktree；主工作区现有未提交 VFX/纹理导入只作对照，不覆盖、不清理、不整体复制。
- 依赖 / 阻塞：依赖当前 `CombatEvents`、`CombatPresentationCoordinator`、Enemy projectile events、Weapon Presentation DA 与 Element reaction 事件契约；依赖 Plan71 统一 Player/Echo 的 `AttackVfxRoot` / `HurtVfxRoot` 相对 Transform 和继承缩放，Plan89 只消费该空间契约；视觉关闭依赖程序用户在 PIE 中逐项验收。
- Writes:
  - `plans/89-character-enemy-vfx-audit-and-fixes.md`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/**`
  - `Source/ReEcho/{Public,Private}/Presentation/Combat/**`（仅在审计证明阶段排序或生命周期适配缺陷时）
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoProjectileActor.*`（仅武器飞行/命中特效适配）
  - `Source/ReEcho/{Public,Private}/{Player,Graybox}/ReEcho{PlayerPawn,EchoActor,EnemyActor}.*`（仅非空间的组件装配/事件绑定缺陷；Player/Echo 根 Transform 归 Plan71）
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatPresentationTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`
  - `Content/VFX/**`、`Content/Mat/**`、`Content/00_Textures/**`、`Content/01_Textures/**` 中审计后具名批准的运行时依赖
  - `Content/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay.uasset` 与具名 Enemy Gameplay Blueprint（仅挂点需 authored 修正且经 Editor 修改时）
  - `Design/Art/VFX/combat_vfx_import_manifest.csv`、`scripts/art/import_combat_vfx.py` 及其聚焦测试（仅正式资产依赖集合变化时）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
- Stable Reads:
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyEventsComponent.h`
  - `Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`
  - `Content/ReEcho/Data/Presentation/Weapons/**`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `plans/71-echo-player-presentation-parity.md`
- 影响模式：`SharedContract`（`Isolated | ReadOnly | SharedContract | Exclusive`，仅说明集成影响，不是写锁）。
- 兼容承诺 / 下游操作：Combat/Enemies/Weapons 继续拥有玩法与命中权威；Niagara 缺失、加载失败、裁剪或播放结束不能改变攻击、伤害、移动、死亡、存档或回放。保留现有稳定 ID、存档字段、XLSX/CSV 数值和角色整体 `CharacterScale` 契约。
- 明确排除：不扫描或整理与运行时语义无关的整包素材库；不整包提交主工作区约 1400 个未跟踪导入；不在 Editor 外手改 `.uasset`；不重绘/替换美术风格；不以固定 Delay、粒子碰撞或 Niagara 完成回调控制玩法；不顺手修改伤害、冷却、射程、元素规则或 Boss 数值；不由 AI 代替用户完成视觉质量验收。

## 锁定目标

1. 建立当前版本所有**运行时可达**人物、回响和怪物战斗特效清单，覆盖武器攻击/飞行/命中、人物与怪物受击、兔子/狐狸特殊行动、元素附着、持续状态和元素反应；每项记录语义、触发事件、目标资产、空间/挂点、朝向、尺寸、排序、生命周期、预加载及实际支持状态。
2. 对清单逐项复现和分类异常，至少区分：资源缺失/错误映射、加载或编译失败、挂点/偏移/缩放错误、朝向或 Local Space 错误、前后景排序/Bounds 裁剪、阶段重复/漏播/残留、玩法载体与视觉载体错位，以及仅属主观美术质量的问题。
3. 修复证据明确的程序适配与项目内资产配置异常，使 Player、Echo、Slime、Rabbit、Fox、Sheep/Boss 在其已实现语义上表现一致；所有循环、跟随和在途特效在 Commit、Impact、Ended、Cancelled、Death、EndPlay、重开/继续等相应边界正确生成、去重和清理。
4. 对尚无正式资产的语义明确记录 `ApprovedFallback | TemporaryFallback | Missing`，不得用错误角色/武器特效静默顶替；对纯主观美术问题形成具名资产与症状交接，不暗中改变美术意图。
5. 给出可重复的自动化/Editor 审计证据和一份逐项 PIE 验收表；只有用户确认尺寸、方向、位置、层级、裁剪、时机和整体观感后才关闭 Plan。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEchoVFX` / `AREA-Presentation`：特效语义目录、实例生命周期、挂点、排序、方向、预加载和资产审计主入口。
  - `MOD-ReEchoPresentation` / `AREA-Presentation`：人物/怪物表现根、动作阶段与渲染排序的只读适配边界。
  - `MOD-ReEcho`：Player/Echo/Enemy Host 和 Projectile Host 的组件装配。
  - `MOD-ReEchoCombat`、`MOD-ReEchoEnemies`、`MOD-ReEchoWeapons`：只读契约影响审阅；除非发现现有事件无法表达已发生的稳定事实，否则不修改逻辑模块。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`、`MOD-ReEchoPresentation.md` 与 `MOD-ReEcho.md`，均已加入 `Writes`；关闭前审阅 `MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoWeapons.md`，若公共契约事实未变化则在执行记录中逐项说明无需修改。
- 设计意图：以资源中立的已发生事件作为同步时钟，由集中 VFX 适配层拥有资产选择和可丢弃视觉实例；Gameplay Host 只提供明确挂点/载体，Niagara 不反向控制玩法。完整清单先于修复，避免只修最显眼症状后留下同类异常。
- 权威状态与依赖：不改变 Combat 的伤害/生命权威、Enemies 的动作/投射物权威、Weapons 的武器攻击权威或 Host 的世界 Transform 权威。VFX 仅拥有 Niagara/Material Billboard 实例、局部表现参数及其清理索引；依赖保持 `ReEcho Presentation -> Combat/Enemies/Weapons events` 单向消费。
- 决策记录：
  1. 范围采用“运行时可达语义清单”，而非扫描 `Content/VFX` 的所有备用素材；后者无法证明玩家可见问题且会扩大到美术资产整理。
  2. 以事件阶段和逻辑载体为同步依据，不加入固定延迟；Flipbook normalized time 仅允许作局部视觉 marker，不能成为提交或命中权威。
  3. 主工作区现有 VFX/纹理改动保留为候选来源；先比较其具名资产与当前权威依赖，只有确定修复某一清单项且通过 Editor/PIE 验证后才精确纳入，禁止整体复制。
  4. 资产本体修正必须经 Unreal Editor 或仓库已有 UE 工具完成；程序自动化负责结构、引用、Local Space、Bounds/排序契约和生命周期，视觉优劣由用户验收。
  5. Plan71 独占 Player/Echo 特效根的空间一致性与公共应用路径；Plan89 不增加第二套位置/缩放补偿，只验证并消费其最终空间快照，避免两个 Plan 同写同一视觉权威。
- 相关文档同步范围：关闭前必审 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；若拓扑、稳定路由未变化，记录“已审阅、无需修改”。维护 `MOD-ReEchoVFX.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEcho.md`；审阅 `MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoWeapons.md`。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待审阅。
  - `shared/CODEBASE_MAP/README.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：待维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：待维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`：待审阅。

## 锁定验收

- [ ] 清单覆盖所有当前 Catalog、Element Catalog、Weapon Presentation DA 和 Host/Projectile 接线可达的 Player、Echo、Slime、Rabbit、Fox、Sheep/Boss 特效，不以“资产可加载”代替实际播放链路检查。
- [ ] 每项异常具有正常/异常对照、运行时日志/事件/组件状态、资产检查或 PIE 画面之一的证据，并标明是程序适配、项目资产配置还是主观美术问题。
- [ ] 所有已支持语义使用正确目标资产和角色/武器映射；不存在同短名误映射、错误 fallback 或缺失依赖静默通过。
- [ ] 附着类特效跟随正确的 `AttackVfxRoot` / `HurtVfxRoot` 或权威 Projectile，世界类特效不叠加人物挂点偏移；Player、Echo 和各 Enemy Blueprint 的 authored 偏移/整体缩放一致生效。
- [ ] 方向性特效在左右、斜向和移动目标场景中方向正确；需要组件旋转的发射器具备 Local Space 或显式 User Parameter 契约，不用玩法方向补偿资产内部无证据偏转。
- [ ] 特效在倾斜正交相机和场景排序下不被角色/地面错误遮挡，无明显 Fixed Bounds 裁剪；尺寸与碰撞/角色比例的关系符合具名契约但不反向改变玩法碰撞。
- [ ] 一次性、循环、跟随、飞行与连线特效按语义正确去重，在 Impact/Ended/Cancelled/Death/EndPlay/重开与 Continue 边界无残留或重复。
- [ ] 缺失/加载失败/提前结束不会影响攻击、伤害、移动、死亡、保存或回放，且留下可定位的限频诊断。
- [ ] C++/资产/导入清单的聚焦自动化、完整构建、项目校验、预构建检查与 `git diff --check` 通过。
- [ ] 用户在 PIE 中完成逐项视觉验收：人物与回响各武器、人物/怪物受击、Rabbit/Fox/Sheep 特殊表现、四元素附着、持续 Burn 及全部已实现元素反应的尺寸、方向、位置、层级、裁剪、时机和整体观感。
- [ ] 仅显式暂存 Plan 具名修复路径；主工作区无关 VFX/纹理/场景资产保持本地，且未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`codex/plan89-vfx-audit-fixes`，批准基线 `origin/main@85004addd96372c084a8aca2c4b89c4eeb5728b6`。
- 引擎/构建可用性：UE 5.8 安装版；执行资产检查/PIE/构建前确认 Editor 已保存关闭并取得 Git-common-dir Unreal 锁。最终程序发布候选必须 `-FullRebuild` 刷新精选预构建包。
- 现有聚焦测试结果：待 Executor 在准确基线上运行 `ReEcho.Presentation.VFX`、`ReEcho.Presentation.Combat`、相关 Projectile/Enemy Host 与 RuntimeAssetPreload 基线；若基线失败先记录原始失败，不把修复后结果倒写为基线通过。
- 共享契约 / 难合并资源风险：`.uasset` 不可语义合并；主工作区已有大量未提交 VFX/纹理导入及 `BP_PlayerGameplay`、Rabbit/Sword 特效修改。任何纳入必须按“准确源路径 -> 准确目标路径 -> 预期语义”列单并保留恢复点。
- 基线损坏时的停止条件：权威事件链本身缺少稳定事实、必要 Blueprint/Asset 无法加载、Niagara 编译错误无法归因、现有主工作区候选来源不明，或修复需要改变伤害/时序/公共 Schema 时，停止越界部分并由 Planner 更新范围或请求用户决定。

## 实现提纲

1. 生成权威运行时清单：枚举两个 VFX Catalog、Weapon Presentation DA、Host/Projectile 组件装配和预加载根；为每个语义建立触发到停止的链路表与预期空间契约。
2. 在未改资产前运行基线资产/自动化审计；用开发测试场景或具名 Spawn 命令覆盖 Player、Echo、Slime、Rabbit、Fox、Sheep/Boss，采集加载、Niagara 编译、组件 Transform、Local Space、Bounds、排序、激活/清理和事件序列证据。
3. 把异常按根因分组：先修 Catalog/映射/挂点/空间/生命周期等可合并程序问题，再处理具名项目适配资产；每组修改后立即复跑最小回归。
4. 对主工作区候选资产仅做逐项差异/依赖审计；需要纳入时先记录准确 include/exclude 清单和恢复点，通过 Unreal Editor 验证后精确复制/提交，禁止批量导入整库。
5. 增强自动化与诊断，使清单完整性、正确目录映射、预加载、Local Space/Renderer/Bounds 关键契约、去重和清理可重复验证。
6. 完整构建最终候选，执行项目校验、预构建检查和 diff 检查；生成逐项 PIE 验收表，由用户完成主观视觉门禁。
7. Planner 评审实际 diff、架构文档和精确资产清单；仅在用户验收通过后关闭并按远端审计门禁集成发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 清单/静态 | Catalog、Weapon Presentation DA、Host 装配、预加载与 import manifest 交叉检查 | 每个运行时可达语义有唯一资产、事件、挂点/空间、生命周期与支持状态；无孤立或错误短名映射 |
| Python 导入（如变化） | `python -m unittest scripts.art.test_import_combat_vfx` 与 importer `--check` | 具名根及递归依赖哈希闭包可复现，目标冲突会停止 |
| UE 资产 | 聚焦 Editor 审计/加载脚本，检查 Niagara 编译、Emitter Local Space、Renderer、Fixed Bounds、引用与组件 Transform | 资产可加载且关键空间/裁剪契约满足；失败具名到 System/Emitter/Renderer |
| C++ 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format` 并审阅 diff | 仅格式化触及的代码，语义差异可审阅 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.VFX`、Combat Presentation、RuntimeAssetPreload、Projectile/Enemy Host 受影响测试 | 映射、阶段、方向、去重、生命周期、投射物跟随及缺失资产降级通过 |
| 完整构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UE 5.8 UHT/UBT 成功并刷新准确精选预构建包 |
| 项目 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目边界、源码指纹、精选包与文本检查通过 |
| 人工 PIE | 逐项矩阵覆盖 Player/Echo 武器、Hurt、Rabbit/Fox/Sheep、元素附着/状态/反应，并检查左右/斜向、倾斜相机、暂停/死亡/重开/Continue | 用户记录每项 `Passed` 或具名异常；所有 `PendingBeforeClose` 清零后方可关闭 |

## 执行记录

### 变化

- 2026-08-24：创建 Plan。范围锁定为全部运行时可达的人物/回响/怪物 VFX，而非整个备用素材库；主工作区现有未提交 VFX/纹理只作隔离候选来源。
- 2026-08-24：用户要求先恢复 Plan90 的生产 VFX 基线并保留已验收 Bow 方向，批准把 Plan89 拆为 prerequisite slice。本发布单元包含 Gun `DamageApplied` authored slot、具名 Bow Niagara 替换、Bow local `+Y` 轴适配及对应测试/文档；首次门禁发现当前主线 RuntimeAssetPreload 测试仍硬编码旧总数并要求缺失 Whip 预热后，用户追加批准仅纳入权威 Gather 一致性与 Whip `TestFalse` 的纯测试修正。Fire/Water body variant 支持矩阵、错误 fallback 诊断、Element preload 和全部 Runtime 实现仍延后，不进入本次主线。

### 证据

- 2026-08-24：`git fetch origin` 后确认批准基线 `a475b44d`；本地旧 main `55a15d15` 是其祖先。传入提交仅修改 Plan87、商店 UI C++/测试、模块文档和精选预构建包，与 `Content/VFX/**` 无直接路径冲突；用户确认采用最新远端 + 保留本地候选的组合适配。
- 2026-08-24：推送前远端新增 `85004add`，仅修改 Plan71，但将 Player/Echo 的 `AttackVfxRoot` / `HurtVfxRoot` 空间一致性锁定为其职责。审计无 Git 路径冲突、存在逻辑耦合；用户确认把 Plan89 重放到该基线，并以“Plan71 拥有空间根、Plan89 消费空间契约并拥有 Niagara/生命周期”的方式组合适配。
- 2026-08-24：静态路线确认运行时入口至少覆盖 `FReEchoCombatVfxCatalog`、`FReEchoElementReactionVfxCatalog`、`UReEchoCombatVfxComponent`、`AReEchoProjectileActor`、Weapon Presentation DA、Player/Echo/Enemy 的 `AttackVfxRoot` / `HurtVfxRoot` 和 RuntimeAssetPreloader。
- 2026-08-24：Plan-only 候选通过 `python scripts/validate_project.py` 与 `git diff --check`；证据级别为 static verified only，尚未声称 UHT/UBT、自动化或 PIE 通过。
- 2026-08-24：最小 slice 重放到 `origin/main@f8e8a40b`。Gun Profile 只把 `DamageApplied` 配置为 `/Game/VFX/People/Bullet/Particle/NS_People_Bullet_spark`；Bow 资产 SHA-256 为 `8F6CC57C4913F9267CC92D64465A664C3330B5A2F2CD856FE0BF11D7DB058D07`，Catalog 只对 `PlayerBowFlight` 返回 authored local `+Y`。没有采用完整候选中的 Element Catalog、CombatVfxComponent 或 RuntimeAssetPreload 变化。
- 2026-08-24：最新主线最小 slice 的首轮 `Build-Editor.cmd -Configuration Development -FullRebuild` 成功（95 actions），精选包 source fingerprint `3a1b54ceeb95`；VFX Catalog 与 Combat Presentation 聚焦自动化通过。RuntimeAssetPreload Catalog 暴露当前主线的两个过期测试契约并失败：权威清单为 37 项但测试仍硬编码 32（`ReEchoRuntimeAssetPreloadTests.cpp:17`），测试仍要求缺失 Whip 特效预热（`:36`）；Completion 通过。用户随后批准纳入该纯测试修正；不改变 Runtime 预热实现或 Element Catalog。
- 2026-08-24：加入获批纯测试修正后的最终 `Build-Editor.cmd -Configuration Development -FullRebuild` 成功（96 actions），精选包 source fingerprint `d535129d7e8c`。同一最终 DLL 上 VFX Catalog、Combat Presentation Capabilities/Lifecycle、RuntimeAssetPreload Catalog/Completion 与 Arena Scene Contract 全部通过；Gun Impact 具名非空路径、Bow local `+Y` 独立轴/任意方向及预热权威 Gather 一致性均受测试锁定。
- 2026-08-24：提交前 final fetch 发现远端前进到 `99bbfe53`，其中 `7d503793` 刷新同一精选预构建包。用户确认组合适配后，将最小 slice 的 7 个语义文件重放到 `origin/main@99bbfe53`；旧 manifest/DLL 未重放，必须由该最终组合重新 FullRebuild。Plan91/92、商店层级卡槽和 `MOD-ReEcho` 变化保持原样，Plan90 文件及所有排除的 Element/CombatVfxComponent Runtime 路径未进入候选。
- 2026-08-24：`origin/main@99bbfe53` 最终组合 `Build-Editor.cmd -Configuration Development -FullRebuild` 成功（96 actions），精选包 source fingerprint `10426a3c8f12`；同一 DLL 上 VFX Catalog、Combat Presentation Capabilities/Lifecycle、RuntimeAssetPreload Catalog/Completion 与 Arena Scene Contract 全部通过。

### 剩余风险

- 主工作区包含来源/完成度未确认的大量二进制资产，不能文本合并或整体采用；任何需要的资产必须逐项经 Editor 与 PIE 验证。
- 自动化可证明结构、映射和部分运行时状态，不能替代特效视觉质量验收。
- Gun Impact authored slot 已通过资产回读和自动化，但真实命中时的尺寸、位置、层级与观感仍为 `PendingFollowUp`；不得声称其视觉已验收。
- Fire/Water body variant 支持矩阵及其 Missing/fallback/preload 修复已从本次发布单元排除，仍保留在本地恢复候选中等待后续验收和集成。

### 人工验收结果/请求

- `Passed`：Bow 左右/斜向方向；用户确认 local `+Y` 候选“箭头没有问题”。
- `PendingFollowUp`：Gun Impact 真实命中视觉，以及 Plan89 原全量人物/怪物/元素 VFX 矩阵。用户明确要求先发布 prerequisite slice 恢复 Plan90；Plan89 保持 `Review`，不关闭。

### 架构文档审阅结果

- `MOD-ReEchoVFX.md`：已维护 Bow authored local `+Y` 方向契约；本 slice 不改变生命周期或 Element 支持矩阵。
- `MOD-ReEcho.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoWeapons.md`、`ARCHITECTURE.md`、`README.md`：已审阅；本 slice 只修正既有 Profile 资产槽与 VFX 适配轴，不改变模块拓扑、Host/Projectile/Gameplay 权威或稳定路由，无需修改。
