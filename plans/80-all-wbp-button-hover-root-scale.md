# Plan 80 - 程序 - 全部交互 WBP 按钮根节点悬停缩放

## 协调

- Planner 负责人：当前对话程序 AI。
- Executor 负责人：待 Planner 按本 Plan 启动独立 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
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
  - `MOD-ReEcho.md`：待审阅。
  - `MOD-ReEchoUI.md`：待审阅。
  - `Design/UI/ReEcho_UI修改指导.md`：待审阅。
  - `ARCHITECTURE.md`：待审阅。
  - `README.md`：待审阅。

## 锁定验收

- [ ] 十个交互 WBP 的全部可点击按钮均完成资产级盘点；每个按钮记录视觉根、底图/文字/图标归属及是否需要层级调整。
- [ ] 鼠标移入任一启用按钮时，完整按钮视觉以中心放大至原 Scale 的 1.05 倍；移出后 Scale/Pivot 与进入前一致。
- [ ] Start Menu、Restart 等透明点击层页面的底图与点击区同步放大，不出现只放大透明区域、底图不动或相邻整组一起放大的情况。
- [ ] `GameSettingsButton` 与 `AboutButton` 的外部底图、内部文字/图标和点击区同步放大；自动化覆盖“Button 有 Content + 单按钮父 Overlay 有视觉兄弟”。
- [ ] Settings 打开、切换分类、返回并再次打开时只存在一套 WBP 界面；WBP Root 不被原生 fallback 替换，ComboBox/Slider 等动态交互层不重复创建或重叠。
- [ ] 普通 Pause、退出到主菜单确认、退出游戏确认、Death、Victory、重开/结算均显示 `WBP_ReEchoRestart` 作者ing面板；资产正常时不出现原生 fallback 面板，既有 Delegate 与保存/不保存语义不变。
- [ ] 禁用按钮不伪装为可交互；键盘/手柄焦点和点击 Delegate、索引映射、确认/取消流程保持不变。
- [ ] 所有修改 WBP 在 Editor 中 Compile/Save 成功，`CompileAllBlueprints` 无新增错误；资产保存后无意外绑定名称/类型变化。
- [ ] 通过 `.clang-format`、`Build-Editor.cmd -Configuration Development -FullRebuild`、UI 聚焦自动化、`validate_project.py`、`git diff --check` 与预构建包一致性检查。
- [ ] 用户在 PIE 验收 Start Menu、Loadout、Settings、Restart 六状态、Trait、Shop、Stats、About 的缩放幅度、底图同步和页面流程。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

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

- 待执行。

### 证据

- 待执行。

### 剩余风险

- 待执行。

### 人工验收结果/请求

- `PendingBeforeClose`。

### 架构文档审阅结果

- 待执行。
