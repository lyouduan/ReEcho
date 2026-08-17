# ReEcho audio source and provenance inventory

This inventory records source ownership and reproducibility. Runtime bindings live in
`Design/Data/ReEchoAudioEvents.xlsx`; this file does not replace the catalog.

## User-provided long audio (Plan46)

The current programmer user supplied these files on 2026-08-17 and explicitly requested
their integration into ReEcho, a local packaged planning build, and the final repository
candidate. On 2026-08-17 the user explicitly instructed the project not to track the
originating generation platform. This inventory therefore makes no unsupported platform,
model, account, or third-party license claim; it records the user delivery and instruction.

| EventId | Project source | User-provided title | SHA-256 |
|---|---|---|---|
| `Music.Menu` | `Design/Audio/Source/Music/Music_Menu.mp3` | `The_Keeper_of_Slow_Hours.mp3` | `8e94af6822420d0c4f029acfa4bfe4e7354d0047185be54b34a7c7fadf6926f4` |
| `Music.Shop` | `Design/Audio/Source/Music/Music_Shop.mp3` | `The_Watchmaker_s_Garden.mp3` | `6aa063c1ea59334c28653ff1c3d55fbf7a4a02e43fa0aa7c1b4fb4ed7e1655a3` |
| `Music.Boss` | `Design/Audio/Source/Music/Music_Boss.mp3` | `Weight_of_the_Hour.mp3` | `4489fb0de715d7b8783806cedaede0120ebb17a174a8d2a0ece06373740a5544` |
| `Music.Death` | `Design/Audio/Source/Music/Music_Death.mp3` | `The_Last_Pendulum_Swing.mp3` | `7d76ece28e278459ca92200b0e947611156e5985d2d05fd09e0b78d598aa65c4` |
| `Music.Victory` | `Design/Audio/Source/Music/Music_Victory.mp3` | `Golden_Hour_Ascent.mp3` | `b12648e82b09ca875aeb7a65e614dc2c5b216b5c38109cdc734d7513a1bacb28` |
| `Ambience.Arena` | `Design/Audio/Source/Ambience/Ambience_Arena.mp3` | `Weight_of_the_Rift.mp3` | `d1574df12554ac5d32bc6266939870ba4a19c59e86754bbfb81c3b48cc1d4c1d` |
| `Ambience.Rain` | `Design/Audio/Source/Ambience/Ambience_Rain.mp3` | `Under_The_Stone_Vault.mp3` | `7dcd36d621bf2d5ae546ba0ec9145dcda650fc5ac36636199309e16eee0efdef` |

## Existing encounter music

| EventId | Source | SHA-256 | Provenance state |
|---|---|---|---|
| `Music.Encounter` | `Content/ReEcho/Audio/Music/The_Iron_Waltz.wav` | `018794a6f28843a898f50b09fd54c0d8ee7bbd81b5c3ec0fa7bb9794ee2d1206` | Existing user-delivered Plan34 source; originating platform is intentionally not tracked per the user's 2026-08-17 instruction, and no unsupported third-party license claim is made. |

## Deterministic baseline one-shots

- Events: all 23 current `OneShot` rows (UI, Combat, Enemy, Boss, Echo, `CameraMove`, `Revive`).
- Generator: `scripts/audio/generate_event_sfx.py`.
- Output: `Design/Audio/Generated/**/*.wav`.
- Signal contract: 48 kHz, mono, PCM16, fixed recipes and per-EventId deterministic seeds.
- License/origin: generated mathematically by repository code; no recording or third-party media is sampled.
- Intended use: replaceable technical/playable baseline. User listening approval is required before Plan46 closes.
- Reproduction: run `python scripts/audio/generate_event_sfx.py`; verify with `--check`.
