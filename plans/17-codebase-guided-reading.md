# Plan 17 - onboarding - guided code reading

## Locked goal

由独立执行者陪同用户在约 20–30 分钟内读通 ReEcho 的主运行链和“时间回响”核心链路，使用户能够从入口自行追踪一局遭遇如何启动、记录、结束并生成下一局回响。

本任务只读：不修改源码、配置、资产或协作文档，不启动 Unreal Editor，不构建。

## Locked acceptance

### 🤖 执行者可验证

- [x] 导览以 `AReEchoGameMode::StartPlay` 为入口，使用实际代码路径和当前行号，不凭文档转述代码。
- [x] 讲清主链：启动 → 遭遇固定步 → 玩家输入/攻击 → 录制 → 遭遇结算 → Run 状态 → 下一局回响播放。
- [x] 每一站先让用户看一个小代码片段，再解释“谁调用它、它改变什么状态、下一站去哪”，不一次倾倒整份文件。
- [x] 明确区分项目核心机制与可跳过的表现/UI 细节。
- [x] 全程保持工作树只读；不改动或认领当前已有的二进制资产变更。

### 🎮 人验

- [x] 用户能够指出一局游戏的总调度入口。
- [x] 用户能够复述录制数据如何进入 `UReEchoRunSubsystem`，又如何被下一局回响消费。
- [x] 用户能够选择一个感兴趣的分支（战斗、成长或 UI），由执行者继续带读。

## Step 0 gate

- 仓库目录：当前 ReEcho 仓库根目录。
- 当前任务无需创建或切换分支；在现有工作树严格只读。
- 工作树已有用户持有的 `.uasset` 与 `Asset/` 变更；不得修改、暂存、清理或解释为本任务产物。
- 先读 `AGENTS.md`，再按其最小读取顺序读取；只从 `shared/CODEBASE_MAP.md` 提取“整体运行流程/回响”匹配行。
- 如果文件或符号已移动，先用 `rg` 重新定位并把新位置告诉用户，不沿用本 Plan 的旧行号硬讲。

## Implementation outline

执行者可以按用户基础和提问调整节奏，但不得改变只读边界和主链目标。

1. **先看总图，不读实现**：用 `docs/ARCHITECTURE.md` 的 Main flow 建立五分钟心智模型，再回到代码验证。
2. **入口与总调度**：先看 `Source/ReEcho/Public/ReEchoGameMode.h:26` 的公开入口和 `:91-95` 的关键回调，再看 `Source/ReEcho/Private/ReEchoGameMode.cpp:183` 的 `StartPlay`、`:361` 的 `BeginNextEncounter`、`:463` 的 `HandleFixedStep`、`:762` 的 `HandleEncounterEnded`。
3. **时间如何推进**：读 `Source/ReEcho/Public/Encounter/ReEchoEncounterDirector.h:7-20` 与 `Source/ReEcho/Private/Encounter/ReEchoEncounterDirector.cpp:14-47`，确认 60 Hz 固定步、暂停和结束事件如何把 GameMode 串起来。
4. **玩家输入如何进入玩法**：从 `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp:159-180` 的输入绑定，追到 `:401-472` 的攻击/技能/武器选择；只选一条攻击路径继续到 `Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp:214-273`，避免第一轮陷入全部 GAS 细节。
5. **记录什么**：读 `Source/ReEcho/Public/Recording/ReEchoRecorderComponent.h:19-40` 的接口，再看 `Source/ReEcho/Private/Recording/ReEchoRecorderComponent.cpp:10-92`；重点确认位置采样、成功技能事件、武器变化与 BuildSnapshot。
6. **一局如何变成下一局**：从 `AReEchoGameMode::HandleEncounterEnded` 追到 `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp:109` 的 `CompleteEncounter` 和 `:337` 的 `AddRecording`，再回到 `AReEchoGameMode::BeginNextEncounter` 的 `Source/ReEcho/Private/ReEchoGameMode.cpp:384-393`。
7. **回响如何消费历史**：先看 `Source/ReEcho/Public/Recording/ReEchoPlaybackComponent.h:41-55`，再看 `Source/ReEcho/Private/Recording/ReEchoPlaybackComponent.cpp:8-40`，确认历史位置插值、技能事件与武器变化如何按时间重放。
8. **收束并分叉**：让用户用自己的话复述主链；只根据用户选择继续读一个方向：GAS/伤害、局内成长/商店，或 UI/表现。

## Verification matrix

| 层 | 检查 | 预期证据 |
|---|---|---|
| 路由 | `rg` 定位上述符号 | 执行者给出当前完整路径与行号 |
| 主链理解 | 用户复述启动、结算和回响链 | 执行者纠正遗漏，直到链路完整 |
| 只读纪律 | 结束时检查 `git status --short` | 不产生属于本任务的新改动；既有资产变更保持不动 |
| 人验 | 用户选择后续专题 | 记录选定方向和仍不清楚的问题 |

## Architecture risks to point out, not solve

- `AReEchoGameMode` 同时承担场景生成、流程调度、UI 创建和菜单协调，是当前最明显的膨胀点。
- `Content/Data/*.json` 尚未成为统一运行时数据源，部分规则仍来自 C++/DeveloperSettings。
- GAS 已是权威战斗路径，但 `UReEchoCombatantComponent` 仍承担兼容适配角色，阅读时要分清新旧边界。
- 第一轮不要展开所有 UI、资源硬引用和视觉 Actor；它们会遮蔽核心回响链。

## Recommended executor model

中档或便宜模型即可；这是以检索、解释和互动节奏为主的只读任务，不需要新架构设计或代码生成。

## Execution notes

### Changed

- 完成只读的核心代码导览；未修改运行时代码、配置或 Unreal 资产。

### Evidence

- 2026-08-10：用户明确确认“Plan 17 已经完成”，作为本任务最终人验拍板。
- 仓库不存在 `plan/17*` 分支或额外 worktree，因此无需执行 worktree/分支清理。
- 本任务不要求 UHT、UBT、自动化或 PIE，不作对应证据声明。

### Remaining risks

- Plan 中列出的 GameMode 膨胀、运行时数据源未统一和 GAS/Combatant 过渡边界仍是既有架构观察，不属于本只读任务的修复范围。

### Human validation requested

- 已完成；用户于 2026-08-10 拍板验收。

## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者，任务是陪我快速读代码，不是替我生成一份长篇总结。

仓库：当前 ReEcho 仓库根目录
先读 AGENTS.md，并按它的最小读取顺序读取；再读 plans/17-codebase-guided-reading.md。只从 CODEBASE_MAP 提取与整体运行流程/回响相关的路由，不要整读 LESSONS。

严格只读：不要切分支、改文件、启动 UE、构建、暂存、提交或清理工作树。现有 .uasset 和 Asset/ 变更属于用户，完全不要碰。

按 Plan 17 的路线互动带读：每次只给我一个小步骤，提供当前文件路径、关键符号和行号，先让我打开/看代码，再解释“谁调用它、改变什么状态、下一站去哪”，然后等我确认或提问再继续。目标是让我读通：StartPlay → Encounter 固定步 → 玩家输入/攻击 → Recorder → CompleteEncounter/RunSubsystem → 下一局 Playback/Echo。

第一轮跳过不影响主链的 UI、视觉和全部 GAS 细节。遇到行号漂移先用 rg 重新定位。结束时让我复述主链，并让我选择继续深入 GAS/伤害、成长/商店或 UI/表现；把仍不清楚的问题简要告诉我即可。禁止自己合并或修改 main。
```
