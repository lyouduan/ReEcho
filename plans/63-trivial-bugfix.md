# Plan 63 — 琐碎 Bug 修复长期积压（Trivial Bugfix Backlog）

- **计划 / Plan**：63
- **角色 / Role**：Programmer
- **秘书状态 / Secretary status**：InProgress（长期更新，living document）
- **工作树 / Worktree**：`ReEcho-plan63` → 分支 `plan/63-trivial-bugfix`
- **目标 / Goal**：收集并修复不构成独立 Plan 的小缺陷、可复现性增强（如调试 GM）、表现/手感微调等琐碎项；保持 main 稳定，每个修复以独立提交 fast-forward 到 main。

## 为什么需要这个 Plan

Demo 稳定化阶段（P0）持续暴露零散小问题：单个修复体量不足以开一个完整 Plan，但又需要编号、可追溯、可回滚。本 Plan 作为"长期积压"容器，持续接收这类改动，避免在主工作树随意打补丁、也避免为鸡毛蒜皮开一堆短命 Plan。

## 使用约定

- 所有实现落在 `plan/63-trivial-bugfix` 工作树/分支；主工作树 `ReEcho` 只接收 fast-forward。
- 每个修复在本文档「修复记录」追加一条，含：现象、根因、改动文件、验证方式。
- 发布前走标准门禁：`Build-Editor -FullRebuild` + `validate_project.py` + 提交 + fast-forward main + push origin/main。
- 若某个修复膨胀为独立系统级改动，应拆分出独立 Plan，不再计入本 Plan。

## 修复记录

### #1 — 新增 `GMGrantCard`：GM 直接获得指定卡牌（用于复现/验证静默刻度 G_2_17）

- **现象 / Symptom**：没有便捷手段在运行中直接拿到某张卡（如静默刻度 `G_2_17`）来观察其整条效果链路；只能靠正常商店/构筑流程，复现成本高。
- **根因 / Root cause**：现有 `ApplyTraitCard` 仅在 `Phase==CardChoice` 且卡牌在 `PendingTraitCardIds` 时生效，无法在任意时刻、对任意卡牌授予。权威构建变更封装 `TryMutateAuthoritativeBuild` 仅在该 .cpp 内部可见，外部（含 GM）无法复用。
- **改动 / Changes**：
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h` + `ReEchoRunSubsystem.cpp`：新增 `UReEchoRunSubsystem::DebugGrantCard(FName CardId)`，复用 `TryMutateAuthoritativeBuild` + `ReEchoCardRuntime::TryGrantCard` + `ReEchoCharacterPromotion::TryPromote`，忽略阶段/候选限制，可随时授予。
  - `Source/ReEcho/Public/ReEchoGameMode.h` + `ReEchoGameMode.cpp`：新增 `GMGrantCard(FName CardId)`（`UFUNCTION(Exec)`），沿用 `EnsureGMCommandAvailable`（`#ifndef _SHIPPING` 守卫）+ `PrintGMResult`（`[GM]` 前缀）。
  - `docs/GM_COMMANDS.md`：补充命令说明。
- **用法 / Usage**：PIE 或 Development 构建按 `~` 打开控制台，输入 `GMGrantCard G_2_17` 即可获得静默刻度；`GMGrantCard` 无参打印用法。Shipping 构建命令体被守卫跳过。
- **验证 / Verification**：`Build-Editor -FullRebuild` 通过；`validate_project.py` 静态校验通过；逻辑复用已验证的 `TryGrantCard` 路径，与正常特质卡授予行为一致。

<!-- 后续修复继续在此处追加 #2、#3…… -->
