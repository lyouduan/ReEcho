# Plan 30 - Gameplay - specific single and multi echo replay

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan30 Executor.
- Task status: `Ready` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: Phase A starts from current local `main` containing accepted Plan29. Phase B first integrates the reviewed local `main` containing accepted Plan28.
- Implementation branch: local `plan/30-specific-echo-replay-runtime` in a separate clean worktree.
- Depends on / Blocks: Phase A may start from accepted Plan29 and is disjoint from Plan28. Phase B remains blocked on Plan28 release/integration of GameMode/player attack ownership. The completed Plan publishes runtime behavior required by Plan31.
- Writes: Phase A owns only Plan29's narrow replay-resolution implementation surface in RunSubsystem plus focused resolver tests. Phase B may additionally write `Source/ReEcho/Public/ReEchoGameMode.h`, `Source/ReEcho/Private/ReEchoGameMode.cpp`, additive Echo initialization/query changes only if required, focused Plan30 runtime tests and this Plan's Execution notes.
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

- Phase A baseline: exact current local `main` containing accepted Plan29; record it before work.
- Phase B baseline: integrate the reviewed local `main` containing accepted Plan28, confirm Plan28 GameMode ownership is released, and rerun affected Plan28/29 checks before touching GameMode or Echo orchestration.
- Engine/build availability: human saves/closes Editor before C++ build; installed UE 5.8 only.
- Existing focused-test result: Plan29 storage/save and Plan28 integration checks are green on this base.
- Active exclusive ownership or shared-contract approval: Phase A must not touch Plan28-owned GameMode/player/pause files. Phase B must wait until Plan28 GameMode/player ownership is released. Stop if Plan29's final API differs from this consumer contract.
- Stop condition if the baseline is broken: do not edit shop UI or reintroduce legacy history/anchor APIs as a workaround.

## Implementation outline

1. **Phase A, may run now:** implement one pure resolver from capability plus stored/latest state to ordered immutable recordings. Add focused tests proving limit 0 fallback and positive-limit empty/single/multi semantics. Commit a clean local checkpoint.
2. **Phase gate:** if accepted Plan28 is not yet present on local `main`, stop after the Phase A checkpoint and report it without claiming Plan30 Review/complete.
3. **Phase B, only after the gate passes:** integrate accepted Plan28, replace single-record GameMode spawning/resume with iteration over the resolver result, and audit every zero/one/many Echo assumption in cleanup, fixed-step, stats and resume.
4. Add cheap deterministic runtime/resume automation and build. Hand actual multi-Echo readability/play-feel validation to the user.

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
