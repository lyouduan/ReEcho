# Plan 121 - 程序 - 四种怪物出生动画资产接入

## 协调

- Planner 负责人：当前程序 Planner。
- Executor 负责人：当前独立 Executor。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Codex`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@7af87dc0`（Plan 121 初版基线为 `1544d988`；后续公共语义与 Born gameplay gate 修订均在发布前审计远端并前移）。
- 本地实现方式（可选，仅作交接说明）：按当前对话确认使用本任务专属 worktree；不得在主工作区直接实现。
- 依赖 / 阻塞：源序列位于 `F:/MiniGame/兔子出生/`、`F:/MiniGame/史莱姆出生/`、`F:/MiniGame/好羊出生序列/`、`F:/MiniGame/好狐狸出生序列/`；执行机必须可读取这些目录并可独占运行 Unreal Editor/Commandlet。Plan114/117/122 已修改主模块、Enemy Host/Presentation、模块文档与精选预构建包；本 Plan 实现需从最新 main 组合适配并重建，不得恢复旧音频、狐狸箭矢、诊断、文档或二进制。
- Writes: `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/ReEcho2DAnimationTags.{h,cpp}`、`Source/ReEchoPresentation/Private/Presentation/Animation2D/ReEcho2DPresentationCatalog.cpp`、`Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.{h,cpp}`、`Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.{h,cpp}`、`Source/ReEcho/Private/Tests/{ReEcho2DAnimationTests,ReEchoEnemyHostTests}.cpp`、`Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h`、`Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp`、`Source/ReEchoEnemies/Private/Tests/ReEchoEnemyLogicTests.cpp`、`Content/ReEcho/DataAsset/Common/Animation2D/SM2D_DefaultCharacter.uasset`、`Content/ReEcho/Art/Animation2D/Enemies/{Rabbit,Slime,Goat,Fox}/Born/**`、`Content/ReEcho/Art/Animation2D/Enemies/{Rabbit,Slime,Goat,Fox}/Flipbooks/*Born*.uasset`、`Content/ReEcho/DataAsset/Enemy/Profiles/{DA_Enemy_RabbitDoll,DA_Enemy_Slime,DA_Enemy_GoatPriest,DA_Enemy_Fox}.uasset`、`scripts/ue/audit_plan82_animation_assets.py`、`shared/CODEBASE_MAP/modules/{MOD-ReEchoPresentation,MOD-ReEcho,MOD-ReEchoEnemies}.md`、本 Plan 执行记录。
- Stable Reads: `plans/82-animation2d-asset-cleanup.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`、`Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/**`、四种怪物现有动画目录与 Profile、`scripts/ue/audit_plan82_animation_assets.py`、相邻导入/修复脚本。
- 影响模式：`Exclusive`。
- 兼容承诺 / 下游操作：新增 `Animation.Born` 为可选公共表现语义；仅当 Born 成功启动时进入 Born Gameplay Gate。Gate 期间怪物 `CanBeDamaged=false`、不接受任何伤害且 EnemyLogic 不产生位移、不得提交普通攻击/特殊技/Boss 攻击，也不得启动 Phase2 Transform；Born 缺失/播放失败不进入 Gate，自然完成后原子恢复可伤害、移动、攻击与 Phase2 许可。出生提交、碰撞、目标选择、普通 AI 感知和计时、位置、预警和存档保持既有语义；Gate 期间不因伤害触发 Death。保留源 PNG，不覆盖四种怪物的其他动画与 DA 字段。
- 明确排除：`BadRabbit/BadSlime/BadGoat/BadFox`、出生预警 VFX、数值表、地图、音频、玩家/Echo 的 Born 绑定及其他角色资产；不让动画完成回调控制怪物可伤害、碰撞、AI 激活或任何玩法状态。

## 锁定目标

把用户提供的四组顺序 PNG 按自然数字顺序导入为 Paper2D Texture/Sprite，并分别组装为兔子、史莱姆、好羊、好狐狸的非循环 Born Flipbook。新增原生 `Animation.Born` 公共表现语义和共享状态机状态；每种怪物的帧资产位于其 `Born` 目录，生成的 Born Flipbook 位于该怪物现有 `Flipbooks` 目录，对应 Enemy Presentation Profile 的 Born Clip 明确引用该 Flipbook。生产怪物完成 Definition/Profile 装配并成功开始 Born 后进入无敌、不可移动且不可攻击的 Born Gameplay Gate；Born 完整结束后才恢复受伤、移动与攻击，并允许 EnemyLogic 触发 Phase2 Transform。Born 播放期间每帧以当前 Sprite 底边中心对齐既有 GroundShadow 脚点，避免序列帧尺寸变化造成悬空或下沉。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoPresentation`、`MOD-ReEchoEnemies`、`MOD-ReEcho` / `AREA-Presentation`、`AREA-Enemies`；Presentation 注册/播放 Born 并提供当前播放状态，主模块 Enemy Host Adapter 将 Born 门禁注入 EnemyLogic 的 Phase2 许可输入，Enemies 仍独占玩法状态推进。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`、`MOD-ReEchoEnemies.md` 与 `MOD-ReEcho.md` 必须加入 Writes，记录 Born 完成门禁、逐帧脚点及 Host/Logic 边界。
- 设计意图：保持“Profile 决定具体表现、Host 拥有 Actor 可伤害状态、EnemyLogic 决定行为/Phase2”的边界；Presentation 只提供 Born 是否成功启动/仍活跃，Host 据此持有明确 Born Gameplay Gate，并把移动/Phase2 类型化许可交给 EnemyLogic，不延迟出生提交或暂停普通 AI 计时。
- 权威状态与依赖：Presentation 模块独占 Born 播放状态与逐帧脚点；ReEcho Host 独占 Born Gameplay Gate 与 `CanBeDamaged` 切换，并写入类型化移动/Phase2许可；ReEchoEnemies 继续独占移动意图、Phase2 判定与状态推进。不存在 Presentation → Enemies 反向依赖，模块依赖方向不变。
- 决策记录：`兔子/史莱姆/好羊/好狐狸` 映射为 `Rabbit/Slime/Goat/Fox`，不是 `Bad*` 目录；兔子 6 帧，其余各 5 帧；已通过 Unreal 回读证明 Rabbit 现有 Born 惯例为 4 FPS、Center Pivot、PPU 1、Translucent，四组统一沿用。2026-08-26 用户在获知 C++/公共契约/FullRebuild 风险后明确确认扩展 Plan，拒绝仅交付未绑定资产的降级方案。
- 相关文档同步范围：更新 `MOD-ReEchoPresentation.md`、`MOD-ReEchoEnemies.md` 与 `MOD-ReEcho.md`；`ARCHITECTURE.md` 和 `README.md` 关闭前审阅，预计无需修改，因为拓扑和稳定路由不变。
- 关闭前逐项填写审阅结果：待 Executor/Planner 根据最终候选补充。

## 锁定验收

- [ ] `Rabbit/Slime/Goat/Fox` 各自存在 `Born` 目录，源 PNG 对应的 Texture 与 Sprite 数量分别为 6/5/5/5，命名稳定且帧序正确。
- [ ] 四个非循环 Born Flipbook 位于各自 `Flipbooks` 目录，逐帧引用对应 Sprite，不跨怪物借用资源。
- [ ] 四个生产 Enemy Profile 的 `Animation.Born` Clip 分别引用本怪物 Born Flipbook；资产保存后重新加载仍保持引用，缺失资源不会影响玩法生成。
- [ ] `Animation.Born` 以原生 GameplayTag 注册并存在于共享 FSM；它是锁至完成、仅 Death 可抢占的非循环瞬时表现，完成后回到 Move，Death 保持最高优先级/终结独占。
- [ ] 新怪物完成 `ConfigureFromDefinition` 的 Profile 装配后尝试一次 Born；缺少 Born 的 Profile 安全 no-op，生成成功、碰撞、普通 AI/Encounter 提交不依赖播放结果；可伤害和移动只在 Born 成功启动时受 Gate 控制。
- [ ] Born 正常播放期间，即便已满足攻击次数、距离或血量等 Phase2 条件，也不得启动 Transform；Born 完整结束后的下一次合格逻辑步才可启动。Born 缺失或播放失败不制造永久门禁，Death 仍可立即抢占。
- [ ] 只有成功启动 Born 的怪物在动画期间 `CanBeDamaged=false`，任何伤害不扣血、不积累 Phase2 攻击次数、不触发受击/Death；Born 完成后恢复原本可伤害状态。缺失或失败 Born 不获得无敌。
- [ ] Born Gameplay Gate 期间 EnemyLogic 不产生位移且不提交普通攻击、特殊技或 Boss 攻击，Host 世界位置保持不变且不会对目标造成攻击伤害；动画完成后的下一合格逻辑步恢复移动和攻击。目标选择、感知与既有冷却/计时继续推进，不因本 Plan 暗中暂停。
- [ ] Born 每帧使用当前 Sprite Bounds 计算主体底边与阴影位置；帧尺寸变化时底部持续贴合 GroundShadow，普通 Move/Attack 与既有 Death authored-pivot 逻辑不变。
- [ ] 不修改 `Bad*`、其他动画、出生预警/生成提交时序、Schema、表格或无关 DA 字段；C++ 差异仅限 Writes 中的 Born 语义/脚点、Enemy Host 许可注入和 EnemyLogic Phase2 门禁。
- [ ] `audit_plan82_animation_assets.py`（必要时以只读扩展检查覆盖新 Born）、`python scripts/validate_project.py`、资产加载检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 中观察四种怪物的出生动作：帧序、速度、尺寸、脚点、朝向、首帧闪烁、结束后回到基础循环均可接受。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@7af87dc0`；Plan114/117/122/124/125 已审计。Plan117/122 与 EnemyActor、EnemyPresentation、测试、模块文档和预构建包存在同路径组合耦合，本 Plan 必须在最新 main 上保留其狐狸箭矢与诊断语义后重建；无产品目标冲突。
- 引擎/构建可用性：执行前检查 Git-common-dir Unreal 锁与 `UnrealEditor` 进程；任何需要关闭 Editor 的命令先请用户保存并关闭。
- 现有聚焦测试结果：Executor 的 UE 回读证明当前基线未注册 `Animation.Born`、四个生产 Profile 均无 Born Clip；Rabbit 旧 Born 为 4 FPS、6 帧、Center Pivot、PPU 1、Translucent。该事实触发原 Plan 停止条件，用户已确认扩展。
- 共享契约 / 难合并资源风险：四个 Profile 与 Paper2D `.uasset` 为二进制独占写面；发布前必须对远端同路径变化做物理及逻辑审计，禁止静默覆盖。
- 基线损坏时的停止条件：源帧不可读/尺寸或透明通道异常、Controller/FSM 无法在不改变玩法状态的情况下实现 Born 完成归宿、Editor/锁被占用、或远端修改同一 C++/Profile/目标目录时停止并报告。

## 实现提纲

1. 注册原生 `Animation.Born`，在共享 FSM 增加优先级 90、锁至完成、仅低于终结 Death 的 Born 状态并定义完成回到 Move；增加 Controller 单元测试覆盖 Attack/Hit/Transform 不可抢占、自然完成归宿、缺失 Clip no-op 与 Death 抢占。
2. 在 Enemy Presentation Host Adapter 暴露一次性 Born 请求和只读播放状态，并在 `ConfigureFromDefinition` 完成 Profile 装配后调用；只有播放成功才激活 Host Born Gameplay Gate 并关闭 `CanBeDamaged`。Host 在 Gate 结束时原子恢复可伤害，并把移动/攻击/Phase2 类型化许可注入 EnemyLogic；Gate 期间不得提交普通攻击、特殊技或 Boss 攻击，但不得用 Gate 控制出生提交、碰撞、目标选择、普通 AI 感知或既有计时推进。
3. Born 活跃时，Enemy Presentation 的脚点与 GroundShadow 宽度/中心使用当前 Sprite Bounds；Death authored pivot 保持既有专用路径，其他语义继续使用 Flipbook 聚合 Bounds。
4. 在专属 Executor worktree 回读四个 Profile、现有 Rabbit/Slime Born 资产、同类 Flipbook 的循环/帧率/材质/像素密度与导入设置；冻结准确替换和新增清单。
5. 通过 Unreal Editor Python/仓库脚本复制源 PNG 并导入 Texture，按项目 Paper2D 惯例创建逐帧 Sprite；不得在 Editor 外生成或手改 `.uasset`。
6. 在四个怪物的 `Flipbooks` 目录创建 4 FPS 非循环 Born Flipbook，按自然数字顺序装帧；只修改四个生产 Profile 的 Born Clip 绑定，保存重载并回读其余字段。
7. 更新三份模块文档和执行记录，运行验证矩阵并交由 Planner 审查源码/二进制范围；PIE 人工验收通过前保持 `Review`。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源输入 | 读取四组 PNG 的数量、自然顺序、尺寸、RGBA/透明通道 | 分别为 6/5/5/5，输入无缺帧且可解码 |
| 资产结构 | Unreal Python/Asset Registry 回读 Texture、Sprite、Flipbook、Profile | 数量、路径、帧序、非循环与四个 Born 引用准确，保存重载后一致 |
| 聚焦审计 | `python scripts/ue/audit_plan82_animation_assets.py`（必要时使用 Plan 内新增只读审计） | 四个生产 Profile 与源贴图导入链通过 |
| C++ 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.Animation2D` | Born tag/状态、完成归宿、缺失 Clip no-op、Death 抢占与既有表现测试通过 |
| Phase2 门禁 | 聚焦 EnemyLogic/Host 自动化 | Born 活跃时三类 Phase2 条件均不启动，完成后启动；缺失/失败不锁死，Death 可抢占 |
| 无敌/移动/攻击 | 聚焦 EnemyLogic/Host/Combat 自动化 | Born 成功才进入无敌；Gate 内伤害为零且无 Hurt/Death/计数，移动意图为零，普通攻击/特殊技/Boss 攻击均无提交与伤害；感知和计时仍推进；完成后恢复，缺失/失败不锁死 |
| 脚点 | 聚焦 Animation2D/Enemy Presentation 自动化与 UE 资产回读 | Born 逐帧底边对齐阴影，普通动画与 Death authored pivot 回归通过 |
| 完整构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新匹配最终源码的精选预构建包 |
| 项目静态 | `python scripts/validate_project.py` | 项目/资产规则通过 |
| 差异卫生 | `git status --short`、精确路径清单、`git diff --check` | 仅 Plan Writes 与获准的导入资产，无机器产物/意外删除 |
| 人工 PIE | 逐个生成 Rabbit/Slime/Goat/Fox | 出生动作视觉可接受并正常回到基础循环 |

## 执行记录

### 变化

- 注册原生 `Animation.Born` 并补入编辑器语义解析；共享 FSM 新增优先级 20、完成回到 Move 的非终结 Born 状态。
- Enemy Host 在 Definition/Profile 装配后调用 `TryPlayBorn`，忽略播放结果，不改变已经提交的玩法状态。
- Rabbit 旧 Born 资产经 Unreal 规范到 `Born/{Textures,Sprites}`；Slime/Goat/Fox 新增同结构资产。四组 4 FPS 非循环 Flipbook 与四个生产 Profile 的默认 Born Clip 已绑定。
- 最小必要偏差：更新后的初始 Writes 漏列 `ReEcho2DPresentationCatalog.cpp`；Planner 已确认将 Born 加入既有候选数组属于已授权公共语义目标且不新增 API。本执行记录及 Writes 已补记。

### 证据

- 四组源序列可读取且为 32 位 ARGB PNG，帧数为 6/5/5/5。
- 独立 Unreal 重载审计：`enemies=4 textures=21 sprites=21 flipbooks=4 profiles=4 issues=0`；四组均为 4 FPS、自然帧序、非循环绑定，FSM Born 完成归宿为 Move。
- 增量 Editor Build 两轮通过（首次 97 actions；语义解析补齐后 4 actions）。最终 FullRebuild 97 actions 通过，刷新 7 个精选模块，源码指纹 `6b0f55f586aa`。
- `ReEcho.Presentation.Animation2D` 实际执行 4 个测试：FootpointAlignment、CookedDeathPivotPolicy、StunPause 通过；AssetProfiles 的 Born 播放、完成回 Move、缺失 no-op 与 Death 抢占断言未报错，但被既有 TimeGuard Phase2 空能力断言（第 484 行）单独阻塞。
- Plan82 全库审计未报告 Born/FSM/Profile 问题，但被本 Plan 明确排除的 BadRabbit 20 张既有未导入源图阻塞；Plan121 独立重载审计提供本任务资产链通过证据。
- `python scripts/validate_project.py` 通过；`python scripts/ue/prebuilt_editor.py check` 通过（7 模块，Build ID `55116800`，源码指纹 `6b0f55f586aa`）；`git diff --check` 通过。
- Planner 集成前远端前进到 Plan120：Fox/Rabbit/Slime 三个 Profile 与其 Phase2 Transform 发生二进制同文件重叠。集成候选先保留最新 main 的 Transform 版本，再通过 Unreal 只追加 Born；独立二次重载审计 `profiles=3 born=3 transform=3 issues=0`，并逐项确认其他 Move/Attack/Hit/Death Clip 仍存在。该组合使 Executor 旧 FullRebuild/资产证据失效，以下最终证据以 Planner 集成候选重跑结果为准。
- Planner 最终组合候选 FullRebuild 94/94 成功；`prebuilt_editor.py check` 通过（7 模块，Build ID `55116800`，源码指纹 `6b0f55f586aa`），`validate_project.py` 与 `git diff --check` 通过。聚焦 Animation2D 自动化在组合候选实际运行 4 项，结果仍为 3 项通过、AssetProfiles 仅受既有 TimeGuard Phase2 空能力断言阻塞，Born 新增断言未失败。
- 完成门禁修订执行：Presentation 增加只读 `IsBornPlaying()`；Host 将其转换为 `FReEchoEnemySenseSnapshot::bPhase2TransitionPermitted`，EnemyLogic 仅在许可关闭时跳过新 Phase2 启动。Born 活跃时的致命伤不再被截获为 Transform，Combat 正常进入 Death，普通 AI/碰撞/受伤路径未增加门禁。
- Born 活跃期间脚点与 GroundShadow 改用当前 Sprite Bounds；`bUseAuthoredDeathPivot` 的判断继续显式要求 Death 活跃，普通语义仍使用 Flipbook 聚合 Bounds。
- 修订后聚焦自动化：`ReEcho.Enemies.Logic` 全部通过，新增 `Phase2.BornPermit` 覆盖攻击次数、距离、血量三类条件的延迟与放行；`ReEcho.Enemies.Host` 全部通过，新增 typed permit 默认开放契约；`ReEcho.Presentation.Animation2D` 的 FootpointAlignment、CookedDeathPivotPolicy、StunPause 通过，AssetProfiles 仍仅被既有 TimeGuard Phase2 空能力断言（第 484 行）阻塞，Born 活跃状态、完成解除与 Death 抢占新增断言未报错。
- 修订候选最终 `Build-Editor.cmd -Configuration Development -FullRebuild` 94/94 成功；预构建包检查通过（7 模块，Build ID `55116800`，源码指纹 `24e91536e9f6`）；`validate_project.py` 与 `git diff --check` 通过。
- 阻塞审查修正：共享 FSM 的 Born 从 priority 20 提升为 90，并保持 `lock_until_playback_complete=true`、`terminal=false`、自然完成回 Move；Attack(40)、Hit(60)、Transform(80) 均不可抢占，只有 terminal Death(100) 可抢占。Unreal 保存后回读完整状态表与上述值一致。
- Animation2D 确定性测试新增 Attack/Hit/Transform 拒绝、拒绝后 `IsBornPlaying()` 仍为真、自然完成解除门禁及 Death 抢占断言；聚焦运行未报告这些断言错误，仍仅有既有 TimeGuard Phase2 空能力断言。Plan82/121 Unreal 资产审计未报告 Born/FSM 问题，仍只被明确排除的 BadRabbit 20 张既有未导入 PNG 阻塞。
- 阻塞审查修正后的最终 FullRebuild 95/95 成功；预构建包刷新为 7 模块、Build ID `55116800`、源码指纹 `96c87a0105f2`。
- Born Gameplay Gate 修订执行：仅 `TryPlayBorn()` 成功时 Host 保存原 `CanBeDamaged` 并关闭伤害；Gate 内 Host 原始伤害入口返回 0，Sense 同时关闭移动、攻击与 Phase2 许可，缺失/失败 Born 不进入 Gate。Presentation 自然完成后 Host 恢复原伤害状态；普通 AI、目标与既有冷却/计时继续推进，Logic 抑制普通攻击、特殊技及 Boss 攻击提交，Host 在世界副作用边界再次拒绝攻击窗口、传送、投射物和伤害。
- `BornMovementPermit` 扩展为攻击门禁回归：Gate 内普通攻击不消费序号、特殊技不进入 Windup、Boss 不产生 AttackWindow，Boss encounter 计时继续；解除后的首个合格步骤分别恢复提交。`BornGameplayGate` 同时覆盖 Host 侧 Boss 传送抑制与完成后恢复；既有成功才 Gate、伤害为零、位置不变、恢复边界和缺失 no-op 保持不变。
- 与最新 Plan117/122 组合后的最终 FullRebuild 98/98 成功；预构建包为 7 模块、Build ID `55116800`、源码指纹 `2e008f409f1b`。
- 恢复边界审查修正：`RestoreRuntimeState` 在配置后同步取消可能启动的 Born、解除 Host Gate，并按恢复后的存活状态设置 `CanBeDamaged`；存档敌人不重播 Born、不获得瞬时无敌或移动门禁，新生成路径不变。Host 回归覆盖“配置先启动 Gate → 恢复快照 → Born/Gate 均关闭且活体可伤害”。
- 攻击门禁审查修正：Host 在成功 Born Gate 内向 `FReEchoEnemySenseSnapshot::bAttackPermitted` 注入 false；Logic 不开始或提交普通攻击、兔子/狐狸特殊技和 Boss 攻击窗口，但继续推进目标采样、普通攻击冷却、Bomber Fuse、Boss cooldown/encounter 等既有计时。Host 在实际世界副作用边界再次拒绝 Gate 内攻击窗口，并阻止 Boss 传送、投射物生成与伤害；自然完成后的首个合格逻辑步恢复，缺失/失败 Born 与运行时恢复仍保持 ungated。
- 攻击门禁最终验证：FullRebuild 96/96 成功并刷新 7 模块预构建包（Build ID `55116800`，源码指纹 `babf2d833441`）；`ReEcho.Enemies.Logic` 与 `ReEcho.Enemies.Host` 全部通过，包含普通/特殊/Boss 抑制、Boss encounter 计时继续、Host 传送抑制及完成后恢复；`prebuilt_editor.py check`、`validate_project.py`、`git diff --check` 通过。首次 validate 在沙箱临时目录权限处失败，扩展权限原命令重跑通过，非项目内容失败。
- 恢复边界修正后的 `ReEcho.Enemies.Host` 全部通过；最终 FullRebuild 97/97 成功，预构建包为 7 模块、Build ID `55116800`、源码指纹 `cc80f41fe521`。

### 剩余风险

- 主体脚点、首帧闪烁、最终视觉节奏与回到 Move 的实际观感仍需用户 PIE 人工验收。

### 人工验收结果/请求

- `PendingBeforeClose`：四种怪物的出生动画视觉与结束过渡。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：已更新 Born 原生语义、FSM 完成归宿与测试边界。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已更新 Enemy Host 在 Definition/Profile 装配后发出 Born 表现请求且不参与玩法裁决；保留 Plan114 音频装配内容。
- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅、无需修改；模块拓扑和依赖方向不变。
- `shared/CODEBASE_MAP/README.md`：已审阅、无需修改；稳定模块标识和路由不变。
