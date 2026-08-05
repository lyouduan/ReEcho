# Plan 16: Bomber trigger and explosion ranges

## Goal

Make bomber fuses start only after the player enters a configured trigger radius, and apply explosion damage only inside a smaller configured damage radius.

## Acceptance

- Bombers no longer start their fuse at spawn or damage the player from arbitrary distance.
- Contact range does not bypass the bomber fuse.
- Trigger radius, damage radius, fuse duration, and damage are project settings.
- Pure range rules have automation coverage.

## Execution notes

- Added deterministic bomber range helpers and config values: 260 trigger radius, 180 damage radius, 1.2 second fuse, 22 damage.
- The fuse is inactive at spawn, starts once on entering trigger range, and continues while the player can escape.
- Bomber contact no longer bypasses the fuse; detonation always destroys the bomber but damages only targets inside the damage radius.
- UE 5.8 Editor build passed. All 14 `ReEcho.*` automation tests passed, including `ReEcho.Enemies.BomberRanges`.
- Integration verification on 2026-08-05: ReEchoEditor built successfully; all 15 `ReEcho.*` tests, static validation, and `git diff --check` passed.
- Human PIE check remains for trigger readability and final range/fuse tuning.
