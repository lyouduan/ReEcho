# Plan 38 - Gameplay - held basic attack repeats through action locks

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan38 Executor.
- Plan authored by (AI side): `Gavyn-side AI`.
- Implementation authored by (AI side): `Gavyn-side AI`.
- Task status: `Closed` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `Passed` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: current published `origin/main` containing Plan37 and the accepted Plan28 input-mode implementation.
- Implementation branch: local `plan/38-held-basic-attack-repeat` in a separate worktree.
- Depends on / Blocks: repairs Plan28's held basic-attack runtime; independent of active Plan32 shop/echo UI work and may execute in parallel.
- Writes: `ReEchoPlayerAbilities.*`; the narrowest additive PlayerPawn/Weapon readiness or retry seam only if required; Plan38 attack-repeat automation; this Plan's Execution notes.
- Stable Reads: Plan28 automatic/manual input-source ownership; existing GAS input spec/cooldown; current weapon attack interval, ordered attack steps and `StepLockRemaining` lifecycle.
- Impact mode: `Isolated`; no shared save, recording, shop, UI, data or audio contract changes.
- Compatibility promise / downstream action: automatic and manual modes continue sharing the authoritative GAS basic-attack path; attack damage, ordered steps, cooldown/attack-speed scaling, target selection, recording exclusion and P pause behavior remain unchanged.
- Explicit exclusions: no direct damage bypass, no separate automatic-only attack timer, no CSV/XLSX rebalance, no attack animation/VFX redesign, no GameMode/shop/UI/save/recording change and no Executor PIE or visual sign-off.

## Locked goal

Holding basic attack must continue producing valid ordered attacks in both automatic and manual modes until the authoritative input is released or gameplay becomes blocked. A temporary weapon action lock must delay/retry the next attack; it must not permanently terminate a still-held GAS attack loop.

The reported deterministic failure is the long sword mismatch: its attack repeat interval is 0.28 seconds while its ordered step lock is 0.8 seconds. The first repeat attempt is temporarily rejected and the current ability ends, while the held input flag remains true and never produces a new press.

## Locked acceptance

- [x] With `W_J_01` or an equivalent deterministic interval/step-lock mismatch, one continuous held input executes at least two valid ordered basic attacks after enough simulated time; it does not stop after the first temporary busy result.
- [x] Automatic and manual held input use the same corrected GAS loop. Automatic mode still requires a valid in-range target; manual mode still uses physical press/release ownership.
- [x] A temporary action-lock/cooldown rejection schedules a bounded future retry without tight per-frame polling, duplicate damage, cooldown bypass or resetting the weapon's ordered-step cursor.
- [x] Releasing input, switching mode, opening a gameplay-blocking menu, losing the automatic target or death ends/releases the loop and prevents later queued attacks.
- [x] Existing attack-speed scaling, weapon damage/order tests, Plan28 targeting/input-source tests and the recording exclusion remain green.
- [x] Focused regression covers the real held-input → GAS → weapon → second-hit seam for both modes, rather than asserting only held flags or lock-duration arithmetic.
- [x] Project validation, C++ formatting, Editor Development build, focused/affected automation and `git diff --check` pass; no generated products outside the current prebuilt-publication contract or machine-local paths enter the task commit.
- [x] The Executor performs no PIE. The user validates continuous attacks, release responsiveness and feel before closure.

## Step 0 gate

- Baseline branch/commit: fetch and create the worktree from the exact published current `origin/main` containing this Plan; record the commit.
- Engine/build availability: use installed UE 5.8; acquire the same-clone Unreal lock and do not close/discard another user's Editor session.
- Existing focused-test result: record current `ReEcho.AttackMode` and relevant weapon/GAS automation or the exact unavailable prerequisite.
- Active exclusive ownership or shared-contract approval: Plan32 writes shop/GameMode/UI paths and does not own the declared attack files. Stop if another active task has since claimed AbilitySystem/Player/Weapon attack-loop writes.
- Stop condition if the baseline is broken: do not hide a pre-existing compile/test failure, rebalance weapon tables, bypass GAS or solve only automatic mode. Report the exact blocker or required public-contract expansion.

## Implementation outline

1. Reproduce the interval-shorter-than-action-lock failure at the smallest real GAS/weapon integration seam.
2. Distinguish a temporary weapon-busy result from released/blocked/invalid terminal conditions. Keep a held ability alive or re-arm it deterministically until the next legitimate readiness point.
3. Preserve the single GAS/weapon authority: do not add a parallel damage or automatic timer path.
4. Add focused auto/manual second-hit and release/cancel regressions, then run the affected Plan28, GAS and weapon suites.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project/source/workflow invariants pass |
| Format/build | repository `.clang-format`; `scripts/ue/Build-Editor.cmd -Configuration Development` | Changed C++ formatted; UHT/UBT succeeds |
| Focused automation | new `ReEcho.AttackMode.HeldRepeat` (or equivalent) | Auto/manual held input produces a second valid hit through the 0.28/0.8 mismatch and cancels on release |
| Affected automation | `ReEcho.AttackMode`, GAS and weapon filters | Existing input, cooldown, targeting, order and scaling behavior stays green |
| Scope | `git diff --check` and explicit path audit | No shop/UI/data/save/recording change or local/generated task artifact |
| Human | User PIE with automatic and manual long-sword hold/release | Continuous attacks and responsive stop feel correct |

## Execution notes

### Changed
- Held manual and automatic basic attack now retry through temporary weapon action locks/cooldown boundaries while authoritative input remains held.
- Added narrow PlayerPawn/Weapon read-only readiness state needed by the GAS repeat loop and focused Plan38 regression coverage.

### Evidence
- Planner reset local `main` back to the pre-Plan38 baseline `origin/main` (`9a8eb7b`) before this integration, discarding later local automation-rework commits and generated-bundle attempts.
- Plan38 original branch `plan/38-held-basic-attack-repeat` merged cleanly into local `main` with no Git conflicts.
- User reported manual testing passed on 2026-08-13.
- Note: upper project rules still contain objective automation requirements for C++ changes; no new automation rerun was performed after the reset because the user explicitly requested rollback to the original Plan38 integration and merge.

### Remaining risks
- Objective focused/affected automation evidence was not re-established in this final reset-and-merge pass. Current acceptance relies on the existing Plan38 branch delivery and user manual validation.

### Human validation result/request
- Status: `Passed` on 2026-08-13 by user manual testing; user reported Plan38 completed with no issue.
