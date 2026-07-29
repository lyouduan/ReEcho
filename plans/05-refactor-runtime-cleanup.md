# Plan 05 - Refactor - Runtime architecture cleanup

## Locked goal

Reduce obsolete runtime code and duplicated configuration without changing combat, encounter, collision, rendering, or UI outcomes.

## Locked acceptance

- [x] Total encounter count has one runtime configuration source.
- [x] The formal encounter HUD replaces the obsolete per-frame debug overlay.
- [x] Hidden arena collision actors no longer allocate unused visual materials.
- [x] Replaced Niagara hit effects and unused JsonUtilities dependencies are removed.
- [x] UE 5.8 Editor build and ReEcho automation pass.
- [x] Windows Shipping Cook/package remains successful.

## Implementation notes

- Keep Plane/Cube meshes that still provide visible sprites, shadows, trajectories, backdrop rendering, or collision.
- Preserve all user-authored assets and unrelated dirty worktree changes.
- Do not delete generated Windows packages; only remove transient Python cache files.

## Execution evidence

- 2026-07-29: Audited runtime references before deletion. `ReEchoTrajectoryActor`, collision/Billboard debug helpers, Paper2D sprite materials, and Plane/Cube meshes remain referenced and were retained.
- 2026-07-29: Added `TotalEncounterCount` to `ReEchoBalanceSettings`/`DefaultGame.ini` and routed GameMode, RunSubsystem, HUD, victory summary, and final-Boss tests through the validated accessor.
- 2026-07-29: Removed `AddOnScreenDebugMessage`, hidden-wall dynamic materials, Niagara plugin/module, unused JsonUtilities module, and generated `scripts/ue/__pycache__`.
- 2026-07-29: UE 5.8 ReEchoEditor Win64 Development build succeeded; all five `ReEcho.*` automation tests passed; clean Shipping Build/Cook/Pak/Archive processed 539 packages and the packaged EXE stayed alive through a 10-second smoke test.
