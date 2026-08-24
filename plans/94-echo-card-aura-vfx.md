# Plan 94 - 程序 - 回响卡牌水草 Aura 特效

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：独立程序 Executor（Plan 发布后启动）。
- Plan 编写方（AI 侧）：`Codex planner-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@8fc70e2a1bef431553f178a825d9695c0e53e7ab`。
- 本地实现方式：用户已确认一任务一 worktree；Planner 使用 `C:\tmp\ReEcho-plan94-echo-card-aura`，Executor 必须从 Plan 发布后的最新 `origin/main` 创建本任务专属独立 worktree，不得在当前脏主工作区实现。
- 依赖 / 阻塞：依赖 Plan47 已有 `Card.EchoElementAura`、`bWaterEchoAura` / `bGrassEchoAura` 规则和 GameMode 每 2 秒、4m 元素结算；依赖现有 Echo `PresentationMotionRoot -> FlipbookRoot / EffectsRoot` 空间结构和 `UReEchoCombatVfxComponent` 生命周期。执行前必须在 Unreal Editor 内确认两个 Niagara System 的循环、Inactive Response、Auto Destroy、Simulation Space、Bounds、Renderer 和递归依赖；若不是可持续跟随 Aura，停止资产接线并请求美术修正，禁止用固定延迟或每帧重复 Spawn 模拟循环。
- Writes:
  - `plans/94-echo-card-aura-vfx.md`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEchoActor.*`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`
  - `Source/ReEcho/Private/Tests/**` 中本 Plan 新增的 Echo Aura 世界生命周期聚焦测试
  - `Content/VFX/Echo/Particle/NS_Echo_Water.uasset`
  - `Content/VFX/Echo/Particle/NS_Echo_Grass.uasset`
  - `Content/VFX/**`、`Content/Mat/**`、`Content/00_Textures/**`、`Content/01_Textures/**` 中由上述两个 System 实际引用且经审计具名批准的准确递归依赖；禁止目录级整体纳入
  - `Design/Art/VFX/combat_vfx_import_manifest.csv`、`scripts/art/import_combat_vfx.py`、`scripts/art/test_import_combat_vfx.py`（仅当两个正式根需要进入现有可复现导入契约时）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`（审阅并记录现有规则快照成为新表现消费者；正文仅在契约事实变化时修改）
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终精选预构建包文件
- Stable Reads:
  - `Content/Data/cards.csv` 的 `G_2_07`、`G_2_08`
  - `Content/Data/card_effects.csv` 的 `G_2_07_AURA`、`G_2_08_AURA`
  - `Source/ReEchoCards/{Public,Private}/Cards/ReEchoCard{Types,Runtime}.*`
  - `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/**`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `plans/47-card-build-runtime-module.md`
  - `plans/71-echo-player-presentation-parity.md`
  - `plans/72-first-encounter-asset-preload.md`
  - `plans/89-character-enemy-vfx-audit-and-fixes.md`
- 影响模式：`SharedContract`（跨 Card 规则只读消费、Echo Host 空间挂点和 VFX Catalog/生命周期；不改变 Cards 或 Combat 公共玩法契约）。
- 兼容承诺 / 下游操作：`G_2_07` / `G_2_08`、BehaviorId、2 秒脉冲、400cm 范围、元素附着/反应顺序、存档和回放语义保持不变；Niagara 缺失、编译失败、被裁剪或提前结束只能缺视觉，不得改变元素结算。水草同时拥有时每个存活 Echo 恰好各一个 Aura 实例。
- 明确排除：不修改 XLSX/CSV、卡牌数值、元素规则、伤害、范围、频率或反应顺序；不把元素状态写入 Echo Combatant；不让 Niagara 碰撞、完成回调或粒子参数驱动玩法；不复用 Attack/Hurt 前景排序；不在 Editor 外修改 `.uasset`；不整体提交当前主工作区的大量未跟踪 VFX/纹理；不由 AI 代替用户判断最终尺寸、中心感和遮挡质量。

## 锁定目标

1. 拥有 `G_2_07 潮汐回响` 时，每个存活 Echo 身上持续显示 `/Game/VFX/Echo/Particle/NS_Echo_Water.NS_Echo_Water`；拥有 `G_2_08 森林回响` 时持续显示 `/Game/VFX/Echo/Particle/NS_Echo_Grass.NS_Echo_Grass`；同时拥有两张卡时两个效果并存且不因 Tick 重复创建。
2. Aura 的空间中心与 Echo 当前角色渲染的稳定视觉中心重合，而不是 Actor 原点、碰撞中心或脚点。角色/Flipbook 始终渲染在两个 Aura 上层；Aura 排序从当前角色动画实际 `TranslucencySortPriority` 动态派生，不使用固定全局值。
3. 新 Echo 生成/初始化当帧取得正确 Aura；卡牌规则变化时幂等刷新；卡牌失效、Echo Death、`EndPlay`、遭遇清理和重开时停用并销毁实例，不残留、不重复。
4. Aura 仅作为现有卡牌规则的可丢弃视觉消费者。现有每 2 秒、4m 敌人水/草附着及元素反应继续由 Cards、GameMode 和 Combat 权威链执行，视觉加载失败不改变玩法。
5. 两个 System 及准确递归依赖进入正式版本、首场预加载和 Shipping cook 投递；其他机器从提交即可获得可加载的完整资产，不夹带无关素材。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoCards / AREA-Cards` 仅提供现有只读 `FReEchoCardRuleSnapshot`；`MOD-ReEcho / AREA-Run` 负责从规则真相向当前 Echo Host 同步表现状态；`MOD-ReEcho / AREA-Presentation` 与文档型 `MOD-ReEchoVFX` 负责 Aura 语义目录、专用挂点、实例、空间、排序和生命周期。`MOD-ReEchoPresentation` 只读提供当前 Flipbook/动画渲染 Transform、Bounds 与排序事实；`MOD-ReEchoCombat` 不变。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoVFX.md`，均已加入 `Writes`；审阅 `MOD-ReEchoCards.md`，只有新增表现消费者改变其公开说明时才更新正文；审阅 `MOD-ReEchoPresentation.md`、`MOD-ReEchoCombat.md` 并在执行记录说明无需修改或具名变化。
- 设计意图：玩法规则只表达“本局是否拥有水/草 Echo Aura”，主模块 Host 把规则同步为资源中立的表现开关，集中 VFX 适配层选择 Niagara 并拥有可丢弃实例。角色视觉中心与动态透明排序来自现有 Presentation 权威，不复制一套固定偏移或固定层级。
- 权威状态与依赖：Cards 继续拥有卡牌构筑和派生规则；Combat 继续拥有元素附着/反应；Echo/Presentation 继续拥有角色视觉 Transform/Bounds/排序；VFX 只拥有两个 Niagara Component 及其缓存启用态。不新增 Runtime Module，不改变依赖方向，不持久化 Aura 实例。
- 决策记录：
  1. 为 Echo Aura 增加独立 Catalog 语义和两个独立实例，不把完整资产路径写进 GameMode/EchoActor，也不以单一枚举互斥水草。
  2. 增加专用 `EchoAuraVfxRoot`，不复用 `AttackVfxRoot` / `HurtVfxRoot`；Aura 属于角色背景状态层，攻击/受击仍走角色前景层。
  3. `EchoAuraVfxRoot` 位于 `EffectsRoot` 分支以避免继承 Flipbook 朝向翻转；其位置依据当前动画组件的稳定视觉 Bounds 中心转换到挂点局部空间。若现有 Profile 已提供稳定中心契约，Executor 可复用；不得以 Actor 零点或脚点替代。
  4. Aura Priority 使用 `OwnerAnimationPriority - 1` 的专用背景策略，并在 Clip/Profile 切换后刷新；不得调用现有把战斗特效置于宿主前景的 `ResolveOwnerSortPriority()`。若 Niagara 多 Renderer 无法服从组件 Priority，须在 Editor 内修正资产 Renderer 排序契约并记录准确资产差异。
  5. GameMode 使用独立 `RefreshEchoCardAuraPresentation(Rules)` 同步全部 Echo，不把视觉刷新嵌入敌人遍历；Echo 初始化完成后主动同步一次，常规 Tick/规则变化继续幂等兜底。
  6. 两个资产当前只存在于主工作区未跟踪候选中。Executor 必须先审计根 System 与递归依赖、记录准确 include/exclude 清单并隔离复制到任务 worktree；禁止从主工作区整体复制 `Content/VFX/Echo` 或公共纹理库。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`，预计拓扑和 AREA 路由不变；维护 `MOD-ReEcho.md`、`MOD-ReEchoVFX.md`；审阅/按事实维护 `MOD-ReEchoCards.md`；审阅 `MOD-ReEchoPresentation.md`、`MOD-ReEchoCombat.md`。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待审阅。
  - `shared/CODEBASE_MAP/README.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：待维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`：待审阅/按事实维护。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`：待审阅。

## 锁定验收

- [ ] `G_2_07`、`G_2_08` 分别驱动 Water/Grass Aura；无卡、仅水、仅草、水草同时拥有时，每个存活 Echo 的对应活动实例数量精确为 `0/1/1/2`，重复 Tick/同步不增长。
- [ ] 新 Echo 初始化当帧获得正确 Aura；规则关闭、Death、`EndPlay`、遭遇结束、Restart/Travel 后实例全部清理，无幽灵粒子或重复组件。
- [ ] Aura 中心使用角色当前稳定视觉 Bounds/Profile 中心；Idle、Walk、Attack、左右朝向、角色切换、缩放和空间校准时，角色渲染稳定处于两个 Aura 中心，不随单帧透明像素 Bounds 抖动。
- [ ] Water/Grass Aura 始终位于所属角色 Flipbook 下层，Priority 动态等于所属角色当前动画 Priority 的背景层；攻击/受击 VFX 原前景层不回归。多个 Echo 交错和场景动态脚点排序下不存在 Aura 错压自身或其他角色的明显错误。
- [ ] 两个 Niagara 经 Editor 确认为持续循环、附着跟随、合适 Simulation Space/Bounds/Renderer；加载失败仅产生限频诊断且玩法链继续。
- [ ] 现有 `G_2_07/G_2_08 -> Card.EchoElementAura -> 2s pulse -> 400cm -> ResolveElementHit` 回归通过；未改卡牌、Combat 或存档公共语义。
- [ ] 两个正式根及准确递归依赖均被 Git 跟踪、可加载、进入预加载清单，并在最新 Shipping IoStore 中可见；不包含 zip、源图、测试场景或无关公共素材。
- [ ] 修改的 C++ 完成 `.clang-format`；最终候选通过 Development `-FullRebuild`、聚焦自动化、`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check`。
- [ ] 用户在 PIE 验收水、草、水草并存、多个 Echo、全部当前角色动画/朝向、Death/Restart/Travel 的中心、层级、尺寸、可读性和整体观感后，人工验收才可设为 `Passed`。
- [ ] 仅显式暂存本 Plan 具名路径；未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或主工作区无关未跟踪资产。

## Step 0 门禁

- 基线分支/提交：Executor 从包含本 Plan 的最新 `origin/main` 创建专属 `codex/plan94-echo-card-aura-vfx` worktree；规划基线为 `origin/main@8fc70e2a1bef431553f178a825d9695c0e53e7ab`。
- 引擎/构建可用性：UE 5.8 安装版；执行 Editor 资产审计、自动化或构建前请用户保存并关闭交互式 Editor，并取得 Git common-dir Unreal 锁。最终候选必须 `-FullRebuild` 刷新准确精选预构建包。
- 现有聚焦测试结果：Executor 在零实现基线上运行 `ReEcho.Presentation.VFX`、`ReEcho.Presentation.RuntimeAssetPreload` 和 Card Echo Aura 相关聚焦测试；记录真实基线失败，禁止把修复后结果倒写为基线通过。
- 共享契约 / 难合并资源风险：主工作区存在大量未跟踪 VFX/纹理，两个 Echo Niagara 也未跟踪；`.uasset` 不可语义合并。复制前记录每个源文件、目标文件、依赖和哈希，目标已有不同内容时停止，不覆盖。C++ 会修改 `ReEchoGameMode`、Echo Host 和集中 VFX 适配，需审计 Plan89/90 及远端后续提交的同行重叠。
- 基线损坏时的停止条件：两个 Niagara 无法加载/编译、不是可持续循环 Aura、递归依赖无法隔离、角色稳定视觉中心无法从现有 Presentation 契约取得、Renderer 无法满足角色上层排序、现有 VFX/Cards 聚焦基线存在阻断性失败，或实现需要修改卡牌/Combat 公共玩法契约时，停止越界部分并回报 Planner。

## 实现提纲

1. 在独立 Executor worktree 建立零实现基线；只读审计主工作区两个根 System，通过 Unreal Asset Registry/Editor 取得递归依赖、循环、Auto Destroy、Simulation Space、Bounds、Renderer 和排序能力，形成准确 include/exclude 清单。
2. 为 Water/Grass 增加集中 VFX 语义、完整路径和预加载枚举；增强资产加载与预加载自动化。精确复制并跟踪两个正式根及批准依赖，不批量纳入目录。
3. 在 Echo Host 增加 Blueprint 可编辑的 `EchoAuraVfxRoot`，由稳定角色视觉中心更新其位置；扩展 Combat VFX Component 配置第三挂点、缓存两个启用状态、分别创建/停止两个循环 Niagara，并使用角色动态 Priority 的专用背景策略。
4. 在 GameMode 增加独立规则到 Echo 表现同步函数；Echo 初始化主动同步，常规更新幂等兜底；Death、`EndPlay`、遭遇清理和规则关闭统一清理。
5. 增加无卡/单卡/双卡、重复同步、生成、状态关闭、Death/EndPlay、缺资产降级、角色中心和动态排序的聚焦测试；回归现有每 2 秒、400cm 元素玩法链。
6. 维护相关模块文档和执行记录；完成格式化、聚焦自动化、`-FullRebuild`、项目/预构建/diff 检查，并验证最新 Shipping IoStore 实际包含两个 System 与依赖。
7. Planner 评审准确 diff、资产 include/exclude 清单、自动化和架构文档；用户在 PIE 完成中心、层级、尺寸和生命周期验收后才可关闭并发布实现。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产审计 | Unreal Editor/Asset Registry 检查两个根的依赖、编译、循环、Inactive Response、Auto Destroy、Simulation Space、Bounds、Renderer | 两个根适合作为持续附着 Aura，准确依赖闭包可隔离且无 Niagara 编译错误 |
| Git 资产边界 | `git status --short`、`git ls-files`、根与递归依赖准确清单/哈希 | 仅具名根和实际依赖进入候选，无目录级夹带 |
| C++ 格式 | 仓库 `.clang-format` 处理所有修改的 `.h/.cpp` 并审阅 diff | 格式与语义差异准确 |
| VFX 聚焦 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.VFX` | Catalog、路径加载、幂等实例、背景排序、中心同步和生命周期通过 |
| 预加载 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.RuntimeAssetPreload` | Water/Grass Aura 路径进入完整、规范、去重的预加载集合 |
| Cards/世界回归 | Card Echo Aura 与 GameMode 世界聚焦自动化 | 无卡/单卡/双卡视觉状态正确，原 2 秒/400cm 水草附着语义不变 |
| 完整构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UE 5.8 UHT/UBT 成功并刷新准确精选预构建包 |
| 项目 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目边界、源码指纹、精选包与文本检查通过 |
| Shipping | 对最新 Windows Shipping `.utoc` 检查 `/Game/VFX/Echo/Particle/NS_Echo_{Water,Grass}` 及依赖 | IoStore 实际包含运行时完整资产 |
| 人工 PIE | 水/草/双卡、多 Echo、Idle/Walk/Attack、左右朝向、角色切换、缩放、Death、Restart/Travel | 用户确认角色位于 Aura 中心且始终在 Aura 上层，尺寸与整体观感通过 |

## 执行记录

### 变化

- 2026-08-24：创建 Plan。锁定 `G_2_07/G_2_08` 驱动 Echo 身上 Water/Grass 常驻 Niagara；玩法 2 秒/400cm 元素结算不变。
- 2026-08-24：根据用户补充，将“角色稳定视觉中心与 Aura 中心重合”和“角色始终渲染在 Aura 上层”升级为锁定目标与 `PendingBeforeClose` 人工验收；Aura 使用动态背景排序，不复用战斗 VFX 前景策略。

### 证据

- 规划审计确认生产 CSV 已把 `G_2_07/G_2_08` 编译为 `bWaterEchoAura/bGrassEchoAura`，GameMode 已用其执行每 2 秒、400cm 的 Water/Grass `ResolveElementHit`；缺口仅为 Echo 自身 Aura 表现。
- 主工作区确认两个候选根位于 `Content/VFX/Echo/Particle/NS_Echo_Water.uasset` 与 `NS_Echo_Grass.uasset`，但当前未被 Git 跟踪；循环和递归依赖尚未由 Editor 验证，列为 Step 0 门禁。
- `origin/main@8fc70e2a` 相对本地主工作区 `cd89afb9` 的传入范围仅为 Plan86 Settings UI、Settings 测试/源码、相关 WBP/纹理、脚本与精选预构建包；与本 Plan Cards/Echo/VFX 源码和两个未跟踪资产无直接路径重叠。用户在获知该范围后要求执行，本 Plan 采用最新远端基线，不修改或合并当前脏主工作区。

### 剩余风险

- 两个 Niagara 是否无限循环、是否围绕组件原点居中、Renderer 是否服从组件 Priority、Fixed Bounds 是否足够以及准确依赖闭包均待 Executor 在 Editor 内确认。
- 多 Echo 动态脚点排序下，单纯 `OwnerPriority - 1` 可能仍与其他角色的排序区间交错；Executor 必须用实际 PIE 证据验证，并在不改变“自身角色始终压住自身 Aura”的前提下集中调整排序带策略，不能使用单个固定常量补丁。

### 人工验收结果/请求

`PendingBeforeClose`：实现完成后由用户在 PIE 验收中心、上下层、尺寸、双卡叠加、多 Echo 交错和生命周期。

### 架构文档审阅结果

- 待 Executor/Planner 在最终候选逐项填写。
