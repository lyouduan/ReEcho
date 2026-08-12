# Plan 29 - UI - full UMG hybrid migration

## Coordination

- Planner owner: Codex.
- Executor owner: Codex.
- Plan authored by (AI side): `ReEcho teammate-side AI`.
- Implementation authored by (AI side): `ReEcho teammate-side AI`.
- Task status: `InProgress`.
- Human validation: `PendingBeforeClose`.
- Planning ref / implementation base: `plan/29-ui-umg-migration` / combined `origin/main` plus Plan 10 HUD UMG commits, explicitly approved by the human after the external-integration audit and renumbered by the human after canonical remote Plan 28 was fetched.
- Implementation branch: `plan/29-ui-umg-migration`.
- Depends on / Blocks: Plan 10 player HUD UMG is the reference implementation. The human explicitly prioritized Plan 29 over Plan 26 and coordinated Inventory/Stats presentation ownership into this Plan; Plan 26 data-consumer behavior remains a compatibility requirement.
- Writes: `Source/ReEcho/{Public,Private}/UI/*`; `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`; new UI-manager/runtime files under `Source/ReEcho/UI`; new `/Game/ReEcho/UI/WBP_*.uasset`; this Plan and live Exchange coordination.
- Stable Reads: Run subsystem, combatant, encounter director, save game, weapon/character/card CSV readers and existing public gameplay delegates.
- Impact mode: `Isolated` C++ batches plus `Exclusive` per new/modified Widget Blueprint asset; `ReadOnly` gameplay/data contracts.
- Compatibility promise / downstream action: preserve existing player-visible flows, delegate semantics, stable IDs, saves, pause determinism and gameplay source-of-truth; UMG never becomes authoritative for purchases, selections, combat, saves or run state.
- Explicit exclusions: no new gameplay screens, no balance/data-schema changes, no canonical workbook/CSV writes, no map changes, and no remote-main publication.

## Locked goal

Convert every currently existing ReEcho UI surface from C++-owned layout to a consistent Unreal UMG/C++ hybrid architecture: UMG owns layout, styling, animation and focus presentation; C++ owns data, validation, events, lifecycle and gameplay decisions. Centralize viewport layers and menu/input coordination without changing existing gameplay behavior.

## Locked acceptance

- [ ] Existing player HUD, encounter HUD, start menu, loadout selection, pause/death/victory menu, settings, trait choice, enemy health bar, damage numbers and weather presentation use designer-authored UMG assets or an explicitly documented hybrid paint surface.
- [ ] Inventory/shop and stats are migrated after Plan 26 ownership is released or explicitly coordinated; until then their behavior remains unchanged.
- [ ] GameMode no longer hard-codes scattered viewport Z-order and per-screen input details; a UI coordination layer owns screen/layer lifecycle.
- [ ] Fixed offer/character/weapon click handlers are replaced with stable-ID-driven reusable entry widgets where their backing data is variable.
- [ ] Presentation-only per-frame polling is replaced by events where the provider already exposes a safe event; required countdown/weather paint ticks remain documented.
- [ ] Existing delegates, save semantics, pause behavior, run state and stable IDs remain compatible.
- [ ] Editor build, `CompileAllBlueprints`, all affected `ReEcho.*` automation, project validation and `git diff --check` pass.
- [ ] Human PIE validates navigation, readability, animation, safe-area layout and DPI at 1280x720, 1920x1080, 2560x1440 and an ultrawide resolution.
- [ ] No generated UE build products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: `plan/29-ui-umg-migration`, containing approved combined adaptation of `origin/main` at `6c1f890` and Plan 10 through `e759026`; this UI work was renumbered from local-only Plan 28 after the canonical remote Plan 28 attack-mode coordination was fetched.
- Engine/build availability: UE 5.8 installed Editor build available; Plan 10 Editor build and 34 `ReEcho.*` tests passed before Plan 28.
- Existing focused-test result: Plan 10 `CompileAllBlueprints` completed with 0 errors/warnings/failed loads.
- Active exclusive ownership or shared-contract approval: claim each WBP before MCP editing; do not enter Plan 26 files while reserved.
- Stop condition if the baseline is broken: stop the affected batch if current Editor build/automation fails before its implementation, if a binary asset lock is held, or if Plan 26 becomes Active on an overlapping surface.

## Implementation outline

1. Add a UI coordination layer and shared screen/layer policy while preserving existing GameMode calls.
2. Migrate persistent HUD surfaces: encounter HUD and enemy health bars; finish event-driven player HUD binding when the provider surface permits it.
3. Migrate start, loadout, pause/death/victory and settings flows to native-parent WBP assets.
4. Migrate trait choice to reusable stable-ID cards and UMG animation.
5. Migrate damage number and weather presentation using hybrid world/paint ownership.
6. After Plan 26 coordination, migrate inventory/shop and stats to reusable item/stat rows.
7. Remove obsolete normal-path WidgetTree construction and scattered viewport/input constants only after every replacement loads and compiles.
8. Run objective checks per batch and finish with named human PIE/DPI validation.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project/source/workflow invariants pass |
| C++ | repository `.clang-format`; `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT exit code 0 |
| Blueprint | `CompileAllBlueprints` plus focused asset load/compile | 0 errors, warnings and failed loads for migrated WBP assets |
| Automation | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | All discovered `ReEcho.*` tests pass |
| Human | Named PIE navigation/visual/DPI matrix | Human result recorded before closure |
| Diff | `git diff --check`; explicit staged paths | Clean diff, no generated or machine-local files |

## Execution notes

### Changed

- Added the first implementation batch: a GameInstance UI manager owns named viewport-layer policy and registered-widget cleanup; GameMode routes all existing widgets through it instead of scattering numeric Z-order constants.
- Centralized menu focus, UI-only/game-and-UI input modes, cursor policy and gameplay-input restoration in the UI manager; GameMode now expresses only whether a screen pauses/blocks abilities.
- Replaced player HUD and enemy health-bar per-frame polling with `OnHealthChanged` subscriptions, including safe rebinding and destruction-time unbinding.
- Added a reusable indexed UMG button and converted loadout and trait choices from ten fixed slot handlers to array-driven index-to-stable-ID resolution.
- Replaced the pause/death/victory menu's four overlapping booleans with explicit screen-mode and quit-prompt state enums while preserving its public GameMode-facing API and presentation priority.
- Converted settings navigation from three dedicated click handlers to a reusable indexed-button collection with validated category selection and array-driven focus/styling.
- Unified Start Menu actions behind one indexed dispatcher and extracted the repeated Start/Settings/Restart C++ fallback button construction into a shared presentation-only helper.
- Added optional named UMG bindings for encounter HUD, enemy health bar, Start Menu, Settings and pause/restart surfaces; native parents now preserve designer-authored WidgetTrees and construct C++ layouts only when no UMG root exists.
- Added UMG-owned dynamic containers for loadout character/weapon entries and trait-card entries; C++ rebuilds reusable indexed children from stable CSV/offer IDs while retaining the full native fallback presentation.
- Created native-parent WBP assets for encounter HUD, enemy health bar, Start Menu, Loadout, Settings, Restart and Trait Choice, and routed runtime construction through hard-referenced designer classes with native fallback.
- Added designer-owned button labels to Start Menu, Settings, Loadout and Restart, centered the menu panels in explicit DPI-scalable safe regions, and nested the stateful quit label inside its button so C++ state changes update the visible UMG presentation.
- Positioned the encounter status and countdown in a right-top safe area; gave the enemy world-space bar a designer-owned background/fill presentation; and made the Trait Choice WBP own the full-screen card anchor field, header, currency and animated needle layers while C++ continues to populate stable-ID cards.
- Moved Trait card dimensions to three designer-owned `TraitCardSlot0..2` SizeBoxes. The normal WBP path now fills those slots without writing width or height; fixed C++ dimensions remain only in the missing-asset fallback path.
- Deferred the post-Trait shop transition to the next frame so the card widget is removed and its Slate click dispatch completes before the shop takes focus; failed shop creation now restores gameplay and advances instead of leaving the run paused.
- Fixed the paused-world transition deadlock: after a card is applied, the world is unpaused while menu input and gameplay-ability blocking remain active, allowing the next-tick callback to open an extra card, the post-Trait shop or the next encounter.
- After explicit human priority coordination over Plan 26, migrated Inventory/Shop and Stats to native-parent WBP presentation shells. C++ retains snapshots, purchase validation and delegates; UMG owns the background, safe-area panels, close controls and dynamic offer container. Shop offers now use the reusable indexed-button dispatcher instead of four fixed handlers.
- Split Trait offers into `WBP_ReEchoTraitCardEntry`: UMG owns each card's button, text hierarchy, padding and styling while the parent keeps stable-index/CardId mapping and reveal sequencing. The existing C++ card construction remains fallback-only.
- Split character and weapon options into shared `WBP_ReEchoLoadoutEntry` instances. UMG owns portrait dimensions/scaling, label layout and selection-button presentation; the parent continues to resolve CSV stable IDs, labels, visual keys and confirmation validity.
- Fixed the post-Trait shop crash by allowing inventory/shop snapshots to arrive before `NativeConstruct`; refresh now waits for complete dynamic offer entries and validates every bound presentation object before indexing it.
- Renamed Loadout entry-local selection colors to avoid a Unity-build anonymous-namespace collision introduced by the entry split.
- Established the in-module `UI/Framework` layer. `EReEchoUIScreen` identifies runtime screens, `UReEchoUIManagerSubsystem` owns the WBP class registry and active instances, and `UReEchoUIFlowCoordinatorSubsystem` owns creation, closure, focus, pause policy and pause-safe transitions.
- Removed every WBP `FClassFinder`, direct `CreateWidget`, `AddToViewport` routing and `RemoveFromParent` lifecycle operation from GameMode. GameMode retains gameplay decisions and typed delegate endpoints while the framework owns presentation lifecycle.

### Evidence

- External integration audited; human selected combined adaptation.
- Batch A Editor Development build passed and all 34 discovered `ReEcho.*` automation tests passed.
- The input-coordination follow-up also passes Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- Event-driven player/enemy health UI passes Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- Indexed loadout/trait selection passes Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- Explicit restart-screen state modeling passes Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- Indexed settings navigation passes Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- Shared menu-button construction and indexed Start Menu dispatch pass Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- Optional UMG binding/fallback integration passes Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- Dynamic Loadout/Trait UMG containers pass Editor Development build and all 34 discovered `ReEcho.*` automation tests.
- The first seven runtime WBP assets compile individually; their C++ routing batch passes Editor Development build, all 34 discovered `ReEcho.*` automation tests, project validation and `git diff --check`.
- The post-Trait shop lifecycle fix passes Editor Development build, both `ReEcho.Shop.*` automation tests, project validation and `git diff --check`; direct PIE confirmation of the card-to-shop transition remains part of human validation.
- The typed UI Framework and GameMode lifecycle migration pass an Editor Development build, all 34 discovered `ReEcho.*` automation tests, project validation and `git diff --check`; Blueprint/PIE visual and navigation regression remain required for the batch.
- `CompileAllBlueprints` processed the current asset set with 0 compile errors, 0 compile warnings and 0 failed loads. The commandlet returned non-zero only because this workstation's Installed DDC/Zen graph has no writable node; `-DDC-ForceMemoryCache` allowed compilation to complete and isolated that infrastructure error.

### Remaining risks

- Large binary-asset surface requires serialized MCP edits and batch verification.
- Plan 26 owns inventory/shop and stats C++ until coordinated.
- After an Editor restart, the current Codex Unreal MCP transport did not reconnect even though the Editor registered all 52 toolsets; WBP creation is paused at that asset boundary rather than bypassing MCP.
- Canonical Plan 28 may later overlap `ReEchoRestartWidget.*` and GameMode for player attack-mode integration; the human directed Plan 29 to continue independently, with combined adaptation preserving both behaviors at integration.

### Human validation result/request

- `PendingBeforeClose`: full menu navigation, visual quality and DPI matrix.
