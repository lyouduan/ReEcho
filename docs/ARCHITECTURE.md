# ReEcho runtime architecture

The AI-oriented routing authority is [`shared/CODEBASE_MAP.md`](../shared/CODEBASE_MAP.md). Use this document as the shorter human overview.

ReEcho is a single UE 5.8 C++ runtime module. It uses the engine template map as a host and generates its graybox arena at runtime from Basic Shapes.

## Main flow

1. `AReEchoGameMode` creates the arena, encounter director, player, enemies and prior-run echo.
2. `AReEchoEncounterDirector` advances a pause-aware 60 Hz fixed step for up to 30 seconds.
3. `UReEchoRecorderComponent` records player positions at 20 Hz and successful active-skill events.
4. `UReEchoPlaybackComponent` interpolates a historical recording for `AReEchoEchoActor`.
5. Player and echo attacks spawn visible sphere projectiles. Niagara is reserved for damage impacts.
6. `UReEchoCombatantComponent` owns shared HP/block behavior; world-space UI widgets display health.
7. Clearing all enemies or reaching the time limit finalizes the recording through `UReEchoRunSubsystem` and advances the six-encounter run.

## Source boundaries

- `Core`: shared enums, structs and balance settings.
- `Encounter`: deterministic encounter clock.
- `Recording`: recording and playback.
- `Run`: run phase/history/anchor.
- `Player`: input, movement and manual attacks.
- `Combat`: shared health, block and damage.
- `Graybox`: runtime player-adjacent actors, enemies, echo, projectiles, hit feedback and world health-bar host.
- `UI`: C++ health-bar widget.
- `Tests`: deterministic recording regression.

`Content/Data/*.json` is design source only and has not yet been imported into runtime UE assets. The only current project gameplay asset is the translucent echo material under `Content/ReEcho/Materials/`.
