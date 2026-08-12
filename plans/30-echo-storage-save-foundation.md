# Plan 30 - Run/Save - echo storage foundation

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan30 Executor (local `plan/30-echo-storage-foundation` worktree).
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `InProgress` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `NotRequired` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: accepted behavior reference `36ca009`; combined adaptation base `origin/main` at `0726bd6`.
- Implementation branch: combined adaptation on local `integration/gavyn-umg-gameplay-20260812`.
- Depends on / Blocks: no implementation dependency on Plan26 or Plan28. Publishes the storage/save contract required by Plans 31 and 32.
- Writes: `Source/ReEcho/Public/Core/ReEchoTypes.h`; `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`; `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`; `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`; focused Plan30 storage/save tests; this Plan's Execution notes.
- Stable Reads: recorder output, immutable `FReEchoRecording`, CSV-backed build snapshots and current save normalization.
- Impact mode: `SharedContract` because it changes run/save state and replaces the legacy `RecordingHistory`/`AnchorId` contract.
- Compatibility promise / downstream action: preserve recording contents, 20 Hz determinism, run-locked build snapshots and in-encounter resume. Plan31 consumes only the published replay-resolution API; Plan32 consumes only storage summaries/commands, never mutable arrays.
- Explicit exclusions: no shop UI, GameMode spawning, Echo actor behavior, card/shop acquisition source, XLSX/CSV/schema, recording samples/events, `.uasset`/`.umap` or visual work.

## Locked goal

Replace the legacy capped recording history plus single anchor with an explicit run-local echo-storage model:

- `PendingRecording`: the just-completed encounter while its store/skip decision is pending.
- `LatestCompletedRecording`: a rolling full recording used only by the legacy/default “replay the immediately previous encounter” behavior; it does not consume a storage slot and is overwritten after every successful encounter.
- `StoredEchoes`: full immutable `FReEchoRecording` values explicitly chosen by the player, capped by storage capacity.
- `SelectedReplayIds`: stable `FGuid` references to stored echoes for the next encounter; array indices are never persistent identity.
- storage capacity and specific-replay limit are distinct persisted run capabilities. `SpecificReplayLimit == 0` means the specific-replay ability has not been acquired; `1` means specific single replay; values above `1` mean specific multi replay.

For the current prototype, storage capacity and the maximum supported specific-replay limit are both `3`. The eventual shop/card acquisition source is deferred, but the contract must accept `SpecificReplayLimit == 0` so Plan31 can preserve automatic previous-encounter replay before acquisition.

## Locked acceptance

- [x] A successful encounter stages exactly one immutable pending recording; a failed encounter stages nothing.
- [x] Store/skip is explicit and can be applied only to the current pending recording. A successful encounter immediately updates `LatestCompletedRecording`; skipping discards only permanent-storage eligibility and keeps that rolling latest value for the immediate default replay path.
- [x] Storing into free capacity copies the pending full recording into `StoredEchoes`; storing at capacity requires a valid replacement slot/ID and never silently evicts a recording.
- [x] Finalizing either store or skip clears the pending decision without changing the rolling latest slot. The next successful encounter immediately overwrites that rolling latest slot without changing previously stored values.
- [x] Stored identity uses unique valid recording GUIDs. Duplicate storage, stale replacement targets, invalid capacities and stale selected IDs are rejected or normalized deterministically.
- [x] Storage capacity and specific-replay limit have independent, clamped APIs suitable for a later card or shop effect. Reducing capacity requires an explicit deterministic policy and cannot silently delete player-selected stored data during ordinary mutation.
- [x] Save data persists pending/latest/stored recordings, capacities and selected replay GUIDs. In-encounter save-and-quit plus restore preserves the exact next-echo decision state.
- [x] Save version is advanced. Compatible v4 saves migrate deterministically: without a valid anchor, the newest history item becomes rolling latest and specific replay remains unavailable; with a valid anchor, that full recording becomes stored/selected specific-single state. Invalid references fail clearly or are normalized without crashes.
- [x] Legacy `RecordingHistory`, `AnchorId`, `SetAnchor`, `ClearAnchor` and ambiguous `GetEchoRecordings` cease to be the live public source after migration; no second competing history remains.
- [x] Focused deterministic tests cover pending lifecycle, skip/store/free-slot/full-slot replacement, capacity rejection, GUID stability, v4 migration and v5 save round-trip.
- [x] Formatting, Editor Development build, focused Plan30 automation, project validation and `git diff --check` pass.
- [x] No generated UE build products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: freshly fetch and audit `origin/main`, integrate the human-selected result into local planning, then branch locally so this Plan and Exchange reservation are present. Read final Plan28/26 ownership without consuming their WIP.
- Engine/build availability: ask the human to save and close this ReEcho Editor before compiling; installed UE 5.8 only.
- Existing focused-test result: record project validation and the current save/recording focused-test baseline, or report exact pre-existing failures.
- Active exclusive ownership or shared-contract approval: stop if another active task owns `Core/ReEchoTypes.h`, RunSubsystem, RunSaveGame or the save contract. Plan26/28 published Writes are currently disjoint.
- Stop condition if the baseline is broken: do not broaden into GameMode/UI/recorder implementation or silently invalidate all prior saves.

## Implementation outline

1. Define small reflected state/summary types only where they are needed by save and future read-only UI. Keep full recordings private behind const queries and command methods.
2. Implement pending, rolling-latest and explicit stored-slot transitions transactionally; validate before mutating.
3. Add separate storage-capacity and specific-replay-limit state/APIs. Do not encode persistent identity as array positions.
4. Advance the save contract and implement explicit v4 normalization into the new model before deleting live reliance on history/anchor.
5. Add cheap deterministic storage/save automation, format, validate and build. Update only this Plan's Execution notes unless its published contract changes.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project/data/workflow invariants pass |
| Format | Repository `.clang-format` on changed `.h`/`.cpp` | Intended C++ formatting only |
| Build | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT exit code 0 |
| Focused automation | Plan30 storage plus affected save/recording tests | State transitions and migration/round-trip pass |
| Whitespace/scope | `git diff --check` and explicit path audit | No malformed or out-of-scope files |
| Human | Not required | This Plan exposes no player-facing UI or feel change |

## Execution notes

### Changed

数据模型与存档基础（全部为本地实现，未触碰 GameMode/UI/录制语义/卡牌商店）：

- `Source/ReEcho/Public/Core/ReEchoTypes.h`
  - 新增 `ReEchoEchoStorage` 命名空间常量：`DefaultStorageCapacity=3`、`MaxStorageCapacity=3`、`SpecificReplayUnavailable=0`、`MaxSpecificReplayLimit=3`。
  - 新增 `UENUM EReEchoEchoStorageResult`（Success / NoPendingRecording / InvalidRecordingId / DuplicateRecordingId / StorageFull / InvalidReplacementTarget / InvalidStorageCapacity / InvalidReplayLimit / ReplayLimitExceeded）。
  - 新增 `USTRUCT FReEchoStoredEchoSummary`（RecordingId、EncounterIndex、Duration、MapId、CharacterId、WeaponId、bSelectedForNextEncounter）。
  - 新增 `USTRUCT FReEchoEchoStorageSummary`（StoredEchoes、StorageCapacity、SpecificReplayLimit、bHasPendingRecording/PendingRecording、bHasLatestCompletedRecording/LatestCompletedRecording、SelectedReplayIds）。
- `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `CurrentSaveVersion` 升到 `5`，新增 `MinimumSupportedSaveVersion = 4`。
  - `RecordingHistory` / `AnchorId` 仅保留为 v4 迁移输入；新增 v5 字段 `bHasPendingRecording`、`PendingRecording`、`bHasLatestCompletedRecording`、`LatestCompletedRecording`、`StoredEchoes`、`SelectedReplayIds`、`StorageCapacity`、`SpecificReplayLimit`。
- `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - 移除旧公共方法 `AddRecording` / `SetAnchor` / `ClearAnchor`。
  - 新增窄事务命令 API：`StagePendingRecording`、`HasPendingRecording`、`SkipPendingRecordingStorage`、`StorePendingRecording`、`StorePendingRecordingReplacing(FGuid)`、`GetEchoStorageSummary`、`SetSelectedReplayIds` / `GetSelectedReplayIds`、`GetStorageCapacity` / `GetSpecificReplayLimit` / `SetStorageCapacity` / `SetSpecificReplayLimit`、`TryGetStoredEcho` / `TryGetPendingRecording` / `TryGetLatestCompletedRecording`、`ResolveReplayRecordings`，以及遗留 facade `GetEchoRecordings`（BlueprintPure，转发到 `ResolveReplayRecordings`）。
  - 私有状态与 helper：`bHasPendingRecording`、`PendingRecording`、`bHasLatestCompletedRecording`、`LatestCompletedRecording`、`StoredEchoes`、`SelectedReplayIds`、`StorageCapacity`、`SpecificReplayLimit`；`ResetEchoStorage`、`NormalizeSelectedReplayIds`、`FindStoredEchoIndex`。
- `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`（约 453 行）
  - 匿名命名空间 helper：`MakeStoredEchoSummary`、`FReEchoEchoStorageRestoreState`、`ContainsRecordingId`、`MigrateV4EchoStorage`（最新历史→latest；有效 anchor→stored+selected+SpecificReplayLimit=1；缺失/无效 anchor→确定性归一化、不崩溃）、`ReadV5EchoStorage`（容量钳制、去重）、`TryNormalizeRestoredRecording`。
  - `IsValidResumableSave` 接受版本 4..5；`StartRun` 改调 `ResetEchoStorage()`；`CompleteEncounter` 改调 `StagePendingRecording(Recording)`。
  - 全部命令实现为“先校验后变更”，失败不产生局部副作用。Store 在 `StoredEchoes.Num() >= max(0,StorageCapacity)` 时返回 StorageFull（无静默驱逐）。`SetStorageCapacity` 拒绝收缩到低于已存储数量；`SetSpecificReplayLimit` 归一化选择；`ResolveReplayRecordings` 在 limit>0 且存在选择时返回所选 stable-GUID 回响，否则返回滚动 latest。
  - `CreateSaveSnapshot` 写入 v5 字段，`RecordingHistory`/`AnchorId` 留空；`RestoreSaveSnapshot` 区分 v4 迁移与 v5 读取并归一化。
- `Source/ReEcho/Private/Tests/ReEchoSaveGameTests.cpp`：旧 `AddRecording`/`SetAnchor` 用法改为 `StagePendingRecording`→`StorePendingRecording`→`SetSpecificReplayLimit(1)`→`SetSelectedReplayIds({Recording.Id})`；断言校验遗留 facade 解析 + 新 summary。
- `Source/ReEcho/Private/Tests/ReEchoEchoStorageTests.cpp`（新增，约 480 行，5 个测试）：`ReEcho.Run.EchoStoragePendingLifecycle`、`ReEcho.Run.EchoStorageSlots`、`ReEcho.Run.EchoStorageCapabilities`、`ReEcho.Run.EchoStorageSaveMigration`、`ReEcho.Run.EchoStorageRecordingPayloadUnchanged`，覆盖全部 16 个场景（成功/失败 pending、skip 保留 latest、空闲槽、满槽拒绝、替换、无效/过期目标、重复 GUID、容量独立、选择上限、去重/归一化、v4 无 anchor/有效 anchor/无效 anchor/空历史、v5 往返、facade 读新状态、录制载荷不变）。
- `plans/29-echo-storage-save-foundation.md`：Executor owner 落位，状态 `Ready`→`InProgress`→`Review`，12 条锁定验收 `[ ]`→`[x]`。
- `shared/PLANNER_EXCHANGE.md`：Plan30 状态 `InProgress`→`Review`，Active 所有权 `Reserved`→`Active`。

### Evidence

- 门禁：`git fetch --prune origin` 通过；remote 仅 `origin`（github.com/lyouduan/ReEcho.git）；`origin/main` 是 `planning/26-31-local` 的祖先（未前进，无需停止/上报）；`plans/29-echo-storage-save-foundation.md` 与 `planning/26-31-local` 均存在；本地分支 `plan/30-echo-storage-foundation` 由 `planning/26-31-local` 创建于独立 worktree `ReEcho-plan29`。
- 格式化：`clang-format --style=file -i` 应用于 6 个变更 C++ 文件（LLVM/Allman、TabWidth 4、ColumnLimit 120），退出码 0。
- 静态校验：`python scripts/validate_project.py` 通过（仅静态，退出码 0）。
- 构建：`scripts/ue/Build-Editor.cmd -Configuration Development` 成功（约 35s，UHT/UBT 退出码 0）。Editor 未运行，无需等待人工关闭。
- 自动化：Plan30 的 5 个测试全部 `Result={Success}`；受影响保存/录制自动化（ReEcho.Run、ReEcho.Recording、ReEcho.Combat.ElementReactionSaveContinuity、ReEcho.Weapons.RunLockAndEquipmentSnapshotParity、DomainRevision）均通过。
- 空白/范围：`git diff --check` 干净（退出码 0）；diff 不含机器本地路径；Binaries/Intermediate/Saved 已被 .gitignore 正确忽略。
- 无 Blueprint（.uasset/.umap）引用旧 API；遗留 `GetEchoRecordings` facade 保留并仅从新状态解析，无第二份历史真相、无 AnchorId 权威。

### Remaining risks

- `GetEchoRecordings` 兼容 facade 为临时保留，Plan31 接受后应由其正式 replay-resolver 替代（当前已转发到 `ResolveReplayRecordings`，不引入新权威）。
- 容量收缩策略采用“拒绝收缩到低于已存储数量”的确定性拒绝，而非自动驱逐；若未来希望支持优雅驱逐需 Plan31/32 显式定义。
- `SpecificReplayLimit == 0` 路径（未获得具体回放能力、默认回放上一次遭遇）由 `ResolveReplayRecordings` 兜底返回滚动 latest，Plan31 运行时需据此实现零/一/多回放。
- v4 迁移对“无效 anchor”采用确定性归一化（不崩溃、不静默交换），但不保留任何孤立引用；若历史中存在多 anchor 冲突以首个有效为准。

### Human validation result/request

`NotRequired` — 本 Plan 不暴露任何玩家可见 UI 或手感变更；仅改变 run/save 内部状态与对外契约。待人工在 Review 阶段确认是否批准进入 main 集成。

### Planner review

- 2026-08-12：逐项复核状态事务、v4→v5 迁移、v5 往返、兼容 facade 与测试覆盖；校正验收文字为“成功结算立即更新 rolling latest，store/skip 只清 pending”。
- Planner 复跑 `python scripts/validate_project.py`、Editor Development 构建、`ReEcho.Run.EchoStorage` 5 项自动化及 `git diff --check`，全部通过。
- 旧基线上的技术实现已验收；因远端 Plan29 UMG 迁移和存档调用时序变化，本 Plan 回到 `InProgress`，待在 `0726bd6` 上完成迁移、保存边界和回归验证后重新关闭。
