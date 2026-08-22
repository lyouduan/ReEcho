# Plan 80 - 程序 - 全部交互 WBP 按钮根节点悬停缩放

## 协调

- Planner 负责人：当前对话程序 AI。
- Executor 负责人：当前任务独立 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Codex`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`Passed`。
- 本地规划基线：`origin/main@e07838f810c3299977c74c6373b13a8def431d6f`；首次实现批准基线：`origin/main@53a47639f5dfdb187b10824274cbea010b1713d7`；人工反馈修订基线：`origin/main@4541f6e1301cb6077ba831e3df69a44e8233ba6b`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree；现有 `codex/ui-hover-feedback` 仅含未发布试验，Executor 接管前必须按本 Plan 审查，不得直接视为已批准实现。
- 依赖 / 阻塞：需要 UE 5.8 Editor、UMG ToolSet 或等价 Editor 作者ing 能力；修改 `.uasset` 前必须确认 Editor 未运行并取得同克隆 Unreal 锁。
- Writes:
  - `plans/80-all-wbp-button-hover-root-scale.md`
  - `Content/ReEcho/UI/WBP_ReEchoStartMenu.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutSelection.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutEntry.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoSettings.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoRestart.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoTraitCardChoice.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoTraitCardEntry.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoInventoryShopScreen.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoStatsScreen.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoAbout.uasset`
  - `Source/ReEcho/{Public,Private}/UI/Framework/ReEchoButtonVisualFeedback.*`
  - `Source/ReEcho/{Public,Private}/UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*`
  - `Source/ReEcho/Private/Tests/ReEchoButtonVisualFeedbackTests.cpp`
  - `scripts/ue/author_plan80_ui_button_hover_roots.py`（仅在需要可重复 Editor 作者ing 时创建）
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Binaries/Win64/` 下 `shared/GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - 上述 WBP 对应的 `Source/ReEcho/{Public,Private}/UI/*Widget.*`
  - `Source/ReEcho/Private/UI/ReEchoMenuWidgetHelpers.*`
  - `Source/ReEcho/Public/UI/ReEchoIndexedButton.h`
  - `Content/ReEcho/Textures/UI/InteractionPlaceholder/`
  - `plans/29-ui-umg-migration.md`
  - `plans/45-ui-interaction-placeholder-assets.md`
- 影响模式：`SharedContract`。统一 WBP 表现层按钮根结构与运行时悬停反馈；不改变按钮事件、稳定索引或玩法命令。
- 兼容承诺 / 下游操作：保留所有 `BindWidget` / `BindWidgetOptional` 名称和类型，保留点击命中区、键盘/手柄焦点、禁用状态、页面流程和现有贴图；无存档、Schema、数据表或玩法迁移。
- 明确排除：不为没有按钮的 `WBP_ReEchoPlayerHud`、`WBP_ReEchoEncounterHud`、`WBP_ReEchoEnemyHealthBar`、`WBP_ReEchoPlayerScreenFeedback` 制造空改动；不改 Weather、Damage Number；不重画底图；不在 Editor 外手改 `.uasset`；不新增 WBP 玩法逻辑。

## 锁定目标

- 全部含交互按钮的 `WBP_ReEcho*` 页面都具备一致的鼠标悬停反馈：移入时按钮整体轻微放大，移出时完整恢复。
- 缩放对象必须是单个按钮的完整视觉根：底图、文字、图标、选中装饰和真实点击区域一起缩放，禁止只放大透明命中按钮而把底图留在原位。
- 默认悬停倍率为 `1.05`，围绕按钮视觉中心缩放；不得改变 Canvas 布局尺寸、相邻按钮排布或命中语义。
- 动态 C++ 按钮也使用同一反馈；其按钮内容本来位于按钮内部时不得为了形式额外复制 WBP 根节点。
- Start Menu 的 `GameSettingsButton` 与 `AboutButton` 即使自身已有文字内容，只要父级是只承载该按钮及其底图/装饰的单按钮 Overlay，仍必须缩放父 Overlay，不能只缩放透明 Button 内容。
- Settings 正常运行只显示并使用 `WBP_ReEchoSettings` 作者ing界面；C++ fallback 不得因单一可选绑定缺失而覆盖已有 WBP Root，动态交互控件必须按稳定名称幂等复用，不得在重复构造时叠加重 UI。
- Pause / 退出确认 / Death / Victory 继续使用已注册的 `WBP_ReEchoRestart` 及同一原生状态机；正常 WBP 资产存在时不得显示原生 fallback 暂停面板。本 Plan 不新增独立 Pause Screen/WBP。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，均已加入 `Writes`。
- 设计意图：WBP 继续拥有按钮视觉层级与布局；C++ Flow Coordinator 只提供通用、无玩法状态的 hover/unhover 反馈。按钮视觉与透明点击层不得作为互不跟随的并列表现单元。
- 权威状态与依赖：不改变状态所有者、Runtime Module 依赖或页面生命周期；只增加 WBP 表现结构约定和通用视觉反馈对象。
- 决策记录：
  - 优先把现有底图/文字/图标收进单按钮视觉根，使根整体缩放；不得用逐页面硬编码坐标模拟底图放大。
  - 通用反馈保留按钮作者ing时已有 Scale/Pivot，并在移出时准确恢复；默认使用中心 Pivot 与 1.05 倍乘法缩放。
  - 不把悬停动画写入每个 WBP 的 Blueprint Graph，避免十个页面复制逻辑；WBP 负责层级，Flow Coordinator 负责统一事件绑定。
  - 无按钮 WBP 不修改；“全部 WBP”按全部交互 WBP 解释，并通过资产盘点证明覆盖范围。
  - 单按钮 Overlay 判定优先于“Button 已有 Content”判定；只有父 Overlay 同时包含多个可交互按钮或不是单按钮视觉单元时才回退缩放 Button。
  - Settings/Restart fallback 必须以“无有效作者ing Root 或关键契约整体不可用”为明确条件，不能用一个 `BindWidgetOptional` 指针作为替换整棵 WBP 树的充分条件；正常路径保留 WBP 为表现权威。
- 相关文档同步范围：维护 `MOD-ReEcho.md`、`MOD-ReEchoUI.md` 与 `Design/UI/ReEcho_UI修改指导.md`；关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 和 `README.md`，预计依赖拓扑与路由标识不变，无事实变化时只记录审阅结论。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：已更新完整视觉根与 Settings/Restart authored-root 权威。
  - `MOD-ReEchoUI.md`：已更新 Overlay 优先、幂等交互层与统一 Restart WBP 路径。
  - `Design/UI/ReEcho_UI修改指导.md`：已更新 WBP 作者ing约束、Is Variable 与 fallback 边界。
  - `ARCHITECTURE.md`：已审阅；模块拓扑、依赖方向和跨模块不变量未变化，无需修改。
  - `README.md`：已审阅；`MOD-ReEchoUI` / `AREA-UI` 标识与路由未变化，无需修改。

## 锁定验收

- [x] 十个交互 WBP 的全部可点击按钮均完成资产级盘点；Editor 日志记录每个按钮的 parent/content/visual root/direct members。
- [x] 自动化确认鼠标移入时以原 Scale 乘 `1.05`，移出后 Scale/Pivot 恢复；用户已完成人工验收。
- [x] Start Menu、Restart 等透明点击层页面的底图与点击区同步放大，不出现只放大透明区域、底图不动或相邻整组一起放大的情况。
- [x] `GameSettingsButton` 与 `AboutButton` 解析到各自单按钮 Overlay；自动化覆盖“Button 有 Content + 单按钮父 Overlay 有视觉兄弟”。
- [x] 自动化确认 Settings 保留 `CanvasPanel_0` 作者ing Root，六个动态 ComboBox/Slider 稳定名称各只存在一份；实际重开视觉待人工验收。
- [x] 自动化确认运行类为 `WBP_ReEchoRestart_C`、保留 `CanvasPanel_0` 作者ing Root 且关键 Pause 绑定存在；六状态实际画面/流程待人工验收。
- [x] 禁用按钮不伪装为可交互；键盘/手柄焦点和点击 Delegate、索引映射、确认/取消流程保持不变。
- [x] 所有修改 WBP 在 Editor 中 Compile/Save 成功，`CompileAllBlueprints` 为 0 error / 0 warning / 0 failed-to-load；关键绑定名称/类型/Is Variable 已核验。
- [x] `.clang-format`、FullRebuild、UI 聚焦自动化、`validate_project.py`、`git diff --check` 与预构建包一致性检查均通过。
- [x] 用户在 PIE 验收 Start Menu、Loadout、Settings、Restart 六状态、Trait、Shop、Stats、About 的缩放幅度、底图同步和页面流程。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@e07838f810c3299977c74c6373b13a8def431d6f`；Plan80 编号前 fetch 无新增外部提交，远端最大编号为 Plan79。
- 引擎/构建可用性：UE 5.8 Development Editor 在试验 worktree 已增量构建成功；Plan 发布与最终候选仍分别执行准确基线的 `-FullRebuild` 门禁。
- 现有聚焦测试结果：试验测试源码可编译，但 `Run-Automation.cmd` 被 UE 对 LinuxArm64/VisionOS 的无关 SDK `ValidatePlatforms` 预检阻断，尚无运行通过证据；Executor 必须保留为基线基础设施问题并尝试安全的项目标准入口，不得误报通过。
- 共享契约 / 难合并资源风险：十个 `.uasset` 为不可文本合并资产，必须串行作者ing；最终 C++ FullRebuild 会与 Plan79 刷新的同名预构建 DLL 重叠，进入 main 前以最新源码统一重建，不做二进制合并。
- 基线损坏时的停止条件：任一目标 WBP 在未改前已无法加载/编译、绑定控件缺失或 Editor 检测到同资产外部修改时，停止该资产的写入，记录基线异常并报告 Planner；不得用重建 WBP 覆盖未知资产变化。
- 首次人工验收反馈（2026-08-22）：`GameSettingsButton` / `AboutButton` 无完整放大；Settings 出现重 UI；Pause 未呈现预期 WBP 面板。Planner 已完成只读诊断并据此扩张锁定目标；首次候选不得发布，返工后重新请求人工验收。

## 实现提纲

1. 在 Editor 中只读导出十个交互 WBP 的 WidgetTree，形成按钮到完整视觉根的盘点表，并标出透明点击层/并列底图问题。
2. 按页面串行调整 WBP 层级：保留绑定按钮类型/名称，把底图、文字与图标放入该按钮或其单按钮根容器，保证缩放边界只包含一个按钮。
3. 实现通用 `UReEchoButtonVisualFeedback`，由 Flow Coordinator 为现有按钮绑定 hover/unhover，乘法应用 1.05 Scale 并恢复原 Scale/Pivot；处理重复绑定和失效 Widget 生命周期。
4. 为通用反馈增加自动化，Compile/Save 每个 WBP，并核对 WidgetTree、绑定契约与资产差异清单。
5. 更新 UI 指导、模块文档和执行记录，完成构建、静态门禁与人工 PIE 验收。
6. 人工反馈返工：调整完整根解析优先级；核验 Settings/Restart 全部关键 WBP 绑定与 Is Variable，收紧整页 fallback 条件并让动态交互层幂等；增加 Settings 重开与 Restart WBP 正常路径的聚焦验证。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| WBP 资产 | UE Editor Compile/Save 十个交互 WBP；`CompileAllBlueprints` | 无新增 Blueprint 错误，绑定控件名称/类型保持，按钮根盘点覆盖完整 |
| UI 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.ButtonVisualFeedback` 及受影响既有 UI 测试 | hover/unhover 恢复、重复绑定和页面契约通过；若 SDK 预检仍阻断则保留准确基础设施证据并阻塞客观关闭 |
| Settings/Restart 回归 | 聚焦资产契约/Widget 自动化；Editor 运行时日志核验实际 Class 与关键绑定 | Settings 不重建/重叠，Restart 正常路径加载 WBP；fallback 仅在资产/契约整体不可用时出现 |
| C++ / 发布包 | `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`git diff --check`、`python scripts/ue/prebuilt_editor.py check` | 项目、资产引用和预构建包一致 |
| 人工 | PIE 逐页检查 Start Menu、Loadout、Settings、Restart 六状态、Trait、Shop、Stats、About | 底图/文字/图标/命中区同步缩放，无裁切、遮挡、整组误缩放或流程回归 |
| DPI | 1280×720、1920×1080、2560×1440、21:9 | 放大状态不越界，不与相邻按钮重叠，不被父容器裁切 |

## 执行记录

### 变化

- `ResolveButtonVisualRoot()` 改为先识别“唯一 Button + 视觉兄弟”的父 Overlay，再回退 Button；因此 Start Menu 的 `About` / `Settings` 整体根会随命中按钮同步缩放。
- Settings/Restart 的原生 `BuildWidgetTree()` 仅在 `WidgetTree` 存在且没有任何 Root 时降级，不再以单一可选绑定判断并覆盖作者ing页面；Settings 动态交互层按六个稳定名称查找复用。
- 通过 UMG ToolSet 把 Settings 的 `DetailText`、`CategoryTitleText`、`GraphicsPanel`、`ControlsPanel` 和 Restart 的 `RootPanel`、`TitleText`、`MessageText` 等实际存在但未勾选的关键控件修正为 `Is Variable=true`；十个目标 WBP 均由 Editor Compile/Save。
- 增加单按钮 Overlay 优先、Settings authored root/交互层幂等、Restart 资产类/authored root/关键绑定自动化，并同步 UI 指导与两个模块文档。

### 证据

- `Build-Editor.cmd -Configuration Development -FullRebuild`：98/98，Succeeded；修正测试根名后增量构建再次成功并刷新 7 模块预构建包。
- `CompileAllBlueprints`：`Compiling Completed with 0 errors and 0 warnings and 0 blueprints that failed to load`。
- `Run-Automation.cmd -Filter ReEcho.UI`：12 项通过，包含 `ButtonVisualFeedback.HoverScale/VisualRoot`、`SettingsInteraction`、`RestartWidgetPresentation`。
- 静态门禁：`validate_project.py`、Python authoring 脚本编译、`prebuilt_editor.py check` 与 `git diff --check` 均通过。
- Editor 资产审计：10/10 `[Plan80Audit] VERIFIED`；`AboutButton -> About:Overlay [AboutButton,ArtAbout]`，`GameSettingsButton -> Settings:Overlay [GameSettingsButton,ArtSettings]`；Settings/Restart Root 均为 `CanvasPanel_0`，列出的关键绑定均为正确类型且 `IsVariable=true`。

### 剩余风险

- UMG 作者ing脚本完成 10 个资产 Compile/Save/VERIFIED 后，UE 5.8 `SemanticSearch` 模块在退出阶段发生 access violation（exit 3）；同一候选的 `CompileAllBlueprints` 可正常 exit 0，资产工作本身已有完成日志。
- 仍需人工 PIE/DPI 验收完整按钮底图同步、Settings 重开无重叠，以及 Restart 普通 Pause/两种确认/Death/Victory 六状态的画面和实际 Delegate 流程。

### 人工验收结果/请求

- `Passed`：用户在返工候选上明确回复“验收通过”。

### 架构文档审阅结果

- `MOD-ReEcho.md`：已同步完整视觉根与 Settings/Restart authored-root 权威。
- `MOD-ReEchoUI.md`：已同步 Overlay 优先、幂等交互层与统一 Restart WBP 路径。
- `Design/UI/ReEcho_UI修改指导.md`：已同步 WBP 作者ing约束、Is Variable 与 fallback 边界。
- `ARCHITECTURE.md`、`CODEBASE_MAP/README.md`：已审阅；模块依赖拓扑与路由标识未变化，无需修改。
