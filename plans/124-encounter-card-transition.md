# Plan 124 - 程序 - 关卡倒计时结算序列与抽卡过渡

## 协调

- Planner 负责人：当前程序用户授权的 Planner（Codex）。
- Executor 负责人：当前程序用户授权的 Executor（Codex，同一任务分阶段执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@9c28276764735b4e38aff04f2932664866a223e3`；Plan-only 发布后以含本 Plan 的最新 `origin/main` 为实现基线。
- 本地实现方式（可选，仅作交接说明）：独立工作树 `C:\tmp\ReEcho-plan124-encounter-card-transition`，本地分支 `codex/plan124-encounter-card-transition`；不在主工作区实现，不推送任务分支。
- 依赖 / 阻塞：依赖 `AReEchoEncounterDirector` 的权威剩余时间/结束事件、Plan54 的同 Stage 局间冻结、现有 TraitChoice 页面和 UI Manager/Flow Coordinator；依赖目标 UE 5.8 的 `MediaAssets` 与 `ImgMedia`。输入为 `F:\MiniGame\frames\frame_0000.png..frame_0120.png`。
- Writes: 本 Plan；`ReEcho.uproject`、`Config/DefaultGame.ini`（仅在 Cook 收集要求时）；`Source/ReEcho/ReEcho.Build.cs`；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/UI/Framework/ReEchoUIScreenTypes.h`、`ReEchoUIManagerSubsystem.*`、必要的 `ReEchoUIFlowCoordinatorSubsystem.*`；新增 `Source/ReEcho/{Public,Private}/UI/ReEchoEncounterTransitionWidget.*`；相关聚焦自动化；新增/维护序列导入、作者ing 与资产审计脚本；`Content/Movies/EncounterTransition/CardChoice/**`；`Content/ReEcho/UI/EncounterTransition/**`；`Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`（经程序用户确认，仅补 `UBillboardComponent` 前置声明）；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`（经程序用户确认，仅修正两个 DevelopmentOnly 作者ing接口的 Shipping 编译条件）；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`；必要的精选 Editor 预构建包。
- Stable Reads: 输入 PNG 序列；`AReEchoEncounterDirector`；`AReEchoGameMode::HandleEncounterEnded/PrepareEncounterIntermission/ProceedToPostEncounterUI/ShowTraitCardChoice`；Plan54；现有 Encounter HUD、TraitChoice WBP/C++、UI Manager/Flow Coordinator、Run/Recording/Enemy Roster 公共契约。
- 影响模式：`SharedContract`，因为修改 Encounter 结束时序和 UI Screen 生命周期，并新增二进制 UI/Media 资产；不改变 Encounter、Run、Cards 或 Enemy 状态所有权。
- 兼容承诺 / 下游操作：Encounter 权威时间仍由 Director 唯一拥有；普通计时关卡在显示 03 时同时开始全屏效果与序列，0 秒才结束 Encounter 并冻结局间战斗；序列提前播完不得提前结算，媒体失败不会阻断原有抽卡或商店流程。卡牌候选仍由现有 Run 事务生成。
- 明确排除：不修改 Encounter 时长、波次、卡牌生成/费用/刷新、商店、Boss 胜利、死亡、录制内容、同 Stage Roster 连续性或角色数值；不把 121 帧导入为常驻未压缩 Texture2D 数组；不在 Unreal Editor 外手改 `.uasset`；不推送任务分支。

## 锁定目标

在每个普通限时 Encounter 的权威剩余时间进入 3 秒阈值、HUD 显示 03 时，同时叠加全屏效果并播放 `frame_0000..frame_0120` 序列；该阶段仍保持正常战斗和 HUD 倒计时。只有权威时间实际到 0 才结束本局并执行 Plan54 局间冻结；序列按 40 FPS 播放约 3 秒，以 Fill 等比铺满视口并居中裁切，不拉伸。

到 0 且 MediaPlayer 发出 `OnEndReached` 后打开下一阶段对应的 TraitChoice 或 Shop 页面，再在 0.4 秒内淡出序列层。若序列提前完成则必须等待权威 0 秒；媒体明确打开/播放失败时幂等回退到原有后局流程，经过时间本身不得绕过播放完成门。Boss、死亡和非计时结束不得播放此序列。新增 `GMTransition3` 仅把有效普通 Encounter 的权威时钟推进到剩余 3 秒，用于完整预览。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Encounter`、`MOD-ReEchoUI`；Run、Cards、Recording、Enemies 仅消费既有契约。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md` 已加入 Writes；关闭前审阅 `ARCHITECTURE.md` 与 `README.md`。
- 设计意图：Gameplay 继续拥有时间、结算与冻结；独立非交互 UI Screen 只拥有倒计时全屏材质、媒体播放、Fill 布局、完成/失败通知与淡出，不反向决定 Encounter 或卡牌结果。
- 权威状态与依赖：Director 的 `GetRemainingTime()` 和 `OnEncounterEnded` 是唯一时间源。GameMode 持有显式表现状态 `None/CountdownPostProcess/PlayingSequence/FadingToCardChoice/Completed`，负责一次性阈值、结束原因资格和回退；Widget 使用 MediaPlayer/ImgMediaSource/MediaTexture 展示位于 Movies 下的流式 PNG 序列。
- 决策记录：
  - 采用 `ImgMedia` 流式读取 121 帧，而不是创建约 957 MiB RGBA 常驻纹理数组；原始 PNG 放在 `Content/Movies` 以支持 Staged/Cook 的非资产媒体文件。
  - 40 FPS 使索引 0 在 0 秒、索引 120 在 3 秒；Widget 不循环，只有 MediaPlayer 的 `OnEndReached` 才标记正常完成。
  - Fill 由保持宽高比的 ScaleBox/等价布局完成，超宽或窄屏居中裁切；不把 1920×1080 强行拉伸。
  - 3～1 秒全屏效果与序列采用同一个 Director 阈值启动，剩余时间从 3 到 1 秒时叠色强度由 0 过渡到 1，1 到 0 秒保持 1；Widget 只消费规范化强度，不拥有计时。
  - `HandleEncounterEnded()` 在权威 0 秒成功完成 Run/Recording 后立即调用 `PrepareEncounterIntermission()`；只有此后媒体 `OnEndReached` 或明确失败才可进入下一 UI，禁止序列提前完成或单纯超时导致提前结算。
  - 每个普通限时 Encounter 都进入序列；Boss、死亡与手动非计时结束保持既有 `ProceedToPostEncounterUI()` 语义。
  - 完成和故障回退共用单一幂等终点：按 Run Phase 初始化 TraitChoice 或 Shop，再淡出/关闭 Transition Screen；重复媒体回调不得重复推进 Run。
- 相关文档同步范围：关闭前审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`；若模块拓扑和稳定索引未变化，前两者只在 Plan 记录无需修改。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：待记录时间归零资格、立即局间冻结和幂等回退流程；
  - `MOD-ReEchoUI.md`：待记录非交互 Transition Screen、Fill 媒体和淡入抽卡契约；
  - `ARCHITECTURE.md` / `README.md`：待审阅，预计 Runtime Module 拓扑与稳定标识不变。

## 锁定验收

- [ ] 输入序列严格为 `frame_0000..frame_0120`、121 帧、统一 1920×1080；仓库媒体目录的文件名、数量、尺寸和哈希清单可审计。
- [ ] 剩余时间大于 3 秒无效果；3～1 秒全屏后处理按权威时间渐强；1～0 秒保持；效果不暂停或改变战斗、HUD、录制与波次。
- [ ] 每个普通限时 Encounter 在 HUD 显示 03 时开始序列；Boss 不播放。03～00 战斗继续，到 0 后局间冻结立即生效，序列完成也不得提前结算。
- [ ] 121 帧按 40 FPS、非循环、Fill 等比居中裁切播放；16:9、超宽和窄屏均不拉伸，首尾帧与约 3 秒总时长正确。
- [ ] 序列完成后按 Run Phase 初始化 TraitChoice 或 Shop，序列层 0.4 秒淡出后页面可交互；只生成/打开一次。
- [ ] 媒体打开失败或播放错误安全回退；未收到 `OnEndReached` 时无论经过多久都不得进入抽卡/商店；重复回调不重复结算。页面关闭、下一 Encounter、死亡、胜利、重启、主菜单和 travel 清理媒体状态。
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
3. 新增非交互 `EncounterTransition` Screen 和 C++ Widget：全屏效果、40 FPS 非循环序列、Fill 布局、完成/错误 Delegate、0.4 秒淡出和幂等清理。
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
| 状态机 | 聚焦 C++ 自动化 | 03 阈值、00 权威门、`OnEndReached`/错误幂等、时间不可绕过、排除路径通过 |
| UI/资产 | Unreal 作者ing 脚本校验、资产加载、`CompileAllBlueprints` | ImgMediaSource/Player/Texture/Material/WBP 可加载，Fill/非循环/参数绑定正确 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新精选预构建包 |
| 回归 | `ReEcho.Encounter`、`ReEcho.StageTransition`、相关 `ReEcho.UI`、Run/Recording 聚焦集合 | 结算、冻结、抽卡、Boss、死亡与录制无回归 |
| Cook | 干净 Windows 策划测试包、Staged/IoStore manifest 与启动烟测 | Movies 序列和 ImgMedia 运行时模块进包，测试包可启动播放 |
| 最终发布 | `Build-Editor.cmd -Configuration Development -FullRebuild`、预构建检查、项目校验、`git diff --check` | 最终组合源码、资产与精选 Editor 包匹配 |
| 人工 PIE | 普通倒计时、宽高比分辨率、Boss/死亡/失败回退矩阵 | 用户确认视觉、时机、Fill、淡入与可交互结果 |

## 执行记录

### 变化

- 2026-08-26：完成只读输入与运行路径审计；用户确认使用 Fill，并明确序列只在关卡时间到 0 后播放，3～1 秒为额外全屏后处理。
- 2026-08-26：新增非交互 Transition Screen 与 C++ Widget；03 时屏幕空间叠色只读投影 Director 时间并以 ImgMedia/MediaTexture 播放 40 FPS PNG，0 秒自然结束时立即复用 Plan54 局间冻结，收到播放完成后初始化下一 UI 并淡出，明确失败时幂等回退。
- 2026-08-26：同步 121 张 PNG 到 `Content/Movies/EncounterTransition/CardChoice`，生成不含机器绝对路径的 SHA-256 清单；启用 ImgMedia、声明 MediaAssets/ImgMedia 依赖与 Movies NonUFS RuntimeDependency。
- 2026-08-26：新增策略自动化，覆盖 3～1 秒强度、0 秒资格、Boss/死亡/Shop/最终关排除与 Fill 宽高比；同步 `MOD-ReEcho.md`、`MOD-ReEchoUI.md`。
- 2026-08-26：程序用户确认继续并要求推送前人工验收；按确认将 `ReEchoEnemyActor.h` 的一行 `UBillboardComponent` 前置声明加入 Writes，解除 Shipping 独立编译基线阻塞，不改变怪物运行时行为。
- 2026-08-26：程序用户再次确认继续；把两个无条件反射的 DevelopmentOnly VFX 作者ing实现移出 `WITH_DEV_AUTOMATION_TESTS`，测试辅助函数仍保持条件编译，不改变 VFX 运行时路由。
- 2026-08-26：用户 PIE 反馈“没有效果”并修订时序：每个普通计时关卡从倒计时 03 开始播放；实现改为 03 同步启动媒体与全屏效果、00 才允许结算，下一阶段兼容抽卡/商店，并新增 `GMTransition3` 权威时钟预览指令。
- 2026-08-26：修订后 Editor Development 构建、`ReEcho.UI.EncounterTransition` 策略自动化、媒体逐帧审计、项目总校验与 `git diff --check` 通过；进入用户 PIE 复验，未提交、未推送。
- 2026-08-26：用户复验仍不可见；日志证明 GM、Screen、ImgMedia Open 均成功，定位为瞬态 MediaTexture 直接作为 Slate Brush 未产出可见画面。新增 `/Game/ReEcho/UI/EncounterTransition` 下四个正式媒体/UI Material 资产，并改为 Native Widget 通过正式 UI Material 采样 MediaTexture；原始 121 帧仍保留在 Movies 流式目录。
- 2026-08-26：用户确认 `MP_EncounterTransition` 自身播放成功但游戏内未显示，并要求播放完才进入下一 UI；源 PNG Alpha 抽样正常，显式设置 Image/White Brush，并删除 3.5 秒正常完成旁路，只接受 `OnEndReached` 或明确失败。随后运行日志证明曾尝试的 External sampler 不适用于目标 UE 5.8，已纠正为 Color。
- 2026-08-26：运行日志证明 External sampler 在目标 UE 5.8 编译失败并回退默认材质，纠正为引擎要求的 Color sampler；同时清理正式 MediaPlayer 资产上的重复 Delegate、强制 Widget 全视口 anchors 并记录真实几何。按用户补充要求，每次 Open 后显式 Seek 0 再 Play，禁止继承资产编辑器预览位置。
- 2026-08-26：最新 PIE 证明 Screen 为 2244×1081、ZOrder 99、媒体从 0 播放并到达末帧、00/末帧双门控正常，问题收敛为 MediaTexture 到 UI Material 的像素合成。按用户“修复”授权，材质不再消费 ImgMedia Alpha，固定 `MediaOpacity=1` 输出 RGB，以黑色透明区覆盖游戏；新增媒体 surface 尺寸日志。
- 2026-08-26：运行日志进一步证明 `MT_EncounterTransition` 始终为 2×2；UE 5.8 `UMediaTexture::UpdatePlayerAndQueue` 只在 Player GUID 变化时创建 sample queue/sink。修复为每个 Widget 创建专属运行时 Player/Texture，动态材质参数 `MediaTexture` 指向该 Texture，确保新 GUID 建立新 video sample sink；正式 IMS、MP、MT、Material 资产仍保留供审计。
- 2026-08-26：运行时专属 Texture 复验仍只观测到 2×2。按 Epic UE 5.8 Media Framework Technical Reference 的兼容建议切换为 H.264 MP4：由 121 张 PNG 生成 `EncounterTransition.mp4`（1920×1080、40 FPS、121 帧、3.025 秒、H.264/yuv420p），新增正式 `FMS_EncounterTransition`，运行时仅打包 MP4 并保持 Opened→Seek 0→Play 及 00/EndReached 双门控。
- 2026-08-26：用户纠正最终产品时序：03→01 只保留全屏后处理，权威倒计时到 00 后才冻结战场并从第 0 帧启动媒体；删除 03 秒提前播放及“媒体提前结束等待 00”的设计，改为媒体末帧后才进入 TraitChoice/Shop。
- 2026-08-26：用户要求动画显示在视口最上层；Transition 层由 99 提升为 10000。像素链恢复官方 UI Material 路径，并统一正式/运行时 MediaTexture 的 NewStyleOutput 与 Color sampler，MID 参数绑定当前运行时 1920×1080 MediaTexture。
- 2026-08-27：纯色对照证明原生 WidgetTree 在 `NativeConstruct` 创建过晚，子控件未进入 Slate 绘制树；改为 `RebuildWidget` 前创建后画面恢复。按用户要求移除紫色诊断底板，并由丢失 Alpha 的 H.264 MP4 切换为 Hap Alpha MOV（RGBA、1920×1080、40 FPS、121 帧、3.025 秒），材质恢复媒体 A→Opacity，Win64 启用 HAPMedia。

### 证据

- 输入目录含连续 `frame_0000.png..frame_0120.png` 共 121 张，全部为 1920×1080；40 FPS 下末帧时间为 3.0 秒。
- `AReEchoGameMode::HandleEncounterEnded()` 当前在成功结算后等待 0.5 秒才调用 `ProceedToPostEncounterUI()`；Plan54 已证明只设置 `bEncounterTransitioning` 不足以冻结独立 Enemy Host，因此新序列前必须立即调用 `PrepareEncounterIntermission()`。
- 本机目标引擎包含启用默认的 ImgMedia 插件及 Win64 Runtime 模块，允许进入实现；Cook 可用性仍是锁定验收项。
- 媒体同步 `--check` 通过：121/121、1920×1080、40 FPS 元数据和逐帧 SHA-256 与输入一致。
- `.clang-format` 已执行；格式化后 Editor Development 构建通过并刷新精选预构建包。
- 自动化通过：`ReEcho.UI.EncounterTransition` 1/1、`ReEcho.StageTransition` 3/3、`ReEcho.Encounter` 4/4；仓库包装器会先打印非 Win64 SDK `MainVersion` 噪声，实际结果以各次 `Saved/Logs/ReEcho.log` 的 `TEST COMPLETE. EXIT CODE: 0` 为准。
- `python scripts/validate_project.py` 与 `git diff --check` 通过。
- 干净 Win64 Shipping 打包已进入组合编译，但被主线既有 `ReEchoEnemyActor.h` 缺少 `UBillboardComponent` 前置声明拦截；Editor 候选与 Plan124 文件未报编译错误。该基线修复不在当前 Writes，等待用户确认是否扩大一行公共头范围后重跑 Cook。
- 用户确认上述一行声明修复后，Shipping 已越过该错误；第二次组合链接暴露另一项主线既有缺口：`SetNiagaraSystemSpriteFacingOwnerUp` 与 `SetNiagaraSystemMeshFacingCameraPlane` 是无条件反射的 DevelopmentOnly UFUNCTION，但实现误置于 `WITH_DEV_AUTOMATION_TESTS`，导致 Shipping 缺少两个符号。需再次确认是否把这两个实现移出测试条件块后重跑 Cook。
- 用户确认第二项最小扩张后，两个 VFX 作者ing实现已移出测试条件块；Shipping/Editor 组合编译、干净 Cook、Stage、Archive 和 10 秒成品启动烟测全部通过。
- `FinalCopyWin64_NonUFSFiles.txt` 精确包含 `frame_0000.png..frame_0120.png` 共 121 项，首尾各一次；归档包同目录存在全部 121 帧。最终 Development FullRebuild 与精选预构建刷新通过，随后 `ReEcho.UI.EncounterTransition` 再次 1/1 通过。
- 正式资产作者ing与重载验证通过：`IMS_EncounterTransition`、`MP_EncounterTransition`、`MT_EncounterTransition`、`M_UI_EncounterTransition` 均保存并通过 Unreal Asset Validation；接入后 Editor Development 构建和 `ReEcho.UI.EncounterTransition` 1/1 通过，无缺失硬引用或材质编译错误。

### 剩余风险

- 全屏后处理材质的具体观感、强度和 Fill 裁切需用户 PIE 判断。
- 121 张 PNG 约 31.3 MiB，会增加仓库与包体；选择流式媒体是为避免约 957 MiB 未压缩常驻纹理内存，但真实 IO 峰值仍需测试包验证。
- ImgMedia 插件标记 Beta；若目标 Windows Cook/运行时不能稳定打开 PNG 序列，本 Plan 按停止条件报告，不会静默退化为常驻 Texture2D 数组。
- 当前 Cook 尚未到资产收集阶段，不能声称 Movies/ImgMedia 已进入成品包；阻塞点为 `Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h` 中 `UBillboardComponent` 未声明的 Shipping 独立编译基线错误。
- 两项主线 Shipping 基线缺口均经程序用户逐项确认后修复；残余风险只剩实际 PIE 中 ImgMedia 首帧打开延迟、屏幕叠色观感、Fill 裁切和淡入手感，必须由用户验收。

### 人工验收结果/请求

- `PendingBeforeClose`：实现和客观验证完成后，请用户按锁定矩阵执行 PIE 视觉与交互验收。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已更新：记录 Director 时间投影、自然 0 秒资格、Plan54 立即冻结和完成/失败幂等抽卡终点。
- `MOD-ReEchoUI.md` 已更新：记录 Transition Screen 层级、屏幕空间叠色、ImgMedia 流式序列、Fill 与淡出契约。
- `ARCHITECTURE.md` 已审阅、无需修改：Runtime Module 拓扑未改变，MediaAssets/ImgMedia 是 ReEcho 的单向引擎插件依赖。
- `README.md` 已审阅、无需修改：稳定模块标识与阅读路由未变化。
