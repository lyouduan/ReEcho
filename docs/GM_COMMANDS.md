# ReEcho GM commands

GM commands use Unreal Engine's development console. Press `~` in PIE or a Development build, then enter a command. Opening the console pauses active gameplay; closing it after submitting or cancelling resumes gameplay only when the console caused that pause. If the game was already paused, closing the console preserves the existing pause. GM commands are intentionally disabled in Shipping builds.

| Command | Effect |
|---|---|
| `GMHelp` | Lists available commands. |
| `GMStatus` | Shows encounter, player HP, Time Shards and echo count. |
| `GMHeal` | Fully heals the living player. |
| `GMHeal 25` | Restores 25 HP, clamped to maximum HP. |
| `GMGod On` | Enables final-damage immunity for the current player. `Off` disables it and `Toggle` switches it. |
| `GMAddShards 100` | Adds 100 Time Shards; negative values subtract without going below zero. |
| `GMSetShards 50` | Sets Time Shards to exactly 50 (clamped to >= 0). Handy to reproduce price-boundary shop bugs. |
| `GMWeather Clear` | Disables weather. `Rain` and `Fog` are also accepted. |
| `GMEndEncounter` | Immediately ends the current non-Boss countdown and runs the normal encounter-completion flow. |
| `GMKillAll` | Defeats all current enemies and lets the normal encounter-completion flow run. |
| `GMSpawnFox` | Spawns one production `M_FOX` enemy about 350 cm from the living player. |
| `GMSpawnFox 500` | Spawns the fox 500 cm toward the arena center; accepted distance is clamped to 150–1000 cm. |
| `GMGotoBoss` | Abandons the active encounter and immediately starts the configured final Boss encounter. |
| `GMGrantCard G_2_17` | Grants the named card directly to the current build, ignoring phase/offer restrictions (debug only). Useful to reproduce card effects such as 静默刻度. No-arg prints usage. |
| `GMElement Flame` | Locks every subsequent player hit to Flame. Also accepts `Lightning`, `Grass`, and `Water`; `None` restores weapon-authored elements. |
| `GMReaction Burn 10` | Prepares and triggers the named authored reaction through the production resolver on the nearest living enemy. |

All commands report to both the screen and Unreal log with a `[GM]` prefix. Invalid weather values print usage instead of changing state.

Reaction combinations:

| Incoming GM element | Existing attachment | Reaction |
|---|---|---|
| `Flame` | `Grass` | `Burn` |
| `Flame` | `Water` | `Vaporize` |
| `Lightning` | `Grass` | `Growth` |
| `Lightning` | `Water` | `Conduct` |
| `Grass` | `Water` | `EnhanceGrass` |
| `Water` | `Grass` | `EnhanceWater` |
