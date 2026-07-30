# ReEcho codebase map

Last verified: 2026-07-23. This file is the shortest authoritative routing index for code retrieval. It describes where behavior lives; `PROJECT_RULES.md` remains the authority for constraints and `PROJECT_STATE.md` for delivery status.

## Minimal retrieval protocol

1. Read `AGENTS.md`, then this file.
2. Read only the row matching the requested task in **Task routing**.
3. For C++ work, read the matching `Public/.../*.h` before its `Private/.../*.cpp`.
4. Read `shared/PROJECT_RULES.md` before editing and only the relevant `shared/LESSONS.md` section when debugging.
5. Do not search `Binaries/`, `Intermediate/`, `Saved/`, or generated files unless diagnosing build output.
6. Do not infer runtime behavior from `Content/Data/*.json`; those files are design source and are not imported at runtime yet.

Useful first commands:

```powershell
rg --files Source\ReEcho Config Content\Data docs scripts shared
rg -n "SymbolName" Source\ReEcho
```

## Repository layout

| Path | Purpose | Read when |
|---|---|---|
| `ReEcho.uproject` | UE 5.8 association and enabled plugins | Modules/plugins/editor integration |
| `Source/ReEcho/` | Single runtime module | Gameplay or runtime code |
| `Config/` | Maps, GameMode, balance and input mappings | Startup, controls or tuning |
| `Content/Data/` | Reviewable design JSON; not runtime-loaded | Cards, characters, enemies, encounters or balance source |
| `Content/ReEcho/Materials/` | Serialized project materials | Visual asset references |
| `Content/ReEcho/Textures/Characters/` | Cooked 2D actor and shadow textures | Player/echo/enemy Billboard visuals |
| `Content/SourceArt/Characters/` | Reviewable PNG sources, including MushroomGirl animation frames and Sprite Sheet | Regenerating or extending 2D actor assets |
| `scripts/ue/` | Installed-engine discovery, build, automation, reproducible Windows Shipping packaging and asset import | Compile/test/package or asset-import workflow |
| `scripts/validate_project.py` | JSON and workflow static validation | Data/workflow changes |
| `docs/` | Human-facing architecture and MCP guides | Tool integration or orientation |
| `shared/` | AI authority, state, rules, routing and coordination | Every AI task |
| `plans/` | Scoped implementation plans and execution notes | Plan-specific work |

Generated folders (`Binaries`, `Intermediate`, `Saved`, Derived Data) are outputs, not source.

## Runtime composition

`/Game/Level00` is the editor and packaged-game startup map. `AReEchoGameMode` creates the runtime arena and gameplay actors inside that world.

```text
DefaultEngine.ini
  -> /Game/Level00
  -> AReEchoGameMode::StartPlay
       -> CreateArena (bounded hidden collision arena, camera-aligned unlit backdrop, light)
       -> spawn AReEchoEncounterDirector
       -> UReEchoRunSubsystem::StartRun
       -> BeginNextEncounter
            -> reset player + begin UReEchoRecorderComponent
            -> spawn prior AReEchoEchoActor when history exists
            -> spawn enemy composition
            -> start deterministic encounter clock

Encounter fixed step
  -> record player position
  -> advance echo playback

All enemies dead or timer expires
  -> finish recording
  -> UReEchoRunSubsystem::CompleteEncounter
  -> next encounter (six total)
```

## Runtime module map

| Area | Primary types | Files | Responsibility |
|---|---|---|---|
| Orchestration | `AReEchoGameMode` | `Public/ReEchoGameMode.h`, `Private/ReEchoGameMode.cpp` | Runtime bounded arena/camera-aligned unlit backdrop, six encounters, configurable deterministic peripheral enemy spawning, cleanup, recording handoff and HUD text |
| Encounter clock | `AReEchoEncounterDirector` | `Encounter/ReEchoEncounterDirector.*` | Pause-aware 60 Hz fixed step, setup phase, timeout/end delegate |
| Billboard screen sizing | `ReEchoBillboardScreenScale` | `Private/Graybox/ReEchoBillboardScreenScale.h` | Shared camera-depth compensation for stable player/echo/enemy projected size |
| Player | `AReEchoPlayerPawn` | `Player/ReEchoPlayerPawn.*` | Map-edge-clamped follow camera with wider player arena clamp, WASD, center-aligned root capsule, mouse-following horizontal sprite facing, four configurable NewCast character textures, manual attacks and visual-only 2D motion |
| Player GAS | `UAbilitySystemComponent`, `UReEchoPlayerGameplayAbility` subclasses | `AbilitySystem/ReEchoPlayerAbilities.*`, `Player/ReEchoPlayerPawn.*` | GAS activation for basic attack, active attack, and weapon slots 1/2/3; combat stats remain in `UReEchoCombatantComponent` during compatibility migration |
| Projectile | `AReEchoProjectileActor` | `Graybox/ReEchoProjectileActor.*` | Visible sphere travel, swept path-vs-capsule hit detection, damage delivery |
| Player weapons | `AReEchoWeaponActor`, `EReEchoWeaponSlot` | `Weapons/ReEchoWeaponActor.*`, `Player/ReEchoPlayerPawn.*` | Slots 1/2/3, DeveloperSettings/DefaultEngine.ini-driven orb and sword attacks, sword mesh and swing animation |
| Sword arc VFX | `AReEchoSwordArcActor` | `Graybox/ReEchoSwordArcActor.*`, `Weapons/ReEchoWeaponActor.cpp` | Layered translucent crescent spawned for each sword swing and faded over a short lifetime |
| Enemy | `AReEchoEnemyActor` | `Graybox/ReEchoEnemyActor.*` | Grunt/shield/bomber/final-Boss stats, six rotating 2D grunt variants and a 2D final-Boss Billboard, capsule damage volume, chase/contact damage and visual-only attack/hit/death motion |
| Echo | `AReEchoEchoActor` | `Graybox/ReEchoEchoActor.*` | Historical movement playback, timestamped previous-encounter weapon timeline playback, automatic shared-weapon attacks, translucent pulsing ghost material and visual-only 2D motion |
| Echo trajectory | `AReEchoTrajectoryActor` | `Graybox/ReEchoTrajectoryActor.*` | Only the active echo's simplified historical positions projected onto the ground as one world-fixed translucent route |
| Combat | `UReEchoCombatantComponent` | `Combat/ReEchoCombatantComponent.*` | Shared stats, HP, block, damage and death delegates |
| Hit VFX | `ReEchoAttackEffects`, `AReEchoHitImpactActor` | `Graybox/ReEchoAttackEffects.*`, `Graybox/ReEchoHitImpactActor.*` | Transparent HitStarburst plane spawned only after non-zero applied damage |
| Damage numbers | `AReEchoDamageNumberActor` | `UI/ReEchoDamageNumberActor.*`, damage callers | Camera-facing floating `-N` text for actual damage applied to player/enemies |
| Recording | `UReEchoRecorderComponent` | `Recording/ReEchoRecorderComponent.*` | 20 Hz positions and successful active-skill events |
| Playback | `UReEchoPlaybackComponent` | `Recording/ReEchoPlaybackComponent.*` | Interpolated historical position and crossed skill events |
| Run state | `UReEchoRunSubsystem` | `Run/ReEchoRunSubsystem.*` | Run phase, encounter index, build, recording history and anchor |
| Shared types | `FReEcho*`, `EReEcho*` | `Core/ReEchoTypes.*` | Stats, build snapshot, recording samples/events, elements and phases |
| Balance config | `UReEchoBalanceSettings` | `Core/ReEchoBalanceSettings.h`, `Config/DefaultGame.ini` | Encounter/fixed-step/recording/global prototype values |
| World health UI | `AReEchoHealthBarActor`, `UReEchoHealthBarWidget` | `Graybox/ReEchoHealthBarActor.*`, `UI/ReEchoHealthBarWidget.*` | Camera-facing WidgetComponent and ProgressBar for player/enemies |
| Encounter HUD | `UReEchoEncounterHudWidget` | `UI/ReEchoEncounterHudWidget.*`, `ReEchoGameMode.*` | Right-top current encounter and remaining-time display; final five seconds turn red |
| Weather scenes | `UReEchoWeatherWidget` | `UI/ReEchoWeatherWidget.*`, `ReEchoGameMode.*`, `ReEchoBalanceSettings.h` | Configurable per-encounter rain streaks and layered drifting fog rendered as input-transparent screen-space presentation |
| Pause/restart UI | `UReEchoRestartWidget` | `UI/ReEchoRestartWidget.*`, `ReEchoGameMode.*` | Esc pause overlay, resume, full-level restart, packaged quit, death-screen and terminal Boss-victory actions |
| Trait choice UI | `UReEchoTraitCardChoiceWidget` | `UI/ReEchoTraitCardChoiceWidget.*`, `Run/ReEchoRunSubsystem.*` | Three unique random offers after a cleared encounter; selection mutates the current build |
| Tests | Recording/GAS/final-Boss automation | `Private/Tests/ReEchoRecordingTests.cpp` | Recording interpolation/timeline, GAS structure and final-Boss victory gating regressions |

Paths in the table are relative to `Source/ReEcho/Public` or `Source/ReEcho/Private`.

## Current gameplay contract

- Player: 2D Billboard character, WASD movement, left mouse/J basic attack, Q/Space power shot, and GAS-routed 1/2/3 weapon switching.
- Echo: 2D Billboard replay actor that follows historical movement, replays timestamped weapon changes, and attacks through the shared weapon implementation.
- Projectile flight has no Niagara. A transparent HitStarburst impact appears only when applied damage is greater than zero.
- Enemies use 2D Billboard visuals for every archetype; the old cube/plane/cone fallback rendering has been removed. Shield enemies remain implemented but are temporarily excluded from encounter composition.
- Enemy hit feedback: short stagger, source-opposed knockback and decaying lateral shake; blocked hits do not trigger it.
- Player, echo and 2D enemies animate their existing static textures with sprite-local bob, squash, lunge and recovery; enemy death adds a short shrink/fall before destruction.
- Player and enemies use compact camera-facing world health bars anchored above their 2D visual.
- Esc pauses into a resume/restart/quit menu; death and Boss victory expose restart/quit; Shipping builds call platform QuitGame.
- Clearing all living enemies ends the encounter immediately; timeout is the fallback.
- Automatic echo attacks are not serialized. Recording stores player position plus successful active-skill events.

## Config, data and assets

| Concern | Source |
|---|---|
| Default map/GameMode | `Config/DefaultEngine.ini`; runtime balance and weapons | `Config/DefaultGame.ini` |
| WASD, mouse/J and Q/Space | `Config/DefaultInput.ini` |
| MCP endpoint/editor preferences | `Config/DefaultEditorPerProjectUserSettings.ini`, `.codex/config.toml` |
| Windows override | `Config/Windows/WindowsEngine.ini` |
| Design registries | `Content/Data/*.json` |
| Echo ghost material | `Content/ReEcho/Materials/M_EchoGhost.uasset` |
| Module dependencies | `Source/ReEcho/ReEcho.Build.cs` |

Design JSON currently covers cards, characters, elements, encounters, enemies, global balance, reactions, statuses and weapons. It is validated statically but not cooked or queried by gameplay code; runtime weapon values live in ReEchoBalanceSettings and DefaultGame.ini.

## Task routing

| Task keywords | Read first | Usually also read |
|---|---|---|
| Startup, arena, rounds, spawn, clear-to-next | `ReEchoGameMode.*` | `EncounterDirector.*`, `RunSubsystem.*`, `DefaultEngine.ini` |
| Input, movement, player attack | `Player/ReEchoPlayerPawn.*` | `DefaultInput.ini`, `ReEchoProjectileActor.*` |
| 2D sprite animation, frame import, attack/hit/death feedback | `Player/ReEchoPlayerPawn.*`, `Graybox/ReEchoEnemyActor.*`, `Graybox/ReEchoEchoActor.*` | `scripts/ue/import_mushroomgirl_frames.py`, `Content/SourceArt/Characters/MushroomGirl/`, imported character textures |
| GAS/player abilities/ASC | `AbilitySystem/ReEchoPlayerAbilities.*`, `Player/ReEchoPlayerPawn.*` | `ReEcho.Build.cs`, `ReEcho.uproject`, `Weapons/ReEchoWeaponActor.*`, GAS automation test |
| Mouse cursor aiming/player facing/camera | `Player/ReEchoPlayerPawn.*` | `GameMode::RestoreGameInput`, `DefaultInput.ini` |
| Weapon switching/sword/melee | `Weapons/ReEchoWeaponActor.*` | `Player/ReEchoPlayerPawn.*`, `Content/Data/weapons.json`, `DefaultInput.ini`, `RunSubsystem::SetEquippedWeapon` |
| Bullet speed/size/color/hit | `Graybox/ReEchoProjectileActor.*` | Player/Echo caller, `EnemyActor::ReceiveGrayboxDamage` |
| Enemy AI, type, shield, bomber, boss | `Graybox/ReEchoEnemyActor.*` | `CombatantComponent.*`, hit effects |
| Damage, HP, block, death | `Combat/ReEchoCombatantComponent.*` | Damage caller and health UI |
| Hit VFX or hit feel | `Graybox/ReEchoAttackEffects.*`, `Graybox/ReEchoEnemyActor.*` | Niagara plugin/material paths |
| Floating damage text | `UI/ReEchoDamageNumberActor.*` | every `ApplyFinalDamage` caller, currently `Graybox/ReEchoEnemyActor.cpp` |
| Echo appearance/attack/playback/weapon timeline | `Graybox/ReEchoEchoActor.*`, `Recording/ReEchoRecorderComponent.*`, `Recording/ReEchoPlaybackComponent.*` | `Recording.WeaponChanges`, `Weapons/ReEchoWeaponActor.*`, player weapon-change event, `M_EchoGhost.uasset` |
| Echo route/trajectory/trail | `Graybox/ReEchoTrajectoryActor.*` | `Core/ReEchoTypes.h`, `Graybox/ReEchoEchoActor.*`, `M_EchoGhost.uasset` |
| Recording determinism/interpolation | `Core/ReEchoTypes.*`, `Recording/*` | `EncounterDirector.*`, recording test |
| Run history, phase, anchor, shops | `Run/ReEchoRunSubsystem.*` | `Core/ReEchoTypes.*`, GameMode |
| Health bars/UI | `UI/ReEchoHealthBarWidget.*`, `Graybox/ReEchoHealthBarActor.*` | `CombatantComponent.*` |
| Encounter countdown/current level HUD | `UI/ReEchoEncounterHudWidget.*` | `ReEchoGameMode.*`, `EncounterDirector.*`, `RunSubsystem.*` |
| Rain, fog, weather scenes | `UI/ReEchoWeatherWidget.*`, `ReEchoGameMode.*` | `Core/ReEchoBalanceSettings.h`, `DefaultGame.ini` |
| Pause/death/restart/quit UI | `UI/ReEchoRestartWidget.*` | `ReEchoGameMode.*`, `PlayerPawn::TogglePauseMenu`, `DefaultInput.ini` |
| Trait cards/card choice | `UI/ReEchoTraitCardChoiceWidget.*`, `Run/ReEchoRunSubsystem.*` | `Content/Data/cards.json`, `ReEchoGameMode.*`, `Core/ReEchoTypes.*` |
| Cards/characters/enemies/balance data | Matching `Content/Data/*.json` | `Content/Data/README.md`, `validate_project.py` |
| Build failure | `scripts/ue/Build-Editor.*` | latest UBT log; matching source only |
| Windows packaging/cook/resource missing | `scripts/ue/package_windows.py`, `ReEchoGameMode.*`, hard asset references, UAT Cook manifests | `Content/ReEcho/Textures/Characters/`, `Saved/Cooked/Windows`, Shipping smoke test |
| Automation | `scripts/ue/Run-Automation.*` | `Private/Tests/*`, `Saved/Logs/ReEcho.log` |
| UE MCP | `docs/UE_MCP.md` | `ReEcho.uproject`, editor settings, `.codex/config.toml` |
| RenderDoc MCP | `docs/RENDERDOC_MCP.md` | `scripts/mcp/Codex-With-RenderDoc.cmd` |
| AI workflow/rules | `shared/PROJECT_RULES.md` | role rule, plan, relevant lessons only |

## Invariants and traps

- Use only the installed/release UE 5.8 build; the separate source checkout is out of scope.
- Close ReEcho Unreal Editor before building C++ DLLs.
- Simulation is 60 Hz, recording is 20 Hz, and the encounter duration is 30 seconds unless an explicit design change updates all contracts.
- Do not serialize automatic attacks into recordings.
- Do not hand-edit `.uasset` or `.umap`; claim serialized assets in `PLANNER_EXCHANGE.md` and modify them through UE.
- `Content/Data` is not runtime truth yet. Changing JSON alone does not change gameplay.
- The arena is runtime-generated. Do not search for a missing project map.
- `AllToolsets` may emit unrelated GameFeatureData/Niagara Python warnings in commandlets; judge ReEcho tests from named automation results.
- Follow `.clang-format` plus the mandatory Unreal C++ section in `PROJECT_RULES.md`.

## Verification routes

```powershell
python scripts\validate_project.py
scripts\ue\Build-Editor.cmd -Configuration Development
scripts\ue\Run-Automation.cmd -Filter ReEcho
git diff --check
```

Pass `-EngineRoot <path>` only at invocation time or use the machine-local `RE_ECHO_UE_ROOT`; never commit a machine path.
