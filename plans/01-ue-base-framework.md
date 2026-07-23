# Plan 01 - UE base framework

## Goal

Turn the empty repository into a UE C++ project whose stable interfaces match the Time Echo v2 specification and can support milestone A without rewriting recording or run state.

## Acceptance

- 60 Hz pause-aware encounter clock with a strict configurable 30-second end.
- 20 Hz position recording, skill-event recording, interpolated playback, and final-position hold.
- Build snapshot, full StatBlock, element state, three-item history, anchor, and six-encounter phase model.
- WASD/Q/Space pawn seam, health/block component, externalized baseline data, and card registry.
- Project descriptors and JSON parse cleanly; UE compile is run when a local engine is available.

## Execution notes

- Implemented source-first because the repository contained no UE project or binary assets.
- Kept loose JSON as reviewable source data; a later Editor Utility should import it into cooked assets.
- No Unreal Engine installation was found in the standard Epic Games location, so this pass cannot claim UHT/UBT validation.
