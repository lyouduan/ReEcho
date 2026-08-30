# Plan 156 - 程序 - UI 细节调整批次

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（首项结算统计文字所见即所得已完成，待人工验收）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@11dc24d2d3e36a3bcec5898bb40dd05385046156`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan156-ui-detail-polish`，分支 `plan/156-ui-detail-polish`。
- 依赖 / 阻塞：用户已确认将胜利/失败结算统计文字改为所见即所得，并已保存关闭 Editor；本项不修改或重建 `.uasset`，保留现有 Designer 布局。
- Writes:
  - `plans/156-ui-detail-polish.md`
  - `Source/ReEcho/Public/UI/ReEchoRestartWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRestartWidgetTests.cpp`
  - `Content/ReEcho/UI/WBP_ReEchoRestart.uasset`（用户保存的胜利页微调；本轮不向失败页自动同步）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Design/UI/ReEcho_UI修改指导.md`
  - 若包含 C++ 变化：精选 Win64 Editor 预构建包及 manifest（仅最终发布门禁刷新）。
- Stable Reads:
  - `Content/ReEcho/UI/` 下用户指定页面及其直接依赖资源。
  - `Source/ReEcho/{Public,Private}/UI/` 中与目标 Widget 配对的类型化绑定和状态逻辑。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`、`Design/UI/ReEcho_UI修改指导.md`。
- 影响模式：`Isolated`（暂定；若后续要求改变公共 UI 状态或跨页面契约，先更新为 `SharedContract`）。
- 兼容承诺 / 下游操作：默认只调整展示、层级、尺寸、位置、字体、Brush、命中区域或可配置入口；保持现有点击、Hover、焦点、导航、委托、数据加载、存档及玩法逻辑不变。运行时内容可以继续填充，但不得覆盖用户明确要求由 Blueprint Designer 拥有的几何和样式。
- 明确排除：在用户逐项确认前不重做整套页面、不替换未指定素材、不清理未证明废弃的资产或控件；不改变玩法数值、数据 Schema、存档格式、随机逻辑或跨页面流程；若细节要求实际需要上述变化，先更新本 Plan 并取得用户确认。

## 锁定目标

1. 按用户后续逐项提供的截图、控件名和目标效果，修正现有 UI 的视觉与交互细节，使 PIE/局内结果与目标一致。
2. 在不破坏运行时数据填充的前提下，优先让需要人工微调的几何、字体、Brush 和样式在 Blueprint Designer 中所见即所得且可直接调整。
3. 保持目标页面现有功能逻辑、事件绑定、输入/焦点、导航、Tooltip、存档与玩法语义不变；视觉修正不得引入第二套状态或在 C++ 中重新覆盖 Designer 已保存参数。
4. 首项锁定为 `WBP_ReEchoRestart` 的 `VictoryCanvas` / `DefeatCanvas`：八组统计标签与数值的位置、尺寸、字体、字号、颜色、对齐、换行和裁切均保留 Blueprint 保存值；删除 `ApplySettlementStatLayout` / `ConstrainSettlementStats`，程序仅继续投影真实数值。不重排、改名或重建现有资产。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-UI`、文档型入口 `MOD-ReEchoUI`；模块拓扑和跨模块契约不变。上述两个模块文档均纳入 Writes。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`。若只改变现有 Widget 的纯视觉参数且不改变文档事实，关闭前记录“已审阅、无需修改”；若改变权威控件、绑定、Designer/C++ 职责或扩展入口，则维护正文并保留在 `Writes`。
- 设计意图：延续现有设置页、卡牌与商店页面已经采用的 Designer-owned 处理方式——Blueprint 负责可视布局与样式，C++ 只提供稳定状态、数据与行为绑定，不在运行时重建或强制覆盖需要人工微调的表现参数。
- 权威状态与依赖：默认不改变 UI 状态所有者、公共委托、输入模式或模块依赖；运行时数据仍由现有逻辑提供，Designer 资产只拥有表现。若具体任务证明必须改变契约，先在 Plan 中记录并重新评估影响模式。
- 决策记录：
  - 使用一个编号 Plan 承载用户即将连续给出的同一批 UI 细节微调，前提是它们共享“只改表现、保持逻辑”的边界；出现独立功能或公共契约变化时拆分新 Plan。
  - 不在目标清单未知时预先运行整页迁移或清理脚本，避免覆盖用户已保存的 Blueprint 微调。
  - 每个 `.uasset` 修改前先确认 Editor 已关闭，修改后以 Widget Compile/Save、资产审计与用户 PIE 截图作为所见即所得证据。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 和 `Design/UI/ReEcho_UI修改指导.md`；只在事实变化时修改正文，否则在执行记录逐项说明无需修改的原因。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoUI.md`：记录已更新的控件/职责事实，或纯视觉微调无需修改；
  - `Design/UI/ReEcho_UI修改指导.md`：记录新增的可调入口，或现有入口未变化；
  - `ARCHITECTURE.md`、`README.md`：记录已更新或已审阅、无需修改及理由。

## 锁定验收

- [x] 胜利/失败各八组统计的全部 32 个标签/数值控件在首次构造、页面切换和再次构造后，布局和样式与构造前 Designer 值一致；同时验证人为微调后的非默认参数不被覆盖。
- [x] 原数值投影函数与事件端点未改；聚焦测试验证基础数值更新及结算页表现，新增测试验证标签文案不被重写。五项战斗统计继续使用原 `RefreshRunStatsValues`，未改变采集或格式化逻辑。
- [ ] 用户指定的每个 UI 细节在对应 Blueprint Designer 和 PIE/局内均达到目标截图或明确描述，并逐项记录证据。
- [ ] 需要人工微调的目标控件可在 Blueprint 中直接选中并调整约定的几何/字体/样式；运行时不再意外覆盖这些参数。
- [ ] 目标页面原有点击、Hover/Pressed 反馈、焦点、导航、Tooltip、数据加载和业务逻辑没有回归。
- [ ] 目标 Blueprint Compile/Save、适用资产审计、项目静态校验与对应构建门禁通过。
- [ ] 用户完成人工视觉与可用性验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@11dc24d2d3e36a3bcec5898bb40dd05385046156`；远端最大已发布 Plan 为 `155`，本任务占用下一个编号 `156`。
- 引擎/构建可用性：首次读取或修改 LFS/UE 资产前运行 `python scripts/setup_lfs.py --check`；执行 Unreal authoring、自动化或构建前确认 Editor 已关闭并取得共享 Unreal 锁。
- 现有聚焦测试结果：规划阶段尚未确定目标 Widget，因此未运行页面级审计或自动化；收到首批具体调整项后补齐基线证据。
- 共享契约 / 难合并资源风险：`.uasset` 为难合并二进制；修改前必须基于最新 main 并避免与其他任务同时编辑同一 Widget。精选 DLL/manifest 只能由最终组合候选全量重建，不能选择任一侧旧产物。
- 基线损坏时的停止条件：目标控件在最新 main 不存在、同一 Widget 存在尚未集成的并行人工修改、要求实际改变业务逻辑/公共契约，或无法在不覆盖用户 Blueprint 微调的情况下应用时，停止越界部分并报告。

## 实现提纲

1. 接收每项 UI 细节后记录目标页面、控件名、现状、目标、素材来源和逻辑不变量，并补齐准确 Writes/验收。
2. 读取最小相关 Widget、配对 C++ 和 `MOD-ReEchoUI` 路线，判断参数由 Blueprint、C++ 还是运行时构造拥有。
3. 通过窄范围、幂等的 Blueprint authoring 或直接 Designer 修改实现；保护用户已保存的其他布局细节，不运行会重建整页的历史脚本。
4. 每完成一组同页面修改即 Compile/Save、审计并向用户提供 PIE 验收入口；发现逻辑需求则先更新 Plan。
5. 完成静态校验、适用构建/自动化、架构文档审阅和用户人工验收后再发布并清理工作分支。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan/静态 | `python scripts/validate_project.py`、`git diff --check` | Plan 编号、格式与项目不变量通过 |
| LFS/资产 | `python scripts/setup_lfs.py --check`、目标 Blueprint Compile/Save 与窄范围资产审计 | 资产完整、可加载且修改保存成功 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development`，发布前 `-FullRebuild` | UHT/UBT 退出码为 0，精选预构建包匹配最终候选 |
| 运行时行为变化时自动化 | 目标页面对应的 `scripts/ue/Run-Automation.cmd -Filter ...` | 受影响交互/绑定报告通过 |
| 人工 | 用户具名 PIE 页面与截图对比 | 视觉、层级、可调性和交互手感通过 |

## 执行记录

### 变化

- 2026-08-31 首项：用户确认将结算统计小字改为所见即所得。删除 `NativeConstruct -> ApplySettlementStatLayout -> ConstrainSettlementStats` 调用链以及失效声明/包含；它原本会覆盖两页八组统计的坐标、18/20 字号、颜色、对齐、换行和裁切。
- 保留 `WBP_ReEchoRestart.uasset` 字节与现有层级，不改名、不重建、不迁移布局；`RefreshRunStatsValues`、关卡/碎片/构筑数值投影、结果页显隐及按钮行为未改。
- 新增 `ReEcho.UI.RestartWidgetAuthoredStats`：对保存默认值与人工自定义参数分别检查全部 32 个统计文本，验证构造、失败页、胜利页和 Slate 重构造都保留样式/Slot/标签文案，并断言真实数值继续更新。
- 用户随后自行微调并保存胜利页，明确要求提交；本次提交包含用户保存的 `WBP_ReEchoRestart.uasset` 和所见即所得修复，不推送远端。`ReEcho.uproject` 的 Editor 机器关联改动单独 stash 保留，不纳入提交；预构建包按仓库描述符重新刷新。
- 只读 UE 资产核查确认：`VictoryEncounterLabel` 已为 X=420、字号20；`DefeatEncounterLabel` 仍为 X=85、字号18。两个 Canvas 为独立布局，胜利微调不会自动应用到失败；不擅自覆盖失败页。此次核查未 Compile/Save 或修改资产。

### 证据

- 规划基线：`origin/main@11dc24d2d3e36a3bcec5898bb40dd05385046156`。
- 编号审计：远端 `origin/main` 最大已发布编号为 `155`，Plan156 无占用。
- 实现基线仍为本工作区 `d26833af`；只读 fetch 观察到远端 `d62a088a`（10 个后续提交），涉及回响/敌人表现、CG 和对应文档，不修改本项 Restart 源码或 WBP；本轮未合入远端或更新 main。最终发布时需重新审计共享模块文档并重建组合候选。
- `python scripts/setup_lfs.py --check`：通过。
- `Build-Editor.cmd -Configuration Development`：通过，97 个编译/链接动作完成，精选预构建包刷新。已知 `CompressImageArray` 弃用警告不属于本项；本次不是最终发布 `-FullRebuild`。
- `Run-Automation.cmd -Filter ReEcho.UI.RestartWidget`：2/2 Success（`RestartWidgetAuthoredStats`、`RestartWidgetPresentation`），退出码 0。证据日志：`Saved/Logs/ReEcho-session-20260831-003151-pid45116.log`；原有缺图回退测试产生预期缺图 Warning，启动阶段还有队列执行前的通用 `Condition failed` 日志，不把它描述为全启动无警告。
- 构建完成后 `python scripts/validate_project.py`、`git diff --check`：通过。先前构建进行中的静态检查因 bundle 尚未刷新失败，已在构建结束后重跑通过。
- 用户胜利页保存后的复核：`Build-Editor.cmd -Configuration Development` 通过并按仓库 `ReEcho.uproject` 刷新 manifest；`ReEcho.UI.RestartWidget` 仍 2/2 Success，日志 `Saved/Logs/ReEcho-session-20260831-005828-pid16112.log`，静态校验与 LFS fsck 通过。UE 只读核查前后蓝图 blob 为 `3b9e5edca94dc32ef29dede429e8d1e23d39b687`，未被脚本另行改写。
- 未提交的 Editor 机器关联变化已保存在 stash `587bef38ddcdc4e001035eed6989d49c00a383ec`（`Plan156 preserve editor-only ReEcho.uproject association`），本项提交不含该变化。

### 剩余风险

- 取消代码覆盖后，原先被掩盖的 Blueprint 排版问题会直接呈现。用户已调整胜利页，失败页仍需在 `DefeatCanvas` 单独微调，或由用户明确要求同步胜利样式；不能再以 C++ 固定坐标掩盖。

### 人工验收结果/请求

- 请用户在 Plan156 的 `WBP_ReEchoRestart > VictoryCanvas / DefeatCanvas` 微调任一统计标签/数值的位置、字号和颜色，Compile/Save 后打开对应结算页，确认与 Designer 一致；数值样例应被实际统计替换，标签与样式保持蓝图值。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已更新：AREA-UI 明确结算统计只投影数值、布局归 WBP。
- `MOD-ReEchoUI.md` 已更新：记录八组统计的 Designer 权威、稳定命名及聚焦回归测试。
- `Design/UI/ReEcho_UI修改指导.md` 已更新：记录胜利/失败统计控件调参路径、命名和运行时内容替换边界。
- `ARCHITECTURE.md` 已审阅、无需修改：不新增模块、依赖或状态所有者，符合表现只消费逻辑结果的不变量。
- `README.md` 已审阅、无需修改：现有 `AREA-UI -> MOD-ReEchoUI` 路由与源文件位置不变。
