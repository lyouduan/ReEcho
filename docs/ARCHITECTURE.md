# ReEcho runtime architecture

This is the short human overview. [`shared/CODEBASE_MAP.md`](../shared/CODEBASE_MAP.md) is the AI-oriented retrieval map; source and tests remain the runtime truth.

ReEcho is a single Unreal Engine 5.8 C++ runtime module. `/Game/Level00` hosts a runtime-generated bounded 2.5D arena and a six-encounter run.

## Main flow

1. `AReEchoGameMode` presents the save-aware start menu. New runs choose one configured character and one of three start-selectable weapons; continue restores a compatible suspended run.
2. The selected stable `WeaponId` is locked for the full run. Legacy CSV `InputSlot` values remain compatibility metadata; hotkeys 1/2/3 do not switch weapons during a run.
3. `AReEchoGameMode` creates the arena, `AReEchoEncounterDirector`, player, enemy composition and the prior recording's echo. The encounter advances on a pause-aware 60 Hz fixed step for up to 30 seconds.

## Runtime UI framework

Runtime UI remains inside the `ReEcho` module but has an isolated `UI/Framework` layer. `EReEchoUIScreen` gives every screen a stable identity. `UReEchoUIManagerSubsystem` owns the designer WBP class registry, active screen instances and viewport layers. `UReEchoUIFlowCoordinatorSubsystem` owns screen creation/closure, focus, input modes, world pause and pause-safe screen replacement. `AReEchoGameMode` supplies gameplay snapshots and handles stable-ID delegates, but does not own asset paths or direct Widget viewport lifecycle.
4. `UReEchoRecorderComponent` samples player position at 20 Hz and records successful active-skill events. Automatic attacks are not serialized.
5. `UReEchoPlaybackComponent` interpolates historical movement for `AReEchoEchoActor`. Player and echo use the run-pinned data snapshot and the run-locked weapon definition; hit resolution targets the current world.
6. GAS owns combat attributes, effects, damage, healing, ability activation and cooldowns. CSV weapon attack steps, parts and element reactions feed the runtime definitions consumed by player and echo attacks.
7. Clearing all enemies or reaching the time limit finalizes the recording through `UReEchoRunSubsystem`. Trait selection opens the Time Shard shop before the next encounter; the final Boss gate settles the run.
8. Confirmed in-encounter exit captures the clock, player, active recording and living enemies before quitting. A failed save never exits.

## Data flow

```text
Design/Data/ReEchoData.xlsx
        -> scripts/data/sync_xlsx_to_csv.py
        -> 16 validated UTF-8 production CSV files in Content/Data
        -> typed CSV readers and FReEchoCsvDataRegistry
        -> immutable run-pinned runtime snapshot
```

- The XLSX workbook is the canonical designer source for migrated character/build, element/status/reaction and weapon/slot domains.
- Generated CSV is the diffable packaged runtime source. Unreal does not parse XLSX or require Excel/Office at runtime.
- Stable behavior/formula/effect/attack-pattern identifiers resolve to registered C++ handlers; spreadsheet prose is never executed as logic.
- Legacy JSON is migration-only for migrated domains. Enemy, economy and remaining global-balance values stay on their current sources until a later domain migration moves them atomically.

## Source boundaries

- `Core`: shared runtime structs, enums and compatibility types.
- `Data`: CSV readers, validation, immutable registry and domain snapshots.
- `AbilitySystem`: GAS attributes, effects, tags and active abilities.
- `Encounter`: deterministic encounter clock and completion signals.
- `Run`: run phase, save/restore, build snapshot, recording history and pinned data revision.
- `Recording`: position/active-skill capture and echo playback interpolation.
- `Player`: movement, input, run initialization and combat ownership.
- `Weapons`: concrete weapon execution, ordered attack steps, equipment derivation and registered patterns.
- `Combat`: shared health/block/damage adapter and element-reaction runtime.
- `Graybox`: runtime actors for enemies, echo, projectiles, hit feedback and arena-adjacent presentation.
- `UI`: start/continue/loadout, settings, pause/save-and-quit, HUD, trait draw, shop/inventory and stats widgets.
- `Tests`: deterministic data, run, recording, GAS, shop, character, reaction and weapon automation.

Serialized `.uasset`/`.umap` content supplies the host map, materials and packaged visuals; gameplay authority remains in C++, validated data and explicit Unreal configuration.
