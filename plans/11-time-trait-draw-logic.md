# Plan 11 — Time trait draw logic

## Goal

Make the three-choice time-trait draw deterministic, diverse, safe against duplicate application, and independent of runtime JSON file I/O.

## Acceptance

- Every draw contains unique card IDs and prefers traits with the lowest owned stack count.
- The same run state produces the same offers; advancing the build changes the deterministic seed.
- Only a card from the current pending offer can be applied, and one offer cannot be applied twice.
- Trait effects and player-facing descriptions remain correct for the six currently supported runtime traits.
- The runtime catalog does not read `Content/Data/cards.json` during a draw or selection.
- Automated tests cover deterministic offers, diversity, pending-offer validation and application.
- Editor build, ReEcho automation, static validation and `git diff --check` pass.

## Execution notes

- Replaced per-draw/per-apply JSON parsing with a six-entry runtime trait catalog and localized player-facing names/descriptions.
- Offers use a run-state-derived deterministic seed, contain unique IDs, and fill from the lowest owned stack count before allowing repeats.
- The subsystem records pending offer IDs; selection rejects cards outside that draw and rejects repeated application after the first success.
- Added `ReEcho.Traits.OffersAreDeterministicAndDiverse` and `ReEcho.Traits.AppliesOnlyPendingOffer` automation coverage.
- ReEchoEditor Win64 Development builds successfully; all eight `ReEcho.*` tests, static validation and `git diff --check` pass.
- The supplied MP4 could not be decoded because the environment has no media decoder and browser startup failed with Windows error 1385; visual draw-animation comparison remains a human check.