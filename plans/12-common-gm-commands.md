# Plan 12 — Common GM commands

## Goal

Provide a reusable development-console GM command set for quickly testing combat, economy and weather without changing Shipping gameplay.

## Acceptance

- Commands are discoverable through `GMHelp` and log their result.
- Status, healing, Time Shards, weather override and enemy-clear commands are available.
- Inputs are validated and resource/health values remain clamped.
- Commands are unavailable in Shipping builds.
- Editor build, ReEcho automation, static validation and `git diff --check` pass.

## Implementation

- Expose Unreal `UFUNCTION(Exec)` commands from `AReEchoGameMode` so the existing `~` console is the only required UI.
- Reuse current combat, run, weather and enemy lifecycle APIs.
- Let `GMKillAll` use normal damage and the normal next-tick encounter completion path.
- Document exact syntax in `docs/GM_COMMANDS.md`.

## Execution notes

- Implemented `GMHelp`, `GMStatus`, `GMHeal`, `GMAddShards`, `GMWeather` and `GMKillAll`.
- Every result is emitted on-screen and to the Unreal log with a `[GM]` prefix.
- Shipping builds reject the command body even if an exec path is invoked.
- Verification: ReEchoEditor Win64 Development build succeeded; all eight `ReEcho.*` automation tests, `scripts/validate_project.py`, and `git diff --check` passed.
- Human PIE follow-up: invoke each command through `~` and confirm console discovery plus on-screen feedback at the target DPI.