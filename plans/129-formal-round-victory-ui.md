# Plan 129 - 程序 - 正式本轮胜利界面接入

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@9a1b1c58`。
- 本地实现方式（可选，仅作交接说明）：`plan/129-formal-round-victory-ui`，`ReEcho-plan129-formal-round-victory-ui` 独立 worktree。
- 依赖 / 阻塞：用户提供 `C:/Users/gavynqiu/Downloads/本轮胜利.svg` 与对应 Figma CSS；实施前需保证同克隆 Unreal Editor 锁可用。
- Writes: `plans/129-formal-round-victory-ui.md`；`Content/ReEcho/UI/WBP_ReEchoRestart.uasset`；`Content/ReEcho/Textures/UI/Formal/RoundVictory/**`；`Content/SourceArt/UI/Formal/RoundVictory/**`；必要的 `scripts/ue/**` 作者ing/审计脚本；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`。
- Stable Reads: `Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp` 的既有胜利流程；`Design/UI/ReEcho_UI修改指导.md`；`Content/ReEcho/UI/WBP_ReEchoTraitCardEntry.uasset` 的正式卡面视觉；用户提供的 SVG/CSS。
- 影响模式：`SharedContract`。同一 `WBP_ReEchoRestart` 同时承载普通暂停、两种退出确认、Death 与 Victory；本 Plan 只改变 Victory 表现，必须保护其余状态和稳定绑定。
- 兼容承诺 / 下游操作：保留 `UReEchoRestartWidget::SetVictoryScreen`、`OnRestartRequested`、暂停/退出委托、焦点与页面生命周期；现有 `BindWidgetOptional` 名称和类型保持不变。缺少新正式纹理时只允许保留既有表现或明确失败，不得影响胜利结算、重开和退出流程。
- 明确排除：不新增第二套 Victory WBP；不修改 Combat、Run、存档、奖励、音乐或关卡推进逻辑；不伪造当前 API 未提供的击败数、金币或五张真实已选卡牌数据；不把整张 SVG/截图作为不可编辑的全屏点击层；不重做本轮失败、普通暂停或退出确认页面。

## 锁定目标

在保持现有胜利结算与重开逻辑完全一致的前提下，把 `本轮胜利.svg` 的正式视觉接入 `WBP_ReEchoRestart` 的 Victory 状态。背景遮罩、结算底板、标题、角色立绘、统计文字、卡牌展示槽和继续/重开按钮应拆成 Designer 中可见、可独立选择、拖动和缩放的作者ing元素；运行时仅覆盖真实已有的文本内容和状态，不回写布局。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`、`AREA-UI`。不修改 Runtime C++ 模块契约，只更新其既有 WBP 表现资产。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，已加入 `Writes`。
- 设计意图：让 WBP 成为 Victory 布局和样式的权威，C++ 继续只负责模式、真实结果文字、委托和生命周期；为后续人工微调保留所见即所得的 1920×1080 设计面。
- 权威状态与依赖：不改变状态所有者或公共 API。`UReEchoRestartWidget` 仍拥有 Victory 模式投影，GameMode 仍拥有胜利流程；WBP 只消费稳定绑定。
- 决策记录：
  1. 复用 `WBP_ReEchoRestart`，不新增独立 Victory 页面，以免分叉暂停/重开流程。
  2. SVG 作为视觉基准拆分导入；文字保持 UMG TextBlock，按钮保持透明真实交互层，避免将整页烘成不可编辑图片。
  3. 当前 `SetVictoryScreen` 仅提供时间碎片与构筑数量；SVG 中无稳定来源的击败数、金币及五张卡只提供 Designer 样例/空槽，不在本 Plan 扩张结果 Schema。
  4. 现有绑定名优先复用；新增纯表现节点使用 `ArtVictory*` / `DesignerVictory*` 前缀并设为不参与命中测试。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md`；维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 的 Victory 作者ing事实；若拓扑和稳定路由不变则只在执行记录中写明无需修改。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [ ] `WBP_ReEchoRestart` 的 Victory 状态与用户 SVG 在 1920×1080 设计面上结构、相对位置、尺寸、颜色和层级一致；关键元素均可在 Designer 中单独调整。
- [ ] `RootPanel`、`TitleText`、`MessageText`、`RestartButton`、`ResumeButton`、`QuitButton` 及既有 Victory 美术绑定名称/类型保持可加载，Blueprint 编译无错误。
- [ ] 进入 Victory 后标题、真实摘要和重开/继续交互正常；普通暂停、两种退出确认、Death 的显隐和委托不回归。
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
3. 将正式 Victory 纹理导入独立目录，以可编辑 Canvas 子项组装结算底板、标题、立绘、统计区、卡槽样例和按钮表现。
4. 复用现有稳定 TextBlock/Button/Image 绑定，只在必要时增加纯表现节点；不修改结算和页面流程。
5. 编译保存 WBP，审计绑定与资源引用；运行聚焦自动化和静态验证，用 MCP 截图与参考图对照。

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

### 证据

- `origin/main` 最大编号为 128，Plan 129 基于 `9a1b1c58` 分配。
- `WBP_ReEchoRestart` 是六状态唯一作者ing入口；`SetVictoryScreen` 当前只接收 `TimeShards` 与 `TraitCount`。

### 剩余风险

- `WBP_ReEchoRestart.uasset` 是难合并二进制共享资产，执行和发布前需再次审计远端。
- SVG 的部分背景图包含旧页面/战斗画面参考，不应作为正式运行时全屏底图重复导入。

### 人工验收结果/请求

- 待实现后请求用户检查 Victory 视觉与交互。

### 架构文档审阅结果

- 待实现后补充。
