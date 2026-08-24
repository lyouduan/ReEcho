# Plan 93 - 程序 - 遭遇 HUD WBP 视觉改造

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Proposed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@99bbfe53e5bd5321f408f5c6ba9987ec7a0845c3`。
- 本地实现方式：一任务一 worktree，`C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan93-encounter-hud-ui`，分支 `plan/93-encounter-hud-ui`；规划者与执行者合一。
- 依赖 / 阻塞：等待用户提供或逐步确认 `WBP_ReEchoEncounterHud` 的目标视觉、素材和分批验收要求；这些内容补入锁定目标和验收后才能进入 `Ready` / `InProgress`。
- Writes:
  - `plans/93-encounter-hud-ui.md`
  - `Content/ReEcho/UI/WBP_ReEchoEncounterHud.uasset`
  - 经用户明确采用且由本页实际消费的 `Content/SourceArt/UI/**`、`Content/ReEcho/Textures/UI/**` 与可复现 `scripts/ue/**` 导入/authoring 脚本
  - `Source/ReEcho/Public/UI/ReEchoEncounterHudWidget.h`、`Source/ReEcho/Private/UI/ReEchoEncounterHudWidget.cpp`（仅在新增真实绑定或展示行为需要时）
  - 与本页契约直接相关的聚焦 UI 测试（具体路径在实现前补齐）
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `Source/ReEcho/Public/UI/Framework/ReEchoUIScreenTypes.h`
  - `Source/ReEcho/Private/UI/ReEchoUIManagerSubsystem.cpp`
  - `Source/ReEcho/Private/UI/Framework/ReEchoUIFlowCoordinatorSubsystem.cpp`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoMinimapCanvasWidget.*`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*` 的 Encounter HUD 创建、显隐和数据推送路径
  - `plans/45-ui-interaction-placeholder-assets.md`、`plans/51-fix-restart-hud-screen-reset.md`、`plans/60-ui-full-reskin.md`
- 影响模式：`Exclusive`（目标 WBP 是不可文本合并的二进制资产；同一时刻只允许本任务编辑该资产）。
- 兼容承诺 / 下游操作：保留 `EReEchoUIScreen::EncounterHud`、`GameplayHud` 层级、`EncounterText` / `CountdownText` 正常绑定、最后 5 秒警示、小地图视图转发、关卡 travel 重建和不接管输入的现有语义；WBP 负责布局与表现，C++ 继续负责只读状态和刷新。
- 明确排除：当前骨架不授权修改遭遇计时、关卡流程、暂停、战斗、存档、数据 Schema、其他页面或 Runtime Module 拓扑；不在 Editor 外手改 `.uasset`；不把整张效果图直接作为交互 HUD。

## 锁定目标

在不改变遭遇计时、关卡流程和小地图数据语义的前提下，按用户后续提供的目标图、素材或逐项说明改造 `WBP_ReEchoEncounterHud` 的视觉与布局。

本 Plan 首次发布仅建立正式编号、任务边界和独立 worktree。具体视觉目标、保留/新增控件、分批顺序与完成判据将在首次 WBP 修改前由用户确认并补入本节；补齐前任务保持 `Proposed`。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`、文档型入口 `MOD-ReEchoUI`；当前实现仍属于 `MOD-ReEcho`，不新增 Runtime Module。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已加入 `Writes`；关闭前核对 Encounter HUD 的 WBP/C++ 分工、绑定和验证路线。
- 设计意图：把最终视觉、布局、锚点和尺寸保留在 WBP，把关卡索引、剩余时间、最后 5 秒警示和小地图数据继续留在 C++ 只读展示路径。
- 权威状态与依赖：本 Plan 不改变状态所有者、屏幕枚举、Viewport 层级、GameMode 到 Widget 的数据方向或模块依赖；若用户目标要求改变公共契约，必须先扩张并重新确认 Plan。
- 决策记录：先发布 `Proposed` 骨架获取正式编号，再在同一 worktree 中随用户输入补齐锁定目标与验收；未补齐前不修改 WBP。目标 WBP 按二进制 Exclusive 资源串行编辑。
- 相关文档同步范围：`shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md` 关闭前审阅；预期因拓扑和路由不变而无需修改。`MOD-ReEchoUI.md` 和 `Design/UI/ReEcho_UI修改指导.md` 按最终真实契约维护。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待关闭前审阅。
  - `shared/CODEBASE_MAP/README.md`：待关闭前审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：待实现后维护或记录无需修改原因。
  - `Design/UI/ReEcho_UI修改指导.md`：待实现后维护或记录无需修改原因。

## 锁定验收

- [ ] 用户提供的目标视觉、素材映射、布局要求和保留行为已在实现前补入本 Plan。
- [ ] `WBP_ReEchoEncounterHud` 保持正确原生父类，`EncounterText`、`CountdownText` 和小地图控件正常更新。
- [ ] HUD 不接管输入；关卡索引、倒计时、最后 5 秒警示、页面显隐与 travel 后重建语义无回归。
- [ ] 目标 WBP Compile/Save 成功，`CompileAllBlueprints` 无错误或加载失败。
- [ ] 按最终变化面执行聚焦自动化、Editor 构建、项目校验和 `git diff --check`。
- [ ] 用户在 PIE 对目标分辨率下的布局、可读性、小地图和倒计时状态报告 `Passed` 或给出具名返工项。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@99bbfe53e5bd5321f408f5c6ba9987ec7a0845c3`。
- 引擎/构建可用性：基线包含 UE 5.8 精选 Editor 包；首次内容修改前需确认 Editor/命令锁和现有会话，最终候选按变化面重新构建。
- 现有聚焦测试结果：基线包含 UI Manager 的 Encounter HUD 创建与 travel reset 覆盖；实现前补充或确认本页所需聚焦测试。
- 共享契约 / 难合并资源风险：`WBP_ReEchoEncounterHud.uasset` 为 Exclusive 二进制资产；编辑前确认没有其他 Editor/worktree 同时修改。
- 基线损坏时的停止条件：`origin/main` 出现批准基线之外的新提交；目标 WBP 正被其他任务编辑；目标要求改变遭遇计时、输入、屏幕层级或公共绑定；需要独占命令时 Editor 会话尚未保存关闭。

## 实现提纲

1. 用户提供第一批目标图、素材或具名改动，Planner 将其写入锁定目标、验收和素材映射，并把 Plan 更新到可执行状态。
2. 审计 `WBP_ReEchoEncounterHud` 当前控件树、绑定、小地图宿主和运行截图；每次只处理一个视觉区块。
3. 通过 Unreal Editor 或可复现 authoring 脚本修改、Compile、Save；不在 Editor 外修改 `.uasset`。
4. 每批完成后运行聚焦客观检查并交给用户做 PIE 视觉验收，再记录返工或通过结果。
5. 完成最终构建、静态检查、架构文档审阅和执行记录后进入评审。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| WBP | 目标资产 Compile/Save；`CompileAllBlueprints` | `WBP_ReEchoEncounterHud` 0 errors、0 failed loads |
| UI 聚焦 | Encounter HUD 创建/travel reset 与实现前补齐的本页聚焦检查 | 创建、刷新、小地图与重建契约通过 |
| C++ | 修改源码时执行 `.clang-format` 与 `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新匹配精选包 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目与差异通过 |
| 发布 | 非纯文档最终候选执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终候选完整构建并刷新允许列表产物 |
| 人工 | 用户确认的分辨率、倒计时普通/最后 5 秒、小地图和页面显隐 PIE 检查 | 用户报告 `Passed` 或具名返工项 |

## 执行记录

### 变化

- 创建 Plan93 `Proposed` 骨架和独立 worktree；尚未修改目标 WBP 或运行时源码。

### 证据

- 编号前已 fetch；当时 `origin/main@513ec51b6d101979b6d2cf548dfc3403762b7966` 与主工作区一致，远端最大 Plan 编号为 92，新任务使用 93。
- 首次发布前 fetch 发现 Plan91 两个新提交推进远端至 `99bbfe53e5bd5321f408f5c6ba9987ec7a0845c3`；只读审计确认不修改 Encounter HUD、无物理冲突或逻辑冲突。经用户确认组合适配后，Plan93 干净变基到该提交并更新本地规划 / 实现基线。
- 已读取 `MOD-ReEchoUI`、UI 修改指导、Encounter HUD 公共头与配对实现，并确认现有 WBP/C++ 边界。

### 剩余风险

- 最终视觉目标和素材尚未提供；当前 Plan 只锁定不应被视觉改造破坏的现有运行时契约。
- WBP 是二进制资源，后续必须在专属 worktree 串行编辑并逐批验证。

### 人工验收结果/请求

- `PendingBeforeClose`：等待用户提供第一批目标图、素材或具名 UI 改动。

### 架构文档审阅结果

- 待实现完成后逐项填写。
