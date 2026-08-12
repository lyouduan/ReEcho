# Plan 29 - Run/Save - echo storage foundation

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Unassigned.
- Task status: `Ready` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `NotRequired` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Planning ref / implementation base: planning ref `coord/gavyn-echo-storage-replay`; implementation starts from freshly fetched, human-approved `origin/main` and consumes this Plan without merging other coordination refs.
- Implementation branch: `plan/29-echo-storage-foundation` in a separate clean worktree.
- Depends on / Blocks: no implementation dependency on Plan26 or Plan28. Publishes the storage/save contract required by Plans 30 and 31.
- Writes: `Source/ReEcho/Public/Core/ReEchoTypes.h`; `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`; `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`; `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`; focused Plan29 storage/save tests; this Plan's Execution notes.
- Stable Reads: recorder output, immutable `FReEchoRecording`, CSV-backed build snapshots and current save normalization.
- Impact mode: `SharedContract` because it changes run/save state and replaces the legacy `RecordingHistory`/`AnchorId` contract.
- Compatibility promise / downstream action: preserve recording contents, 20 Hz determinism, run-locked build snapshots and in-encounter resume. Plan30 consumes only the published replay-resolution API; Plan31 consumes only storage summaries/commands, never mutable arrays.
- Explicit exclusions: no shop UI, GameMode spawning, Echo actor behavior, card/shop acquisition source, XLSX/CSV/schema, recording samples/events, `.uasset`/`.umap` or visual work.

## Locked goal

Replace the legacy capped recording history plus single anchor with an explicit run-local echo-storage model:

- `PendingRecording`: the just-completed encounter while its store/skip decision is pending.
- `LatestCompletedRecording`: a rolling full recording used only by the legacy/default “replay the immediately previous encounter” behavior; it does not consume a storage slot and is overwritten after every successful encounter.
- `StoredEchoes`: full immutable `FReEchoRecording` values explicitly chosen by the player, capped by storage capacity.
- `SelectedReplayIds`: stable `FGuid` references to stored echoes for the next encounter; array indices are never persistent identity.
- storage capacity and specific-replay limit are distinct persisted run capabilities. `SpecificReplayLimit == 0` means the specific-replay ability has not been acquired; `1` means specific single replay; values above `1` mean specific multi replay.

For the current prototype, storage capacity and the maximum supported specific-replay limit are both `3`. The eventual shop/card acquisition source is deferred, but the contract must accept `SpecificReplayLimit == 0` so Plan30 can preserve automatic previous-encounter replay before acquisition.

## Locked acceptance

- [ ] A successful encounter stages exactly one immutable pending recording; a failed encounter stages nothing.
- [ ] Store/skip is explicit and can be applied only to the current pending recording. Skipping discards permanent-storage eligibility but still promotes the recording to `LatestCompletedRecording` for the immediate default replay path.
- [ ] Storing into free capacity copies the pending full recording into `StoredEchoes`; storing at capacity requires a valid replacement slot/ID and never silently evicts a recording.
- [ ] Finalizing either store or skip clears the pending decision and promotes the same completed recording to the rolling latest slot. The next successful encounter overwrites the rolling latest slot without changing previously stored values.
- [ ] Stored identity uses unique valid recording GUIDs. Duplicate storage, stale replacement targets, invalid capacities and stale selected IDs are rejected or normalized deterministically.
- [ ] Storage capacity and specific-replay limit have independent, clamped APIs suitable for a later card or shop effect. Reducing capacity requires an explicit deterministic policy and cannot silently delete player-selected stored data during ordinary mutation.
- [ ] Save data persists pending/latest/stored recordings, capacities and selected replay GUIDs. In-encounter save-and-quit plus restore preserves the exact next-echo decision state.
- [ ] Save version is advanced. Compatible v4 saves migrate deterministically: without a valid anchor, the newest history item becomes rolling latest and specific replay remains unavailable; with a valid anchor, that full recording becomes stored/selected specific-single state. Invalid references fail clearly or are normalized without crashes.
- [ ] Legacy `RecordingHistory`, `AnchorId`, `SetAnchor`, `ClearAnchor` and ambiguous `GetEchoRecordings` cease to be the live public source after migration; no second competing history remains.
- [ ] Focused deterministic tests cover pending lifecycle, skip/store/free-slot/full-slot replacement, capacity rejection, GUID stability, v4 migration and v5 save round-trip.
- [ ] Formatting, Editor Development build, focused Plan29 automation, project validation and `git diff --check` pass.
- [ ] No generated UE build products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: freshly fetch and record `origin/main`; read current Exchange and the final Plan28/26 ownership state without merging their coordination refs.
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
| Focused automation | Plan29 storage plus affected save/recording tests | State transitions and migration/round-trip pass |
| Whitespace/scope | `git diff --check` and explicit path audit | No malformed or out-of-scope files |
| Human | Not required | This Plan exposes no player-facing UI or feel change |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request
