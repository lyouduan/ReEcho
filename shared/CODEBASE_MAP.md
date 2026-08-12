# ReEcho codebase map

Last verified: 2026-08-11. This file is the shortest authoritative routing index for code retrieval. It describes where behavior lives; `PROJECT_RULES.md` remains the authority for constraints. Delivery facts come from source, tests, closed Plans and Git rather than a duplicated status snapshot.

## Minimal retrieval protocol

1. Read `AGENTS.md`, then this file.
2. Read only the row matching the requested task in **Task routing**.
3. For C++ work, read the matching `Public/.../*.h` before its `Private/.../*.cpp`.
4. Read `shared/PROJECT_RULES.md` before editing and only the relevant `shared/LESSONS.md` section when debugging.
5. Do not search `Binaries/`, `Intermediate/`, `Saved/`, or generated files unless diagnosing build output.
6. Do not infer runtime behavior from legacy `Content/Data/*.json`; CSV is the target runtime data source and JSON is migration-only until later domain plans finish.

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
| `Design/Data/ReEchoData.xlsx` | Canonical designer XLSX authoring workbook; machine Tables generate runtime CSV | Data authoring, XLSX migration or Plan25 checks |
| `Design/Data/ReEchoData使用说明.md` | Chinese designer guide for editable Tables, field rules, CSV generation, errors and submission | Before changing production balance/configuration in the workbook |
| `Design/Data/ReEchoData策划验收清单.md` | Designer QA onboarding, isolated clone, XLSX/CSV/PIE acceptance, recovery, feedback template and AI setup prompt | Before first-time Plan25 designer acceptance or handing setup to a designer's AI |
| `Content/Data/` | Runtime CSV contract plus read-only migration JSON | Data contract, fixtures, cards, characters, enemies, encounters or balance source |
| `Content/ReEcho/Materials/` | Serialized project materials | Visual asset references |
| `Content/ReEcho/Textures/Characters/` | Cooked 2D actor and shadow textures | Player/echo/enemy Billboard visuals |
| `Content/SourceArt/Characters/` | Reviewable PNG sources, including MushroomGirl animation frames and Sprite Sheet | Regenerating or extending 2D actor assets |
| `scripts/ue/` | Installed-engine discovery, build, automation, reproducible Windows Shipping packaging and asset import | Compile/test/package or asset-import workflow |
| `scripts/data/sync_xlsx_to_csv.py` | Deterministic XLSX Table to UTF-8 CSV generator, check mode and transactional publish | Data authoring sync or generated CSV drift |
| `scripts/validate_project.py` | CSV, legacy JSON and workflow static validation | Data/workflow changes |
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
       -> show UReEchoStartMenuWidget
            -> new game: UReEchoRunSubsystem::StartRun
            -> continue: load safe checkpoint or suspended encounter
       -> BeginNextEncounter or ResumeSavedEncounter
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

Esc -> pause menu -> exit
  -> confirmation: continue game or save-and-quit
  -> encounter save captures clock, player, active recording and living enemies
```

## Runtime module map

| Area | Primary types | Files | Responsibility |
|---|---|---|---|
| Orchestration | `AReEchoGameMode` | `Public/ReEchoGameMode.h`, `Private/ReEchoGameMode.cpp` | Runtime bounded arena/camera-aligned unlit backdrop, six encounters, configurable deterministic peripheral enemy spawning, cleanup, recording handoff and HUD text |
| UI framework | `UReEchoUIManagerSubsystem`, `UReEchoUIFlowCoordinatorSubsystem`, `EReEchoUIScreen` | `UI/Framework/*`, `UI/ReEchoUIManagerSubsystem.*`, `ReEchoGameMode.*` | Central WBP class registry, typed active-screen lifecycle, viewport layers, focus/input policy, pause-safe screen transitions; GameMode retains gameplay decisions and delegate endpoints |
| Encounter clock | `AReEchoEncounterDirector` | `Encounter/ReEchoEncounterDirector.*` | Pause-aware 60 Hz fixed step, setup phase, timeout/end delegate |
| Billboard screen sizing | `ReEchoBillboardScreenScale` | `Private/Graybox/ReEchoBillboardScreenScale.h` | Shared camera-depth compensation for stable player/echo/enemy projected size |
| Player | `AReEchoPlayerPawn` | `Player/ReEchoPlayerPawn.*` | Map-edge-clamped follow camera with wider player arena clamp, WASD, center-aligned root capsule, mouse-following horizontal sprite facing, four configurable NewCast character textures, manual attacks and visual-only 2D motion |
| GAS combat | `UAbilitySystemComponent`, `UReEchoCombatAttributeSet`, native GameplayEffects/tags and player abilities | `AbilitySystem/*`, `Player/ReEchoPlayerPawn.*`, `Combat/ReEchoCombatantComponent.*`, `Graybox/ReEchoEnemyActor.*` | Player/enemy runtime attributes, effect-based damage/heal, tag-driven activation, cooldown commit and AbilityTask held attacks; CombatantComponent is the legacy-facing adapter |
| Projectile | `AReEchoProjectileActor` | `Graybox/ReEchoProjectileActor.*` | Visible sphere travel, swept path-vs-capsule hit detection, damage delivery |
| Player weapons | `AReEchoWeaponActor`, `FReEchoCsvWeaponRow`, `EReEchoInputSlot` | `Weapons/ReEchoWeaponActor.*`, `Player/ReEchoPlayerPawn.*`, `UI/ReEchoLoadoutSelectionWidget.*`, `Data/ReEchoWeaponCsvReader.*` | CSV-driven concrete WeaponIds, compatibility-only InputSlot mapping, ordered attack steps, start-selectable loadout options, run-locked recorder/echo initialization by stable WeaponId |
| Sword arc VFX | `AReEchoSwordArcActor` | `Graybox/ReEchoSwordArcActor.*`, `Weapons/ReEchoWeaponActor.cpp` | Layered translucent crescent spawned for each sword swing and faded over a short lifetime |
| Enemy | `AReEchoEnemyActor` | `Graybox/ReEchoEnemyActor.*` | Grunt/shield/bomber/final-Boss stats, six rotating 2D grunt variants and a 2D final-Boss Billboard, capsule damage volume, chase/contact damage and visual-only attack/hit/death motion |
| Echo | `AReEchoEchoActor` | `Graybox/ReEchoEchoActor.*` | Historical movement and recorded weapon-change playback, automatic shared-weapon attacks, translucent pulsing ghost material and visual-only 2D motion |
| Echo trajectory | `AReEchoTrajectoryActor` | `Graybox/ReEchoTrajectoryActor.*` | Only the active echo's simplified historical positions projected onto the ground as one world-fixed translucent route |
| Combat | `UReEchoCombatantComponent`, `ReEchoElementReaction` | `Combat/ReEchoCombatantComponent.*`, `Combat/ReEchoElementReaction.*` | Shared stats, HP, block, damage/death delegates and CSV-driven element reaction execution |
| Hit VFX | `ReEchoAttackEffects`, `AReEchoHitImpactActor` | `Graybox/ReEchoAttackEffects.*`, `Graybox/ReEchoHitImpactActor.*` | Transparent HitStarburst plane spawned only after non-zero applied damage |
| Damage numbers | `AReEchoDamageNumberActor` | `UI/ReEchoDamageNumberActor.*`, damage callers | Camera-facing floating `-N` text for actual damage applied to player/enemies |
| Recording | `UReEchoRecorderComponent` | `Recording/ReEchoRecorderComponent.*` | 20 Hz positions and successful active-skill events |
| Playback | `UReEchoPlaybackComponent` | `Recording/ReEchoPlaybackComponent.*` | Interpolated historical position and crossed skill events |
| Run state/save | `UReEchoRunSubsystem`, `UReEchoRunSaveGame` | `Run/ReEchoRunSubsystem.*`, `Run/ReEchoRunSaveGame.h` | Run phase, encounter index, CSV-backed build, inventory, pending/latest/stored echo state, stable-GUID replay selection, v4 migration, safe checkpoints and explicit suspended-encounter persistence |
| CSV data registry | `FReEchoCsvDataRegistry` | `Data/ReEchoCsvDataRegistry.*`, `Data/ReEchoWeaponCsvReader.*`, `Content/Data/*.csv`, `Design/Data/ReEchoData.xlsx`, `scripts/data/sync_xlsx_to_csv.py` | Versioned CSV manifest loading, XLSX authoring sync, validation, behavior/effect/formula/attack-pattern allowlists and immutable runtime snapshots |
| Shared types | `FReEcho*`, `EReEcho*` | `Core/ReEchoTypes.*` | Stats, build snapshot, recording samples/events, elements, phases and suspended encounter/enemy runtime state |
| Balance config | `UReEchoBalanceSettings` | `Core/ReEchoBalanceSettings.h`, `Config/DefaultGame.ini` | Encounter/fixed-step/recording/global prototype values |
| Health UI | `UReEchoPlayerHudWidget`, `AReEchoHealthBarActor`, `UReEchoHealthBarWidget` | `UI/ReEchoPlayerHudWidget.*`, `Graybox/ReEchoHealthBarActor.*`, `UI/ReEchoHealthBarWidget.*` | Top-left portrait/live health HUD for the player; camera-facing world bars remain enemy-only |
| Encounter HUD | `UReEchoEncounterHudWidget` | `UI/ReEchoEncounterHudWidget.*`, `ReEchoGameMode.*` | Right-top current encounter and remaining-time display; final five seconds turn red |
| Inventory/shop | `UReEchoInventoryShopWidget`, `UReEchoRunSubsystem` | `UI/ReEchoInventoryShopWidget.*`, `Run/ReEchoShopCatalog.h`, `Run/ReEchoRunSubsystem.*`, `ReEchoGameMode.*` | B/M menus over supplied full-screen art; Time Shard purchases enter run-local inventory and immediately mutate the build snapshot |
| Player/echo stats | `UReEchoStatsWidget` | `UI/ReEchoStatsWidget.*`, `ReEchoGameMode.*`, `Graybox/ReEchoEchoActor.*` | Tab-paused two-column live stats over the imported blurred clockwork background; handles runs without an active echo |
| Weather scenes | `UReEchoWeatherWidget` | `UI/ReEchoWeatherWidget.*`, `ReEchoGameMode.*`, `ReEchoBalanceSettings.h` | Configurable per-encounter rain streaks and player/echo-centered fog-of-war rendered as input-transparent screen-space presentation |
| GM/debug commands | `AReEchoGameMode` exec functions | `ReEchoGameMode.*`, `docs/GM_COMMANDS.md` | Development-console status, healing, Time Shards, weather override and enemy-clear commands; rejected in Shipping |
| Start/continue/loadout UI | `UReEchoStartMenuWidget`, `UReEchoLoadoutSelectionWidget` | `UI/ReEchoStartMenuWidget.*`, `UI/ReEchoLoadoutSelectionWidget.*`, `Run/ReEchoRunSaveGame.h`, `ReEchoGameMode.*` | Blocking startup choice followed by CSV-backed character and initial-weapon selection before encounter 1; continue restores the saved current loadout |
| Pause/restart UI | `UReEchoRestartWidget` | `UI/ReEchoRestartWidget.*`, `ReEchoGameMode.*` | Esc pause overlay, resume, full-level restart, two-step save-and-quit confirmation, death-screen and terminal Boss-victory actions |
| Trait choice UI | `UReEchoTraitCardChoiceWidget` | `UI/ReEchoTraitCardChoiceWidget.*`, `Run/ReEchoRunSubsystem.*`, `ReEchoGameMode.*` | Animated centered three-choice presentation with current Time Shards; only the pending offer mutates the build, then the existing shop opens before the next encounter |
| Tests | Recording/GAS/run/save/shop/trait automation | `Private/Tests/*` | Recording interpolation/timeline, GAS structure, final-Boss gate, safe/suspended save snapshots, shop purchase and deterministic pending-trait regressions |

Paths in the table are relative to `Source/ReEcho/Public` or `Source/ReEcho/Private`.

## Current gameplay contract

- Player: pre-run character and CSV weapon selection, run-locked weapon loadout, 2D Billboard character, WASD movement, left mouse/J basic attack and Q/Space power shot.
- Echo: 2D Billboard replay actor that follows historical movement and attacks through the shared weapon implementation, including recorded weapon-change playback.
- Projectile flight has no Niagara. A transparent HitStarburst impact appears only when applied damage is greater than zero.
- Enemies use 2D Billboard visuals for every archetype; the old cube/plane/cone fallback rendering has been removed. Shield enemies remain implemented but are temporarily excluded from encounter composition.
- Enemy hit feedback: short stagger, source-opposed knockback and decaying lateral shake; blocked hits do not trigger it.
- Player, echo and 2D enemies animate their existing static textures with sprite-local bob, squash, lunge and recovery; enemy death adds a short shrink/fall before destruction.
- Player and enemies use compact camera-facing world health bars anchored above their 2D visual.
- Esc pauses into a resume/restart/quit menu. Quit requires confirmation and a successful save; Continue or Esc cancels and resumes. Death and Boss victory expose restart/quit; Shipping builds call platform QuitGame.
- Clearing all living enemies ends the encounter immediately; timeout is the fallback.
- Automatic echo attacks are not serialized. Recording stores player position plus successful active-skill events.

## Config, data and assets

| Concern | Source |
|---|---|
| Default map/GameMode | `Config/DefaultEngine.ini`; runtime global/encounter balance | `Config/DefaultGame.ini` |
| WASD, mouse/J and Q/Space | `Config/DefaultInput.ini` |
| MCP endpoint/editor preferences | `Config/DefaultEditorPerProjectUserSettings.ini`, `.codex/config.toml` |
| Windows override | `Config/Windows/WindowsEngine.ini` |
| Runtime CSV contract | `Content/Data/*.csv`, `Source/ReEcho/Data/*` |
| Migration-only design registries | `Content/Data/*.json` |
| Echo ghost material | `Content/ReEcho/Materials/M_EchoGhost.uasset` |
| Module dependencies | `Source/ReEcho/ReEcho.Build.cs` |

CSV currently contains the runtime foundation manifest/schema/smoke tables, canonical character/build tables, element/status/reaction tables and weapon-domain tables loaded by `FReEchoCsvDataRegistry` at module startup. Current playable character base stats/default weapons, the six-card trait draw pool, forge choices, promotion role buckets, card numeric effects, combat elements, necessary statuses, six ordered reactions, concrete weapons, ordered attack steps, slot profiles, parts and part effects read from CSV; burn DOT state, vaporize squared damage, growth ReactionEfficiency-scaled radius attachment, configured-radius conduct chaining and enhancement blocking execute through `ReEchoElementReaction`. Legacy JSON still covers encounters, enemies and global balance; migrated character/card/element/status/reaction/weapon JSON is migration-only review material.

## Task routing

| Task keywords | Read first | Usually also read |
|---|---|---|
| Startup, arena, rounds, spawn, clear-to-next | `ReEchoGameMode.*` | `EncounterDirector.*`, `RunSubsystem.*`, `DefaultEngine.ini` |
| Input, movement, player attack | `Player/ReEchoPlayerPawn.*` | `DefaultInput.ini`, `ReEchoProjectileActor.*` |
| 2D sprite animation, frame import, attack/hit/death feedback | `Player/ReEchoPlayerPawn.*`, `Graybox/ReEchoEnemyActor.*`, `Graybox/ReEchoEchoActor.*` | `scripts/ue/import_mushroomgirl_frames.py`, `Content/SourceArt/Characters/MushroomGirl/`, imported character textures |
| GAS, abilities, attributes, effects, cooldown, tags | `AbilitySystem/*`, `Player/ReEchoPlayerPawn.*` | `docs/GAS_ONBOARDING.md`, `Combat/ReEchoCombatantComponent.*`, `Graybox/ReEchoEnemyActor.*`, `Weapons/ReEchoWeaponActor.*`, GAS automation tests |
| Mouse cursor aiming/player facing/camera | `Player/ReEchoPlayerPawn.*` | `GameMode::RestoreGameInput`, `DefaultInput.ini` |
| Initial weapon selection/run-locked weapon/sword/melee/element reactions | `UI/ReEchoLoadoutSelectionWidget.*`, `Weapons/ReEchoWeaponActor.*`, `Weapons/ReEchoWeaponRuntime.*`, `Data/ReEchoWeaponCsvReader.*`, `Combat/ReEchoElementReaction.*`, `Data/ReEchoElementReactionCsvReader.*` | `Graybox/ReEchoProjectileActor.*`, `Graybox/ReEchoEnemyActor.*`, `Player/ReEchoPlayerPawn.*`, `Content/Data/weapons.csv`, `Content/Data/weapon_types.csv`, `Content/Data/attack_steps.csv`, `Content/Data/slot_profiles.csv`, `Content/Data/parts.csv`, `Content/Data/part_effects.csv`, `RunSubsystem::StartRun`, `RunSubsystem::TryEquipParts`, `FReEchoBuildSnapshot::EquipmentBaseStats/EquipmentBaseRuleFlags/EquippedParts`, `FReEchoBuildSnapshot::WeaponDomainRevision` |
| Bullet speed/size/color/hit | `Graybox/ReEchoProjectileActor.*` | Player/Echo caller, `EnemyActor::ReceiveGrayboxDamage` |
| Enemy AI, type, shield, bomber, boss | `Graybox/ReEchoEnemyActor.*`, `Graybox/ReEchoBomberRules.*` | `CombatantComponent.*`, `ReEchoBalanceSettings.h`, `DefaultGame.ini`, hit effects |
| Damage, HP, block, death | `Combat/ReEchoCombatantComponent.*` | Damage caller and health UI |
| Hit VFX or hit feel | `Graybox/ReEchoAttackEffects.*`, `Graybox/ReEchoEnemyActor.*` | Niagara plugin/material paths |
| Floating damage text | `UI/ReEchoDamageNumberActor.*` | every `ApplyFinalDamage` caller, currently `Graybox/ReEchoEnemyActor.cpp` |
| Echo appearance/attack/playback/run-locked weapon | `Graybox/ReEchoEchoActor.*`, `Recording/ReEchoRecorderComponent.*`, `Recording/ReEchoPlaybackComponent.*` | Pinned `RunSubsystem::GetRunDataSnapshot()`, `Recording.BuildSnapshot.WeaponId`, initialization-only `Weapons/ReEchoWeaponActor::SelectWeaponById`, `M_EchoGhost.uasset` |
| Echo route/trajectory/trail | `Graybox/ReEchoTrajectoryActor.*` | `Core/ReEchoTypes.h`, `Graybox/ReEchoEchoActor.*`, `M_EchoGhost.uasset` |
| Recording determinism/interpolation | `Core/ReEchoTypes.*`, `Recording/*` | `EncounterDirector.*`, recording test |
| Echo storage/replay selection, run phase, save and shops | `Run/ReEchoRunSubsystem.*`, `Run/ReEchoRunSaveGame.h` | `Core/ReEchoTypes.*`, GameMode |
| CSV runtime data, schema, fixtures and XLSX authoring | `Data/ReEchoCsvDataRegistry.*`, domain readers under `Private/Data/*CsvReader.*` | `Design/Data/ReEchoData.xlsx`, `Design/Data/ReEchoData.migration.md`, `scripts/data/sync_xlsx_to_csv.py`, `scripts/data/test_sync_xlsx_to_csv.py`, `Content/Data/README.md`, `Content/Data/*.csv`, `validate_project.py`, data automation tests |
| Player portrait/health HUD, enemy health bars | `UI/ReEchoPlayerHudWidget.*`, `UI/ReEchoHealthBarWidget.*`, `Graybox/ReEchoHealthBarActor.*` | `Player/ReEchoPlayerPawn.*`, `ReEchoGameMode.*`, `CombatantComponent.*` |
| Encounter countdown/current level HUD | `UI/ReEchoEncounterHudWidget.*` | `ReEchoGameMode.*`, `EncounterDirector.*`, `RunSubsystem.*` |
| Rain, fog, weather scenes | `UI/ReEchoWeatherWidget.*`, `ReEchoGameMode.*` | `Core/ReEchoBalanceSettings.h`, `DefaultGame.ini` |
| Inventory, backpack, shop, store, Time Shards | `UI/ReEchoInventoryShopWidget.*`, `Run/ReEchoShopCatalog.h`, `Run/ReEchoRunSubsystem.*` | `ReEchoGameMode.*`, `Player/ReEchoPlayerPawn.*`, `DefaultInput.ini`, imported UI textures |
| Player stats, echo stats, Tab panel | `UI/ReEchoStatsWidget.*`, `ReEchoGameMode.*` | `Graybox/ReEchoEchoActor.*`, `Combat/ReEchoCombatantComponent.*`, `Player/ReEchoPlayerPawn.*`, `DefaultInput.ini` |
| Pause/death/restart/quit UI | `UI/ReEchoRestartWidget.*` | `ReEchoGameMode.*`, `PlayerPawn::TogglePauseMenu`, `DefaultInput.ini` |
| Trait cards/card choice/character promotion/role build | `UI/ReEchoTraitCardChoiceWidget.*`, `Run/ReEchoRunSubsystem.*`, `Run/ReEchoCharacterPromotion.*` | `Content/Data/cards.csv`, `Content/Data/card_effects.csv`, `Content/Data/characters.csv`, `Content/Data/character_aliases.csv`, `ReEchoGameMode.*`, `Core/ReEchoTypes.*`, `Weapons/ReEchoWeaponActor.*` |
| Cards/characters/elements/reactions/enemies/balance data | Matching `Content/Data/*.csv` and migration-only JSON | `Content/Data/README.md`, `validate_project.py` |
| GM, debug command, cheat, console | `ReEchoGameMode.*`, `docs/GM_COMMANDS.md` | Matching gameplay subsystem or actor API |
| Build failure | `scripts/ue/Build-Editor.*` | latest UBT log; matching source only |
| Windows packaging/cook/resource missing | `scripts/ue/package_windows.py`, `ReEchoGameMode.*`, hard asset references, UAT Cook manifests | `Content/ReEcho/Textures/Characters/`, `Saved/Cooked/Windows`, Shipping smoke test |
| Automation | `scripts/ue/Run-Automation.*` | `Private/Tests/*`, `Saved/Logs/ReEcho.log` |
| UE MCP | `docs/UE_MCP.md` | `ReEcho.uproject`, editor settings, `.codex/config.toml` |
| RenderDoc MCP | `docs/RENDERDOC_MCP.md` | `scripts/mcp/Codex-With-RenderDoc.cmd` |
| AI workflow/rules | `AGENTS.md`, `shared/PROJECT_RULES.md` | first-contact route to `PROGRAMMER_RULES.md`, `DESIGNER_RULES.md` or `ARTIST_RULES.md`; programmer tasks then use the matching duty rule; `PLANNER_EXCHANGE.md` for live scope/ownership; `WORKFLOW.md` only for rationale |

## Invariants and traps

- Use only the installed/release UE 5.8 build; the separate source checkout is out of scope.
- Close ReEcho Unreal Editor before building C++ DLLs.
- Simulation is 60 Hz, recording is 20 Hz, and the encounter duration is 30 seconds unless an explicit design change updates all contracts.
- Do not serialize automatic attacks into recordings.
- Do not hand-edit `.uasset` or `.umap`; claim serialized assets in `PLANNER_EXCHANGE.md` and modify them through UE.
- CSV under `Content/Data` is the target runtime data source. Changing JSON alone does not change gameplay.
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
