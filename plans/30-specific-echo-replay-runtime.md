# Plan 30 - Gameplay - specific single and multi echo replay

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan30 Executor.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `InProgress` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: current local `main` containing accepted Plan29 and the locked Plan28 behavior contract. Plan28 implementation is integrated later by the Planner.
- Implementation branch: local `plan/30-specific-echo-replay-runtime` in a separate clean worktree.
- Depends on / Blocks: may execute in parallel with Plan28 under explicit human approval. Both may edit GameMode locally; the Planner owns semantic integration and preserves both behaviors. The completed Plan publishes runtime behavior consumed independently by Plan31's parallel implementation.
- Writes: Plan29's narrow replay-resolution implementation surface in RunSubsystem; `Source/ReEcho/Public/ReEchoGameMode.h`; `Source/ReEcho/Private/ReEchoGameMode.cpp`; additive Echo initialization/query changes only if required; focused Plan30 tests; this Plan's Execution notes.
- Stable Reads: Plan29 storage/capability APIs, immutable recordings, current Echo actor initialization/playback and encounter resume flow.
- Impact mode: `SharedContract` for replay resolution; otherwise isolated runtime orchestration.
- Compatibility promise / downstream action: before acquiring specific replay (`SpecificReplayLimit == 0`), spawn exactly the rolling previous-encounter echo as today. Once the limit is positive, spawn only the explicitly selected stored echoes. Preserve each recording's original build/WeaponId and current recording semantics.
- Explicit exclusions: no storage/shop UI, no card/shop acquisition source, no recording/schema edits beyond Plan29 state, no Echo automatic-attack serialization, no runtime weapon switching, no XLSX/CSV or visual assets.

## Locked goal

Unify specific single- and multi-encounter replay as one selected-ID mechanism:

- `SpecificReplayLimit == 0`: the ability is unavailable, so the next encounter automatically replays `LatestCompletedRecording` when present.
- `SpecificReplayLimit == 1`: resolve exactly zero or one explicitly selected stored recording.
- `SpecificReplayLimit > 1`: resolve up to that many explicitly selected stored recordings in stable selection order.

Once specific replay is available, an empty selection intentionally produces no echo; it does not silently fall back to the latest encounter. Each resolved recording spawns its own Echo actor for the current encounter.

## Locked acceptance

- [x] With no specific-replay ability, encounter N+1 spawns only encounter N's rolling latest recording; older stored selections cannot override it.
- [x] With limit one, only the selected stored GUID resolves. With a larger limit, all valid selected GUIDs resolve once, in stable order, up to the limit.
- [x] Once the specific ability is active, empty/stale selection resolves to no echo plus a clear deterministic normalization/error path; it never restores implicit latest behavior.
- [x] Multiple Echo actors initialize from their own immutable recording/build snapshot and play simultaneously without sharing mutable playback, weapon or position state.
- [x] New encounters and resumed suspended encounters resolve the same selected set. A mid-encounter save/continue does not change echo count, identities or playback time.
- [x] Each Echo retains existing automatic basic attacks and successful active-skill playback. Automatic attacks remain unrecorded and current-world target/hit resolution remains unchanged.
- [x] GameMode no longer hard-codes `GetEchoRecordings(1)` or a single Echo; cleanup, stats/UI consumers and encounter fixed-step safely handle zero, one or several actors.
- [x] Focused deterministic tests cover latest fallback, specific single, specific multi, empty/stale/duplicate IDs, ordering, limit clamps and save-resume resolution.
- [x] Executor performs formatting, project validation, Editor build, focused Plan30 automation and `git diff --check`, then gives the user a short PIE checklist. The user owns simultaneous-replay clarity and gameplay validation.
- [x] No generated products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: exact current local `main` containing accepted Plan29; record it before work. Do not consume or alter Plan28's WIP branch.
- Engine/build availability: human saves/closes Editor before C++ build; installed UE 5.8 only.
- Existing focused-test result: Plan29 storage/save and Plan28 integration checks are green on this base.
- Active exclusive ownership or shared-contract approval: the human explicitly approved concurrent GameMode work by Plans28/30. Plan30 must not edit Plan28's Player/Pause UI files and must record every GameMode integration point for the Planner. Stop if Plan29's final API differs from this consumer contract.
- Stop condition if the baseline is broken: do not edit shop UI or reintroduce legacy history/anchor APIs as a workaround.

## Implementation outline

1. Implement one pure resolver from capability plus stored/latest state to ordered immutable recordings.
2. Replace single-record GameMode spawning/resume with iteration over the resolver result without modifying Plan28's Player/Pause UI files.
3. Audit every zero/one/many Echo assumption in cleanup, fixed-step, stats and resume; record the GameMode functions changed so the Planner can combine Plan28/30 behavior explicitly.
4. Add cheap deterministic resolver/runtime/resume automation and build. Hand actual multi-Echo readability/play-feel validation to the user.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project invariants pass |
| Format/build | `.clang-format`; `scripts/ue/Build-Editor.cmd -Configuration Development` | Intended formatting and UHT/UBT success |
| Focused automation | Plan30 resolver plus affected save/recording tests | Latest/single/multi/resume behavior passes |
| Whitespace/scope | `git diff --check` and path audit | No out-of-scope changes |
| Human | User PIE with latest fallback, one selected and several selected echoes | User reports `Passed` or concrete rework |

## Execution notes

### Changed

零/单/多回响运行时，全部本地实现，未触碰禁止修改的 UI/录制/卡牌商店/XLSX/CSV/蓝图：

- `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp` — 修正单一 resolver `ResolveReplayRecordings`：
  - `SpecificReplayLimit > 0`：只返回 `SelectedReplayIds` 指向的 `StoredEchoes`，**绝不再回退到 rolling latest**；空/失效选择确定性地解析为**零个**回响（本 Plan 的核心修复）。
  - `SpecificReplayLimit == 0`：只返回 `LatestCompletedRecording`（存在时），已存储/残留选择不能覆盖该行为。
  - resolver 只读，不改变 storage/selection 状态。
- `Source/ReEcho/Private/ReEchoGameMode.cpp` — 零/单/多 Echo 生成：
  - `BeginNextEncounter`：原 `GetEchoRecordings(1)` 单生成改为遍历 `ResolveReplayRecordings(MaxStorageCapacity)`，每个录制独立 `InitializeEcho`（自身不可变录制 + build snapshot）生成一个 `AReEchoEchoActor`。
  - `ResumeSavedEncounter`：同样遍历 resolver 结果，每个回响在生成后 `AdvanceEcho(SavedState.EncounterTime)` 快进到保存时间。
  - 未新增/删除 GameMode 字段，复用既有 `Echoes` 数组。
- `Source/ReEcho/Private/Tests/ReEchoEchoReplayRuntimeTests.cpp`（新增，5 测试）：LatestFallback、SpecificSingle、SpecificMulti（顺序/limit 钳制/溢出原子拒绝）、EmptyStale（空选择→零 + 失效 GUID 原子拒绝且保留有效选择）、SaveResume。
- `plans/30-specific-echo-replay-runtime.md`：状态 `InProgress`→`Review`，10 条锁定验收 `[ ]`→`[x]`。
- `shared/PLANNER_EXCHANGE.md`：Plan30 状态 `InProgress`→`Review`，Active ownership `Reserved`→`Active`。

### GameMode 函数/字段变更记录（交由 Planner 逻辑合并 Plan28/30）

- `BeginNextEncounter`（生成块）：由单录制单 Echo 改为遍历 resolver 结果，每录制一个独立 Echo。
- `ResumeSavedEncounter`（生成块）：同上，并对每个 Echo `AdvanceEcho` 到保存时间。
- 未改动：`ClearCombatants`（已遍历全部 `Echoes`）、`HandleFixedStep`（已遍历全部 `Echoes`）、`PrintGMResult`（按 `Echoes.Num()` 计数）、`SetFogRevealSources(Player, Echoes.IsEmpty()?nullptr:Echoes[0].Get())`（仍单源——见风险）、`InitializeStats` 的 Stats 查询（仍读取首个 Echo 的 stats——见风险，未改 UI 文件）。

### Evidence

- clang-format（`--style=file`）应用于 3 个变更 C++ 文件：退出码 0。
- `python scripts/validate_project.py`：PASS，退出码 0（静态校验，含迁移/CSV/工作流/Unreal 描述符）。
- `scripts/ue/Build-Editor.cmd -Configuration Development`：Succeeded，退出码 0（按 EXECUTOR_RULES 持有 Unreal 锁；ReEcho-plan28 有一个编辑器实例在运行，但属不同 worktree，本 worktree 的 build/automation 不冲突）。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho`：AUTOMATION_EXIT=0；整个 `ReEcho` 套件零 `Result={Fail}`。Plan30 的 5 个测试全部 `Result={Success}`；Plan29 的 EchoStorage/SaveGame/Storage 等受影响测试全部通过。
- `git diff --check`：退出码 0。变更路径限定为 `Source/ReEcho/{Private/ReEchoGameMode.cpp, Private/Run/ReEchoRunSubsystem.cpp, Private/Tests/ReEchoEchoReplayRuntimeTests.cpp}`、`plans/30-specific-echo-replay-runtime.md`、`shared/PLANNER_EXCHANGE.md`；无机器本地路径，无 Binaries/Intermediate/Saved 等生成目录。

### Remaining risks

- Stats/UI 单槽限制：`InitializeStats(PlayerStats, PlayerHealth, EchoStats, EchoHealth, bHasEcho)` 只消费单个 Echo 的 stats。当存在多个被选回响时，GameMode 仍传入首个 Echo 的 stats（安全、不崩溃、未改 UI 文件）。真正的多 Echo stats 展示交由用户/Planner（Plan26 StatsWidget 不在本 Plan 允许修改范围内）。
- 雾揭示：`SetFogRevealSources` 仅接受单 actor，GameMode 仍传 `Echoes[0]`；多源雾揭示未实现（超出范围，且为保守起见未改天气 widget）。
- 同时回放的可读性/手感与玩法校验按本 Plan 的 Human-validation 步骤明确交由用户在 PIE 中完成（见下）。

### Human validation result/request

`PendingBeforeClose`（保持不变）。运行时行为已由自动化覆盖；用户需在 PIE 中按以下清单校验并回报 `Passed` 或具体返工：

1. 默认（`SpecificReplayLimit == 0`）：完成一场遭遇后，下一场只生成上一场的 rolling latest 回响（单个）。
2. 获得回放能力且 limit=1 并选择 1 个存储回响：下一场只生成该选定回响。
3. limit>1 并选择多个存储回响：下一场按选择顺序生成多个独立 Echo，各自回放、位置、武器、状态相互独立。
4. limit>0 但选择为空：下一场不生成任何 Echo（不回退到 latest）。
5. 暂停/恢复（suspended encounter）：恢复后 Echo 数量、身份、播放时间与选择一致。

### Planner integration review (rework required)

- After merging Plan30 and Plan31, an integrated Unity Build failed because
  `ReEchoEchoReplayRuntimeTests.cpp` and `ReEchoEchoStorageTests.cpp` define the same
  file-local `CreateStartedRun` helper name. UE Unity Build combines both source files into one
  translation unit, so the helper must be renamed or otherwise made collision-safe.
- Plan30 returns to `InProgress` until the integrated editor build and focused automation pass.
