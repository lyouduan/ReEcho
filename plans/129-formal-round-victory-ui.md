# Plan 129 - 程序 - 正式本轮胜利与失败界面接入

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`Passed`（2026-08-27，用户完成正式胜利/失败页面、按钮反馈及历史 UI 清理的手工检查，并明确授权推送远端合并主分支）。
- 本地规划 / 实现基线：`origin/main@9a1b1c58`。
- 本地实现方式（可选，仅作交接说明）：`plan/129-formal-round-victory-ui`，`ReEcho-plan129-formal-round-victory-ui` 独立 worktree。
- 依赖 / 阻塞：用户提供 `C:/Users/gavynqiu/Downloads/本轮胜利.svg`、`C:/Users/gavynqiu/Downloads/本轮失败.svg` 与对应 Figma CSS；实施前需保证同克隆 Unreal Editor 锁可用。
- Writes: `plans/129-formal-round-victory-ui.md`；`Content/ReEcho/UI/WBP_ReEchoRestart.uasset`；`Content/ReEcho/Textures/UI/Formal/RoundVictory/**`；`Content/ReEcho/Textures/UI/Formal/RoundDefeat/**`；`Content/SourceArt/UI/Formal/RoundVictory/**`；`Content/SourceArt/UI/Formal/RoundDefeat/**`；必要的 `scripts/ue/**` 作者ing/审计脚本；`Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp`；`Source/ReEcho/Private/Tests/ReEchoRestartWidgetTests.cpp`；`Design/UI/ReEcho_UI修改指导.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`。
- Stable Reads: `Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp` 的既有胜利流程；`Design/UI/ReEcho_UI修改指导.md`；`Content/ReEcho/UI/WBP_ReEchoTraitCardEntry.uasset` 的正式卡面视觉；用户提供的 SVG/CSS。
- 影响模式：`SharedContract`。同一 `WBP_ReEchoRestart` 同时承载普通暂停、两种退出确认、Death 与 Victory；本 Plan 只改变 Death/Victory 表现，必须保护暂停、确认状态和稳定绑定。
- 兼容承诺 / 下游操作：保留 `UReEchoRestartWidget::SetVictoryScreen`、`OnRestartRequested`、暂停/退出委托、焦点与页面生命周期；现有 `BindWidgetOptional` 名称和类型保持不变。缺少新正式纹理时只允许保留既有表现或明确失败，不得影响胜利结算、重开和退出流程。
- 明确排除：不新增第二套结算 WBP；不修改 Combat、Run、存档、奖励、音乐或关卡推进逻辑；不伪造当前 API 未提供的击败数、金币或真实已选卡牌数据；不把整张 SVG/截图作为不可编辑的全屏点击层；不重做普通暂停或退出确认页面。

## 锁定目标

在保持现有胜利、失败、重开和返回主菜单逻辑一致的前提下，把 `本轮胜利.svg` 与 `本轮失败.svg` 的正式视觉接入 `WBP_ReEchoRestart` 的 Victory/Death 状态。背景遮罩、结算底板、标题、角色立绘、统计文字、卡牌展示槽和按钮应拆成 Designer 中可见、可独立选择、拖动和缩放的作者ing元素；运行时仅覆盖真实已有的文本内容和状态，不回写布局。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`、`AREA-UI`。不修改公开 Runtime API；仅为既有 WBP 增加 Victory 专用的可选作者ing绑定和表现投影。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，已加入 `Writes`。
- 设计意图：让 WBP 成为 Victory 布局和样式的权威，C++ 继续只负责模式、真实结果文字、委托和生命周期；为后续人工微调保留所见即所得的 1920×1080 设计面。
- 权威状态与依赖：不改变状态所有者或公共 API。`UReEchoRestartWidget` 仍拥有 Victory 模式投影，GameMode 仍拥有胜利流程；WBP 只消费稳定绑定。
- 决策记录：
  1. 复用 `WBP_ReEchoRestart`，不新增独立 Victory 页面，以免分叉暂停/重开流程。
  2. SVG 作为视觉基准拆分导入；文字保持 UMG TextBlock，按钮保持透明真实交互层，避免将整页烘成不可编辑图片。
  3. 当前 `SetVictoryScreen` 仅提供时间碎片与构筑数量；SVG 中无稳定来源的击败数、金币及五张卡只提供 Designer 样例/空槽，不在本 Plan 扩张结果 Schema。
  4. 现有绑定名优先复用；新增纯表现节点使用 `ArtVictory*` / `DesignerVictory*`、`ArtDefeat*` / `DesignerDefeat*` 前缀并设为不参与命中测试。
  5. 失败页不伪造 SVG 中缺少权威来源的击败数与金币，改投影真实到达关卡、构筑数量与时间碎片；“重开整局”复用既有重开委托，“返回主菜单”复用既有主菜单退出委托。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md`；维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 的 Victory 作者ing事实；若拓扑和稳定路由不变则只在执行记录中写明无需修改。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [ ] `WBP_ReEchoRestart` 的 Victory 状态与用户 SVG 在 1920×1080 设计面上结构、相对位置、尺寸、颜色和层级一致；关键元素均可在 Designer 中单独调整。
- [ ] `WBP_ReEchoRestart` 的 Death 状态与用户 SVG 在 1920×1080 设计面上结构、相对位置、尺寸、颜色和层级一致；关键元素均可在 Designer 中单独调整。
- [ ] `RootPanel`、`TitleText`、`MessageText`、`RestartButton`、`ResumeButton`、`QuitButton` 及既有 Victory 美术绑定名称/类型保持可加载，Blueprint 编译无错误。
- [ ] 进入 Victory/Death 后标题、真实摘要与继续/重开/返回主菜单交互正常；普通暂停和两种退出确认的显隐与委托不回归。
- [ ] SVG/CSS 中的纯视觉图层不拦截真实按钮；不提交 SVG 中的完整背景截图作为第二套运行时游戏画面。
- [ ] `python scripts/validate_project.py`、`git diff --check`、`CompileAllBlueprints` 及聚焦 Restart UI 自动化通过。
- [ ] 使用 Unreal MCP/Editor 截图与 SVG 渲染图对照，并由用户完成最终视觉和可用性人工验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@9a1b1c58`。
- 引擎/构建可用性：UE 5.8 安装版；Editor/命令必须经 Git common-dir Unreal 锁串行化。
- 现有聚焦测试结果：实施前记录当前 WBP 加载/编译与 `ReEcho.UI.Restart`（若名称变化则用 `Automation List` 确认）的基线结果。
- 共享契约 / 难合并资源风险：`WBP_ReEchoRestart.uasset` 是同一页面六种状态的二进制资产，属于高冲突共享资产；发布前必须审计远端同路径变化并重新验证所有状态。
- 基线损坏时的停止条件：WBP 当前无法加载/保存、稳定绑定已在远端改变、Unreal 锁由其他活动进程持有，或 SVG 无法可靠拆出正式前景元素时停止对应风险步骤并报告。

## 实现提纲

1. 渲染并解析 SVG/CSS，建立 1920×1080 参考图、视觉图层清单和来源清单。
2. 检查 `WBP_ReEchoRestart` 当前 WidgetTree、稳定绑定与六状态显隐；记录 Victory 基线截图。
3. 将正式 Victory/Defeat 纹理导入独立目录，以两个可编辑 Canvas 的直属子项组装结算底板、标题、立绘、统计区、卡槽样例和按钮表现。
4. 复用现有稳定 TextBlock/Button/Image 绑定；增加 Victory/Defeat 专用 Canvas、真实数值 TextBlock 与按钮的可选绑定，继续和重开仍转发既有重开委托，失败页返回按钮转发既有主菜单委托。
5. 编译保存 WBP，审计绑定与资源引用；运行聚焦自动化和静态验证，用 Editor 截图与参考图对照。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| SVG/CSS | 浏览器/图像渲染检查；来源清单与分层清单 | 1920×1080 参考图可复现，导入元素可追溯 |
| WBP 作者ing | Unreal Editor Compile/Save；`CompileAllBlueprints` | `WBP_ReEchoRestart` 无编译错误，稳定绑定存在且类型正确 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Restart`（以实际列出的测试名为准） | Pause/Death/Victory/退出确认状态回归通过 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目规则、资源引用和文本 diff 检查通过 |
| 发布门禁 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合候选完整构建并刷新匹配预构建包；仅在发布实现时执行 |
| 人工 | 1920×1080 PIE：Victory 视觉、继续/重开、普通暂停、两种退出确认、Death | 用户确认视觉与交互；主观验收未由 AI 代替 |

## 执行记录

### 变化

- 2026-08-27：完成现有契约只读审计并建立正式 Plan；尚未修改实现资产。
- 2026-08-27：Plan-only 提交 `f6a61d44` 已发布并核验存在于 `origin/main`，开始实现。
- 2026-08-27：从用户 SVG/CSS 拆出可追溯的结算底板、角色、玫瑰、按钮、卡槽和碎片图层，导入 `/Game/ReEcho/Textures/UI/Formal/RoundVictory`。
- 2026-08-27：在 `WBP_ReEchoRestart` 内增加 1920×1080 `VictoryCanvas`；正式视觉、五个卡槽样例、全部文字和透明继续按钮均为 Canvas 直接子项，可在 Designer 中独立拖动和缩放。
- 2026-08-27：Victory 运行时只投影真实关卡总数、时间碎片与构筑数量；继续按钮复用既有 `OnRestartRequested`，普通暂停、退出确认和 Death 保持原流程。
- 2026-08-27：用户追加正式失败页；从失败 SVG/CSS 拆出结算底板、角色、枯花、碎片、按钮与五个卡槽图层，导入 `/Game/ReEcho/Textures/UI/Formal/RoundDefeat`。
- 2026-08-27：同一 WBP 增加 1920×1080 `DefeatCanvas`，视觉、文字、五个卡槽样例和透明按钮均为 Canvas 直属子项；运行时投影真实到达关卡、时间碎片与构筑数量。
- 2026-08-27：失败页“重开整局”复用既有重开委托，“返回主菜单”复用既有退出到主菜单委托；普通暂停、两种退出确认与 Victory 显隐互斥保持不变。
- 2026-08-27：正式胜利/失败页的透明命中按钮复用统一 `1.05` 倍中心悬停反馈；因按钮底图与文字是 Canvas 兄弟节点，运行时仅把同一视觉反馈同步到对应底图和标签，不修改用户在 WBP 中微调后的几何。
- 2026-08-27：清理正式结算页替代后不再生效的旧结算层：从 `WBP_ReEchoRestart` 删除 `ArtRestartCharacter`、`ArtResultSummaryPanel`、`ArtSelectedCardsPanel`、`ArtVictoryTitle`、`ArtDefeatTitle`，同步删除 8 个旧结果纹理、对应占位源图与旧胜利/失败参考图；保留普通暂停、退出确认以及保存失败仍使用的 `ArtRestartDialogPanel`。
- 2026-08-27：删除 C++ 中旧结算标题、摘要、死亡提示与旧结果美术显隐分支；Victory/Death 只由正式 `VictoryCanvas` / `DefeatCanvas` 表现，运行时逻辑和委托不变。占位导入脚本也不再重建已废弃的结果素材。

### 证据

- `origin/main` 最大编号为 128，Plan 129 基于 `9a1b1c58` 分配。
- `WBP_ReEchoRestart` 是六状态唯一作者ing入口；`SetVictoryScreen` 当前只接收 `TimeShards` 与 `TraitCount`。
- Development Editor 构建成功并刷新预构建包；`python scripts/validate_project.py`、预构建校验和 `git diff --check` 通过。
- `CompileAllBlueprints` 结果为 0 error；`ReEcho.UI.RestartWidgetPresentation` 聚焦自动化通过。
- `audit_plan129_round_victory.py` 验证正式绑定、Canvas 直接子项、五个卡槽样例和关键 Figma 几何。
- `audit_plan129_round_defeat.py` 验证正式失败绑定、Canvas 直接子项、五个卡槽样例和关键 Figma 几何。
- `CompileAllBlueprints` 最终结果为 0 error、0 warning、0 failed-to-load；`ReEcho.UI.RestartWidgetPresentation` 最终结果为 Success；预构建包校验通过，源哈希为 `1787498ccaef`。
- 悬停反馈修改后的 Development Editor 构建成功，预构建源哈希为 `6a7507ceac12`；`ReEcho.UI.RestartWidgetPresentation`、`ReEcho.UI.ButtonVisualFeedback.HoverScale` 与 `ReEcho.UI.ButtonVisualFeedback.VisualRoot` 均为 Success，`python scripts/validate_project.py` 与 `git diff --check` 通过。
- 旧结算层清理后的最终 Development Editor 构建成功，预构建源哈希为 `dbc8ecb1b061`；`audit_plan129_restart_legacy.py` 验证 5 个旧控件与 8 个旧纹理均不存在，`ReEcho.UI.RestartWidgetPresentation` 结果为 Success。

### 剩余风险

- `WBP_ReEchoRestart.uasset` 是难合并二进制共享资产，发布前仍需再次审计远端。
- 1920×1080 PIE 的最终视觉与继续按钮手感需要用户人工验收；AI 不替代主观验收。

### 人工验收结果/请求

- 2026-08-27：用户确认 Plan129 可以推送远端并合并主分支，人工验收通过。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已同步 Victory/Defeat Designer 权威与运行时投影事实。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅：本次未改变模块拓扑、状态所有者或跨模块依赖，无需修改。
- `shared/CODEBASE_MAP/README.md` 已审阅：本次未新增稳定 ID、目录路由或模块索引，无需修改。
