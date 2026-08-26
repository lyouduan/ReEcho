# Plan 124 - 程序 - 关卡倒计时结算序列与抽卡过渡

## 协调

- Planner 负责人：当前程序用户授权的 Planner（Codex）。
- Executor 负责人：当前程序用户授权的 Executor（Codex，同一任务分阶段执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@9c28276764735b4e38aff04f2932664866a223e3`；Plan-only 发布后以含本 Plan 的最新 `origin/main` 为实现基线。
- 本地实现方式（可选，仅作交接说明）：独立工作树 `C:\tmp\ReEcho-plan124-encounter-card-transition`，本地分支 `codex/plan124-encounter-card-transition`；不在主工作区实现，不推送任务分支。
- 依赖 / 阻塞：依赖 `AReEchoEncounterDirector` 的权威剩余时间/结束事件、Plan54 的同 Stage 局间冻结、现有 TraitChoice 页面和 UI Manager/Flow Coordinator；依赖目标 UE 5.8 的 `MediaAssets` 与 `ImgMedia`。输入为 `F:\MiniGame\frames\frame_0000.png..frame_0120.png`。
- Writes: 本 Plan；`ReEcho.uproject`、`Config/DefaultGame.ini`（仅在 Cook 收集要求时）；`Source/ReEcho/ReEcho.Build.cs`；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/UI/Framework/ReEchoUIScreenTypes.h`、`ReEchoUIManagerSubsystem.*`、必要的 `ReEchoUIFlowCoordinatorSubsystem.*`；新增 `Source/ReEcho/{Public,Private}/UI/ReEchoEncounterTransitionWidget.*`；相关聚焦自动化；新增/维护序列导入、作者ing 与资产审计脚本；`Content/Movies/EncounterTransition/CardChoice/**`；`Content/ReEcho/UI/EncounterTransition/**`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`；必要的精选 Editor 预构建包。
- Stable Reads: 输入 PNG 序列；`AReEchoEncounterDirector`；`AReEchoGameMode::HandleEncounterEnded/PrepareEncounterIntermission/ProceedToPostEncounterUI/ShowTraitCardChoice`；Plan54；现有 Encounter HUD、TraitChoice WBP/C++、UI Manager/Flow Coordinator、Run/Recording/Enemy Roster 公共契约。
- 影响模式：`SharedContract`，因为修改 Encounter 结束时序和 UI Screen 生命周期，并新增二进制 UI/Media 资产；不改变 Encounter、Run、Cards 或 Enemy 状态所有权。
- 兼容承诺 / 下游操作：Encounter 权威时间仍由 Director 唯一拥有；3～1 秒效果仅表现，0 秒才结束 Encounter、冻结局间战斗并播放序列；序列或媒体失败不会阻断原有结算。卡牌候选仍由现有 Run 事务生成，页面继续由现有 TraitChoice 屏承载。
- 明确排除：不修改 Encounter 时长、波次、卡牌生成/费用/刷新、商店、Boss 胜利、死亡、录制内容、同 Stage Roster 连续性或角色数值；不把 121 帧导入为常驻未压缩 Texture2D 数组；不在 Unreal Editor 外手改 `.uasset`；不推送任务分支。

## 锁定目标

在普通限时 Encounter 的权威剩余时间进入 3～1 秒区间时叠加全屏后处理；该阶段仍保持正常战斗和 HUD 倒计时。只有权威时间实际到 0 且玩家存活、后续阶段确为免费抽卡时，才立即结束本局、执行 Plan54 局间冻结并播放 `frame_0000..frame_0120` 结算序列。序列按 40 FPS 播放约 3 秒，以 Fill 等比铺满视口并居中裁切，不拉伸。

序列播放完成后先打开并初始化现有 TraitChoice 页面，再在 0.4 秒内淡出序列层，使画面逐渐过渡到可交互抽卡界面。媒体打开失败、播放错误或超时必须幂等回退到原有抽卡流程；Boss 提前结束、死亡、直接商店和非时间归零结束不得播放此序列。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Encounter`、`MOD-ReEchoUI`；Run、Cards、Recording、Enemies 仅消费既有契约。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md` 已加入 Writes；关闭前审阅 `ARCHITECTURE.md` 与 `README.md`。
- 设计意图：Gameplay 继续拥有时间、结算与冻结；独立非交互 UI Screen 只拥有倒计时全屏材质、媒体播放、Fill 布局、完成/失败通知与淡出，不反向决定 Encounter 或卡牌结果。
- 权威状态与依赖：Director 的 `GetRemainingTime()` 和 `OnEncounterEnded` 是唯一时间源。GameMode 持有显式表现状态 `None/CountdownPostProcess/PlayingSequence/FadingToCardChoice/Completed`，负责一次性阈值、结束原因资格和回退；Widget 使用 MediaPlayer/ImgMediaSource/MediaTexture 展示位于 Movies 下的流式 PNG 序列。
- 决策记录：
  - 采用 `ImgMedia` 流式读取 121 帧，而不是创建约 957 MiB RGBA 常驻纹理数组；原始 PNG 放在 `Content/Movies` 以支持 Staged/Cook 的非资产媒体文件。
  - 40 FPS 使索引 0 在 0 秒、索引 120 在 3 秒；Widget 不循环，播完后发出一次完成事件。3.5 秒独立超时只作故障保险。
  - Fill 由保持宽高比的 ScaleBox/等价布局完成，超宽或窄屏居中裁切；不把 1920×1080 强行拉伸。
  - 3～1 秒后处理采用 UMG 全屏动态材质覆盖世界与 HUD：剩余时间从 3 到 1 秒时强度由 0 过渡到 1，1 到 0 秒保持 1；到 0 开始序列时清零。材质只消费规范化强度，不拥有计时。
  - `HandleEncounterEnded()` 在成功完成 Run/Recording 后立即调用 `PrepareEncounterIntermission()`，先冻结玩家与保留敌人，再开始序列；禁止继续使用旧 0.5 秒延时让战斗对象偷跑。
  - 仅“自然计时归零 + 玩家存活 + 完成后阶段为 CardChoice”进入序列。其他路径保持既有 `ProceedToPostEncounterUI()` 语义。
  - 完成和故障回退共用单一幂等终点：初始化 TraitChoice，再淡出/关闭 Transition Screen；重复媒体回调或超时不得重复生成卡牌或推进 Run。
- 相关文档同步范围：关闭前审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`；若模块拓扑和稳定索引未变化，前两者只在 Plan 记录无需修改。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：待记录时间归零资格、立即局间冻结和幂等回退流程；
  - `MOD-ReEchoUI.md`：待记录非交互 Transition Screen、Fill 媒体和淡入抽卡契约；
  - `ARCHITECTURE.md` / `README.md`：待审阅，预计 Runtime Module 拓扑与稳定标识不变。

## 锁定验收

- [ ] 输入序列严格为 `frame_0000..frame_0120`、121 帧、统一 1920×1080；仓库媒体目录的文件名、数量、尺寸和哈希清单可审计。
- [ ] 剩余时间大于 3 秒无效果；3～1 秒全屏后处理按权威时间渐强；1～0 秒保持；效果不暂停或改变战斗、HUD、录制与波次。
- [ ] 只有普通限时 Encounter 到 0 才开始序列；3、2、1 秒均不会提前播放。到 0 后局间冻结立即生效，敌人、玩家、投射物和玩法计时不在序列期间推进。
- [ ] 121 帧按 40 FPS、非循环、Fill 等比居中裁切播放；16:9、超宽和窄屏均不拉伸，首尾帧与约 3 秒总时长正确。
- [ ] 序列完成后现有 TraitChoice 已初始化，序列层 0.4 秒淡出后卡牌可交互；只生成/打开一次。
- [ ] 媒体打开失败、播放错误和 3.5 秒超时均安全回退；重复回调不重复结算或抽卡。页面关闭、下一 Encounter、死亡、胜利、重启、主菜单和 travel 清理媒体与计时器。
- [ ] Boss 提前胜利、玩家死亡、直接 Shop 与非计时结束不播放序列，既有路径无回归。
- [ ] ImgMedia/MediaAssets 在 Editor 加载与 Windows Cook 中可用，Movies 序列被打包；不存在 121 张常驻 Texture2D 的内存回归。
- [ ] `.clang-format`、聚焦自动化、Editor Development 构建、项目校验、`git diff --check`、最终 FullRebuild 与必要资产加载/Cook 检查通过。
- [ ] 用户在 PIE 验收 3～1 秒效果强度、0 秒切换、Fill 裁切、序列观感、淡入抽卡与失败回退；AI 不代签主观视觉结果。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@9c28276764735b4e38aff04f2932664866a223e3`；规划时远端最大编号为 Plan123，本 Plan 使用 124。
- 引擎/构建可用性：本机 `F:\UnrealEngine_Dev\UE_5.8` 存在 `Engine/Plugins/Media/ImgMedia/ImgMedia.uplugin`，含 Win64 Runtime 模块；准确候选仍须证明项目启用、Media Source 加载和 Cook 收集。规划阶段未复用旧构建证据。
- 现有聚焦测试结果：Plan-only 阶段仅执行项目校验和 `git diff --check`；实现前记录 Encounter/UI/StageTransition 相关基线。
- 共享契约 / 难合并资源风险：`ReEchoGameMode.*`、UI Screen 枚举/Manager、`.uproject`、二进制 WBP/Media 资产和精选预构建包是共享热点；发布前任何同路径远端变化均须重新审计并在最新 main 组合验证。
- 基线损坏时的停止条件：Plan124 编号被远端占用；输入序列缺帧/尺寸变化；ImgMedia 在项目或 Windows Cook 中不可用；自然计时结束原因无法可靠区别于 Boss/死亡；实现需要改变卡牌、关卡或 Enemy 产品规则；出现同路径真实语义冲突。

## 实现提纲

1. 发布并核验本 Plan；从新 `origin/main` 继续，读取 Executor 规则、相关 `LESSONS` 小节和最小配对实现，记录聚焦测试基线。
2. 新增确定性媒体同步/审计脚本，把 121 张 PNG 复制到 `Content/Movies/EncounterTransition/CardChoice`，验证连续编号、尺寸、哈希和零额外文件；使用 Unreal 作者ing 脚本创建 ImgMediaSource、MediaPlayer、MediaTexture、UI 材质实例与 Transition WBP。
3. 新增非交互 `EncounterTransition` Screen 和 C++ Widget：动态后处理材质、40 FPS 非循环序列、Fill 布局、完成/错误 Delegate、3.5 秒保险、0.4 秒淡出和幂等清理。
4. 在 GameMode 中建立显式表现状态与自然归零资格；Tick 只投影 3～0 秒强度，`HandleEncounterEnded()` 在 0 秒成功结算后立即进入 Plan54 局间冻结并播放序列，其他结束原因沿用既有流程。
5. 让序列完成/故障统一初始化 TraitChoice 并淡出；为下一 Encounter、死亡、Boss、重启、主菜单和 travel 增加幂等清理。
6. 新增阈值、资格、幂等、失败回退、Screen 生命周期和资产存在性聚焦自动化；维护 `MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`。
7. 格式化并执行 Editor 构建、聚焦自动化、项目校验、媒体资产加载、Windows Cook/manifest 与最终 FullRebuild；冻结最终候选并审计远端差异。
8. 用户完成 PIE 主观验收后更新 Plan、关闭并按 `main-publish-lock` 发布最终候选；确认远端后安全清理任务 worktree。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan 结构、编号与仓库规则通过 |
| 输入/媒体 | 序列同步脚本 `--check`、图片尺寸/连续性/哈希审计 | 121 张 1920×1080 PNG 与输入一致，无缺帧/多帧 |
| 状态机 | 聚焦 C++ 自动化 | 3～1 强度、0 秒资格、完成/错误/超时幂等、排除路径通过 |
| UI/资产 | Unreal 作者ing 脚本校验、资产加载、`CompileAllBlueprints` | ImgMediaSource/Player/Texture/Material/WBP 可加载，Fill/非循环/参数绑定正确 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新精选预构建包 |
| 回归 | `ReEcho.Encounter`、`ReEcho.StageTransition`、相关 `ReEcho.UI`、Run/Recording 聚焦集合 | 结算、冻结、抽卡、Boss、死亡与录制无回归 |
| Cook | 干净 Windows 策划测试包、Staged/IoStore manifest 与启动烟测 | Movies 序列和 ImgMedia 运行时模块进包，测试包可启动播放 |
| 最终发布 | `Build-Editor.cmd -Configuration Development -FullRebuild`、预构建检查、项目校验、`git diff --check` | 最终组合源码、资产与精选 Editor 包匹配 |
| 人工 PIE | 普通倒计时、宽高比分辨率、Boss/死亡/失败回退矩阵 | 用户确认视觉、时机、Fill、淡入与可交互结果 |

## 执行记录

### 变化

- 2026-08-26：完成只读输入与运行路径审计；用户确认使用 Fill，并明确序列只在关卡时间到 0 后播放，3～1 秒为额外全屏后处理。

### 证据

- 输入目录含连续 `frame_0000.png..frame_0120.png` 共 121 张，全部为 1920×1080；40 FPS 下末帧时间为 3.0 秒。
- `AReEchoGameMode::HandleEncounterEnded()` 当前在成功结算后等待 0.5 秒才调用 `ProceedToPostEncounterUI()`；Plan54 已证明只设置 `bEncounterTransitioning` 不足以冻结独立 Enemy Host，因此新序列前必须立即调用 `PrepareEncounterIntermission()`。
- 本机目标引擎包含启用默认的 ImgMedia 插件及 Win64 Runtime 模块，允许进入实现；Cook 可用性仍是锁定验收项。

### 剩余风险

- 全屏后处理材质的具体观感、强度和 Fill 裁切需用户 PIE 判断。
- 121 张 PNG 约 31.3 MiB，会增加仓库与包体；选择流式媒体是为避免约 957 MiB 未压缩常驻纹理内存，但真实 IO 峰值仍需测试包验证。
- ImgMedia 插件标记 Beta；若目标 Windows Cook/运行时不能稳定打开 PNG 序列，本 Plan 按停止条件报告，不会静默退化为常驻 Texture2D 数组。

### 人工验收结果/请求

- `PendingBeforeClose`：实现和客观验证完成后，请用户按锁定矩阵执行 PIE 视觉与交互验收。

### 架构文档审阅结果

- 待实现完成后按“架构影响与设计决策”逐项填写。
