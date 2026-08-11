# Plan 18: Initial loadout selection

## Goal

Before encounter 1 of a new run, require the player to select one character and one initial weapon.

## Runtime contract

- New game opens the loadout panel after the start menu and before encounter 1 begins.
- The panel reads enabled selectable characters from the typed CSV snapshot and current supported weapons from the runtime weapon registry. Cat remains available to legacy saves but is not offered for a new run.
- Confirmation is disabled until both choices are explicit.
- Confirming writes the character and weapon into `CurrentBuild`, saves the planning checkpoint, and then starts encounter 1.
- Continue restores the saved character and weapon without reopening the loadout panel.
- The selected weapon is immutable for the full run. Continue and echo initialization restore it from the saved build snapshot.
- Continue restores the weapon active at the saved checkpoint.

## Verification

- Format all changed C++ files.
- Build the Editor target with the editor closed.
- Run all `ReEcho.*` automation, including loadout, run-locked recording and save restoration coverage.
- Run static validation and `git diff --check`.
- Human PIE: verify new game blocks on both choices, encounter 1 uses the selection, 1/2/3 do not switch weapons, and continue restores the saved weapon.

## Execution notes

- Added a mergeable C++ loadout panel between new-game selection and encounter 1.
- Character choices now come from the shared typed character snapshot; the runtime-only Cat row is excluded by table metadata.
- Weapon choices currently come from the existing runtime weapon registry and retain their presentation mapping until Plan 24 migrates weapon presentation to CSV.
- A later direct user decision supersedes the earlier integration choice: remove the complete runtime-switch chain while retaining pre-run selection, save restoration and echo initialization.
- Missing character or weapon configuration fails loudly at run start or save restoration instead of silently falling back.
- Static validation and `git diff --check` must be rerun after integration.
- Development Editor build passes, including the C++ loadout widget and its image cards.
- The remote pre-integration branch passed its original loadout and save coverage; the combined local baseline is reverified after conflict resolution.
- Human PIE verification of card layout, texture readability, and input flow remains pending.
