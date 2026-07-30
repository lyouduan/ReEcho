# Plan 06 — Rain and fog weather scenes

## Goal

Add rain and fog scene presentation to the runtime-generated arena without changing combat simulation or hand-editing serialized Unreal assets.

## Acceptance

- Rain renders as animated screen-space streaks over configured encounters.
- Fog renders as layered drifting translucent bands over configured encounters.
- Encounter weather is configured through `UReEchoBalanceSettings` / `DefaultGame.ini`.
- Weather does not intercept gameplay input and does not change deterministic recording or combat.
- ReEchoEditor builds, ReEcho automation passes, and `git diff --check` passes.
- Human PIE validation confirms readability and tuning.

## Implementation

- Add a lightweight full-screen `UReEchoWeatherWidget` that paints rain and fog with Slate primitives.
- Select the weather at encounter start from configured rain/fog encounter index lists.
- Keep weather visual-only and independent of arena collision, camera ownership, and encounter timing.

## Execution notes

- Added `UReEchoWeatherWidget`, a full-screen native Slate renderer with deterministic rain placement and animated fog bands.
- Added `RainEncounterIndices` and `FogEncounterIndices` to `UReEchoBalanceSettings`; defaults assign encounters 1/2/4 to rain and 3/5/6 to fog. Rain wins if an index appears in both arrays.
- `AReEchoGameMode` creates the input-transparent widget below the formal HUD and updates it after each encounter begins.
- The effect is visual-only: no collision, combat, encounter clock, recording, or playback semantics changed.
- Verification: ReEchoEditor Win64 Development build succeeded; all five `ReEcho.*` automation tests passed; `scripts/validate_project.py` and `git diff --check` passed.
- Remaining risk: automated null-RHI tests cannot judge composition. Human PIE validation must tune rain density/opacity and fog thickness against the current arena art.
- PIE feedback showed that thick anti-aliased fog lines read as horizontal white stripes. The fog renderer now uses one low-opacity full-screen vertical gradient plus four broad, softly fading gradient layers with restrained drift; the corrected Editor DLL links successfully and all five tests still pass.

### Technical implementation sequence

1. Configure encounter indices in `DefaultGame.ini`.
2. Resolve the active weather in `AReEchoGameMode::UpdateWeatherScene`.
3. Animate time in `UReEchoWeatherWidget::NativeTick`.
4. Paint rain streaks or fog bands in `NativePaint` using Slate line primitives.
5. Switch weather immediately after `RunSubsystem->BeginEncounter`, while keeping the widget `HitTestInvisible`.
