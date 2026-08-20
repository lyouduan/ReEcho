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

### #2 — 修复六武器生产表触发启动期 CSV Fatal

- **现象 / Symptom**：Plan62 已把输入槽扩展到 `1..6`，且五个可选武器使用 `LoadoutOrder 1、2、4、5、6`；但 Editor/自动化在模块启动时依次因旧的 `1..3` 顺序上限、匕首删除前的 `78/16/62` 部件计数、以及“至少存在一个 AttackPatternReplacement/UniqueBehavior”而 Fatal。
- **根因 / Root cause**：`ReEchoWeaponCsvReader.cpp` 的输入槽解析和枚举已经扩展到 6，但三组生产数据审计仍绑定删除匕首前的内容；删除匕首后，当前生产表不再含由匕首提供的替换攻击模式和唯一行为部件效果。
- **改动 / Changes**：校验上限改为从 `EReEchoInputSlot::Slot6` 派生的具名常量；部件审计同步为当前权威表的 `70` 总行、`10` 具名行和 `60` 禁用空名行；保留两类效果的 Schema/Behavior 校验能力，但不再强制生产表必须至少配置一条。继续保留 `LoadoutOrder` 唯一性和 `StartSelectable` 必须绑定输入槽等既有规则，不重排生产表。
- **验证 / Verification**：Plan58 合入最新 main 的组合候选已通过启动期 CSV 加载、VFX 与兔子投射物聚焦自动化、Editor `-FullRebuild`、静态项目校验和 diff 检查。

### #3 — 启动崩溃排查（实为工作树落后 main）+ 保留 `GMGrantCard` 诊断日志

- **现象 / Symptom**：从 `plan/63-trivial-bugfix` 工作树直接打开编辑器启动即崩溃。
- **根因 / Root cause**：plan63 工作树当时落后 `origin/main` 共 6 个提交（Plan58 兔子投射物视觉/碰撞对齐等），其 `ReEchoEnemyActor` / VFX 的启动期改动修复了崩溃；plan63 未合入这些提交，故复现崩溃。并非 Plan63 自身代码引入缺陷。
- **改动 / Changes**：
  - 将 `origin/main`（`3164f52`）fast-forward 合并进 `plan/63-trivial-bugfix`，消除启动崩溃。
  - 保留并正式提交 `DebugGrantCard` / `GMGrantCard` 上的诊断日志（`[DebugGrantCard]` / `[GMGrantCard]` 前缀，`LogReEcho` Warning 级），用于后续任意卡牌授予的可追溯复现；日志仅打印 `Phase / EncounterIndex / TimeShards / OwnedCardIds.Num()` 等只读状态，不影响构建快照。
  - 二进制冲突（ReEcho.dll / prebuilt.json）按 main 版本解决后本地增量重建，不随本提交入库，待发布时由 `-FullRebuild` 重新生成。
- **验证 / Verification**：用户实机打开合并后的 plan63 工作树，启动崩溃消失；`GMGrantCard G_2_17` 诊断日志链路打印正常（enter → grant ok → done，卡数 +1）。本条目结束，Plan 保持开放等待下一琐碎 bug。

<!-- 后续修复继续在此处追加 #4、#5…… -->
