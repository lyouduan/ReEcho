# Plan 51 - 程序 - Restart 后战斗 HUD 跨关卡屏幕重置

## 协调

- Planner 负责人：Gavyn-side AI（程序路线 Planner）
- Executor 负责人：Gavyn-side AI（于独立 plan worktree 实现）
- Plan 编写方（AI 侧）：`Gavyn-side AI`
- 实现编写方（AI 侧）：`Gavyn-side AI`
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）
- 本地规划 / 实现基线：origin/main `9aa99c2`
- 本地实现方式：一任务一 worktree `ReEcho-plan51-restart-hud-reset`
- 依赖 / 阻塞：无
- Writes:
  - `Source/ReEcho/Private/UI/ReEchoUIManagerSubsystem.cpp`
  - `Source/ReEcho/Public/UI/ReEchoUIManagerSubsystem.h`
  - `Source/ReEcho/Private/Tests/ReEchoUIManagerSubsystemTests.cpp`（新增聚焦自动化测试）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（`SetupArena` / `HandleRestartRequested` / `SetGameplayPresentationVisible`）
  - `Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`
  - `Source/ReEcho/Private/UI/ReEchoUIManagerSubsystem.cpp`（`CreateScreen` / `CloseScreen` / `Deinitialize`）
- 影响模式：`Isolated`（仅 UI 框架子系统内部生命周期钩子；无公共契约 / Schema / 存档 / 跨模块运行时依赖变化）
- 兼容承诺 / 下游操作：
  - `OpenScreen` / `CloseScreen` 公共 API 行为不变；`ResetScreens` 为 UIManager 内部方法，不对外暴露新契约。
  - `PreLoadMap` 绑定仅清空 `ActiveScreens` 并 `RemoveFromParent`，不改变局内屏幕开关语义。
  - 下游 Restart / 退出到主菜单 / Continue 行为不变，仅 HUD 在重载关卡后正确重建并挂载视口。
- 明确排除：
  - 不改 HUD Widget（`PlayerHud` / `EncounterHud` / `Weather`）的视觉、布局或数据绑定逻辑。
  - 不改 `OpenLevel` 重开流程，不引入无缝旅行（seamless travel）。
  - 不处理"非 travel"造成的屏幕残留（该类已由调用方 `CloseScreen` 覆盖，本 Plan 只解决 travel 这一类）。
  - 不重构 UI 框架其他部分，不为旧残留 widget 加任何 fallback 显示逻辑。

## 锁定目标

玩家从死亡 / 胜利界面点击"重新开始"（或暂停界面"退出到主菜单"）后，重载关卡进入新一局时，战斗 HUD（血条 `PlayerHud`、计时 `EncounterHud`、天气 `Weather`）正常创建、挂载到视口并可见可更新；开始菜单等其它屏幕不受影响。

本修复同时消除"任何屏幕跨 `OpenLevel` 残留导致重载后不显示"的整类隐患，而非只修恰好发现的三个 HUD 屏。

修改本节需 Planner / 用户同意。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`；`MOD-ReEchoUI`（文档型逻辑入口，当前实现仍属 `MOD-ReEcho`）。无新增 / 变更 Runtime Module，不新增稳定 `MOD-*` / `AREA-*`。
- 对应模块文档：
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：必须进入 Writes，补充"`UReEchoUIManagerSubsystem` 在关卡 travel 时重置 `ActiveScreens`"这一生命周期事实（见 §相关文档同步范围）。
  - `shared/CODEBASE_MAP/README.md`：无需修改（理由：本 Plan 不新增 / 拆分 Runtime Module，仅修正已有 UI 子系统的生命周期钩子，全局拓扑与索引不变）。
- 设计意图：
  - `UReEchoUIManagerSubsystem` 是 `UGameInstanceSubsystem`，其 `ActiveScreens` 字典在 `OpenLevel`（non-seamless 整图重载）时**不会**被销毁——只有 UWorld / GameMode / PlayerController 会重建，而 GameInstance 跨 travel 存活。
  - 引擎在 `LoadMap` 期间会 `RemoveAllViewportWidgets`，把旧 HUD 从视口 `RemoveFromParent` 摘掉；但旧 UObject 仍 `IsValid` 且停留在 `ActiveScreens` 中。
  - 新 GameMode 的 `SetupArena` 经 `OpenScreen(EncounterHud)` → `CreateScreen` 命中短路：`if (UUserWidget* ExistingScreen = GetScreen(Screen)) return ExistingScreen;`——直接返回那个已脱离视口的旧 widget，**跳过 `AddToLayer`（`AddToViewport`）**。
  - 之后 `SetGameplayPresentationVisible(true)` 只把旧 widget 的 `Visibility` 设为 `Visible`，但它并未挂回视口，因此血条 / 计时永不渲染。
  - 修复把"travel 后屏幕必须重建"变成 `UIManager` 自身的不变量：在关卡加载前清空 `ActiveScreens`（先 `RemoveFromParent` 再 `Reset`），使新世界总是走"新建 + `AddToViewport`"分支。职责归位到持有该状态的模块，而不是在每个 travel 调用方补 `CloseScreen`。
- 权威状态与依赖：不改变状态所有者、公共契约、模块 / 目录职责或依赖方向；仅新增 UIManager 内部 reset 钩子，不引入对其它模块的新依赖。
- 决策记录：
  - 否决方案 A（在 `HandleRestartRequested` 手动 `CloseScreen` 三个 HUD）：仅修恰好发现的 3 个屏，属补丁式；未来任何新增屏若漏写 `CloseScreen` 会重演同类 bug；且把"travel 后清屏"职责摊到调用方（正是当初漏掉 HUD 的根因）。用户已认可 A 为补丁式、C 更合理。
  - 选定方案 C（UIManager 在 travel 时统一 `ResetScreens`）：治本，消除整类"跨关卡屏幕残留"隐患；与项目"不掩盖非法状态 / 不为残留加 fallback"的纪律一致。
  - 钩子选型：绑定 `FCoreUObjectDelegates::PreLoadMap`（`Initialize` 中 `AddUObject` 绑定，`Deinitialize` 中 `Remove` 解绑，handler 调用 `ResetScreens`）。
    - 备选 1：在 `CreateScreen` 内比对"当前 World"与"上次创建 World"，世界变化则重置——更侵入，且仍需在 `CreateScreen` 内新增世界追踪状态。
    - 备选 2：在全部 travel 调用方（`HandleRestartRequested` / `HandleExitToMainMenuRequested` / `HandleContinueRequested`）显式调用——即方案 A 的泛化，仍把职责摊到调用方。
    - `PreLoadMap` 最贴合"travel 前清空"语义，且自动覆盖 Restart / 退出到主菜单 / Continue 所有 `OpenLevel` 路径。
  - 已知代价 / 注意：
    - `PreLoadMap` 在**首次进游戏**也会触发，但此时 `ActiveScreens` 为空，`ResetScreens` 为 no-op，无副作用。
    - 实现时必须按本机引擎头文件 `CoreUObjectDelegates.h` 核对 `PreLoadMap` 实际签名：UE5 通常为 `DECLARE_MULTICAST_DELEGATE_OneParam(FPreLoadMap, const FString&)`；个别版本可能提供 `PreLoadMapWithContext` 变体。签名以本机引擎为准对齐。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已更新——补充 `UReEchoUIManagerSubsystem` 在关卡 travel 时重置 `ActiveScreens` 的事实，并注明 `ReEchoRestartWidgetTests` 只测 Restart Widget 表现、不覆盖子系统级重置。
  - `shared/CODEBASE_MAP/README.md`：无需修改（无新 MOD / AREA；理由：本 Plan 不新增 / 拆分 Runtime Module，只修正已有子系统的生命周期钩子）。
  - 其它 `modules/MOD-*.md`（ReEcho / ReEchoCombat / ReEchoRun 等）：无需修改（本 Plan 不改变 Combat / Run / Encounter 等模块职责或契约）。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoUI.md` 已更新：具体变化为新增"travel 时重置 `ActiveScreens`"章节与测试引用。
  - `README.md` 已审阅、无需修改：本 Plan 不新增 / 拆分 Runtime Module。
  - 其它 `MOD-*.md` 已审阅、无需修改：职责与契约未变。

## 锁定验收

- [ ] 死亡 / 胜利 → 点击"重新开始"后，新一局战斗 HUD（血条 / 计时 / 天气）可见且计时与血条正常更新（人工 PIE）。
- [ ] 暂停 → "退出到主菜单" → 新游戏，HUD 正常显示（人工 PIE）。
- [ ] Continue（读取存档）进入后 HUD 正常显示（人工 PIE）。
- [ ] 自动化：`ReEcho.UIManagerSubsystem.ResetOnTravel` 通过——`ResetScreens` 后 `ActiveScreens` 清空、widget 离屏；再次 `OpenScreen` 得到**全新实例**（身份不同于重置前）。
- [ ] `scripts/ue/Build-Editor.cmd` 通过（实现期 UHT / UBT 退出码 0）。
- [ ] `python scripts/validate_project.py` 通过。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支 / 提交：origin/main `9aa99c2`（已 `git fetch`，无他人新提交，审计通过）。
- 引擎 / 构建可用性：实现期需关闭 Editor 后运行 `scripts/ue/Build-Editor.cmd -Configuration Development`（发布候选再 `-FullRebuild`）。
- 现有聚焦测试结果：`ReEcho.UI.RestartWidgetPresentation` 已存在，仅测 `WBP_ReEchoRestart` 表现，不测 UIManager 重置，与本修复无冲突。
- 共享契约 / 难合并资源风险：无（仅 C++ 与 markdown / 代码库文档，无 XLSX / CSV / 二进制资产）。
- 基线损坏时的停止条件：若 `fetch` 发现 origin/main 前进，则重新审计物理 / Git / 逻辑冲突后再继续。

## 实现提纲

在不修改锁定目标、验收或已发布契约的前提下，可以优化实现细节。

1. 检查最小相关代码 / 数据表面：`ReEchoUIManagerSubsystem`（Initialize / Deinitialize / CreateScreen / CloseScreen / AddToLayer）、`ReEchoGameMode::SetupArena`。
2. 抽取 `ResetScreens()`：将当前 `Deinitialize` 中"遍历 `ManagedWidgets` `RemoveFromParent` + `Reset`；`ActiveScreens.Reset()`"提取为可复用方法。
3. 在 `Initialize` 绑定 `FCoreUObjectDelegates::PreLoadMap` → `HandlePreLoadMap` → `ResetScreens()`；`Deinitialize` 中 `Remove` 解绑并调用 `ResetScreens()`。
4. 新增聚焦自动化测试 `ReEcho.UIManagerSubsystem.ResetOnTravel`：构造子系统（或借 GameInstance 子系统），`OpenScreen` 两个屏，`ResetScreens()` 后断言 `ActiveScreens.Num() == 0` 且 widget 不在视口；模拟 `PreLoadMap` 广播后再次 `OpenScreen` 得到新实例。
5. 更新 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 记录 travel 重置事实。
6. 在独立 worktree `ReEcho-plan51-restart-hud-reset` 实现并提交；实现期跑 `Build-Editor`，发布候选跑 `-FullRebuild` + `validate_project.py`。

## 验证矩阵

| 层级 | 命令 / 检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目 / 源码不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd` | UHT / UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | `ReEcho.UIManagerSubsystem.ResetOnTravel` 及相关报告通过 |
| 需要人工时 | 具名 PIE / 可用性任务 | Restart / 退出到主菜单 / Continue 后 HUD 可见（记录人工结果或明确延期跟进） |

## 执行记录

### 变化

（尚未实现；实现在 `ReEcho-plan51-restart-hud-reset` worktree。）

### 证据

### 剩余风险

- `PreLoadMap` 引擎签名需按本机 `CoreUObjectDelegates.h` 对齐；个别版本可能用 `PreLoadMapWithContext`。
- 自动化测试需在无 PIE 环境下验证"不在视口"判定，按 UMG `IsInViewport` / `GetCachedGeometry().IsValid()` 选择稳定断言。

### 人工验收结果 / 请求

- 待实现后 PIE：Restart / 退出到主菜单 / Continue 三条路径下 HUD 均正常显示。

### 架构文档审阅结果
