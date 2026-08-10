# Plan 18: Initial loadout selection and run-locked weapon

## Goal

Before encounter 1 of a new run, require the player to select one character and one weapon. The selected weapon remains fixed for the full run.

## Runtime contract

- New game opens the loadout panel after the start menu and before encounter 1 begins.
- The panel offers Heart, Spade, Clover, and Diamond with their existing character portraits, plus the three runtime-supported weapons with existing weapon/effect artwork. Cat remains available to legacy saves but is not offered for a new run.
- Confirmation is disabled until both choices are explicit.
- Confirming writes the character and weapon into `CurrentBuild`, saves the planning checkpoint, and then starts encounter 1.
- Continue restores the saved character and weapon without reopening the loadout panel.
- No gameplay input, GAS ability, run-subsystem setter, recording event, or echo playback event may change weapons during a run.
- Echoes use the weapon stored in their recording build snapshot.

## Verification

- Format all changed C++ files.
- Build the Editor target with the editor closed.
- Run all `ReEcho.*` automation, including locked-loadout recording coverage.
- Run static validation and `git diff --check`.
- Human PIE: verify new game blocks on both choices, encounter 1 uses the selection, 1/2/3 no longer switch weapons, and continue restores the same loadout.

## Execution notes

- Added a mergeable C++ loadout panel between new-game selection and encounter 1.
- Character selection shows the existing four non-Cat character textures; Cat was removed from the new-run choices.
- Weapon selection shows the existing Moon Staff, Crescent Weapon, and Staff Light Wave artwork in aspect-preserving cards.
- Removed weapon switching input mappings, GAS abilities/tags, mutable run-state setter, weapon-change recording timeline, and echo replay switching.
- Preserved `ConfigureLockedWeapon` for applying the saved choice when the player or an echo is spawned.
- Static validation and `git diff --check` pass; removed-switch symbol scan is clean.
- Development Editor build passes, including the C++ loadout widget and its image cards.
- All 16 `ReEcho.*` automation tests pass, including locked-loadout recording and save restoration coverage.
- The latest combined regression after the post-draw shop work passes all 17 `ReEcho.*` tests.
- Human PIE verification of card layout, texture readability, and input flow remains pending.
