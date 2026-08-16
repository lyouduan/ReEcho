# ReEcho GM commands

GM commands use Unreal Engine's development console. Press `~` in PIE or a Development build, then enter a command. They are intentionally disabled in Shipping builds.

| Command | Effect |
|---|---|
| `GMHelp` | Lists available commands. |
| `GMStatus` | Shows encounter, player HP, Time Shards and echo count. |
| `GMHeal` | Fully heals the living player. |
| `GMHeal 25` | Restores 25 HP, clamped to maximum HP. |
| `GMAddShards 100` | Adds 100 Time Shards; negative values subtract without going below zero. |
| `GMWeather Clear` | Disables weather. `Rain` and `Fog` are also accepted. |
| `GMKillAll` | Defeats all current enemies and lets the normal encounter-completion flow run. |
| `GMGotoBoss` | Abandons the active encounter and immediately starts the configured final Boss encounter. |

All commands report to both the screen and Unreal log with a `[GM]` prefix. Invalid weather values print usage instead of changing state.