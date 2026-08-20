# ReEcho GM commands

GM commands use Unreal Engine's development console. Press `~` in PIE or a Development build, then enter a command. They are intentionally disabled in Shipping builds.

| Command | Effect |
|---|---|
| `GMHelp` | Lists available commands. |
| `GMStatus` | Shows encounter, player HP, Time Shards and echo count. |
| `GMHeal` | Fully heals the living player. |
| `GMHeal 25` | Restores 25 HP, clamped to maximum HP. |
| `GMAddShards 100` | Adds 100 Time Shards; negative values subtract without going below zero. |
| `GMSetShards 50` | Sets Time Shards to exactly 50 (clamped to >= 0). Handy to reproduce price-boundary shop bugs. |
| `GMWeather Clear` | Disables weather. `Rain` and `Fog` are also accepted. |
| `GMEndEncounter` | Immediately ends the current non-Boss countdown and runs the normal encounter-completion flow. Resume gameplay before using it. |
| `GMKillAll` | Defeats all current enemies and lets the normal encounter-completion flow run. |
| `GMSpawnFox` | Spawns one production `M_FOX` enemy about 350 cm from the living player. |
| `GMSpawnFox 500` | Spawns the fox 500 cm toward the arena center; accepted distance is clamped to 150–1000 cm. |
| `GMGotoBoss` | Abandons the active encounter and immediately starts the configured final Boss encounter. |
| `GMGrantCard G_2_17` | Grants the named card directly to the current build, ignoring phase/offer restrictions (debug only). Useful to reproduce card effects such as 静默刻度. No-arg prints usage. |

All commands report to both the screen and Unreal log with a `[GM]` prefix. Invalid weather values print usage instead of changing state.
