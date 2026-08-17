# Plan 46 - 程序 - 音频资源、运行时硬化与全量语义触发接入

## 协调

- Planner 负责人：Gavyn-side Planner（当前程序用户授权的合并 Planner/Executor）。
- Executor 负责人：Gavyn-side Executor。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`InProgress`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划基线：`origin/main@87cde6c`；初始实现基线：Plan-only 发布提交 `origin/main@0fa465c`。Plan34 在该发布提交中关闭，其延期项由本 Plan 接管；2026-08-17 用户发现菜单无声后明确授权扩大 Plan46，扩围基线为已审计并合入本地任务分支的 `origin/main@4b05ee1`。
- 本地实现方式（可选，仅作交接说明）：独立工作树 `ReEcho-plan46-audio`、本地分支 `plan/46-audio-resource-integration`；发布前精选正式提交并刷新预构建包，不推送任务分支。
- 依赖 / 阻塞：依赖已关闭 Plan33/34 的稳定音频 API、31 个 EventId、14 列目录 Schema 与五总线设置；原 Proposed Plan35/36 的全部非战斗/战斗触发范围由本 Plan 吸收并关闭，避免重复实现。用户已提供 7 个 AI 生成的长音频源文件；其具体生成工具/授权说明需在关闭前由用户确认并写入来源记录。
- Writes: `plans/34-audio-catalog-settings.md`、`plans/35-audio-noncombat-integration.md`、`plans/36-audio-combat-echo-integration.md`、本 Plan；`Design/Data/ReEchoAudioEvents.xlsx`、`Content/Data/audio_events.csv`、`scripts/data/author_audio_events.py`；`Design/Audio/**`、`scripts/audio/**`、`Content/ReEcho/Audio/**`；`Config/DefaultGame.ini`；`Source/ReEchoAudio/**` 的运行时硬化、跨 World 通用事件队列与自动化；`Source/ReEcho/**` 中 GameMode、UI Flow、Combat 音频适配、Enemy/Echo 世界宿主及聚焦测试；`shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`。
- Stable Reads: 用户提供的 `The_Keeper_of_Slow_Hours.mp3`、`The_Watchmaker_s_Garden.mp3`、`Weight_of_the_Hour.mp3`、`The_Last_Pendulum_Swing.mp3`、`Golden_Hour_Ascent.mp3`、`Weight_of_the_Rift.mp3`、`Under_The_Stone_Vault.mp3`；现有 `The_Iron_Waltz`；`MOD-ReEchoCombat` 的最终攻击/命中/受伤/击杀/死亡事件；`MOD-ReEchoEnemies` 的 Enemy/Boss Intent 与 Archetype；`MOD-ReEchoWeapons` 的成功 Commit；Run、Recording、Weather 与 UI 当前权威生命周期。
- 影响模式：`Exclusive`，因为权威音频 XLSX、生成 CSV、同名 SoundWave 资产和 cook 配置必须作为单一发布单元更新。
- 兼容承诺 / 下游操作：保持现有 31 个 EventId、14 列 CSV 顺序、五总线、存档和录制字节不变；调用方只发布稳定 EventId、StateId、位置、粗粒度来源和 VariantId，不直接持有音频资产或 `UAudioComponent`；音频缺失、静音或未驻留不得改变任何玩法结果。
- 明确排除：不新增武器/敌人 Variant Schema；不修改 Combat/Weapons/Enemies 独立模块的权威结算或节拍；不新增复活、镜头运动或其他玩法来制造音频触发；不为等待音效而延迟 UI、攻击、死亡、重开或关卡切换；不做最终响度、混音、无缝循环或美学通过声明；不创建 Exchange/PR/远端任务分支；不安装第三方音频转换工具。按用户要求使用本地 Plan46 worktree/branch 隔离实现。

## 锁定目标

把用户提供的 7 条长音频和仓库确定性生成的短促音效接入现有音频目录，使全部 31 个稳定 EventId 都拥有可解析、可预载、可 cook 的 SoundWave，并从当前游戏的权威生命周期实际发布；保持玩法只发布稳定语义，音频缺失或加载失败仍不得阻塞玩法。

- `Music.Menu`、`Music.Shop`、`Music.Boss`、`Music.Death`、`Music.Victory` 使用用户提供的对应 MP3。
- `Ambience.Arena`、`Ambience.Rain` 使用用户提供的对应 MP3。
- 保留已接入的 `Music.Encounter` / `The_Iron_Waltz`，并使所有 Music/Ambience 目录项符合循环状态契约。
- 通过仓库本地、固定种子/固定参数的 Python 合成器生成其余 23 个 OneShot：UI 6、Combat 6、Enemy 3、Boss 3、Echo 3、`CameraMove`、`Revive`。这些是可替换的程序化基线候选，不是最终音效设计。
- 更新权威 XLSX 并确定性生成 CSV；不手改 CSV 形成第二事实来源。
- 资源通过 Unreal Editor Python 正式导入 `.uasset`，不在 Editor 外制造或修改 `.uasset`。
- 背景状态：开始/装载使用 `Music.Menu`，普通遭遇使用 `Music.Encounter`，Boss 遭遇使用 `Music.Boss`，特质/商店间歇使用 `Music.Shop`，终局使用 `Music.Death` / `Music.Victory`；遭遇环境在 `Ambience.Arena` 与 `Ambience.Rain` 间按权威天气切换。
- UI：所有注册屏幕按钮统一发布 hover/基础 confirm；关闭/返回、事务拒绝、购买成功与卡牌选择成功分别追加 `UI.Cancel`、`UI.Error`、`UI.Purchase`、`UI.CardSelect`，不以音频决定事务结果。
- 世界语义：成功玩家攻击和最终 Combat 结果发布六个 `Combat.*`；普通 Enemy、Boss、Echo 使用自身 Spawn/Attack/Death-or-End 事件并携带位置；致死伤害不额外发布普通 `Combat.Hurt`。
- `CameraMove` 只在实际把 PlayerController 切到固定竞技场相机时发布一次；`Revive` 映射到现有“死亡后重开”流程，通过音频模块的通用 next-world 队列在新 World 可用后发布，不新增复活玩法、不阻塞 `OpenLevel`。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoAudio`、`MOD-ReEcho`、文档型 UI 入口 `MOD-ReEchoUI`；`MOD-ReEchoCombat`、`MOD-ReEchoWeapons`、`MOD-ReEchoEnemies` 只提供既有稳定结果/Intent，不修改其权威逻辑。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`，均已加入 `Writes`；关闭前审阅 `MOD-ReEchoCombat`、`MOD-ReEchoWeapons`、`MOD-ReEchoEnemies`、`ARCHITECTURE.md` 和 `README.md`。
- 设计意图：资源制作/映射、加载、播放策略、跨 World 延迟投递和 cook 保证继续归音频模块；GameMode/UI/世界宿主只把各自已发生的权威结果翻译为 EventId/StateId，不把音频返回路径接回玩法。
- 权威状态与依赖：`Design/Data/ReEchoAudioEvents.xlsx` 仍是目录事实来源；`Content/Data/audio_events.csv` 是生成运行时来源；SoundWave 是目录软路径指向的播放资产。依赖方向保持 `ReEcho -> ReEchoAudio`。
- 决策记录：长音频由 UE 5.8 直接导入 MP3，避免安装转换工具；短音效使用确定性仓库生成器，便于复现和替换；所有动态 CSV 软引用资产通过显式 cook 路径进入包；不为本轮资源增加变体表。
- 相关文档同步范围：维护 `MOD-ReEchoAudio.md` 的资源位置、加载/cook、循环和衰减事实；`ARCHITECTURE.md`、`README.md` 与 `MOD-ReEcho.md` 关闭前逐项审阅，只有拓扑、路由或受影响契约变化时才修改正文。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoAudio.md`：已按最终资源、cook、非阻塞与衰减实现更新；
  - `ARCHITECTURE.md`：已审阅运行时拓扑，无需修改；
  - `README.md`：已审阅路由标识，无需修改；
  - `MOD-ReEcho.md`：已审阅稳定调用方契约，无需修改。

## 锁定验收

- [x] 权威 XLSX 与生成 CSV 保持锁定 14 列和完整 31 个稳定 ID；全量 `--check` 通过，旧非音频 CSV 字节不变。
- [x] 31 个目录行都有非空合法软路径；Editor 中全部路径解析为 `USoundWave`，资产类、循环属性和 2D/3D 声道约束符合目录用途。
- [ ] 7 个用户长音频及 23 个确定性短音效拥有准确来源/生成记录；用户提供资源的工具/授权状态不被 AI 猜测。
- [x] Music/Ambience 循环状态不会因未驻留资产进行同步加载；预载未完成/失败仍安全且可重试，不阻塞玩法线程。
- [x] `AttenuationMin/Max` 实际影响空间 OneShot/Loop 的 Unreal 播放组件；非空间事件不启用衰减覆盖。
- [x] CSV 文本软引用指向的 `/Game/ReEcho/Audio/**` 在 Windows cook/package 中存在，不依赖地图或硬引用偶然带入。
- [x] author 脚本重建的音频目录不会清空已批准资产绑定；生成器和目录映射有聚焦测试。
- [x] 音频模块没有新增对 `ReEcho`、Combat、Weapons、Enemies、Echo 或 UI 类型的反向依赖；存档、录制和玩法结果不变。
- [ ] 31 个 EventId 均有实际生产发布路径或当前机制对应路径：背景 8、UI 6、Combat 6、Enemy 3、Boss 3、Echo 3、`CameraMove`、`Revive`；不存在仅测试引用的孤立事件。
- [ ] 音乐/环境状态从权威流程一次切换且幂等：菜单、普通遭遇、Boss、商店、死亡、胜利、Arena/Rain 均不每 Tick 重发，不残留旧 Loop。
- [ ] UI hover/confirm 由屏幕框架统一绑定；cancel/error/purchase/card-select 从真实用户操作和事务结果发布，音频禁用时 UI 结果完全相同。
- [ ] Enemy/Boss/Echo Spawn、Attack、Death/End 从世界宿主的成功初始化、已提交动作和终止生命周期发布；玩家 Combat 六类来自既有最终 Combat 事件，致死结果不叠加普通 Hurt。
- [ ] `CameraMove` 只对应真实 `SetViewTarget`；死亡重开把 `Revive` 放入通用 next-world 队列，`OpenLevel` 不等待音频且非死亡重开不误发。
- [ ] 必需数据检查、项目校验、C++ 格式化（可用时）、聚焦自动化、Editor build、最终 `-FullRebuild`、预构建包刷新和 `git diff --check` 通过。
- [ ] 用户在 PIE 中抽听 7 条长音频和每组短音效，验收可听性、循环、响度、空间感及是否需要替换；AI 不代签主观结果。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：发布并核验本 Plan 后，以新的 `origin/main` Plan-only 提交为实现基线。
- 引擎/构建可用性：UE 5.8 安装版存在；官方 5.8 文档确认支持直接导入 `.mp3`；当前未发现运行中的 Unreal Editor。
- 现有聚焦测试结果：规划前 `python scripts/data/sync_xlsx_to_csv.py --check` 与 `python scripts/validate_project.py` 通过，证据为 static only。
- 共享契约 / 难合并资源风险：权威 XLSX、生成 CSV、`.uasset` 与 cook 配置必须同候选发布；导入前取得同克隆 Unreal 锁。
- 基线损坏时的停止条件：远端 main 出现他人新提交；MP3 导入失败或音轨不可读；用户资源授权无法记录；导入需要 Editor 外手改 `.uasset`；运行时硬化必须改变 EventId/Schema/玩法 API。

## 实现提纲

1. 发布并核验 Plan34 关闭 + Plan46 Plan-only 候选；实现前重新 fetch。
2. 复制用户源文件到项目来源目录并记录哈希/来源待确认字段；实现固定参数短音效生成器和聚焦测试。
3. 用 Unreal Editor Python 批量导入长短音频、设置循环/加载属性并验证全部资产类和路径。
4. 使用 Spreadsheets 工作流修改权威 XLSX；同步生成 CSV；修正 author 脚本的完整映射可复现性。
5. 在 `ReEchoAudio` 内完成非阻塞状态加载、衰减应用、next-world 通用投递和资产 cook 所需的最小运行时/配置硬化。
6. 在 GameMode/UI Flow/Combat 适配/Enemy/Echo 世界宿主中接入全部 31 个稳定语义；为一次性、致死去重、跨 World 和禁用音频不改变玩法增加聚焦测试。
7. 执行数据、静态、自动化、资产加载、cook、构建和相关模块文档审阅；把完整可玩候选交给用户听感验收。
8. 人工验收通过后，在最终集成候选上执行 `-FullRebuild`、刷新预构建包、最终 fetch 审计并发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 生成 | 聚焦 Python 测试；重复生成并比较哈希 | 23 个 WAV 名称、格式、时长和字节确定性 |
| 数据 | `python scripts/data/sync_xlsx_to_csv.py --check` | 三份权威 XLSX 与所有生产 CSV 无漂移 |
| 资产 | UE Python 导入/加载审计 | 31 个路径均解析为 SoundWave；循环与声道属性准确 |
| 静态 | `python scripts/validate_project.py`、反向 include/依赖审计 | Schema、来源、cook、模块边界通过 |
| C++ | `.clang-format`（可用时）、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 编译链接五个 Runtime Module |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Audio` 或等价配置化 Editor 聚焦运行 | Catalog、预载、衰减、设置和假后端测试通过 |
| 语义接入 | 聚焦 UI/Run/Combat/Enemy/Echo 自动化与生产引用审计 | 31 个 ID 均有生产路径；一次性、致死去重、跨 World 与音频禁用回归通过 |
| Cook | Windows clean cook/package 及 staged asset audit | 动态软引用音频进入包且可加载 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`、项目校验、`git diff --check` | 最终集成版本与精选预构建包匹配 |
| 人工 | PIE 抽听矩阵 | 用户报告通过或列出需替换/调音资源 |

## 执行记录

### 变化

- 2026-08-17 用户在 PIE 菜单确认无声；日志证明音频设备和 31 行目录正常，但 `Music.Menu` 只有测试/常量引用。用户明确要求扩大 Plan46 并继续到现有资源全部实际接入；本 Plan 因此吸收 Proposed Plan35/36，状态从 `Review` 退回 `InProgress`。

- 用户提供的 7 个 MP3 已复制到 `Design/Audio/Source/**`，记录原文件名与 SHA-256；23 个 OneShot 由固定参数/固定种子的仓库生成器生成，均为 48 kHz mono PCM16。
- Unreal Editor Python 已导入/配置 31 个 SoundWave（含既有 `Music.Encounter`），长音与环境资产设为循环、OneShot 设为非循环。
- `ReEchoAudioEvents.xlsx` 的 31 个 `AssetPath` 已完整绑定，并通过统一同步器生成 `audio_events.csv`；author 脚本保留全部绑定。
- 后端移除状态播放同步加载；OneShot/Loop 共用播放前组件配置并把目录最小/最大距离落实为球形线性衰减。
- `/Game/ReEcho/Audio` 加入显式 AlwaysCook；新增 Editor 资产审计脚本检查 SoundWave、循环标记和空间音效 mono 约束。
- 按用户要求将实现迁移到 `ReEcho-plan46-audio` 独立工作树；本地 WIP 恢复点已记录在任务分支历史中，未发布远端任务分支。

### 证据

- `python scripts/audio/generate_event_sfx.py --check`：23 个确定性 WAV 匹配。
- 权威工作簿通过 artifact-tool 表格/公式/双工作表渲染检查；`sync_xlsx_to_csv.py --check` 通过，31 行目录路径均非空，非音频生产 CSV 无变化。
- author 映射聚焦检查：31 个 EventId 与 31 个 AssetPath 精确覆盖；7 个用户源文件 SHA-256 与来源清单匹配。
- `python scripts/validate_project.py`：通过（static only）。
- `.clang-format`（四个改动 C++/Header）与 `scripts/ue/Build-Editor.cmd -Configuration Development`：通过；五模块预构建包已刷新，当前源码指纹 `9afab8c287f0`。
- UE 资产审计：31/31 目录路径解析为 `USoundWave`，循环标记与空间音效 mono 约束通过。
- `ReEcho.Audio` 聚焦自动化：13/13 通过，退出码 0；覆盖目录原子重载、异步预载失败重试、空间 OneShot/Loop 衰减传播和设置策略。
- Windows Shipping clean `BuildCookRun`：通过，Cook 766 packages、0 errors；归档可执行文件存在，IoStore 清单对目录 31/31 资产包路径命中。

### 剩余风险

- 用户提供的 7 个 MP3 的生成平台、账号授权范围或许可证尚未记录；关闭/最终发布前必须由用户确认，AI 不推断。
- 程序化短音效是技术可用基线，最终音色和响度可能需要用户替换或重调。
- `Music.Menu` 导入时 UE 报告文件尾部直流偏移，循环接缝可能爆音，必须由用户试听决定是否回源重生成或后处理。
- 最终 `-FullRebuild` 与匹配预构建包只在用户听感/授权确认后的最终集成版本执行；当前 Review 候选不冒充最终发布门禁通过。
- 远端规则提交 `origin/main@4b05ee1` 已经用户确认后合入本地任务分支；它改变项目校验规则，因此扩围前证据只保留为资源/后端历史证据，完整语义接入后的构建和自动化必须重跑。

### 人工验收结果/请求

`PendingBeforeClose`：用户抽听所有长音频类别及 UI/Combat/Enemy/Boss/Echo 短音效组，确认循环、响度、空间感和是否接受程序化基线。

### 架构文档审阅结果

- `MOD-ReEchoAudio.md`：已更新 Plan46 资源位置、异步驻留/失败重试、实际衰减、AlwaysCook 与资产审计事实。
- `ARCHITECTURE.md`：已审阅，无需修改；Runtime Module 拓扑、权威状态和依赖方向均未变化。
- `README.md`：已审阅，无需修改；未新增、删除或重命名架构路由标识。
- `MOD-ReEcho.md`：已审阅，无需修改；主模块仍只发布稳定语义，未修改调用点、公共 API 或玩法类型。
