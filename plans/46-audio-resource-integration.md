# Plan 46 - 程序 - 音频资源接入与运行时硬化

## 协调

- Planner 负责人：Gavyn-side Planner（当前程序用户授权的合并 Planner/Executor）。
- Executor 负责人：Gavyn-side Executor。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@87cde6c`；Plan34 在本 Plan 发布候选中关闭，其延期项由本 Plan 接管。
- 本地实现方式（可选，仅作交接说明）：在当前 `main` 上按小批量修改；发布前精选正式提交并刷新预构建包。
- 依赖 / 阻塞：依赖已关闭 Plan33/34 的稳定音频 API、31 个 EventId、14 列目录 Schema 与五总线设置；不依赖 Proposed Plan35/36 的玩法调用点。用户已提供 7 个 AI 生成的长音频源文件；其具体生成工具/授权说明需在关闭前由用户确认并写入来源记录。
- Writes: `plans/34-audio-catalog-settings.md`、本 Plan；`Design/Data/ReEchoAudioEvents.xlsx`、`Content/Data/audio_events.csv`、`scripts/data/author_audio_events.py`；`Design/Audio/**`、`scripts/audio/**`、`Content/ReEcho/Audio/**`；`Config/DefaultGame.ini`；必要的 `Source/ReEchoAudio/**` 运行时硬化与自动化；`shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`。
- Stable Reads: 用户提供的 `The_Keeper_of_Slow_Hours.mp3`、`The_Watchmaker_s_Garden.mp3`、`Weight_of_the_Hour.mp3`、`The_Last_Pendulum_Swing.mp3`、`Golden_Hour_Ascent.mp3`、`Weight_of_the_Rift.mp3`、`Under_The_Stone_Vault.mp3`；现有 `The_Iron_Waltz`；`Source/ReEcho/**` 的现有稳定语义发布点；Proposed Plan35/36。
- 影响模式：`Exclusive`，因为权威音频 XLSX、生成 CSV、同名 SoundWave 资产和 cook 配置必须作为单一发布单元更新。
- 兼容承诺 / 下游操作：保持现有 31 个 EventId、14 列 CSV 顺序、五总线、调用方 API、存档和录制字节不变；Plan35/36 后续只消费已绑定的稳定语义，不直接持有资产或 AudioComponent。
- 明确排除：不新增武器/敌人 Variant Schema；不新增或移动玩法触发点；不修改战斗、武器、敌人、Echo、Run 或 UI 具体类型；不做最终响度、混音、无缝循环或美学通过声明；不创建 Exchange/PR/任务分支；不安装第三方音频转换工具。

## 锁定目标

把用户提供的 7 条长音频和仓库确定性生成的短促音效接入现有音频目录，使全部 31 个稳定 EventId 都拥有可解析、可预载、可 cook 的 SoundWave；保持玩法只发布稳定语义，音频缺失或加载失败仍不得阻塞玩法。

- `Music.Menu`、`Music.Shop`、`Music.Boss`、`Music.Death`、`Music.Victory` 使用用户提供的对应 MP3。
- `Ambience.Arena`、`Ambience.Rain` 使用用户提供的对应 MP3。
- 保留已接入的 `Music.Encounter` / `The_Iron_Waltz`，并使所有 Music/Ambience 目录项符合循环状态契约。
- 通过仓库本地、固定种子/固定参数的 Python 合成器生成其余 23 个 OneShot：UI 6、Combat 6、Enemy 3、Boss 3、Echo 3、`CameraMove`、`Revive`。这些是可替换的程序化基线候选，不是最终音效设计。
- 更新权威 XLSX 并确定性生成 CSV；不手改 CSV 形成第二事实来源。
- 资源通过 Unreal Editor Python 正式导入 `.uasset`，不在 Editor 外制造或修改 `.uasset`。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoAudio`；`MOD-ReEcho` 只作为现有稳定调用方审阅，不修改其玩法代码。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`，已加入 `Writes`；关闭前审阅 `MOD-ReEcho`、`ARCHITECTURE.md` 和 `README.md`。
- 设计意图：资源制作/映射、加载、播放策略和 cook 保证继续归音频模块；玩法、UI、敌人和武器只认识 EventId/StateId。
- 权威状态与依赖：`Design/Data/ReEchoAudioEvents.xlsx` 仍是目录事实来源；`Content/Data/audio_events.csv` 是生成运行时来源；SoundWave 是目录软路径指向的播放资产。依赖方向保持 `ReEcho -> ReEchoAudio`。
- 决策记录：长音频由 UE 5.8 直接导入 MP3，避免安装转换工具；短音效使用确定性仓库生成器，便于复现和替换；所有动态 CSV 软引用资产通过显式 cook 路径进入包；不为本轮资源增加变体表。
- 相关文档同步范围：维护 `MOD-ReEchoAudio.md` 的资源位置、加载/cook、循环和衰减事实；`ARCHITECTURE.md`、`README.md` 与 `MOD-ReEcho.md` 关闭前逐项审阅，只有拓扑、路由或受影响契约变化时才修改正文。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoAudio.md`：待按最终资源、cook、非阻塞与衰减实现更新；
  - `ARCHITECTURE.md`：待审阅运行时拓扑；
  - `README.md`：待审阅路由标识；
  - `MOD-ReEcho.md`：待审阅稳定调用方契约。

## 锁定验收

- [ ] 权威 XLSX 与生成 CSV 保持锁定 14 列和完整 31 个稳定 ID；全量 `--check` 通过，旧非音频 CSV 字节不变。
- [ ] 31 个目录行都有非空合法软路径；Editor 中全部路径解析为 `USoundWave`，资产类、循环属性和 2D/3D 声道约束符合目录用途。
- [ ] 7 个用户长音频及 23 个确定性短音效拥有准确来源/生成记录；用户提供资源的工具/授权状态不被 AI 猜测。
- [ ] Music/Ambience 循环状态不会因未驻留资产进行同步加载；预载未完成/失败仍安全且可重试，不阻塞玩法线程。
- [ ] `AttenuationMin/Max` 实际影响空间 OneShot/Loop 的 Unreal 播放组件；非空间事件不启用衰减覆盖。
- [ ] CSV 文本软引用指向的 `/Game/ReEcho/Audio/**` 在 Windows cook/package 中存在，不依赖地图或硬引用偶然带入。
- [ ] author 脚本重建的音频目录不会清空已批准资产绑定；生成器和目录映射有聚焦测试。
- [ ] 音频模块没有新增对 `ReEcho`、Combat、Weapons、Enemies、Echo 或 UI 类型的反向依赖；存档、录制和玩法结果不变。
- [ ] 必需数据检查、项目校验、C++ 格式化（可用时）、聚焦自动化、Editor build、最终 `-FullRebuild`、预构建包刷新和 `git diff --check` 通过。
- [ ] 用户在 PIE 中抽听 7 条长音频和每组短音效，验收可听性、循环、响度、空间感及是否需要替换；AI 不代签主观结果。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

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
5. 在 `ReEchoAudio` 内完成非阻塞状态加载、衰减应用和资产 cook 所需的最小运行时/配置硬化。
6. 执行数据、静态、自动化、资产加载、cook、构建和文档审阅；把候选交给用户听感验收。
7. 人工验收通过后，在最终集成候选上执行 `-FullRebuild`、刷新预构建包、最终 fetch 审计并发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 生成 | 聚焦 Python 测试；重复生成并比较哈希 | 23 个 WAV 名称、格式、时长和字节确定性 |
| 数据 | `python scripts/data/sync_xlsx_to_csv.py --check` | 三份权威 XLSX 与所有生产 CSV 无漂移 |
| 资产 | UE Python 导入/加载审计 | 31 个路径均解析为 SoundWave；循环与声道属性准确 |
| 静态 | `python scripts/validate_project.py`、反向 include/依赖审计 | Schema、来源、cook、模块边界通过 |
| C++ | `.clang-format`（可用时）、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 编译链接五个 Runtime Module |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Audio` 或等价配置化 Editor 聚焦运行 | Catalog、预载、衰减、设置和假后端测试通过 |
| Cook | Windows clean cook/package 及 staged asset audit | 动态软引用音频进入包且可加载 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`、项目校验、`git diff --check` | 最终集成版本与精选预构建包匹配 |
| 人工 | PIE 抽听矩阵 | 用户报告通过或列出需替换/调音资源 |

## 执行记录

### 变化

### 证据

### 剩余风险

- 用户提供的 7 个 MP3 的生成平台、账号授权范围或许可证尚未记录；关闭/最终发布前必须由用户确认，AI 不推断。
- 程序化短音效是技术可用基线，最终音色和响度可能需要用户替换或重调。

### 人工验收结果/请求

`PendingBeforeClose`：用户抽听所有长音频类别及 UI/Combat/Enemy/Boss/Echo 短音效组，确认循环、响度、空间感和是否接受程序化基线。

### 架构文档审阅结果
