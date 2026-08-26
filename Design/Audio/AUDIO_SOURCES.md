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

## User-provided formal replacements (Plan114)

The current programmer user supplied `C:\Users\gavynqiu\Documents\miniGame\音频` on
2026-08-26 and requested that the mapped files replace the technical placeholders.
`ECHO音频配置说明.xlsx` is the mapping handoff; the runtime authority remains
`Design/Data/ReEchoAudioEvents.xlsx`. The originating platform and license/attribution
facts were not provided, so this inventory records user delivery and hashes without
claiming third-party release rights.

| EventId(s) | Project source | User-provided file | SHA-256 |
|---|---|---|---|
| `Music.Menu` | `Design/Audio/Source/Music/Music_Menu.mp3` | `MainMenu_v1.mp3` | `dd598665d3ae5624dea06956974d3ca844abf9c860c0cb0edd5ad37bd393d880` |
| `Music.Shop` | `Design/Audio/Source/Music/Music_Shop.mp3` | `STAGE3_v3.mp3` | `9bd5830a0a2166c592223d7f355e04df22df1cefa81fa8c041a0c19a26f3f4a0` |
| `Music.Boss` | `Design/Audio/Source/Music/Music_Boss.mp3` | `STAGE3_v2.mp3` | `8e13193b3775008079c507bd9b147f5769a5ded7b01f811de4baaad8457d9fc3` |
| `UI.Hover` | `Design/Audio/Source/Formal/UI/UI_Hover.mp3` | `触发带叮响机关开关音效 01.mp3` | `67d37f70b23249af1dcfed8156cb53e35660c4689b9c51b56e92ded846f22333` |
| `UI.Confirm` | `Design/Audio/Source/Formal/UI/UI_Confirm.mp3` | `cilck.mp3` | `64d9331804512e308f52ecbac75e0124b79b6ed46144ec740d9b6129a7880156` |
| `UI.Cancel`, `UI.Error`, `Revive` | `Design/Audio/Source/Formal/UI/UI_Cancel.wav` | `取消.wav` | `c5a7cd3ed51c37861898155fdb024700d92ee2e8a188713dd65692c47d36821b` |
| `UI.Purchase` | `Design/Audio/Source/Formal/UI/UI_Purchase.wav` | `购买.wav` | `c252826d1a1fc3eb45ef2a3586dc7ea155c013af2a8df549eb0d99e1c4fb529d` |
| `UI.CardSelect` | `Design/Audio/Source/Formal/UI/UI_CardSelect.wav` | `选卡.wav` | `6be0d30f185abd5df1aed287cdfb1f293b5b3f6030911e7f2f0a35c6b15bb5c5` |
| `Combat.Hurt` | `Design/Audio/Source/Formal/Combat/Combat_Hurt.wav` | `A_short_impact_hit,__#4-1787644382401.wav` | `4cd439549c4ed1f258dcd9fffb5edf9f7618eb1b276227557d3e401a1d9f62ee` |
| `Combat.Death` | `Design/Audio/Source/Formal/Combat/Combat_Death.wav` | `失败.wav` | `b9ea39cae4c135c9c522d7bb06fd584dd315e8a0489bc51f3407c3cc6e733b5e` |
| `Enemy.Death` | `Design/Audio/Source/Formal/Enemy/Enemy_Death.mp3` | `15通用受击音效.mp3` | `d69fe3c797559854561325f0a4625def8f22042e19d4e2e34f4a0eb910c607ca` |
| `Enemy.Spawn` (source) | `Design/Audio/Source/Formal/Enemy/Enemy_Spawn_Source.wav` | `用后半段，怪物出生.wav` | `c6fd22f3536f66a852de21ae620a0922f8872e8c24a49d292c123eaa987d9703` |
| `Boss.Death` | `Design/Audio/Source/Formal/Boss/Boss_Death.wav` | `胜利.wav` | `6dc0c505aa964aedcf4e29138fb577f3fccad060e94b58ed641f8a218f8db237` |
| `CameraMove` | `Design/Audio/Source/Formal/Flow/CameraMove.wav` | `时钟转动.wav` | `902e15545e4afe4258009e7c6848941fc0181c9c93aa438f152672024d72a236` |

`Enemy.Spawn` imports `Design/Audio/Derived/Enemy/Enemy_Spawn.wav`, generated by
`python scripts/audio/prepare_formal_audio.py`. The transformation takes the exact
second half of the two-second source and averages stereo PCM16 pairs to the mono format
required by the existing 3D catalog contract; `--check` verifies byte identity.

## User-provided formal variants (Plan114)

| EventId / VariantId | Project source | User-provided file | SHA-256 |
|---|---|---|---|
| `Combat.Attack / W_J_01` | `Design/Audio/Source/Formal/Variants/CombatAttack/W_J_01.mp3` | `剑快速地劈砍两下-YS070510.mp3` | `1df71196d411fa11b5c1a7269488059be967fe5070d1c2b6ed3dd4155a805bbc` |
| `Combat.Attack / W_J_04` | `Design/Audio/Source/Formal/Variants/CombatAttack/W_J_04.wav` | `镰刀.wav` | `1a58ea16575dccabab8acb393383ee725386472793fbfe9befd13ab50ebcb2e1` |
| `Combat.Attack / W_J_08` | `Design/Audio/Source/Formal/Variants/CombatAttack/W_J_08.mp3` | `弩.mp3` | `cfdfc405152eadbc3f31f51f900b555ada70b93a02308a60295429245d534506` |
| `Combat.Attack / W_J_09` | `Design/Audio/Source/Formal/Variants/CombatAttack/W_J_09.wav` | `枪.wav` | `3630dca5877c81653ef5a5049bb4f98f51ed2cb26cb1411e85908375de1d3bb5` |
| `Combat.Hit / Flame` | `Design/Audio/Source/Formal/Variants/CombatHit/Flame.mp3` | `火元素攻击A.mp3` | `b9c5d877622bd70cb77e55ebe3d3f8aac583f885cc7ea6ceb505d3729cecb8a9` |
| `Combat.Hit / Lightning` | `Design/Audio/Source/Formal/Variants/CombatHit/Lightning.wav` | `电.wav` | `1cc5eab6416b0a7675527d0d00c93f765b6849bf219e3c768f6c11d545ad18df` |
| `Combat.Hit / Grass` | `Design/Audio/Source/Formal/Variants/CombatHit/Grass.wav` | `草元素.wav` | `741dff2f7f42bccf5d35c73ec0f361c1480dbc2ae98ee7a34f1fe94f9514cf47` |
| `Combat.Hit / Water` | `Design/Audio/Source/Formal/Variants/CombatHit/Water.wav` | `草元素 (2).wav` | `caa2dc1adc4dca8de23955a690c4e6616c587f2b5711bf390d38a095662778be` |
| `Music.Encounter / Stage.1` | `Design/Audio/Source/Formal/Variants/MusicEncounter/Stage_1.mp3` | `STAGE1_v2.mp3` | `e44508de4be1bfc786ed597afdb00dd510d5067b979ae07d1c62dbec4e5a70e4` |
| `Music.Encounter / Stage.2` | `Design/Audio/Source/Formal/Variants/MusicEncounter/Stage_2.mp3` | `STAGE2_v1.mp3` | `65930d4240bc8ecec8a92609b81fddd26f595c64702fcf158b0e39c5b5c5d3bb` |
| `Music.Encounter / Stage.3` | `Design/Audio/Source/Formal/Variants/MusicEncounter/Stage_3.mp3` | `STAGE3_v1.mp3` | `343d3efedd5c4547b71f6347028d3c8375d55e8aa920f4454e2c7e2e121da4c7` |

MP3 one-shots are decoded by UE 5.8 through `scripts/audio/export_formal_audio_wav.py`.
`scripts/audio/prepare_formal_audio.py` then averages every spatial stereo source to tracked
mono PCM16 WAVs under `Design/Audio/Derived/**`; runtime SoundWaves import only those derived
files. `validate_formal_audio_sources.py` verifies all 25 user-delivered source hashes, while
`prepare_formal_audio.py --check` verifies each derived file byte-for-byte.
