# Plan 73 - 程序 - 元素反应 Niagara 改造与旧表现删除

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：当前对话独立程序 Executor（Plan 发布后启动）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@a85f909e06cadd6c10b7104a2b130410887ca174`。
- 本地实现方式：用户已确认不采用一任务一 worktree，直接在当前本地 `main` 工作区执行；现有 63 项跟踪修改和 1069 项未跟踪内容均视为受保护本地工作，只显式暂存本 Plan 路径。
- 依赖 / 阻塞：使用现有 `/Game/VFX/Element` Niagara 交付；开始接线前必须在 Editor 内确认每个 System 的依赖、循环、Simulation Space、尺寸和参数。若没有可安全表达 Grass/Water 持续附着的资产，则保留旧附着文字并停止删除阶段，请用户/美术补齐或确认视觉取舍。
- Writes:
  - `plans/73-element-reaction-niagara-migration.md`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoCombat/Private/Combat/ReEchoElementHitResolver.cpp`
  - `Source/ReEchoCombat/Private/Tests/**` 中本 Plan 聚焦契约测试
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoElementReactionVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`（只读暴露当前 Definition 的稳定 PresentationId）
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`
  - `Source/ReEcho/Private/Presentation/Loading/ReEchoRuntimeAssetPreloader.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`
  - `Content/VFX/Element/**` 中经 Editor 审计确认的 Niagara 根及其准确递归依赖；不自动纳入压缩包、源图或整套公共素材库
  - `Design/Art/VFX/combat_vfx_import_manifest.csv`、`scripts/art/import_combat_vfx.py`、`scripts/art/test_import_combat_vfx.py`（仅在正式依赖闭包需要纳入当前导入契约时）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Design/Data/ReEchoData.xlsx` 的 `元素体系Y / tblElements / tblReactions`
  - `Content/Data/elements.csv`、`reactions.csv`、`statuses.csv`
  - `Source/ReEchoCombat/{Public,Private}/Combat/ReEchoElementRuntime.*`
  - `Source/ReEcho/Public/Combat/ReEchoElementReaction.h`
  - `Source/ReEcho/Private/Combat/ReEchoElementReaction.cpp`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
- 影响模式：`SharedContract`（新增 Combat 公共只读事件，并修改主模块世界表现与敌人旧表现；不改变元素结算 Schema、状态所有权或模块依赖方向）。
- 兼容承诺 / 下游操作：当前六个 ReactionId、BehaviorId、伤害公式、附着消费、免疫、Growth 范围、Conduct 目标顺序、保存恢复和卡牌通知语义保持不变；Niagara 缺失或播放失败只能缺视觉，不得改变结算。
- 明确排除：不修改 `ReEchoData.xlsx` 或生成 CSV；不改普通元素弹道、武器 `NEXT` 指示、Projectile `ElementLabel`、UI 元素图标、武器贴图、兔子 Billboard；不让 Niagara 搜索目标、应用伤害、决定元素状态或驱动玩法时序；不批量提交当前本地公共纹理库、zip、实验 Niagara 或无关 VFX。

## 锁定目标

1. 保持当前六种元素反应设定不变，在反应全部结算完成后发布一次资源中立的 `FReEchoElementReactionResolvedEvent`，携带稳定反应身份、前后元素和 Combat 已确定的真实受影响目标。
2. `/Game/VFX/Element` 的 Niagara 表达六种反应：Burn 使用 Fire，Vaporize 使用 Water，Growth 对实际新增草附着目标使用 Grass，Conduct 按权威顺序对真实连锁目标使用 Electricity，两种 Enhance 按本次进入的 Grass/Water 区分。
3. Grass/Water 当前附着由持续 Niagara 表达；反应消费、Boss Cleanse、死亡和 EndPlay 后及时清理，不保存或恢复瞬时 Niagara。
4. 在新附着表现和六种反应表现通过客观检查与 PIE 人工验收后，整体删除敌人旧元素状态表现三件套 `ElementAuraRing + ElementAttachmentLabel + ElementAuraLight`，包括创建、配置、更新、朝向、脉冲和测试契约；不保留新旧双轨。
5. 元素 VFX 加入首场异步预加载和 Shipping cook 投递；其他机器从正式提交可获得准确根资产及依赖。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoCombat / AREA-AbilityCombat`（新增结算结果事件）；`MOD-ReEcho / AREA-Presentation`（Host 事件装配和旧敌人文字表现删除）；文档型 `MOD-ReEchoVFX`（元素状态与反应 Niagara Catalog、生命周期和预加载）。`MOD-ReEchoPresentation` 仅 Stable Read，独立 2D Profile/FSM 公共契约不变。
- 对应模块文档：维护 `MOD-ReEchoCombat.md`、`MOD-ReEcho.md`、`MOD-ReEchoVFX.md`，均已加入 `Writes`；关闭前审阅 `MOD-ReEchoPresentation.md`。
- 设计意图：Combat 发布已经确定的反应事实，表现层只选择和管理 Niagara；持续元素状态与一次性反应分开消费 `OnElementStateChanged` / `OnElementReactionResolved`，避免从状态差异反推反应或让粒子重新计算目标。
- 权威状态与依赖：元素状态、反应规则、目标集合、半径、伤害和保存仍归 `ReEchoCombat`；主模块只拥有可丢弃 Niagara Component。`ReEcho` 继续单向依赖 `ReEchoCombat`，不增加反向依赖或新 Runtime Module。
- 决策记录：
  1. 新建 `FReEchoElementReactionResolvedEvent`，而非扩写 `FReEchoElementStateChangedEvent`；后者表达持续状态，缺少 ReactionId 和多目标拓扑。
  2. 反应事件在完整世界行为结束后只发布一次；Growth/Conduct 直接携带结算后的 `AffectedTargets` 顺序，VFX 不做范围查询。
  3. 使用独立 `FReEchoElementReactionVfxCatalog`，避免把角色变体和元素行为继续塞入连续枚举；路径映射集中且可枚举预加载。
  4. 仅当现有 Niagara 适合持续附着时删除旧文字；若不适合，停止删除阶段而不是把一次性资产强制循环。
  5. Niagara 内使用纹理/材质是正常资产依赖；禁止的是主模块继续通过 TextRender、Plane 或 Billboard 冒充元素反应。
- 相关文档同步范围：`ARCHITECTURE.md` 必审，预计模块依赖拓扑不变；`README.md` 必审，预计 AREA 路由不变；更新 `MOD-ReEchoCombat.md`、`MOD-ReEcho.md`、`MOD-ReEchoVFX.md`；审阅 `MOD-ReEchoPresentation.md`。
- 关闭前逐项填写审阅结果：在执行记录中逐项记录上述文档“已更新”或“已审阅、无需修改”及原因。

## 锁定验收

- [ ] Combat 只在有效反应完整结算后发布一次事件；普通伤害、普通附着、免疫阻止和无效命中不发布。
- [ ] 六个当前 ReactionId 映射正确，Growth/Conduct Niagara 实例只对应权威 `AffectedTargets`，顺序和数量无表现侧重算。
- [ ] Grass/Water 持续附着、Burn/Vaporize/Growth/Conduct/Enhance 瞬时表现与 Cleanse/Death/EndPlay 清理通过聚焦测试和 PIE。
- [x] `ElementAuraRing + ElementAttachmentLabel + ElementAuraLight` 旧元素状态表现三件套及其创建、配置、更新、朝向、脉冲和测试契约引用为零；武器 `NEXT`、Projectile 元素提示和 UI 图标保持不变。
- [ ] 元素玩法回归测试证明六种反应的伤害、附着、范围、连锁、免疫、强化和保存语义未变化。
- [ ] 元素 Niagara 根和准确依赖可加载、进入预加载清单并在 Shipping IoStore 中可见；不发布 zip、无关源图或整套未引用公共素材。
- [ ] 修改的 C++ 完成格式化；最终候选通过 Development `-FullRebuild`、聚焦自动化、`validate_project.py` 与 `git diff --check`。
- [ ] 用户在 PIE 验收四种怪物体型、六种反应、Player/Echo 触发、连续反应、透明排序、Restart/Travel 和战斗可读性。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或本 Plan 之外的本地脏内容。

## Step 0 门禁

- 基线分支/提交：`origin/main@a85f909e06cadd6c10b7104a2b130410887ca174`；Plan 编号基于远端最大 Plan72 后分配。
- 引擎/构建可用性：UE 5.8 安装版；当前未发现运行中的 Unreal Editor。Editor/命令前使用 Git common-dir 锁，交互式 Editor 启动前仍需确认本地脏二进制资产边界。
- 现有聚焦测试结果：Plan72 发布候选的 `ReEcho.Presentation.VFX.Catalog`、`ReEcho.Presentation.RuntimeAssetPreload` 通过；本 Plan 不复用其对新增公共事件和元素资产的证据。
- 共享契约 / 难合并资源风险：`ReEchoCombatContracts.h` 是跨模块公共 ABI；所有依赖模块必须 FullRebuild。`.uasset` 为 Exclusive 二进制，当前 `/Game/VFX/Element` 及大量依赖处于未跟踪/已修改状态，必须按实际引用闭包显式选择，禁止目录级 `git add`。
- 基线损坏时的停止条件：远端新增提交、元素 Niagara 依赖缺失/无法加载、没有适合持续附着的资产、现有资产内部运动/循环与契约冲突、或无法隔离无关本地资源时停止并报告，不删除旧表现。

## 实现提纲

1. 在 Editor 内审计 Element Niagara 的可加载性、依赖、循环、参数、Simulation Space、尺寸和目标变体，形成准确语义映射与依赖闭包。
2. 在 `ReEchoCombat` 增加反应完成事件并在完整结算后发布；用自动化锁定发布条件、一次性和目标顺序。
3. 新增元素反应 VFX Catalog；扩展 Combat VFX Component 分别管理持续附着与瞬时反应，并加入预加载枚举。
4. 验证 Grass/Water 附着、六种反应、Cleanse 和生命周期；通过后整体删除 `ElementAuraRing + ElementAttachmentLabel + ElementAuraLight` 旧表现三件套，不触及明确排除项。
5. 更新相关模块文档、导入/依赖清单和执行记录；完成 FullRebuild、聚焦自动化、静态检查、Shipping 容器检查和 PIE 人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产审计 | Unreal Editor 加载确认根 System、递归依赖、循环、参数和空间模式 | 所有正式映射可加载且用途明确 |
| C++ 格式 | 仓库 `.clang-format` 处理修改的 `.h/.cpp` | diff 仅含预期语义和格式 |
| Combat 回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Combat.Element` 及既有 ElementReaction 用例 | 六种玩法语义不变，新事件条件正确 |
| VFX 聚焦 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.VFX` | 映射、加载、目标投影、生命周期和旧文字删除契约通过 |
| 预加载 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.RuntimeAssetPreload` | 元素 Niagara 路径齐全、规范、去重 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码 0 并刷新精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与文本不变量通过 |
| Shipping | 对最新 Windows Shipping `.utoc` 检查正式 `/Game/VFX/Element` 根和依赖 | 容器包含实际运行时资产 |
| 人工 | PIE 逐一触发六种反应、四怪物、Player/Echo、Cleanse、Death、Restart/Travel | 用户记录 Passed 或明确延期 |

## 执行记录

### 变化

- 新增资源中立的反应完成事件，在有效反应全部结算后发布一次，包含前后元素与 Combat 权威目标顺序。
- 新增独立 Element Niagara Catalog；Combat VFX Component 消费持续附着与六类瞬时反应，并在死亡/EndPlay 清理。
- 首场异步预加载纳入正式 Element Niagara 根；未改变反应公式、数据表和目标选择。
- 初次资产审计只能证明 Grass/Water1 为无限循环候选；随后用户因旧表现遮挡新 Niagara、无法完成独立视觉验收，明确批准先整体删除旧三件套再继续验收。
- 设定一致性复审后，草/水附着仍无秒数并在反应、死亡或战斗结束清除；同 Stage 保留的敌人和 Player 进入局间时显式重置元素战斗态。
- Burn 改由权威三秒状态驱动绑定目标的 Fire Niagara；Growth 只消费各目标最终 Grass 附着，避免重复瞬时草；Vaporize、Conduct 与两种 Enhance 均绑定各自存活目标并使用目标自身排序。
- Conduct 事件新增 Combat 权威的发现边 `ReactionLinks`；表现层逐边把源/目标世界坐标写入 Electricity 的 `User.StartPosition / User.EndPosition`，不再把目标列表误当作独立落点特效。
- Slime、Rabbit、Fox 三套 Fire Niagara 的启用发射器已改为 Local Space，保持绑定目标后粒子随怪物移动。
- 已整体删除 `ElementAuraRing + ElementAttachmentLabel + ElementAuraLight` 的组件创建、Presentation 注入、状态订阅、更新、相机朝向、脉冲、层级修复和 Host 刷新入口；武器 `NEXT`、Projectile 元素提示和 UI 图标未改。
- Rabbit projectile 的启用发射器 `Fountain004/005` 已改为 Local Space；`CombatVfxLayerTrace` 降为 `VeryVerbose`，正常 PIE 不再以 Warning 高频刷屏。
- 新增 Development 控制台命令 `GMElement <None|Flame|Lightning|Grass|Water>` 与 `GMReaction <Burn|Vaporize|Growth|Conduct|EnhanceGrass|EnhanceWater> [damage]`；`GMElement` 持续覆盖玩家后续攻击元素直到再次指定或用 `None` 恢复武器规则，`GMReaction` 选择最近存活敌人并调用正式元素结算路径，Conduct 会为全部存活敌人准备水附着以便测试空间连锁。
- GM 暂停策略改为控制台生命周期所有：打开 GM 控制台时暂停，提交或取消并关闭控制台时继续；如果打开前已经暂停则保持原暂停。各 `GM*` 命令不再自行暂停，`GMEndEncounter/GMGotoBoss` 可在控制台暂停期间正常执行。
- 新增 `GMGod <On|Off|Toggle>`，通过 Combatant 的 Development 最终伤害门禁保护当前玩家；无敌期间伤害为零且不消费格挡，关闭后恢复正式结算，Shipping 固定不可用。
- Growth 事件半径改为已乘元素反应效能的最终生效半径；Growth/Conduct 主目标不再重复发布元素状态事件；元素 Niagara 缺失增加一次性路径告警。

### 证据

- `Saved/Logs/ReEcho.log` 的 `[Plan73Audit]`：10 个 Niagara System 加载成功，Grass/Water/Fire/Electricity 直接依赖可解析。
- 复审改为稳定 PresentationId 路由并补齐公共事件上下文/测试后，Development `-FullRebuild` 通过并刷新 7 个模块的精选预构建包；`validate_project.py` 与 `git diff --check` 通过。
- `ReEcho.Presentation.RuntimeAssetPreload` 2/2 成功；元素 Catalog 路径已进入去重后的默认清单。
- 设定一致性修复后的 Development `-FullRebuild` 通过（101 actions，7 模块预构建包刷新）；`ReEcho.StageTransition.WorldContinuity` 成功并锁定局间清除附着/强化状态。
- `ReEcho.Combat.ElementCleanseCommand`、`ReEcho.Combat.ElementReactions` 成功；ElementReaction 的新增事件与目标顺序断言无 Expected/Actual 失败，仍仅被当前 Animation2D 资源错误日志判失败。
- `ReEcho.Presentation.RuntimeAssetPreload` 再次 2/2 成功；VFX Catalog 本次新增状态驱动语义断言无失败，整体仍只被兔子 `Fountain004/005` 非 local-space 既有断言判失败。
- 新增聚焦断言覆盖普通附着零发布、Burn 恰好一次及完整身份/上下文、Conduct 权威目标顺序；`ElementReactionWorld` 新断言均未报错，但用例仍被当前工作区既有 `Animation.Idle could not resolve` Error 日志判失败。
- `ReEcho.Presentation.VFX.Catalog` 的本次新增语义断言未报错，但用例被当前脏兔子 Niagara 的两个 emitter 非 local-space 既有断言判失败。
- 三套 Fire `.uasset` 已由 Niagara 编辑器实际写盘，并重新读取确认启用发射器 `bLocalSpace=true`；`ReEcho.Presentation.VFX.Catalog` 复跑后不再出现任何 Fire local-space 失败，仍仅剩兔子 projectile 的既有 `Fountain004/005` 两项失败。
- `ReEcho.Combat.ElementCleanseCommand`、`ReEcho.Combat.ElementReactions` 在当前 Editor 内复跑 2/2 成功；Conduct 用例锁定五个目标对应四条权威发现边。
- 本轮 `validate_project.py` 与 `git diff --check` 通过；独立命令行自动化启动被本机 UE 5.8 的 LinuxArm64/VisionOS SDK `MainVersion` 平台校验阻断，因此改用当前 Editor 的 Automation Controller 完成聚焦复验。
- 旧三件套删除后 Development `-FullRebuild` 通过（102 actions，7 模块预构建包刷新）；Editor 内聚焦复验中 ElementCleanse、ElementReactions、RuntimeAssetPreload 3/3 成功，VFX Catalog 仍只剩兔子 projectile `Fountain004/005` 两项既有 local-space 失败。
- Rabbit projectile 与 GM Debug 补充后的 Development `-FullRebuild` 再次通过（102 actions，7 模块预构建包刷新）；VFX Catalog、ElementCleanse、ElementReactions 3/3 成功。
- PIE 实测 `GMReaction Burn`、`GMReaction Conduct`（2 个准备目标）及 `GMElement Water` 均执行并输出结果；最新会话 `CombatVfxLayerTrace=0`、元素 Niagara 缺失=0、Niagara Error/Warning=0。
- 全 GM 自动暂停统一化后 Development `-FullRebuild` 再次通过（102 actions，7 模块预构建包刷新）；修正了“进入命令前已经暂停时 `SetGamePaused(true)` 返回 false”的误判。
- GM 控制台生命周期暂停与 `GMGod` 补充后 Development `-FullRebuild` 通过（104 actions，7 模块预构建包刷新）；`ReEcho.GAS.DebugInvulnerability` 命令行启动仍被本机 LinuxArm64/VisionOS SDK `MainVersion` 平台校验阻断，新增断言已编译但未运行。

### 剩余风险

- 持续 Grass/Water 的 Simulation Space、尺寸、透明排序与移动跟随仍需在无旧表现遮挡的 PIE 中复验；旧表现已删除，当前仍需完成新 Niagara 的最终视觉验收。
- Catalog 仅按生产 CSV 已证明的 `Enemy.Slime / Enemy.Rabbit / Enemy.Fox` PresentationId 路由；未证明稳定身份的 Goat 不进入运行时假设或预加载清单。
- Player/Echo 当前没有专用 Fire/Water 体型资产，若其未来允许成为元素反应目标，会使用通用 Slime 回退；需在玩法范围确认或补资产后再解除该风险。
- Conduct 连线的最终宽度、持续时间、端点偏移及快速移动目标下的视觉跟随仍需 PIE 人工验收；当前代码按反应结算瞬间的权威端点驱动现有 Electricity System。

### 人工验收结果/请求

`PendingBeforeClose`：旧元素状态表现已按用户批准先行整体删除；请在无旧表现遮挡的 PIE 中验证 Grass/Water 持续附着、六反应、三种当前生产怪物、Player/Echo、Cleanse、Death、Restart/Travel。

### 架构文档审阅结果

- `MOD-ReEchoCombat.md`：已更新新增公共结果事件及权威边界。
- `MOD-ReEcho.md`：已更新 Host 消费、清理与旧文字门禁状态。
- `MOD-ReEchoVFX.md`：已更新 Catalog、生命周期及预加载职责。
- `MOD-ReEchoPresentation.md`：已审阅，无需修改；2D Profile/FSM 公共契约未变化。
- `ARCHITECTURE.md`、`CODEBASE_MAP/README.md`：已审阅，无模块依赖拓扑或 AREA 路由变化。
