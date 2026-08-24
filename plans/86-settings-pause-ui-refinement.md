# Plan 86 - 程序 - 设置页与暂停页 UI 协同完善

## 协调

- Planner 负责人：Codex。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`InProgress`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（设置页与暂停页的视觉比例、可读性、命中和页面往返需要用户逐轮 PIE 判断）。
- 本地规划 / 实现基线：`origin/main@52b1648cbb40719115624bf449392a1e15cc1bec`。
- 本地实现方式：一任务一 worktree，`C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan86-settings-pause-ui`，分支 `plan/86-settings-pause-ui`；规划者与执行者合一。
- 依赖 / 阻塞：沿用 Plan34/45 已发布的音频设置、画面设置、暂停六态与退出流程；Plan81 的完整键位重绑定仍是独立 Proposed 范围，本 Plan 不静默接管其输入映射/持久化决策。用户将按页面提供目标图、运行截图或 UE Designer 调整结果，双方逐页验收。
- Writes:
  - `plans/86-settings-pause-ui-refinement.md`
  - `Content/ReEcho/UI/WBP_ReEchoSettings.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoRestart.uasset`
  - `Source/ReEcho/Public/UI/ReEchoSettingsWidget.h`、`Source/ReEcho/Private/UI/ReEchoSettingsWidget.cpp`（仅在真实控件绑定或交互修复需要时）
  - `Source/ReEcho/Public/UI/ReEchoRestartWidget.h`、`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`（仅在暂停状态/按钮绑定修复需要时）
  - `Source/ReEcho/Private/Tests/ReEchoSettingsInteractionTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRestartWidgetTests.cpp`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 经用户明确采用、且只被两页实际消费的 `Content/SourceArt/UI/**`、`Content/ReEcho/Textures/UI/**` 与可复现 `scripts/ue/**` 导入/作者ing脚本
- Stable Reads:
  - `Source/ReEcho/Private/UI/Framework/ReEchoUIFlowCoordinatorSubsystem.cpp`
  - `Source/ReEcho/Private/UI/ReEchoUIManagerSubsystem.cpp`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEchoAudio/**` 及已发布的音频总线/用户设置契约
  - `plans/34-audio-catalog-settings.md`、`plans/45-ui-interaction-placeholder-assets.md`、`plans/51-fix-restart-hud-screen-reset.md`、`plans/81-settings-ui-complete.md`
- 影响模式：`Exclusive`（两个 WBP 是不可文本合并的二进制资产；同一时刻只允许本任务编辑对应资产）。
- 兼容承诺 / 下游操作：不新增或删除 `EReEchoUIScreen`，不改变设置页/暂停页稳定 Widget 名称和类型化 Delegate；保持三滑条聚合音效语义、画面应用/恢复语义、暂停/恢复、设置往返、退出到主菜单与退出游戏的保存/不保存流程。WBP 管理布局、尺寸、样式和焦点表现；C++ 不在正常作者ing路径中重建整页或回写固定坐标。
- 明确排除：不借 UI 返工新增玩法设置、输入系统、存档格式或页面枚举；不在本 Plan 内实现 Plan81 的完整键位重绑定，除非用户明确更新锁定目标；不把整张效果图作为交互页面；不修改战斗、商店或主菜单；不提交未精选 Editor 中间产物或机器本地文件。

## 锁定目标

与用户逐轮完善设置页面和暂停页面，使两页在 Unreal UMG Designer 中可持续手调，并在运行时保持目标视觉与真实交互一致：

1. 设置页的画面、声音、键位三个页签、字段、滑条、下拉框、恢复默认、应用与关闭入口在 WBP 中拥有清晰可编辑的布局；真实画面/音频交互继续生效，未实现能力不伪装成已实现。
2. 暂停页的普通暂停、退出到主菜单确认、退出游戏确认，以及既有胜利/失败/重开状态共享同一 WBP 时，显隐和按钮含义准确，不出现多余底板、空白交互层或被装饰图遮挡的命中区域。
3. 两页在每轮用户反馈后只调整相关页面/状态；不以 C++ 固定坐标覆盖用户在 Designer 中保存的位置，且不破坏键盘/手柄焦点、鼠标命中、Esc/P 返回和设置页往返。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`、文档型入口 `MOD-ReEchoUI`；实现仍属于 `MOD-ReEcho`，不新增 Runtime Module。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已加入 Writes；若仅调整 WBP 位置且既有职责事实不变，关闭时记录具名审阅结论而不制造无意义正文 diff。`MOD-ReEcho.md` 仅审阅，不预设修改。
- 设计意图：把视觉作者ing权稳定留在 WBP，把 C++ 限制为真实状态、设置值、事件和生命周期；这样用户可直接在 UE 中调整，而程序刷新不会还原位置。
- 权威状态与依赖：不改变音频服务、`UGameUserSettings`、UI Manager/Flow Coordinator 或 GameMode 的权威状态与依赖方向；两页只消费状态并发送既有类型化请求。
- 决策记录：
  - 设置页与暂停页分别按小批次修改和验收，避免一次改两个二进制 WBP 后无法定位视觉回归。
  - 正常运行使用作者ing WBP；原生 `BuildWidgetTree()` 仅保留无 Root 时的最低可用 fallback，不因可选控件缺失覆盖整页。
  - 装饰 Image 使用 `HitTestInvisible`，承载真实交互的父容器使用合适的 `SelfHitTestInvisible`；透明 Button/Slider/ComboBox 保持完整命中范围。
  - 对用户需要手调的位置，控件必须是 Canvas 的可选直接子项或具有明确可编辑 Slot；动态列表内容可由 C++ 填充，但宿主位置由 WBP 决定。
  - Plan81 的键位重绑定涉及输入事实来源和持久化，不与纯 UI 完善混合；需要时由用户明确决定是激活 Plan81 还是扩张 Plan86。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预期无模块拓扑/依赖变化。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预期无稳定标识/路由变化。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：关闭前维护两页 WBP/C++ 职责与测试路线。
  - `Design/UI/ReEcho_UI修改指导.md`：记录最终可手调控件、稳定绑定和人工验收步骤。
- 关闭前逐项填写审阅结果。

## 锁定验收

- [ ] 设置页三个页签能正确切换；当前页、未选中页、字段文字与底板对齐，白底白字/错层/命中遮挡等问题全部按用户反馈关闭。
- [ ] 画面页的下拉框与亮度滑条、声音页的三条真实音量滑条及输出设备下拉框可操作；恢复默认、应用、关闭及返回路径语义保持正确。
- [ ] 暂停页普通状态显示继续游戏、退出至主菜单、退出游戏和设置入口；两个退出确认状态分别显示正确标题与保存/不保存/返回操作，无额外大底板。
- [ ] 两页需要手调的主要布局在 WBP Designer 中可编辑，页面刷新/状态切换不会由 C++ 回写固定坐标；装饰层不拦截输入。
- [ ] `WBP_ReEchoSettings`、`WBP_ReEchoRestart` Compile/Save 成功，`CompileAllBlueprints` 为 0 errors、0 warnings、0 failed loads。
- [ ] `ReEcho.UI.SettingsInteraction` 与 `ReEcho.UI.RestartWidgetPresentation` 聚焦自动化通过；源码变化时 Development Editor 构建通过。
- [ ] `python scripts/validate_project.py` 与 `git diff --check` 通过；发布候选满足 `GIT_RULES.md` 的最终构建/精选预构建包门禁。
- [ ] 用户在 PIE 对设置页、普通暂停、两种退出确认、设置页往返、鼠标命中和至少 1920×1080 布局报告 `Passed` 或给出具名返工项。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@52b1648cbb40719115624bf449392a1e15cc1bec`。
- 引擎/构建可用性：UE 5.8 安装版及仓库 Windows 工具链可用；Plan-only 发布走纯文档例外，首次内容/C++ 候选按变化面重新构建。
- 现有聚焦测试结果：基线已有 `ReEcho.UI.SettingsInteraction` 与 `ReEcho.UI.RestartWidgetPresentation`；开始实现前重新运行作为当前证据。
- 共享契约 / 难合并资源风险：`WBP_ReEchoSettings.uasset`、`WBP_ReEchoRestart.uasset` 为 Exclusive 二进制资产；编辑前确认没有另一 Editor/worktree 同时修改，逐资产 Compile/Save。
- 基线损坏时的停止条件：远端出现批准基线之外的新提交；WBP 正被其他任务编辑；用户目标要求改变输入/存档/页面契约；Editor 会话未保存关闭而命令需要独占。

## 实现提纲

1. 建立两页当前控件树、运行截图、稳定绑定和聚焦测试基线；记录用户希望先处理的页面与目标状态。
2. 设置页按“画面 → 声音 → 键位/占位 → 公共按钮/关闭”逐块调整，每块完成后 Compile、运行聚焦检查并交给用户复测。
3. 暂停页按“普通暂停 → 退出到主菜单确认 → 退出游戏确认 → 设置页往返”逐态调整，复用稳定按钮而不复制行为入口。
4. 将需要手调的布局迁到 WBP 可编辑 Slot；C++ 只绑定数据/显隐/事件，并增加防止运行时位置回写和命中遮挡的自动化断言。
5. 完成跨页面焦点、Esc/P、鼠标命中及 1920×1080 人工验收；同步 Plan 执行记录、UI 指导与架构文档审阅结果。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 基线/静态 | `python scripts/validate_project.py`；`git diff --check` | 项目与差异通过 |
| WBP | 逐资产 Compile/Save；`CompileAllBlueprints` | 两页 0 errors、0 warnings、0 failed loads |
| C++ | `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码 0，精选包与源码匹配 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.SettingsInteraction`；`-Filter ReEcho.UI.RestartWidgetPresentation` | 两页交互/显隐/作者ing契约通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`（非纯文档候选） | 最终集成候选完整构建并刷新允许列表产物 |
| 人工 | 设置三页签、暂停三态、设置往返与 1920×1080 PIE | 用户报告 `Passed` 或具名返工项 |

## 执行记录

### 变化

- Plan86 已编号并建立独立 worktree；第一批设置页返工采用用户交付的 `标签浅.png` / `标签深.png`，当前页显示浅色纸张标签，未选中页显示深色纸张标签。
- 保持三个 WBP 页签 Overlay、标签文字与真实 Button 的布局/命中关系不变；C++ 只依据当前分类切换浅/深 Brush，不回写 Designer 坐标。
- 三个页签 Overlay 中遗留的六个无资源白色装饰 `Image` 已折叠，避免其默认白 Brush 从纸张贴图透明边缘透出矩形框。
- 用户交付的 `滑动条把手.png` 已作为真实 Slider 的 Thumb 样式接入；三条作者化音频滑条直接使用，动态亮度滑条从总音量滑条继承同一风格，拖动逻辑与 WBP 布局保持不变。
- 滑条深色 Fill 与真实 Thumb 统一采用缩进后的把手中心区间：0%/100% 均保留半把手宽，任意百分比的颜色分界都落在把手中心，消除端点凸出与中段缝隙。
- 清除三条作者化声音 Slider 遗留的 Overlay Slot 左右 Padding，使真实把手的运动轨道与底板/Fill 美术使用同一完整宽度；此前两端分离而中段接近的问题由该缩短运动区间导致。
- 声音页三条轨道的 `HorizontalBoxSlot` 从会随整行拉伸的 `Fill` 改为按美术期望尺寸布局的 `Automatic`；其 Track/Fill 原始宽度与已验收的亮度条同为 802 px，消除声音条在约 23%–78% 外才逐渐分离的几何差异。
- 依据 UE 5.8 `SSlider` 实际绘制规则关闭 `IndentHandle`：该选项会额外扣除两倍 Thumb 宽度，正是声音页两端无法贴合的剩余原因；关闭后 0%/100% 时把手外缘分别贴合底图两端，Fill 分界继续与把手中心一致。
- 设置页底部两个真实按钮接入用户交付的 `设置按钮浅.png` / `设置按钮深.png`：恢复默认使用浅色纸张，应用使用深色纸张；保留 WBP 位置、文字层和原有点击行为。
- 设置页底部按钮的布局权经资产审计确认在 `WBP_ReEchoSettings/SettingsLayoutCanvas`：`RestoreDefaultsButton` 与 `ApplyAndReturnButton` 均为直接 Canvas 子控件并拥有独立 `CanvasPanelSlot`，C++ 只绑定事件；自动化契约禁止后续把两者迁回运行时布局。
- 清理设置页作者ing树中的折叠遗留内容：删除旧标题/说明文本、六张旧白底页签图、五行旧画面占位布局、重复页签/按钮文字及占位提示；保留仍被音频服务绑定的隐藏控制行。清理脚本在删除前后校验两个底部按钮的 Canvas 坐标与尺寸完全不变。
- 声音页输出设备框由会随行拉伸的 `Fill` 改为 802×50 固定美术尺寸，与画面页四个下拉框一致；整体左移 39 px 并在右侧预留布局占位，使左右边界同时对齐三条音量轨道，真实 ComboBox 交互层继续完整覆盖框体。
- 声音页可见控件从 `VerticalBox > HorizontalBox` 自动排版迁入 `AudioPanel > AudioDesignerCanvas`：三组标签、滑条逻辑根、百分比、静音按钮以及输出设备标签/下拉框均拥有独立 `CanvasPanelSlot`，可在 UMG Designer 中直接拖动和改尺寸；轨道/填充/把手仍封装在同一 Overlay 内避免内部漂移，C++ 只复制作者化槽位建立交互层。

### 证据

- 编号前已 fetch；远端最大 Plan 编号为 85，新任务使用 86，基线为 `origin/main@52b1648cbb40719115624bf449392a1e15cc1bec`。
- 已审计设置/暂停原生 Widget、现有聚焦测试、Plan81 与 `MOD-ReEchoUI`；本 Plan 不改变已发布的屏幕枚举、音频总线或退出端点。
- `标签浅.png` / `标签深.png` 均确认是 208x68、32bpp ARGB；导入为 `T_UI_Settings_TabLight` / `T_UI_Settings_TabDark`，WBP 默认状态已 Compile/Save。
- Development Editor 增量构建通过；`ReEcho.UI.SettingsInteraction` 找到 1 项并以 `Result={Success}` 完成，新增断言覆盖画面默认浅、声音/键位默认深及切换后的状态互换。
- `CompileAllBlueprints`：0 errors、0 warnings、0 failed loads；`python scripts/validate_project.py` 与 `git diff --check` 通过。
- 设置页清理前后通过 UMG ToolSet 全树审计：节点数由 136 降至 104，22 个废弃根及其 10 个子节点全部消失；资产 Compile/Save 成功，两个底部按钮的 Canvas Position/Size 前后完全一致。清理后 Development Editor 构建与 `ReEcho.UI.SettingsInteraction` 再次通过，测试同时断言全部旧节点不再存在。

### 剩余风险

- 两个 WBP 是二进制资产，任何并行编辑都会形成难合并冲突；必须逐资产小批次推进。
- 第一批标签素材为 208x68 ARGB，与旧 269x64 纯色占位图宽高比不同；先沿用 WBP 现有页签 Slot 做运行复测，若用户要求再仅在 Designer 调整尺寸/间距。

### 人工验收结果/请求

- `PendingBeforeClose`：当前批次等待用户复测设置页三个标签的浅/深状态、切换与命中。

### 架构文档审阅结果

- 待实现完成后逐项填写。
