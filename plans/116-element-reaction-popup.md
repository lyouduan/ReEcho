# Plan 116 - 程序 - 元素反应字世界空间弹出表现

## 协调

- Planner 负责人：Codex（Gavyn-side AI）。
- Executor 负责人：Codex（Gavyn-side AI）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@a5d269911fe359646c6ac75e7228e52314edf683`。
- 本地实现方式（可选，仅作交接说明）：`feat/element-reaction-popup`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan116-element-reaction-popup`。
- 依赖 / 阻塞：依赖 Combat 已发布的 `FReEchoElementReactionResolvedEvent`；执行 UE Editor/命令前遵守同克隆 Unreal 锁。传入 Plan 115 只新增 `plans/115-first-wave-spawn-telegraph.md`，与本 Plan 无路径、逻辑或运行时耦合。
- Writes:
  - `plans/116-element-reaction-popup.md`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoElementReactionPopupActor.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`
  - `Source/ReEcho/Private/Tests/ReEchoCombatHudTests.cpp`
  - `Content/SourceArt/UI/CombatHud/ElementReactions/`
  - `Content/ReEcho/Textures/UI/CombatHud/ElementReactions/`
  - `Content/ReEcho/Materials/UI/ElementReactions/`
  - `Content/ReEcho/UI/CombatHud/BP_ReEchoElementReactionPopup.uasset`
  - `scripts/ue/import_element_reaction_popup_assets.py`
  - `scripts/ue/author_element_reaction_popup.py`
  - `Design/UI/ReEcho_元素反应字调参指南.md`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - 精选 Win64 Editor 预构建包及 manifest（仅最终发布门禁刷新）。
- Stable Reads:
  - `Content/Data/reactions.csv`
  - `Source/ReEchoCombat/{Public,Private}/Combat/ReEchoCombatContracts.*`
  - `Source/ReEchoCombat/Private/Combat/ReEchoElementHitResolver.cpp`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoTimeShardPickupActor.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoDamageNumberActor.*`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
- 影响模式：`SharedContract`。只读消费既有 Combat 反应事件，不修改 Combat 公共结构或玩法结算；与 Enemy Presentation、Combat HUD 世界空间表现及 UE 内容资产存在集成面。
- 兼容承诺 / 下游操作：现有元素反应 Niagara、元素附着、伤害结算、伤害跳字和时间碎片拾取不改变；缺失反应字资产时只跳过文字表现并告警，不阻塞玩法。交付后美术可在 `BP_ReEchoElementReactionPopup` Class Defaults 调整尺寸、持续时间、上浮高度和透明曲线参数。
- 明确排除：不修改元素反应配表、反应伤害/范围/连锁拓扑、Niagara 资产、时间碎片拾取逻辑；本轮只审计伤害数字的 overkill 语义，不把 `AppliedDamage` 改为 `RawDamage`，除非用户另行明确确认。

## 锁定目标

当怪物完成一次有效元素反应结算时，在该反应的主目标上方弹出对应美术反应字：`Reaction.Burn → 灼烧`、`Reaction.Vaporize → 蒸发`、`Reaction.Growth → 生长`、`Reaction.Conduct → 导电`、`Reaction.Enhance → 强化`。反应字采用世界空间、始终朝向玩家相机的透明图片表现，像时间碎片完成收集时一样向上漂浮并逐渐透明，生命周期结束后自动销毁。

每个权威 `FReEchoElementReactionResolvedEvent` 只在 `PrimaryTarget` 位置生成一个反应字；Growth 的附着目标和 Conduct 的连锁目标不额外重复弹字。两种 Enhance 组合共用“强化”资产。表现不得重新推导元素组合、半径或连锁关系，也不得写回 Combat 状态。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`MOD-ReEchoPresentation`、`AREA-UI`、`AREA-Presentation`；`MOD-ReEchoCombat` 仅作为稳定事件来源审阅，不修改。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`、`MOD-ReEchoPresentation.md`，均已加入 `Writes`。
- 设计意图：把反应字保持为主模块中的可丢弃只读表现；Combat 继续只发布资源中立的 `ReactionBehaviorId` 和主目标，UI Actor 只负责资产映射与动画，Enemy Presentation 只负责订阅和生成。
- 权威状态与依赖：不新增玩法权威。Combat 事件仍是“是否发生反应、发生在哪个主目标”的唯一权威；PNG/UTexture/Material/Blueprint 只拥有视觉和可调动画参数。依赖方向保持 `ReEcho presentation adapter → ReEchoCombat contract`，Combat 不依赖 UI 或资产。
- 决策记录：
  - 使用独立 `AReEchoElementReactionPopupActor`，不把图片塞进文字型 `AReEchoDamageNumberActor`，避免字体/数字生命周期与反应图案耦合。
  - 复用时间碎片收集表现的“世界 Material Billboard + 动态 Opacity + 上浮”模式，但使用独立材质和 Blueprint 调参面，不依赖货币拾取 Actor。
  - 只消费 `OnElementReactionResolved`，不从 Hurt 颜色或当前附着反推反应，确保无伤害的 Growth/Enhance 也能显示且 Conduct 只显示一次。
  - 导入五张单体透明 PNG；`_preview_transparent.png` 仅作外部预览，不进入生产资产。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；若拓扑/索引不变，关闭时记录无需修改。更新上述三个模块文档中的世界空间反应字职责、资产路径、事件消费和调参入口；审阅 `MOD-ReEchoCombat.md`、`MOD-ReEchoVFX.md` 并记录是否无需修改。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅。
  - `README.md`：待审阅。
  - `MOD-ReEcho.md`：待更新或记录无需修改。
  - `MOD-ReEchoUI.md`：待更新。
  - `MOD-ReEchoPresentation.md`：待更新。
  - `MOD-ReEchoCombat.md`：待审阅，预期无需修改（公共事件不变）。
  - `MOD-ReEchoVFX.md`：待审阅，预期无需修改（Niagara 路线不变）。

## 锁定验收

- [x] 五种 `ReactionBehaviorId` 均映射到正确的透明反应字资产；两种强化触发共用“强化”。
- [x] 每次权威反应事件只在主目标上方生成一个反应字，Growth/Conduct 不按受影响目标重复生成。
- [x] 反应字在可调持续时间内平滑上浮并从完全不透明渐隐到透明，结束后销毁；始终正确朝向正交相机。
- [x] 持续时间、世界高度/尺寸、上浮高度与渐隐曲线至少可从 `BP_ReEchoElementReactionPopup` Class Defaults 调整。
- [x] 缺失 Blueprint、材质或单张纹理时不影响反应结算；可使用原生类 fallback 或跳过并输出明确告警。
- [x] 伤害数字现状审计有代码证据：当前显示 `AppliedDamage`，致死 overkill 被钳制到受击前剩余生命；本 Plan 不改变该语义。
- [x] C++/资产聚焦自动化、UE 5.8 Editor 构建、静态校验和最终 `-FullRebuild` 发布门禁通过。
- [x] 用户在 `Level00` 手测五类反应字的映射、尺寸、遮挡、上浮速度和渐隐观感并确认通过。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`feat/element-reaction-popup@a5d269911fe359646c6ac75e7228e52314edf683`，准确基于当时最新 `origin/main`。
- 引擎/构建可用性：项目标准 UE 5.8 安装版已在前序主线发布中完成 FullRebuild；本任务执行时重新验证。
- 现有聚焦测试结果：前序 `ReEcho.Combat.ElementReaction*` 4 个用例与 `ReEcho.UI.CombatHud.Formatting` 已通过；基线事件只发布一次、伤害跳字资产和颜色映射已有覆盖。本任务不得把这些前序结果冒充最终候选证据。
- 共享契约 / 难合并资源风险：新增独立资产路径，避免修改既有伤害数字 uasset；`ReEchoEnemyPresentationComponent.*` 和 `ReEchoCombatHudTests.cpp` 是文本共享热点，发布前需审计。UE Editor 操作使用 Git common-dir 锁。
- 基线损坏时的停止条件：反应事件未在正式敌人 Host 发布、资产透明通道损坏、Editor 导入不能生成可 Cook 纹理/材质、或必须修改 Combat 公共契约才能满足目标时，停止扩张并回报 Planner/用户。

## 实现提纲

1. 将五张批准 PNG 归档到 `Content/SourceArt/UI/CombatHud/ElementReactions/`，编写幂等 Editor 导入与 Material/Blueprint authoring 脚本。
2. 新增反应字 Actor：按 `ReactionBehaviorId` 解析纹理，构造世界 Material Billboard，动态写入 `Opacity`，按 Blueprint 默认参数完成上浮、相机朝向和销毁。
3. Enemy Presentation 绑定/解绑既有 `OnElementReactionResolved`，只对本 Host 为 `PrimaryTarget` 的事件生成一次反应字。
4. 增加映射、资产加载、默认动画与单事件单生成边界的聚焦测试；维护相关模块文档和 Plan 执行记录。
5. 执行格式化、增量构建、聚焦自动化、静态校验；人工验收通过后再执行最终 FullRebuild、远端审计、发布与清理。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 素材静态 | PNG 尺寸/Alpha 审计；Editor 脚本幂等导入与资产加载 | 五张单体图保持透明通道，映射资产均可加载；预览拼图不进入生产 |
| C++ 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format`；`git diff --check` | 格式与空白检查通过，无无关重排 |
| 构建 | `scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| UI 聚焦自动化 | `UnrealEditor-Cmd ... Automation RunTests ReEcho.UI.CombatHud` | 映射、资产、Class Defaults/fallback 与动画辅助逻辑通过 |
| Combat 回归 | `UnrealEditor-Cmd ... Automation RunTests ReEcho.Combat.ElementReaction` | 现有反应结算与单事件契约保持通过 |
| 静态 | `python scripts\validate_project.py` | 项目、资产依赖与 UTF-8 不变量通过 |
| 最终发布 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合候选的精选 Editor 包与源码指纹刷新并通过 |
| 人工 | `Level00` 分别触发灼烧、蒸发、生长、导电、强化 | 映射正确；每次一个；尺寸、遮挡、上浮与渐隐观感通过 |

## 执行记录

### 变化

- 已完成当前伤害数字语义审计：`UReEchoCombatantComponent::ApplyFinalDamage` 将实际扣血钳制到剩余生命，`FReEchoDamageEvent` 同时保留 `RawDamage/AppliedDamage`，`UReEchoEnemyPresentationComponent::HandleCombatHurt` 当前使用 `AppliedDamage` 生成跳字。
- 已归档五张生产 PNG，并新增幂等纹理导入、半透明 Material/Blueprint authoring 脚本。
- 已新增独立世界空间反应字 Actor；Enemy Presentation 只消费既有权威反应事件并按主目标生成一次。

### 证据

- 外部素材目录包含五张生产单体透明 PNG：导电 `922×898`、强化 `1161×855`、生长 `1178×789`、蒸发 `980×977`、灼烧 `950×979`，均为 32-bit ARGB。
- Plan 编号分配时发现远端已发布 Plan 115；本 Plan 顺延到 116。传入提交 `a5d269911fe359646c6ac75e7228e52314edf683` 仅新增 Plan 文档，与本任务无物理、逻辑或运行时耦合。
- 首次增量构建准确暴露两个材质参数名误用 `constexpr FName` 的 `C2131`；已改为普通命名空间 `const FName`，未改变运行时语义，等待重建确认。
- 首次纹理导入在“灼烧”后准确失败：UE Python 的纹理属性名应为 `srgb`，脚本误写为 `s_rgb`，导致其余四张纹理尚未执行导入；已修正属性名，等待重新导入与聚焦测试确认。
- 修正后 `git diff --check` 通过；`python scripts/validate_project.py` 通过 CSV/XLSX、UTF-8、项目描述符与工作流静态门禁。当前只记录静态证据，不冒充尚被另一 Editor 会话阻塞的资产/自动化验证。
- 修正后的绝对路径 Editor 导入日志逐张输出五条 `[ElementReactionPopupImport] PASS`，物理目录存在五个 `T_UI_Reaction_*.uasset`；材质/Blueprint 第二次作者ing输出 `status=preserved`，证明幂等且没有覆盖 Class Defaults。
- 增量 `Development Editor` 构建通过，预构建源码指纹为 `f9e489f2d75b`；最终发布仍将按门禁重新执行 `-FullRebuild`。
- `ReEcho.UI.CombatHud.Formatting` 1/1 通过，覆盖五种映射/纹理加载、材质 BlendMode、Blueprint 父类和渐隐辅助函数。
- `ReEcho.Combat.ElementReaction*` 4/4 通过：`ElementReactionBurnRefresh`、`ElementReactions`、`ElementReactionSaveContinuity`、`ElementReactionWorld`，既有权威反应结算未回归。
- 用户已在 `Level00` 手测并反馈“感觉没什么问题”，人工验收通过；其在 `BP_ReEchoElementReactionPopup` 保存的调参修改作为候选资产一并保留。
- 已新增 `Design/UI/ReEcho_元素反应字调参指南.md`，说明策划入口、九项 Class Defaults、渐隐指数的准确曲线语义、建议范围、常用组合、验收清单和禁止修改边界，并由 UI 总指导建立索引。
- 发布前抓取到 `origin/main@0b339df4`，新增玩家受伤碰撞/兔子参数、狐狸冲刺运行时碰撞、Plan117 狐狸箭头可见性与批量生成。与本任务的文本路径交集只有精选预构建包及 `MOD-ReEcho.md`：前者先采用传入主线版本并由最终 FullRebuild 统一重建，后者保留主线狐狸/兔子说明并合并本任务反应字表现边界。Combat 反应事件、Enemy Presentation Component 和本任务资产路径均无传入修改，未发现逻辑或运行时耦合冲突。
- `origin/main@0b339df4` 合入后的最终 `Development Editor -FullRebuild` 94/94 动作通过，UBT `Result: Succeeded`，精选预构建源码指纹刷新为 `b5beec384a1c`。
- 最终组合候选再次通过 `ReEcho.UI.CombatHud.Formatting` 1/1 与 `ReEcho.Combat.ElementReaction*` 4/4；无 Automation Error/Fatal。
- 以正式候选 `c9eb4900` 使用空 expected-value lease 原子取得 `main-publish-lock`；锁内重新 fetch 后 `origin/main` 仍为 `0b339df4` 且已是候选祖先。锁内再次执行 `Development Editor -FullRebuild`，94/94 动作通过、`Result: Succeeded`、源码指纹保持 `b5beec384a1c`；随后静态校验再次通过。
- 锁内最终候选 `b377239ced286df21621a6c0fa92c334049984a8` 已普通快进发布到 `origin/main`；发布后确认远端 main 与候选完全一致，并用准确 commit lease 删除 `main-publish-lock`。

### 剩余风险

- 交付图片含较宽透明/装饰边界，最终世界尺寸和锚点需人工在真实相机与怪物体型下验收。
- 同屏高频反应可能产生重叠；本 Plan 先保持一事件一实例，不增加合批、队列或去重时间窗。

### 人工验收结果/请求

- `Passed`：用户已在本任务 `Level00` 手测并确认表现无明显问题。

### 架构文档审阅结果

- `ARCHITECTURE.md`：已审阅；未新增 Runtime Module、公共依赖边或权威状态，拓扑不变，无需修改。
- `README.md`：已审阅；既有 `AREA-UI`、`AREA-Presentation` 与模块索引已覆盖本次代码位置，无需修改。
- `MOD-ReEcho.md`：已更新 Enemy Presentation 接线与 `AREA-UI` 的一次性元素反应字职责。
- `MOD-ReEchoUI.md`：已更新 Plan116、五类反应字映射、单主目标生成、资产路径和 Blueprint 调参入口。
- `MOD-ReEchoPresentation.md`：已更新边界说明；反应字属于主模块适配，不进入独立 Presentation Runtime Module/FSM。
- `MOD-ReEchoCombat.md`：已审阅；继续只发布既有权威反应事件，公共契约和伤害语义未改，无需修改。
- `MOD-ReEchoVFX.md`：已审阅；既有 Niagara 状态/反应语义路线不变，反应字是并行只读 UI 表现，不修改 VFX Catalog/Component，无需修改。
