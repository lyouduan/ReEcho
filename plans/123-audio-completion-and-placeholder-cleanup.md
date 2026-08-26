# Plan 123 - 程序 - 剩余正式音频接入与表外占位清理

## 协调

- Planner 负责人：Gavyn-side Planner（当前程序用户授权的同一 AI 规划、评审与集成）。
- Executor 负责人：Gavyn-side Executor（同一 AI 分阶段执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：Plan-only `origin/main@a703ab0e506feda2f1d1cc41f342ef191768db6c`；2026-08-27 经用户确认后合入最新 `origin/main@7b63217ccb32a04f022d2870e65c51e6fb547ea8` 继续实现。
- 本地实现方式（可选，仅作交接说明）：独立工作树 `ReEcho-plan123-audio-completion-cleanup`，本地分支 `plan/123-audio-completion-cleanup`；Plan-only 发布后在同一工作树继续实现，不推送任务分支。
- 依赖 / 阻塞：依赖 Plan46 的基础音频服务、31 个稳定基础 EventId、AlwaysCook 与语义生产路由，以及 Plan114 的 15 列 `(EventId, VariantId)` 目录、42 个正式/变体资源和基础回退。需求映射以用户交付 `C:\Users\gavynqiu\Documents\miniGame\音频\ECHO音频配置说明.xlsx` 的可见内容为来源；用户已确认冲突项以“项目音频事件清单”页及 Plan114 当前映射为准，并确认本 Plan 的胜利路由与精确清理清单。
- Writes: 本 Plan；`Design/Audio/AUDIO_SOURCES.md`、`Design/Audio/Source/**`、`Design/Audio/Generated/**`；`Design/Data/ReEchoAudioEvents.xlsx`、`Content/Data/audio_events.csv`、`Content/Data/csv_schema.csv` 的 AudioEvents Schema 行、`scripts/data/author_audio_events.py`；`Content/ReEcho/Audio/**`；`scripts/audio/**`；`Source/ReEchoAudio/Public/ReEchoAudioEvents.h`、Audio Types/Catalog/Policy/Backend 配对实现与聚焦自动化；`Source/ReEcho/Private/Combat/ReEchoCombatAudioAdapterComponent.*`、`Source/ReEcho/Private/Graybox/ReEchoTimeShardPickupActor.cpp`、`Source/ReEcho/{Public,Private}/UI/ReEchoTraitCardChoiceWidget.*`、`ReEchoTraitCardEntryWidget.*`、`UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*`、必要的通用按钮音频绑定对象、`Source/ReEcho/ReEchoGameMode.*` 及相关聚焦测试；必要的精选 Editor 预构建包；`shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`。
- Stable Reads: 用户交付的 `音频/音乐/**`、`音频/音效/**` 与上述 XLSX 可见行列；Plan46/Plan114 的当前来源哈希、正式 SoundWave、目录 Schema 和安全回退；`FReEchoDamageEvent.ReactionBehaviorId` / `FReEchoElementReactionResolvedEvent.ReactionBehaviorId`；RunSubsystem 的成功拾取、装备与结算结果；卡牌揭示动画既有完成时序；现役武器表中的四个稳定 WeaponId。
- 影响模式：`Exclusive`，因为权威音频 XLSX/生成 CSV、同名或删除的 SoundWave、来源记录、导入/验证脚本和 Cook 清单必须作为一个完整候选审计，二进制资产不能静默覆盖并行修改。
- 兼容承诺 / 下游操作：保留现有 31 个基础 EventId、现有 11 个变体、五总线、存档与录制字节；为新增语义增加稳定 EventId，但不改变玩法成功条件。清理项继续保留无资产目录定义，使现有生产者安全 no-op 且不产生未知事件；未知/空变体仍回退基础目录项。用户负责最终可听性、响度、循环和审美验收。
- 明确排除：不接入没有生产 `WeaponId` 的法杖攻击，不新增武器或改武器表；不把工作簿目录中未映射的 `New*` 候选音乐自动选为正式资源；不改变 `Music.Menu -> MainMenu_v1.mp3`、`UI.CardSelect -> 选卡.wav`、`UI.Cancel/UI.Error/Revive -> 取消.wav`；不修改战斗、反应、拾取、购买、装备、卡牌或关卡推进的成功条件；不让音频完成回调驱动玩法；不删除用户提供的 `Music.Death/Music.Victory/Ambience.Arena/Ambience.Rain`，也不删除仍承担未知变体回退的 `Music.Encounter/Combat.Attack/Combat.Hit` 基础资产；不使用 Exchange，不推送任务分支。

## 锁定目标

继续把策划表已明确但尚未接入的正式音频映射到稳定语义成功点，并清理策划表明确范围之外的原有程序化占位资产。所有新增或静音事件都必须可由权威目录审计、可 Cook、可安全降级；玩法只发布稳定语义，音频缺失、静音或播放失败不得改变任何玩法结果。

### 正式接入

- 元素反应：以单一稳定基础事件 `Combat.Reaction` 携带 `ReactionBehaviorId` 变体，接入 `Reaction.Burn -> 火元素攻击A.mp3`、`Reaction.Vaporize -> 汽化.wav`、`Reaction.Growth -> 植物法术生长_1_V2.mp3`、`Reaction.Conduct -> 雷元素-电光一闪.mp3`、`Reaction.Enhance -> 植物法术生长_1_V1.mp3`。只消费已经结算的反应事件，不从音频模块反查 Combat 类型。
- 流程与 UI：新增 `Item.Pickup -> 道具拾取.wav`、`UI.CardReveal -> 卡牌出现.wav`、`UI.Equip -> 卸下符文.wav`、`UI.Unequip -> 卸下符文.wav`、`Flow.Victory -> 胜利.wav`。
- `Item.Pickup` 只在 `GrantTimeShards` 成功后发布；`UI.CardReveal` 在一次完整揭示或单卡刷新揭示实际开始时按锁定节奏发布，不由音频影响可选状态；`UI.Equip/UI.Unequip` 只在权威事务成功且装备集合确实变化时发布。
- `Flow.Victory` 只用于普通 Encounter 成功完成；Boss 击败继续由既有 `Boss.Death -> 胜利.wav` 播放，禁止同一 Boss 结算同时发布两者。

### 表外占位清理

- 以下八个稳定事件保留目录行但清空 `AssetPath`，成为显式静音定义：`Combat.Block`、`Combat.Kill`、`Enemy.Attack`、`Boss.Spawn`、`Boss.Attack`、`Echo.Spawn`、`Echo.Attack`、`Echo.End`。
- 删除这八项对应的 `/Game/ReEcho/Audio/**` SoundWave `.uasset` 和 `Design/Audio/Generated/**` 源 WAV；同步更新生成器、来源清单、导入脚本、目录作者脚本、资产/路由验证和 Cook 预期，确保删除资源不会被重建或残留在候选中。
- Catalog 与 Service 必须把“已定义但 AssetPath 为空”解释为安全静音 no-op，区别于未知 EventId；资产验证只要求非空路径可解析，并单独审计显式静音集合和已删除路径不存在。

### 2026-08-27 策划试听返修

- `UI.Confirm`、`UI.Purchase` 和其他存在前导静音的事件不再依赖导入脚本硬编码裁切；权威目录扩展非负 `StartTimeSeconds`，策划可直接在 XLSX 中调整每个事件/变体的起播点，运行时 OneShot 与 Loop 均消费该值。
- 通用按钮反馈继续发布稳定 `UI.Hover/UI.Confirm`；UI Flow 递归覆盖嵌套 UserWidget，并提供 Blueprint 可调用的单按钮绑定入口，供运行时晚建按钮显式补绑。资产、音量、Pitch、Cooldown、并发、暂停策略和起播点仍只在权威音频表配置。
- 普通选卡与商店选卡在可交互卡面被有效点选时发布一次 `UI.CardSelect`，后续确认、领取或应用事务不重复播放；卡组付款继续只在付款成功边界发布 `UI.Purchase`，避免领取时重复购买声。
- 玩家死亡继续消费 Combat 已发布的稳定死亡事件；返修必须验证 Player 的 Audio Adapter 路由和 `Combat.Death` 可听起播点，不新增 GameMode 双发。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoAudio`、`MOD-ReEcho`、`MOD-ReEchoUI`；`MOD-ReEchoCombat` 仅 Stable Read 已发布的反应结算契约，不修改其状态或依赖。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md` 已加入 Writes；关闭前审阅 `MOD-ReEchoCombat.md`、`ARCHITECTURE.md` 和 `README.md`。
- 设计意图：目录继续唯一拥有 EventId/VariantId 到资产的映射和显式静音状态；主模块适配器只在权威成功结果后发布 EventId、稳定 VariantId 与可选位置。新增 UI/流程/拾取/反应语义不改变领域状态所有者，不把具体 Actor、Widget 或 Combat 类型泄漏进 `ReEchoAudio`。
- 权威状态与依赖：`Design/Data/ReEchoAudioEvents.xlsx` 仍是音频目录真源，`Content/Data/audio_events.csv` 是确定性运行时来源，`Content/ReEcho/Audio/**` 是目录软路径指向的可 Cook SoundWave。依赖方向保持 `MOD-ReEcho -> MOD-ReEchoAudio`；Audio 不反向依赖玩法模块。
- 决策记录：冲突映射以工作簿“项目音频事件清单”页和 Plan114 当前生产映射为准；反应采用一个 EventId 加稳定 Behavior 变体，避免五个散乱 EventId；Boss 胜利继续由 Boss.Death 表达，Flow.Victory 仅补普通 Encounter；八个表外占位事件采用“保留稳定定义、移除资产”而非删除 EventId，避免现有生产者产生未知事件警告。装备与卸下暂按表内同一个 `卸下符文.wav` 文件实施，听感取舍留给人工验收。试听返修采用“XLSX 配播放参数、Blueprint/C++ 配稳定语义发布”的双层配置，不把具体 Widget 类型或素材路径泄漏进 Audio 模块。
- 相关文档同步范围：关闭前逐项审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoAudio.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md` 和只读契约来源 `MOD-ReEchoCombat.md`；只有拓扑、索引或模块事实真实变化时修改正文。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoAudio.md`：已记录 52 行目录、44 个绑定行/8 个显式静默行、16 列 Schema、`StartTimeSeconds` 与资产验证边界；
  - `MOD-ReEcho.md`：已记录反应、拾取、装备、胜利与按钮语义翻译边界；
  - `MOD-ReEchoUI.md`：已记录递归按钮反馈、可覆盖的稳定语义、选卡/购买去重与卡牌揭示边界；
  - `MOD-ReEchoCombat.md`：已审阅，无需修改；本 Plan 只消费既有最终死亡与 `ReactionBehaviorId` 事件；
  - `ARCHITECTURE.md` / `README.md`：已审阅，无需修改；模块拓扑、项目入口和索引未变化。

## 锁定验收

- [x] 5 个反应变体和 `Item.Pickup/UI.CardReveal/UI.Equip/UI.Unequip/Flow.Victory` 均有目录、可加载 SoundWave、生产触发路径和聚焦自动化或结构化审计证据。
- [x] 普通 Encounter 胜利只发布一次 `Flow.Victory`；Boss 击败只使用 `Boss.Death`，不存在双播。
- [x] 八个表外占位事件仍是已知稳定定义但请求时安全静音；对应八个 SoundWave、Generated 源 WAV 和生成/导入预期均已删除，仓库与 Cook 清单无残留。
- [x] 权威音频 XLSX 与生成 CSV 同步，16 列 Schema 和复合键唯一；全部 `StartTimeSeconds >= 0`，非空 AssetPath 可解析、可预载、可 Cook，空路径仅限锁定显式静音集合。
- [x] `ReEchoAudio` 没有新增对 ReEcho/Combat/Weapons/Run/UI/Presentation 类型的反向依赖；音频失败不改变任何事务或流程结果。
- [x] 必需静态检查、数据检查、Plan123 聚焦自动化、资产审计、Editor 构建、最终 `-FullRebuild` 与精选预构建包通过；按用户后续明确要求不制作 Shipping 包，以本工作树 `ReEcho.uproject` 作为策划 PIE 测试入口。完整 `ReEcho.*` 的主线独立失败已逐项记录，不误报全绿。
- [ ] 用户在 PIE/测试包验收新增 10 项的可听性、触发时机、响度、反应区分度、装备/卸下同源音与胜利去重；AI 不代签主观结果。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f6877c53f5b9f3c9d05328fbf94815b9464c9b7c`；Plan-only 发布并核验后，以新的 `origin/main` Plan 提交为实现基线。
- 引擎/构建可用性：Plan114 最终组合候选已证明 UE 5.8 FullRebuild、42 行资产审计与聚焦自动化可用；本 Plan 不复用其最终证据，准确候选仍须重跑。规划时未发现运行中的 Unreal Editor。
- 现有聚焦测试结果：Plan114 的 `ReEcho.Audio` 13/13、42/42 资产与项目校验为历史基线；当前规划阶段只执行只读审计，Plan-only 发布执行静态项目校验和 `git diff --check`。
- 共享契约 / 难合并资源风险：权威 XLSX、生成 CSV、SoundWave `.uasset`、导入/生成脚本和精选 DLL 是共享热点；删除资产可通过 Git 恢复，但会改变 Cook 清单与占位生成契约。任何远端同路径变化都会使相关导入、构建、Cook 和人工试听证据失效。
- 基线损坏时的停止条件：Plan-only 推送前编号被占用；用户交付源文件缺失/损坏；显式静音不能通过现有 Schema 表达且需破坏兼容迁移；稳定成功点不存在或必须改变玩法条件；远端出现同路径音频/Schema/C++ 修改并产生真实语义冲突。

## 实现提纲

1. 发布并核验本 Plan；读取 Executor 规则和最小配对实现，重新盘点用户源文件哈希、目录行、资产路径及精确删除目标。
2. 第一阶段接入无跨域或低耦合语义：反应变体、拾取、卡牌揭示和普通胜利；补充常量、目录、来源、导入与聚焦路由测试。
3. 第二阶段在权威装备集合变化的事务成功点接入 `UI.Equip/UI.Unequip`，验证购买、重装、同槽替换和未变化成功请求不会误播。
4. 第三阶段将八个表外占位事件迁移为显式静音，删除准确 SoundWave/Generated 源文件，并收紧生成、导入、来源、资产与 Cook 审计。
5. 维护相关模块文档，执行数据同步、格式化、构建、自动化、资产加载和静态检查；取得 Unreal 锁后导入/删除 UE 资产，不在 Editor 外手改 `.uasset`。
6. 在最终集成候选执行 `Development -FullRebuild` 与精选预构建检查；按用户要求不制作 Shipping 包，直接交付本工作树 `ReEcho.uproject` 供策划 PIE 主观试听。
7. 人工验收通过后更新执行记录并关闭 Plan；按 `main-publish-lock` 协议发布最终候选，确认远端后安全清理本地任务分支/worktree。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan 结构、编号和仓库静态规则通过 |
| 来源/生成 | `validate_formal_audio_sources.py`、`prepare_formal_audio.py --check`、占位生成器聚焦检查 | 新增正式源哈希可追溯；保留派生可重建；八个删除占位不再被生成 |
| XLSX/CSV | `python scripts/data/sync_xlsx_to_csv.py --check`、聚焦数据测试 | 16 列 Schema、非负起播点、复合键、10 项正式映射和八项显式静音一致 |
| 路由 | `python scripts/audio/validate_audio_event_routes.py` 及聚焦静态审计 | 新增事件/反应变体有生产路径；保留静音事件仍为已知语义 |
| UE 资产 | 同克隆 Unreal 锁下运行导入/删除脚本与 `validate_audio_catalog_assets.py` | 非空路径全部解析为 SoundWave；显式静音集合准确；八个已删除资产不存在 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0并刷新聚焦预构建包 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Audio` 及受影响 ReEcho/UI/Shop/Combat 聚焦套件 | 反应变体、成功点、静音 no-op、去重与回退通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`、预构建检查、`python scripts/validate_project.py`、`git diff --check` | 最终组合源码、内容和精选 Editor 包匹配 |
| 本地策划候选 | 打开本工作树 `ReEcho.uproject`，PIE 运行 `/Game/Level00` | 直接消费最终 Editor DLL、CSV 与 SoundWave；不复用旧 Shipping 包 |
| 人工 | PIE/测试包按 10 项新增语义与静音清理矩阵试听 | 用户报告 Passed，或列出需返工的触发/映射/响度问题 |

## 执行记录

### 变化

- 2026-08-26：完成只读需求盘点。用户确认以“项目音频事件清单”页维持 Plan114 已有冲突映射；确认普通胜利与 Boss.Death 分路；确认精确清理八个表外程序化占位事件。
- 2026-08-26：确认反应结算已有稳定 `ReactionBehaviorId`，拾取、装备、卡牌揭示和 Encounter 结束均存在不改变玩法权威的语义接入点；法杖仍无生产 `WeaponId`，排除在本 Plan 外。
- 2026-08-26：Plan-only 已发布并核验为 `origin/main@a703ab0e506feda2f1d1cc41f342ef191768db6c`；实现工作树进入 `InProgress`。
- 2026-08-26：新增 6 个稳定 EventId、5 个反应变体与生产语义路由；接收并哈希验证 7 个新源文件，Burn/普通胜利复用既有正式资产；权威 XLSX 扩展为 52 行，其中 44 行绑定资产、8 行为锁定显式静默。
- 2026-08-26：删除八个精确 Generated 源 WAV，并从生成器、导入和资产验证预期中移除；对应 `.uasset` 已在同克隆锁下由 Editor 脚本确认零引用后精确删除。
- 2026-08-27：用户确认接收策划试听返修并扩展本 Plan；Fetch 审计发现主线前进 58 个提交，安全检查点 `edd87d6b` 后合入 `origin/main@7b63217c`。唯一文本冲突 `ReEchoGameMode.cpp` 已按 Plan124 过渡状态机与 Plan123 单次 `Flow.Victory` 语义手工合并，Plan127 商店/选卡事务保持主线权威。
- 2026-08-27：再次 fetch 后安全合入只新增 Plan128 文档的 `origin/main@9a1b1c58`，无音频物理、逻辑或耦合冲突。策划试听返修新增 16 列 `StartTimeSeconds`、嵌套 Widget 递归按钮绑定、Blueprint 可调用的语义覆盖入口，并把普通/商店卡面选择统一到 `UI.CardSelect`，卡组领取不再重复购买或选卡声。
- 2026-08-27：Development Editor 构建成功并刷新 7 模块精选预构建包；XLSX/CSV 严格同步、37/37 EventId 生产路由与项目静态校验通过。UE 解码/派生/导入完成后，全新 Editor 会话验证 52 行目录为 44 个绑定 SoundWave + 8 个显式静默行，八个退役占位包均不存在。

### 证据

- 远端最大编号为 Plan122，本 Plan 使用下一个空闲编号 123。
- Plan120 在规划确认后进入 `origin/main`；只触及怪物 Transform Flipbook、纹理、敌人 Profile 与专用脚本，没有音频路径、目录 Schema 或本 Plan 触发入口的物理/逻辑冲突。
- 工作簿三张 Sheet 均无隐藏行列；“音频配置说明”与“项目音频事件清单”的冲突映射已由用户明确取舍，不再作为实施歧义。
- 当前八个清理目标在表中明确标记为未列出的 Generated 占位；目录 AssetPath 可选且现有音频基础自动化已覆盖已定义空资源的安全 no-op。
- 正式源哈希验证通过 32/32，保留 Generated 回归通过 15/15；权威工作簿保护、数据区解锁、五个完整列验证和 16 列 CSV 字节同步均通过。配置的起播点为 `UI.Confirm=0.14`、`UI.Purchase=1.28`、`UI.CardSelect=0.38`、`Combat.Death=3.45`、反应/装备等相应非负值，后续可由策划直接改表。
- UE 5.8 的 `EditorAssetLibrary.delete_asset` 在无 Source Control 工作树中完成 ForceDelete 且确认零包引用，但可能遗留可写包文件；Editor 脚本只在零引用与 API 成功后精确移除对应八个 `.uasset`，下一独立 Editor 会话重新扫描并通过“不存在”验证。
- 聚焦自动化：`ReEcho.Audio` 13/13、`ReEcho.UI.Button` 3/3、`ReEcho.UI.Shop` 3/3 全部通过；`ReEcho.Shop` 13 项中与本 Plan 有关的购买/选卡/刷新等 11 项通过，2 项主线独立失败为武器大师奖励数值和 Encounter 5 ShopTiers；`ReEcho.Combat` 13 项中 12 项通过，独立失败为 Conduct 玩法伤害/来源断言。三项失败均不在本 Plan 修改路径，未越界修改玩法或数值。
- 最终组合候选执行 `Build-Editor.ps1 -Configuration Development -FullRebuild` 成功，101/101 构建动作完成并刷新 7 个精选模块，源码指纹 `e4219fb73a02`；完整 `ReEcho.*` 记录 177 成功/22 失败，22 项均位于未修改的攻击、卡牌、Combat、敌人/数据、GAS、表现、掉落、Shop、Trait、HUD 或武器领域，本 Plan 的 Audio/Button/UI Shop 聚焦套件仍全绿。

### 剩余风险

- `UI.Equip` 与 `UI.Unequip` 按策划表使用同一个 `卸下符文.wav`；程序只保证触发正确，是否需要分音由用户试听决定。
- 新增反应和 UI OneShot 的响度、前导静音、空间感与区分度只能由用户在实际设备上验收。
- 用户交付文件的外部许可/署名信息仍未提供；来源清单只记录用户交付与哈希，不作可发布权利声明。
- 二进制 SoundWave、XLSX 和精选预构建包难以自动合并；发布前远端同路径变化必须重新审计和重建。

### 人工验收结果/请求

- `PendingBeforeClose`：请从本工作树 `ReEcho.uproject` 进入 PIE，试听本轮 click/hover/购买/普通与商店选卡/角色死亡返修，以及新增 10 项、胜利去重和八项静音清理结果。

### 架构文档审阅结果

- `MOD-ReEchoAudio.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md` 已同步本轮目录、起播点与语义绑定事实。
- `MOD-ReEchoCombat.md` 已审阅；最终死亡与反应结算契约未改变，无需修改。
- `ARCHITECTURE.md`、`README.md` 已审阅；模块拓扑与项目入口未改变，无需修改。
