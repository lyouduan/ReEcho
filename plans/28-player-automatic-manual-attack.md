# Plan 28 - Gameplay/UI - automatic and manual player attacks

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Unassigned; start only from the published planning ref after reading this Plan.
- Task status: `Ready` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Planning ref / implementation base: planning ref `coord/gavyn-plan28-player-attack-modes`; implementation starts from current `origin/main` at planning time (`6c1f890`) and must integrate any human-approved newer main before Review.
- Implementation branch: `plan/28-player-attack-modes` in a separate clean worktree.
- Depends on / Blocks: depends on the current run-locked weapon implementation, GAS basic-attack loop and pause-aware encounter clock. It does not block Plan26's read-only Inventory/Stats UI work.
- Writes: `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`; `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`; `Source/ReEcho/Public/ReEchoGameMode.h`; `Source/ReEcho/Private/ReEchoGameMode.cpp`; `Source/ReEcho/Public/UI/ReEchoRestartWidget.h`; `Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`; additive weapon/enemy query changes only if required for effective range and deterministic target tie-breaking; new Plan28-only automation under `Source/ReEcho/Private/Tests/`; this Plan's Execution notes.
- Stable Reads: existing GAS abilities/tags, weapon definitions and ordered attack steps, enemy alive/runtime identity, recorder contract, pause/settings menu behavior and encounter timing.
- Impact mode: `Isolated`. Any public query added for target selection must be additive and must not change the Plan24/26 typed data consumer contract.
- Compatibility promise / downstream action: keep current weapon damage, ordered steps, GAS cooldowns, active-skill input, run-locked WeaponId, recording schema, save schema, Echo behavior and existing B/M/Tab menus unchanged. Reuse the existing Esc pause menu and preserve its resume/restart/settings/save-and-quit, death and victory behavior. Plan26 may continue in parallel because its declared Writes are disjoint.
- Explicit exclusions: no XLSX/CSV/schema changes; no save-format or recording-format change; no Echo auto-attack change; no runtime weapon switching; no active-skill automation; no new `.uasset`/`.umap` or art; no visual sign-off by the Executor.

## Locked goal

Add mutually exclusive automatic and manual basic-attack modes for the player. A spawned player defaults to automatic mode. Keep `Esc` as the single pause entry and add `Automatic Attack` / `Manual Attack` choices to the existing pause menu; choosing either mode applies it, closes the pause menu and resumes play.

Automatic mode deterministically targets the nearest living enemy inside the current weapon attack's effective range, aims at it and drives the existing GAS basic-attack/cooldown path continuously. Manual mode preserves the existing mouse aim and held left-mouse/`J` basic attack. `Q`/Space active skill remains manual in both modes. Automatic basic attacks are never serialized into Echo recordings.

Attack mode is session-local in this Plan: it is not added to save data. A newly spawned player, including one created while continuing a saved run, starts in automatic mode.

## Locked acceptance

- [ ] A fresh or continued encounter's newly spawned player reports and behaves as `Automatic` before any attack-mode input.
- [ ] Existing `Esc` input remains the only pause entry: it pauses the world, blocks gameplay abilities and opens the existing `UReEchoRestartWidget`; no `P` mapping or second attack-mode modal is introduced.
- [ ] The existing pause menu visibly shows the current attack mode and offers `Automatic Attack` and `Manual Attack`. Choosing either applies exactly one mode, closes the pause menu and restores game input/time; `Esc` retains its existing resume/toggle behavior without changing the selection.
- [ ] Existing start/loadout, trait, shop, inventory, stats, settings, death/victory and pause-menu ownership cannot stack. Adding attack-mode choices does not alter resume, restart, settings, save-and-quit confirmation, save failure, death or victory actions.
- [ ] Automatic mode selects the nearest living in-range enemy with a deterministic tie-break, updates aim/facing, holds the existing GAS basic-attack input while a target is valid, releases it when the target dies/leaves range or a menu/mode transition occurs, and reacquires safely.
- [ ] Automatic mode does not attack when no target is in range. Manual mode ignores automatic acquisition and preserves current mouse-facing plus held left-mouse/`J` behavior.
- [ ] Both modes use the same current weapon definition, ordered attack step, cooldown, damage and VFX path. Active skill behavior and input remain unchanged.
- [ ] Recorder output remains positions plus successful active-skill events only; changing attack mode and automatic/basic attacks create no recording events and require no save/recording revision.
- [ ] Focused automation covers only cheap deterministic seams that are faster and more reliable in code: default/mode transitions, deterministic nearest-target selection and the non-recording contract.
- [ ] The Executor completes C++ formatting, project validation, Editor Development build, focused Plan28 automation and `git diff --check`; the Planner decides whether broader `ReEcho.*` automation is proportionate after integration.
- [ ] The Executor does not perform PIE, screenshots, visual judgment, menu click-through or broad gameplay regression. It hands the user a runnable branch/build plus a short ordered test checklist as soon as the objective gate passes.
- [ ] The human owner validates the actual automatic/manual behavior, target reacquisition, pause-menu interaction, readability and play feel before Plan closure.
- [ ] No generated UE build products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: create the implementation worktree from a freshly fetched, human-approved `origin/main`; record the exact commit. The planning baseline was `6c1f890`.
- Engine/build availability: confirm the ReEcho Editor is saved and closed before C++ build/automation; use installed UE 5.8 only.
- Existing focused-test result: record current project validation and compile availability. Do not spend the Executor pass on broad pre-change automation unless a concrete failure requires diagnosis.
- Active exclusive ownership or shared-contract approval: re-read live Exchange. Plan26's Inventory/Stats files are disjoint; stop if another active task claims Player, GameMode, input, attack-mode UI or weapon/enemy target queries.
- Stop condition if the baseline is broken: do not disguise pre-existing compile/test failures or broaden into save/recording/data/UI refactors. Report the exact blocker to the Planner.

## Implementation outline

1. Introduce a small, testable attack-mode/target-selection unit with an explicit automatic default and deterministic nearest-target tie-break. Do not copy weapon data or parse CSV directly.
2. Let `AReEchoPlayerPawn` own the current mode and the automatic-target/held-basic-input lifecycle. Use the current GAS basic-attack path rather than calling weapon damage separately. Switch aim source between automatic target and mouse without two systems fighting over rotation.
3. Obtain the current ordered attack step's effective range through existing weapon state or the narrowest additive query needed. Do not add a second range constant like Echo's legacy `AutoTargetRange`.
4. Extend the code-built `UReEchoRestartWidget` with current-mode presentation and two mode-choice delegates/buttons. Let GameMode apply the selection through the Player and reuse the existing pause/input restoration path. Do not add `P`, another widget or another pause state.
5. Ensure every close, mode switch, death/menu transition and no-target transition releases any synthetic held basic input. Keep active-skill recording unchanged.
6. Add only Plan28's cheap deterministic automation, format changed C++, validate and build. Hand off a runnable result and concise user test checklist without Executor PIE or visual review. Update only this Plan's Execution notes unless scope/ownership/contracts actually change.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project/source/data/workflow invariants pass |
| Format | Repository `.clang-format` on every changed `.h`/`.cpp` | Only intended formatting changes remain |
| Build | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT exit code 0 with installed UE 5.8 |
| Focused automation | `scripts/ue/Run-Automation.cmd -Filter ReEcho.AttackMode` | Cheap deterministic default, transitions, targeting and non-recording checks pass |
| Integration automation | Planner-selected affected or full `ReEcho.*` after integration, only when the final diff/base warrants it | Integration confidence without delaying the user's earlier hands-on test |
| Whitespace/scope | `git diff --check` and explicit path audit | No whitespace errors, generated products or out-of-scope files |
| Human | User PIE: default auto; Esc pause menu and mode display; auto/manual on ranged and melee weapons; no target/reacquire; resume/restart/settings/save-and-quit coexistence | Executor supplies only the launch/checklist handoff; user records `Passed` or concrete rework before closure |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request
