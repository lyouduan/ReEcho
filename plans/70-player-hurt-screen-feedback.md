# Plan 70 - 程序 - 玩家受伤与低血量全屏红光反馈

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：待 Plan 发布后分配独立程序 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划基线：`origin/main@2027652`；批准实现基线：`origin/main@2983308`（已组合 Plan68 WS4/WS5 与 Windows 打包加固，并在组合源码上重新 FullRebuild）。
- 本地实现方式：用户已确认不采用一任务一 worktree，直接在当前本地 `main` 工作区执行；必须保护现有未跟踪的 BadRabbit/Rabbit Death 美术目录。
- 依赖 / 阻塞：受伤事实复用 `UReEchoCombatEventsComponent::OnHurt`；生命变化复用 `UReEchoCombatantComponent::OnHealthChanged`。最终红光颜色、范围、曲线与呼吸观感需要用户在 PIE 验收。
- Writes:
  - `plans/70-player-hurt-screen-feedback.md`
  - `Source/ReEcho/Public/UI/ReEchoPlayerHudWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoPlayerHudWidget.cpp`
  - `Source/ReEcho/Public/UI/ReEchoPlayerScreenFeedbackWidget.h`（新增）
  - `Source/ReEcho/Private/UI/ReEchoPlayerScreenFeedbackWidget.cpp`（新增）
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（仅显式传入玩家 Combat Events）
  - `Source/ReEcho/Private/Tests/ReEchoPlayerScreenFeedbackWidgetTests.cpp`（新增聚焦测试）
  - `Content/ReEcho/UI/WBP_ReEchoPlayerHud.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoPlayerScreenFeedback.uasset`（新增）
  - `Content/ReEcho/Materials/UI/M_UI_PlayerHurtVignette.uasset`（新增；准确目录可由 Executor 在既有 UI 资产规范内调整并同步本 Plan）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatantComponent.h`
  - `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `plans/49-combat-niagara-vfx-integration.md`
  - `plans/51-fix-restart-hud-screen-reset.md`
- 影响模式：`SharedContract`（扩展 Player HUD 初始化参数并新增 UI 内部表现契约；只读消费 Combat 事件，不修改伤害结算、生命权威或 Combat Module）。
- 兼容承诺 / 下游操作：红光反馈缺失、资产加载失败或 Widget 未绑定时只降低视觉反馈，不得影响伤害、生命、暂停、Restart、存档或战斗流程；原生 Widget fallback 必须可安全运行。
- 明确排除：不使用 `SceneViewExtension`、自定义 RDG Pass 或 Camera Post Process；不修改伤害数值、格挡语义、死亡流程、Combat 公共事件结构或生产数据表；不把视觉参数写入 Combat；不修改或提交现有未跟踪 BadRabbit/Rabbit Death 资产。

## 锁定目标

1. 玩家受到 `AppliedDamage > 0` 且非格挡的最终伤害时，屏幕左右立即出现一次短促红光，并平滑衰减；连续受击重触发单一效果，不生成或排队多个 Widget。
2. 玩家生命降低时允许出现持续的低血量红光；血量越低可配置为越明显，但启用阈值、强度、颜色、边缘宽度、柔和度和呼吸速度均为表现层可调参数，不固化当前讨论数值为玩法常量。
3. 瞬时受击脉冲与持续低血量底色使用独立输入，在统一上限内合成；回血应平滑减弱或关闭低血量效果，死亡时停止循环并让死亡流程接管。
4. 效果只属于当前玩家的 Player HUD；敌人/Echo 受伤、零伤害、格挡、初始化、存档恢复不会误触发受击脉冲。
5. 全屏表现适配窗口模式、16:9 与 21:9；中心战斗区域保持可读，暂停/结算等更高 UI 层不被遮挡。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEcho` / `AREA-UI`：新增 Player HUD 内部的全屏反馈子控件与显式事件接线。
  - `MOD-ReEchoUI`：维护 Player HUD/WBP 分工、全屏反馈入口和人工验收边界。
  - `MOD-ReEchoCombat` / `AREA-AbilityCombat`：仅稳定读取既有 `OnHurt` 与生命变化事件；不修改模块代码或权威契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，二者均已加入 `Writes`；关闭前审阅 `MOD-ReEchoCombat.md`，若事件事实未变化则在执行记录中写明无需修改。
- 设计意图：Combat 只发布结算事实，Player HUD 只绑定并路由当前玩家事件，独立 Screen Feedback Widget 拥有效果状态，WBP/UI Material 拥有视觉形状与可调曲线。任何表现失败均不得反向改变玩法。
- 权威状态与依赖：玩家生命仍由 `UReEchoCombatantComponent`/Combat 权威拥有；新 Widget 只保存瞬时显示状态。`ReEcho` 主模块继续单向依赖 `ReEchoCombat`，不新增 Runtime Module 或反向依赖。
- 决策记录：
  1. 采用 `UMG + User Interface Material`，而非 `SceneViewExtension`：本需求不需要 SceneColor/GBuffer/RDG，UMG 更符合现有 UI 生命周期、分辨率适配和美术调参边界。
  2. 新建 `UReEchoPlayerScreenFeedbackWidget`，不把动画状态堆入血条控件；未来治疗、护盾破裂等屏幕反馈可复用该表现入口，但本 Plan 不预建未使用的通用事件总线。
  3. HUD 初始化显式接收 `UReEchoCombatEventsComponent`，不从 Combatant Owner 隐式查找组件；依赖可见且便于测试。GameMode 只负责装配，不保存反馈状态。
  4. `OnHurt` 只驱动瞬时脉冲，`OnHealthChanged` 只驱动持续低血量输入，避免回血/初始化误触发受击。
  5. 低血量映射采用 WBP 可编辑曲线或等价可调配置，C++ 仅归一化生命比例、过滤事件并钳制安全范围；最终默认参数由人工 PIE 调整。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计无 Runtime 拓扑或跨模块不变量变化，无需修改正文。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计无新稳定模块/AREA 路由，无需修改正文。
  - `MOD-ReEcho.md`：维护 Player HUD 对 Combat 最终事件的只读表现装配与代码位置。
  - `MOD-ReEchoUI.md`：维护全屏反馈控件、WBP/材质职责、可调参数和验证路径。
  - `MOD-ReEchoCombat.md`：仅审阅既有事件是否足够；不因只读消费者制造文档 diff。
- 关闭前逐项填写审阅结果：在执行记录记录每份相关文档“已更新”或“已审阅、无需修改”及理由。

## 锁定验收

- [ ] 玩家实际受伤出现左右红光并在配置时长内衰减；零伤害、格挡、敌人/Echo 受伤不会触发。
- [ ] 低血量红光随配置曲线变化，回血平滑减弱；瞬时脉冲与持续底色合成后不超过配置上限。
- [ ] 连续受击只重触发单实例反馈，不叠加 Widget、不形成长期不透明红屏。
- [ ] 初始化、重新绑定、销毁、Restart/Travel 后 Delegate 数量正确，无重复触发或悬空引用。
- [ ] 原生 fallback 与 `WBP_ReEchoPlayerHud` 均可安全运行；反馈资产缺失不影响玩法。
- [ ] 聚焦自动化覆盖过滤、强度钳制、低血量映射输入和重绑定清理；修改的 C++ 完成格式化。
- [ ] Editor Development 构建、聚焦自动化、`python scripts/validate_project.py`、`git diff --check` 通过；发布候选完成 `-FullRebuild` 并刷新允许的预构建包。
- [ ] 用户在 PIE 验收 16:9/21:9、连续受击、回血、严重低血量、暂停与死亡切换时的视觉质量。
- [ ] 未提交现有未跟踪美术目录或精选预构建允许列表之外的 UE 生成产物。

## Step 0 门禁

- 基线分支/提交：`origin/main@2983308`；Plan 发布前已完成外部提交审计和组合 FullRebuild，实现发布前再次 fetch 审计。
- 引擎/构建可用性：UE 5.8 安装版；构建/Editor 命令前使用 Git common-dir Unreal 锁，并要求用户保存和关闭交互式 Editor。
- 现有聚焦测试结果：不复用旧 HUD/VFX 构建证据；本候选重新验证。
- 共享契约 / 难合并资源风险：`WBP_ReEchoPlayerHud.uasset` 为 Exclusive 二进制资产，执行前必须确认同路径无外部变化；当前未跟踪 BadRabbit/Rabbit Death 目录不在 Writes，必须保持原样。
- 基线损坏时的停止条件：fetch 发现新外部提交、WBP 同路径变化、基线构建失败或无法安全创建/编辑 UE 资产时停止并报告，不用文本方式修改 `.uasset`。

## 实现提纲

1. 新增独立 Screen Feedback 原生 Widget，提供低血量输入、受击重触发和受控合成；保留无资产 fallback。
2. Player HUD 显式绑定 Combat Events 与 Combatant，在重新初始化和销毁时对称解绑；GameMode 装配当前玩家的 `Combatant`、`CombatEvents` 和头像。
3. 通过 Unreal Editor 创建 UI Material、反馈 WBP，并把反馈子控件接入 `WBP_ReEchoPlayerHud`；所有全屏节点 `HitTestInvisible`，保持更高屏幕层覆盖关系。
4. 新增聚焦自动化，验证纯计算/过滤/重绑定契约；维护相关 `CODEBASE_MAP` 文档。
5. 完成构建、静态检查与自动化后交给用户 PIE 调参验收；视觉参数返修不改变 Combat 契约。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| C++ 格式 | 仓库 `.clang-format` 处理修改的 `.h/.cpp` | diff 仅含预期语义与格式 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.PlayerScreenFeedback` | 过滤、映射、重触发/重绑定相关报告通过 |
| Blueprint/资产 | Unreal Editor Compile/Save + `CompileAllBlueprints` 或等价资产加载检查 | WBP/材质可加载且绑定完整 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与文本不变量通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终候选及精选预构建包匹配 |
| 人工 | PIE：16:9/21:9、连续受击、回血、低血量、暂停、死亡/Restart | 用户记录 Passed 或明确延期 |

## 执行记录

### 变化

Plan-only，尚未执行实现。

### 证据

Plan 编写基于 `origin/main@2027652` 的 Player HUD、GameMode 装配、Combat Contracts、Plan49/51 与 `MOD-ReEcho`/`MOD-ReEchoUI` 当前事实；发布前发现远端前进至 `a07efb6`，已完成无冲突组合、重新 FullRebuild 与最新静态校验，批准实现基线为 `origin/main@2983308`。

### 剩余风险

- 最终视觉参数必须经 PIE 人工判断；自动化不能证明舒适度和可读性。
- `WBP_ReEchoPlayerHud.uasset` 是二进制独占写入面，实施期间若远端同路径变化需停止审计。

### 人工验收结果/请求

`PendingBeforeClose`：实现完成后由用户验收红光强度、覆盖范围、呼吸节奏和多宽高比表现。

### 架构文档审阅结果

待实现与评审阶段填写。
