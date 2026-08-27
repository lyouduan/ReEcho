# 智者第六关 - 特质卡选择界面确认与刷新均失效导致软锁

> 本文件只提供报告字段；登记、分支、确认、合并和清理规则以 `shared/DESIGNER_RULES.md` 为唯一权威。实际报告文件名不加编号、日期或状态前缀。

## 基本信息

- 类型：Bug
- 状态：待确认
- 策划身份：cadmanwwang
- 创建时间：2026-08-27
- 源分支：main
- 源提交：284af2f5
- 当时的 `origin/main`：284af2f5
- 登记分支：`issue/cadmanwwang/sage-l6-trait-choice-softlock`
- 登记分支提交哈希：2ab50c26c51ca8f94ee086991c1cdef4c03e0122
- Merge 分支：无

## 描述

- 当前行为或现象：
  - 在“智者”模式（日志实测角色 `J_DIAMOND`、武器 `W_J_04` 镰刀）第六关开启后，弹出三张特质卡选择界面 `WBP_ReEchoTraitCardChoice_C_15`（日志实测为第 7 次遭遇 `encounter=7`、tier=3）。
  - 玩家点击“刷新”：全部 `FREE_CARD_SLOT_REFRESH_REQUEST` 均被 `FREE_CARD_SLOT_REFRESH_REJECTED`，原因 `The current run state disables post-encounter card refreshes`（即便 `canRefresh=1`、碎片 `shards=206` 足够）。
  - 玩家点击“确认”：连续多次 `ConfirmButton` 的 `BUTTON_CLICK` 均被记录，但**没有任何 `SCREEN_CLOSE_FLOW`**，界面不关闭、不应用所选卡——确认无效。
  - 玩家被卡死约 8.5 分钟，最终在 `12:24:14Z` 通过 `SCREEN_RESET_ALL`（强制重置）才脱出。即玩家反馈的“直接呕掉”。
- 目标行为：
  - 该特质卡选择界面应可正常刷新（若规则允许），且点击确认后能正常关闭并应用所选卡；至少“刷新”与“确认”二者之一必须可用，不得陷入既不可刷新也不可确认的状态。
- 适用场景：特质卡三选一（post-encounter / free 三选一）界面，tier=3 遭遇后。
- 影响范围：该模式下后期关卡的特质卡选择，可能导致进度卡死、需强制重置（丢失本次运行）。
- 明确排除：
  - 非致命崩溃：会话日志无 `Fatal error` / `Assertion failed`，属纯 UI 软锁。
  - 非碎片不足：实测 `shards=206`，远超刷新成本。
  - 非前 6 次遭遇：encounter=2/3/4/5/6 的同款界面刷新与确认均正常（见 `UIInteractionAudit.log`），问题首次出现于 `encounter=7`。

## 复现与验收

- 运行环境与构建类型：本次对应本地构建（编辑器内运行，会话 `ReEcho-session-20260827-200212-pid272536.log`，PID 272536）。
- 前置条件：进入“智者”模式，推进至第六关附近（日志实测为第 7 次遭遇 `encounter=7`、tier=3），触发 post-encounter 三选一特质卡界面。
- 复现步骤：
  1. 进入“智者”模式，推进至第六关（日志为 `encounter=7`, `tier=3`）。
  2. 触发特质卡三选一界面 `WBP_ReEchoTraitCardChoice_C_15`（弹出于 `12:15:54.875Z`）。
  3. 点击任一“刷新”按钮 → 被拒绝。
  4. 选卡后点击“确认” → 无反应，界面不关闭。
  5. 反复尝试均无效，直至强制重置脱出。
- 出现频率：本次会话中该界面首次出现即必现（1/1）。需程序进一步确认在 tier=3 后期遭遇是否 100% 复现。
- 验收标准：该界面“刷新”（若规则允许）或“确认”至少其一可用；确认后能正常关闭并应用所选卡，不再陷入软锁。

## 日志与证据

- 已提交日志：`UIInteractionAudit.log` 片段（本次卡死窗口，另存于下方路径）
- 原始日志文件名：
  - `Saved/Logs/UIInteractionAudit.log`
  - `Saved/Logs/ReEcho-session-20260827-200212-pid272536.log`
- 覆盖或截取时间范围：`2026-08-27T12:15:54.875Z`（C_15 弹出）~ `2026-08-27T12:24:14.151Z`（`SCREEN_RESET_ALL`）。片段文件：`issues/cadmanwwang/bugs/logs/sage-l6-trait-choice-softlock/UIInteractionAudit_fragment.log`
- 关键标记与摘要：
  - `SCREEN_OPEN_CREATED screen=TraitChoice widget=WBP_ReEchoTraitCardChoice_C_15 ... input=GameAndUI pause=1`（12:15:54.875）
  - 同帧 `SCREEN_CLOSE_FLOW screen=EncounterTransition`（12:15:54.883）—— 选择界面弹出与遭遇转场关闭几乎同帧，疑似 run 阶段已越过 `CardChoice`。
  - 多次 `BUTTON_CLICK ... button=ConfirmButton`（12:15:59 ~ 12:16:01 等）无后续 `SCREEN_CLOSE_FLOW`。
  - `FREE_CARD_SLOT_REFRESH_REQUEST ... canRefresh=1 shards=206` → `FREE_CARD_SLOT_REFRESH_REJECTED reason="The current run state disables post-encounter card refreshes"`。
  - `SCREEN_RESET_ALL`（12:24:14.151，卡死约 8.5 分钟后）。
- 未附日志的原因：无（已附片段）。

## 调查与交接

- 已检查的文件、字段或资产：
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`：`TryRefreshTraitCardSlot` 在 `Phase != EReEchoRunPhase::CardChoice || GetCardRules().bDisableShopRefresh` 时返回 `"The current run state disables post-encounter card refreshes"`（约 2420-2438 行）。
  - `Source/ReEcho/Private/UI/ReEchoTraitCardChoiceWidget.cpp`：`HandleConfirmClicked()` → `SelectOffer(SelectedOfferIndex)`（729-732 行）；确认按钮启用条件 `bRevealComplete && bHasSelection && CanSelectOffer(...)`（587-591 行）。
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`：`TryRefreshTraitCardSlot`、`SkipPostEncounterCardChoiceForStageTransitionCg`、`PendingTraitCardOfferEncounterIndex` 等声明。
- 已尝试操作及结果：
  - 反复点击“刷新” → 全部被拒绝（日志实证）。
  - 选卡后反复点击“确认” → 界面无响应（日志实证：点击被记录但不关闭）。
  - 最终只能强制重置脱出。
- 已排除方向：
  - 非碎片不足（实测 `shards=206`）。
  - 非前序遭遇（encounter 2-6 同界面正常）。
  - 非武器/伤害/战斗逻辑问题（本次为纯 UI 软锁）。
- 建议程序检查方向：
  1. 重点排查 `ReEchoRunSubsystem::TryRefreshTraitCardSlot` 的 Phase 判定：C_15 弹出时 `Phase` 是否仍等于 `CardChoice`？从“刷新被拒 + 确认无反应”看，run 阶段很可能在界面弹出时已被推进到非 `CardChoice`（如 `Planning`）。
  2. 排查 `WBP_ReEchoTraitCardChoice_C_15` 弹出（12:15:54.875）与 `EncounterTransition` 关闭（12:15:54.883）几乎同帧的时序：是否“遭遇转场已结束、阶段已前进”却仍弹出了 free 特质卡选择，导致界面“看起来可交互、实则阶段已不匹配”。
  3. 确认 `SelectOffer` / `HandleConfirmClicked` 在 `Phase != CardChoice` 时是否静默失败（解释确认点击被记录但不关闭界面）。
  4. 验证“智者”模式第六关（日志 `encounter=7`, `tier=3`）是否本就该弹 free 三选一；若是，则该界面的刷新/确认必须在该阶段可用，而非依赖已结束的 `CardChoice` 阶段。
- 依赖与跨团队影响：涉及 `ReEchoRunSubsystem`（run 阶段机）与 `ReEchoTraitCardChoiceWidget`（UI），可能需要设计与程序共同确认“第六关后是否应弹卡、弹卡时 run 应处于何阶段”。

## 人工确认

- 策划对描述的确认：（待 cadmanwwang 确认）
- 策划对验收结果的确认：（待合入后回归）
- 产品取舍确认：无

## 解决记录

- 最终 Merge 提交：（待填）
- 最终 `origin/main` 提交：（待填）
- 结果与残余事项：（待填）
