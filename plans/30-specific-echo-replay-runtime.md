# Plan 30 - Gameplay - specific single and multi echo replay

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Unassigned; do not start while status is `Proposed`.
- Task status: `Proposed` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Planning ref / implementation base: planning ref `coord/gavyn-echo-storage-replay`; final base must contain accepted Plan29 and accepted Plan28.
- Implementation branch: future `plan/30-specific-echo-replay-runtime` in a separate clean worktree.
- Depends on / Blocks: blocked on Plan29's accepted storage/save contract and Plan28 release of GameMode/player attack ownership. Publishes runtime behavior required by Plan31.
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

- [ ] With no specific-replay ability, encounter N+1 spawns only encounter N's rolling latest recording; older stored selections cannot override it.
- [ ] With limit one, only the selected stored GUID resolves. With a larger limit, all valid selected GUIDs resolve once, in stable order, up to the limit.
- [ ] Once the specific ability is active, empty/stale selection resolves to no echo plus a clear deterministic normalization/error path; it never restores implicit latest behavior.
- [ ] Multiple Echo actors initialize from their own immutable recording/build snapshot and play simultaneously without sharing mutable playback, weapon or position state.
- [ ] New encounters and resumed suspended encounters resolve the same selected set. A mid-encounter save/continue does not change echo count, identities or playback time.
- [ ] Each Echo retains existing automatic basic attacks and successful active-skill playback. Automatic attacks remain unrecorded and current-world target/hit resolution remains unchanged.
- [ ] GameMode no longer hard-codes `GetEchoRecordings(1)` or a single Echo; cleanup, stats/UI consumers and encounter fixed-step safely handle zero, one or several actors.
- [ ] Focused deterministic tests cover latest fallback, specific single, specific multi, empty/stale/duplicate IDs, ordering, limit clamps and save-resume resolution.
- [ ] Executor performs formatting, project validation, Editor build, focused Plan30 automation and `git diff --check`, then gives the user a short PIE checklist. The user owns simultaneous-replay clarity and gameplay validation.
- [ ] No generated products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: exact accepted Plan29 plus accepted Plan28 integration base; record it before work.
- Engine/build availability: human saves/closes Editor before C++ build; installed UE 5.8 only.
- Existing focused-test result: Plan29 storage/save and Plan28 integration checks are green on this base.
- Active exclusive ownership or shared-contract approval: Plan28 GameMode/player ownership must be released. Stop if Plan29's final API differs from this consumer contract.
- Stop condition if the baseline is broken: do not edit shop UI or reintroduce legacy history/anchor APIs as a workaround.

## Implementation outline

1. Implement one pure resolver from capability plus stored/latest state to ordered immutable recordings.
2. Replace single-record GameMode spawning/resume with an iteration over the resolver result.
3. Audit every zero/one/many Echo assumption in cleanup, fixed-step, stats and resume; change only runtime orchestration that demonstrably requires it.
4. Add cheap deterministic resolver/resume automation and build. Hand actual multi-Echo readability/play-feel validation to the user.

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

### Evidence

### Remaining risks

### Human validation result/request
