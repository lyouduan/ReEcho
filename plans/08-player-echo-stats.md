# Plan 08 — Player and echo stats overlay

## Goal

Add a Tab-opened full-screen panel showing the current player and active echo attributes over a blurred version of the supplied clockwork illustration.

## Acceptance

- Tab opens and closes the stats panel, including while the game is paused.
- The panel pauses combat, releases the mouse, and Escape closes it before opening the pause menu.
- Player values come from the current combatant/build state.
- Echo values come from the currently spawned echo after its efficiency multiplier; the panel clearly handles runs without an echo.
- The supplied image is retained as reviewable source, with a blurred/darkened project asset used as the cooked background.
- Editor build, ReEcho automation, static validation, and `git diff --check` pass.
- Human PIE validation confirms legibility, DPI scaling, and the intended background blur.

## Implementation

- Import the image-edited background as `/Game/ReEcho/Textures/UI/StatsBackground`.
- Add a native `UReEchoStatsWidget` with two attribute columns.
- Expose read-only current stats from `AReEchoEchoActor`.
- Let GameMode own pause/menu lifecycle and supply live player/echo values.
- Bind Tab through `AReEchoPlayerPawn` and `DefaultInput.ini`.

## Execution notes

- Used the built-in ImageGen edit flow to preserve the supplied composition while applying global soft focus, darkening, and reduced saturation. Original and edited sources are retained under `Content/SourceArt/UI/`.
- Imported the edited image as cooked `/Game/ReEcho/Textures/UI/StatsBackground` through `scripts/ue/import_stats_background.py`.
- Added a native two-column stats widget, Tab paused-menu input, Escape-first-close behavior, and GameMode-owned lifecycle.
- Player values read the live player combatant; echo values read the active echo combatant after its efficiency multiplier, with an explicit no-echo state.
- ReEchoEditor Win64 Development links successfully; all six ReEcho automation tests, `scripts/validate_project.py`, and `git diff --check` pass.
- Remaining risk: human PIE validation is required for text alignment, Chinese glyph coverage, DPI scaling, and subjective blur/readability balance.
