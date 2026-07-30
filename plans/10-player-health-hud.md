# Plan 10 — Player portrait and health HUD

## Goal

Add a persistent top-left player portrait and live health bar inspired by the supplied reference, while removing only the player's world-space overhead health bar.

## Acceptance

- The top-left HUD shows the configured player's portrait, current/max health text, and a live red health bar.
- The HUD is presentation-only, input-transparent, and remains visible during combat.
- Player damage and encounter reset immediately update the HUD.
- The player no longer has a world-space health bar above the sprite; enemy health bars remain unchanged.
- Editor build, affected automation, static validation and `git diff --check` pass.
- Human PIE confirms sizing, safe-area placement and readability at supported resolutions.

## Implementation

- Add a native `UReEchoPlayerHudWidget` bound to the player's combatant and configured character texture.
- Let GameMode create the HUD after run character configuration.
- Remove the player-only `AReEchoHealthBarActor` spawn and ownership from `AReEchoPlayerPawn`.

## Execution notes

- Added an input-transparent native top-left HUD with a framed current-character portrait, red health fill and live current/max numeric text.
- GameMode creates the HUD after run character configuration so the portrait matches `DefaultCharacterId` and the widget binds the live player combatant.
- Removed only the player pawn's world-space `AReEchoHealthBarActor`; enemy spawn paths and enemy health bars are unchanged.
- ReEchoEditor Win64 Development builds successfully; all six `ReEcho.*` automation tests pass.
- Remaining risk: human PIE must confirm safe-area placement, portrait crop, Chinese glyphs and DPI scaling.
- Follow-up HUD polish: removed the opaque shared panel, placed the circular portrait medallion over the left edge of the standalone health bar, increased health-number prominence, and tightened spacing to match the supplied reference.
