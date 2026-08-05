# Plan 14 — Weapon 3 elemental reactions

## Goal

Replace weapon slot 3's fixed elemental projectile behavior with a deterministic three-element attachment/reaction loop using Water, Flame and Grass.

## Acceptance

- Weapon 3 projectiles visibly use Water, Flame and Grass.
- Water + Grass and Grass + Flame react in either order and deal exactly 2x base damage.
- Water + Flame, identical elements and invalid elements do not react.
- A reaction consumes the current attachment; a normal elemental hit attaches or replaces the element.
- Player and echo share the same weapon implementation.
- ReEchoEditor builds, ReEcho automation passes, static validation and `git diff --check` pass.
- Human PIE confirms projectile colors, reaction pacing and damage-number readability.

## Implementation

- Add a pure `ReEchoElementReaction` rules module around the existing `EReEchoElement` and `FReEchoElementState` types.
- Rotate weapon 3 through Water -> Grass -> Flame -> Grass so both supported reactions occur without adding another input mode.
- Carry the element on `AReEchoProjectileActor` and resolve attachment/reaction on `AReEchoEnemyActor` before GAS damage application.
- Use blue/red/green projectiles and damage numbers for normal elemental hits; use gold damage numbers for reactions.
- Keep fixed 2x reaction damage independent of other build coefficients for this initial version.

## Execution notes

- Runtime design JSON remains design-only; reaction rules are compiled and covered by automation.
- Unsupported pairs replace the attachment deterministically instead of stacking multiple elements.
- Enemy Configure and weapon Initialize reset reaction sequence/state for deterministic encounters.
- Editor build passed after the new source/test files and feedback interface were added.
- Final verification: ReEchoEditor Win64 Development build passed; all eleven ReEcho.* automation tests passed, including ReEcho.Combat.ElementReactions; static validation and git diff --check passed.
- Readability follow-up: black lit spheres were replaced by unlit colored W/F/G glyphs, and weapon 3 now shows a camera-facing NEXT: <ELEMENT> indicator. The final editor link and all eleven automation tests passed after this change.
- Added persistent enemy attachment visualization: an unlit pulsing element-colored ring, camera-facing element label, and matching point-light aura. Reaction consumption and enemy death clear all three layers. Final Editor link and all eleven automation tests passed after this addition.
- Integration verification on 2026-08-05: ReEchoEditor built successfully; all 15 `ReEcho.*` tests, static validation, and `git diff --check` passed.
