# Plan 82 - 程序 - 2D 动画资产整理与可选状态契约

## 协调

- Planner 负责人：当前程序用户 + Codex。
- Executor 负责人：Codex（同一 AI 规划并执行）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@7c69a037`；当前主工作区中未提交的 `Content/ReEcho/Art/Animation2D/**` 与相关 Profile 仅作为用户已确认的美术输入源，不在主工作区直接实施。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:/Users/binnanliang/Documents/ReEcho-worktrees/animation2d-asset-cleanup`，分支 `codex/animation2d-asset-cleanup`。
- 依赖 / 阻塞：发布本 Plan 后，将当前主工作区内具名动画资产增量安全复制到独立 worktree；所有 `.uasset` 重命名、移动、导入和引用修复必须通过 Unreal Editor API 完成。当前 `WBP_ReEchoSettings` 残留变量 GUID ensure 会阻塞通用 Automation 入口，须与动画结果分开记录。
- Writes: `Content/ReEcho/Art/Animation2D/**`；`Content/ReEcho/DataAsset/{Character,Enemy,Common}/**` 中 Animation2D Profile/FSM/Catalog；`Config/DefaultGameplayTags.ini`（若存在 Idle 原生标签声明）；`Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/**`；主模块 Animation2D Host 适配与聚焦测试；`scripts/ue/*animation*`、`scripts/ue/*presentation*` 中仍使用旧动画契约的脚本；`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；本 Plan；精选 Win64 Editor 预构建包。
- Stable Reads: 当前主工作区未提交动画资产、`Source/ReEchoPresentation/**`、主模块 Player/Enemy Presentation Host、Plan40/50/71/74/77、`shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`。
- 影响模式：`Exclusive`；大量二进制动画资产与 Profile 迁移不可文本合并，且公共 Animation2D 语义契约变化会影响玩家、Echo 和全部怪物表现。
- 兼容承诺 / 下游操作：Gameplay、AI、伤害、移动和死亡权威不变；缺少某语义 Clip 合法且不再作为错误；请求未配置语义时保持当前有效表现，不借用其他语义 Clip；动作结束优先回到 `Move` 基础循环，没有 `Move` 时保持当前安全表现。旧资产路径仅在迁移期间通过 Editor Redirector/fixup 保证引用收口，发布候选不得依赖未提交 Redirector。
- 明确排除：不重画贴图、不改变帧内容或主观播放速度；不补造角色本来不存在的动作；除下述敌人终结死亡收口外，不修改其他战斗/VFX/武器时序；不顺带修复 `WBP_ReEchoSettings`；不整理 `Content/ReEcho/Art/Animation2D` 之外的通用 VFX 纹理库。

## 锁定目标

整理当前玩家、Echo、普通怪和 Boss 的 2D 动画资产，使目录、命名和 DataAsset 引用可以按角色与语义直接理解；把当前动画目录内尚未导入的受支持 2D 源贴图导入 UE，并确保其 Sprite/Flipbook 消费链完整或明确标记为仅源素材。

从公共动画契约中删除 `Animation.Idle`。除 `Move` 基础循环外，Charge、Attack、Hit、Transform、Born、Death 等状态全部为角色可选能力：Profile 未配置即表示该角色不需要该表现，请求该语义时不得报契约错误、不得借用另一语义动画，也不得影响玩法结果。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoPresentation` / `AREA-Presentation` 直接修改；`MOD-ReEcho` 的 Player/Enemy Host 适配和测试受公共契约影响。`MOD-ReEchoEnemies` 仅作为只读事件来源审阅，不改变其 AI/行为权威。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，两者已加入 `Writes`；审阅 `MOD-ReEchoEnemies.md` 并在本 Plan 记录是否需要更新。
- 设计意图：Profile 是“该外观真正拥有的表现能力集合”，不是要求每个角色填写同一张完整状态表。Gameplay 仍可发送统一语义事件，Presentation 对缺失能力做无副作用忽略。
- 权威状态与依赖：删除 `Idle` 公共语义和 FSM 状态；默认稳定表现从 Idle 改为可选 `Move` 基础循环。资产所有权仍归 Character/Enemy Profile，状态播放仍归 Presentation Controller，依赖方向不变。
- 决策记录：不再用 Walk/Hit 等现有 Flipbook填充缺失 Charge/Death；这种占位虽然能通过旧“全状态必填”审计，但会制造错误表现。保留统一语义事件接口，同时把 Profile Clip 变为稀疏集合，可兼容不同角色能力。
- 资产目录决策：目标采用 `Art/Animation2D/{Players|Echos|Enemies}/<Role>/<Semantic>/{Textures|Sprites}` 与角色级 `Flipbooks/`；Flipbook 使用规范语义名。Base/Phase2 仍由 Profile AnimationSet 区分，不通过含义不明的缩写资产名表达状态。
- 相关文档同步范围：必审 `ARCHITECTURE.md`（预期依赖拓扑不变）、`README.md`（预期稳定路由不变）、`MOD-ReEchoPresentation.md`（更新可选状态、无 Idle、缺失状态行为与资产布局）、`MOD-ReEcho.md`（更新 Host 的基础状态语义）、`MOD-ReEchoEnemies.md`（审阅事件契约是否仍准确）。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：已更新稀疏语义、Move 归宿与 Plan82 审计入口。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已更新 Profile/FSM 路径、无 Idle 状态与缺失语义 no-op。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：已审阅；AI/行为事件权威未变化，无需更新。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；模块依赖拓扑未变化，无需更新。
  - `shared/CODEBASE_MAP/README.md`：已审阅；模块路由未变化，无需更新。

## 锁定验收

- [ ] `Animation.Idle` 不再存在于生产 GameplayTag、FSM、Profile、Controller 默认完成归宿、脚本或测试契约中。
- [ ] 所有生产 Profile 指向唯一的 `DataAsset/Common/Animation2D/SM2D_DefaultCharacter`；旧重复 FSM 被安全移除且无有效引用。
- [ ] Profile 可只配置角色真实拥有的语义；缺失语义的解析/播放请求是合法 no-op，保持当前有效表现且不改变玩法。
- [ ] `Move` 存在时作为基础循环；没有 `Move` 的 Profile 仍能安全显示 Blueprint/首个有效表现或保持现状，不崩溃、不隐藏玩法 Actor。
- [ ] 当前动画目录内受支持且应参与生产的源贴图均已导入 Texture2D；每个生产 Flipbook 的 KeyFrame/Sprite/Texture 引用可加载，无 Missing/Redirector 残留。
- [ ] 目录与资产名按角色/阶段/语义可辨识；删除确认无引用的重复、旧版和误放资产，更新所有硬编码脚本/测试路径。
- [ ] 只读资产审计列出每个生产 Profile 的真实语义覆盖、循环规则和 Flipbook 路径，并对可选缺失状态不报错。
- [ ] `ReEcho.Presentation.Animation2D` 与相关 Player/Enemy Presentation 测试通过；若仍被独立 UI 基线 ensure 阻塞，必须提供绕过 UI 的等价聚焦审计证据并明确未运行项。
- [ ] `-FullRebuild`、项目校验、预构建包校验和 `git diff --check` 通过。
- [ ] 用户在 PIE 验收玩家/Echo/兔子/狐狸/山羊/史莱姆的移动、攻击、变身、出生、受击和死亡中实际存在的动作；确认无首帧闪烁、错误循环或动作结束卡死。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@7c69a037`。
- 引擎/构建可用性：UE 5.8 Win64 预构建包可加载；只读 Plan74 Profile 审计已能通过 `UnrealEditor-Cmd -ExecutePythonScript` 运行。
- 现有聚焦测试结果：旧审计报告 23 项问题：16 个 Profile 仍引用旧 FSM，Fox Phase2 缺 Transform，TimeGuard Phase2 缺 6 个语义；通用 Automation 被独立 `WBP_ReEchoSettings.Button_Close` GUID ensure 阻塞。
- 共享契约 / 难合并资源风险：Animation2D `.uasset`、Profile、FSM、Catalog 为二进制 Exclusive 表面；主工作区存在大量未提交美术输入，迁移时必须列出精确源/目标，不得覆盖用户其他 Content 改动。
- 基线损坏时的停止条件：若源贴图与现有 Flipbook帧无法建立唯一对应、两个同名资产均有有效外部引用、或删除 Idle 后基础表现需求无法由 `Move`/现状安全承接，停止相关资产删除并请求用户决定，不猜测美术意图。

## 实现提纲

1. 快照主工作区当前 Animation2D 增量清单，将仅限本 Plan Writes 的新增/修改/删除迁入独立 worktree并复核字节与删除集合。
2. 扩展只读审计，输出源贴图导入缺口、重复 FSM、Profile 稀疏语义覆盖、Flipbook→Sprite→Texture 完整性和 Redirector 状态。
3. 通过 UE Editor API 导入未加载贴图、创建/修复 Sprite 与 Flipbook，并按统一目录/语义重命名移动；先修引用再删除旧资产。
4. 删除 `Animation.Idle` 并把 Controller/Host 完成归宿改为 `Move` 或安全 no-op；允许 Profile 稀疏 Clip，不再补造动作。
5. 更新生产 Profile、FSM、Catalog、脚本、测试和模块文档，运行聚焦审计与自动化。
6. 完整重编译并刷新预构建包，执行项目/差异验证；提交候选后请求 PIE 人工验收，验收前保持 `Review`。
7. 收口敌人终结死亡：致命 Hurt 显式标记并抑制普通受击表现；Host 立即停止 Logic、碰撞、新攻击及非死亡表现，保留已发射投射物；Death 独占播放一次，实际完成后销毁，缺失 Clip 时下一安全帧销毁，并以实际动画时长加宽限作为防卡死 watchdog。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产静态 | Animation2D 清单/命名/源图导入差异审计 | 无未解释的源图缺口、重复 FSM、旧路径或 Redirector |
| UE 资产契约 | `UnrealEditor-Cmd -ExecutePythonScript=scripts/ue/audit_plan74_animation_contracts.py`（随新契约更新） | 全部生产 Profile、Clip、Flipbook、Sprite、Texture 可加载；可选状态缺失合法 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.Animation2D` 及受影响 Host 测试 | 聚焦测试通过，或准确记录独立 UI 启动阻塞 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 项目 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 全部通过 |
| 人工 | PIE 覆盖玩家/Echo/全部怪物真实状态 | 动画正确、循环/停止/完成归宿正确、无错误占位 |

## 执行记录

### 变化

- 删除表现层原生 `Animation.Idle` 与旧枚举成员；Move 成为唯一基础循环，保留旧枚举数值以避免序列化漂移。
- Controller 对缺失 Move/动作执行无副作用 no-op；已有 Blueprint Flipbook 可作为首次配置安全画面，但不再打印缺失状态错误或隐藏 Actor。
- 16 个生产 Profile 统一指向 Common FSM，移除全部 Idle Clip 与 7 个空 Flipbook 条目；删除旧重复 FSM。
- 从主工作区迁入已确认的狐狸/兔子死亡与出生等动画增量，并按导入源记录补导 59 个 Texture2D；不自动发明 Sprite、Flipbook 或语义映射。
- 新增可重复执行的 Plan82 整理脚本与只读审计，更新受影响测试及模块文档。
- 新增致命 Hurt 标记与 Host 所有的终结死亡序列；死亡期间普通 Hit/Attack/VFX/阴影均被清除或拒绝，Death 不循环且不会回到 Move，完成回调销毁 Actor，无 Death Clip 时安全延迟至下一帧销毁。

### 证据

- 2026-08-23 基线只读审计：当前资产目录 1136 个文件，其中 731 个 `.uasset`、405 个源图片、角色级 `Flipbooks` 目录中 50 个 `.uasset`。
- 2026-08-23 旧 Plan74 契约审计可运行，报告 23 个问题；证明资产可加载但 FSM 迁移和必填语义契约已经过期。
- 2026-08-23 Plan82 整理：`profiles=16 imported_textures=59 explicitly_deleted_skipped=20`。
- 2026-08-23 新二进制只读审计：`profiles=16 states=6 imported_sources=398 issues=0`。
- 2026-08-23 FullRebuild：96 actions，成功并刷新 7 个精选 Editor 模块；源指纹随后因枚举显式数值注释性兼容调整需最终重跑。
- 2026-08-23 敌人死亡增量 Editor Build：成功；`ReEcho.Presentation.Animation2D` 2/2 通过，`ReEcho.Enemies.Host.CompositionAndSave` 1/1 通过。启动仍报告既有 LinuxArm64/VisionOS `MainVersion` 警告与 `WBP_ReEchoSettings.Button_Close` GUID ensure，但聚焦测试已实际执行并完成。
- 2026-08-23 最终 FullRebuild：99 actions 成功并刷新 7 个精选 Editor 模块，源指纹 `e81b79e38211`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 全部通过。
- 2026-08-23 从本地 main 的未提交美术输入中定向迁入八组 Death 资源；八个生产 Enemy Profile 已绑定对应非循环 Death：Grunt/Shield/Bomber/Slime 共用 Slime，Rabbit/Fox 使用各自资源，GoatPriest/TimeGuard 共用 Goat。更新后 Plan82 审计仍为 `profiles=16 states=6 imported_sources=398 issues=0`，`ReEcho.Presentation.Animation2D` 2/2 通过。

### 剩余风险

- 历史 Plan40/74 一次性配置脚本仍保留旧契约文本，未删除以避免超出本任务授权；生产入口已经切换为 Plan82 脚本。
- 当前 UI 基线 `WBP_ReEchoSettings.Button_Close` ensure 与跨平台 SDK 校验仍会阻塞通用自动化入口。
- 角色实际帧序、脚点、循环观感与缺失状态切换仍需 PIE 人工验收。
- Death 实际观感、特效清除时机、完成帧销毁和 Boss 已发射投射物延续仍需 PIE 人工验收；watchdog 仅防资产异常导致 Actor 永不销毁。

### 人工验收结果/请求

`PendingBeforeClose`：待实现后由用户在 PIE 验收。

### 架构文档审阅结果

Presentation 与主模块文档已更新；Enemies、ARCHITECTURE、README 审阅后确认无需改动。
