# Plan 39 - Presentation - 2D sequence animation integration

## Coordination

- Planner owner: Codex.
- Executor owner: Codex.
- Plan authored by (AI side): `ReEcho teammate-side AI`.
- Implementation authored by (AI side): `ReEcho teammate-side AI`.
- Task status: `Closed`.
- Human validation: `Passed` through iterative PIE feedback and the explicit merge request.
- Original implementation base: `origin/main@ccebbf2`; integration base: `origin/main@f5051e4`.
- Implementation branch: `plan/39-2d-sequence-animation`.
- Depends on / Blocks: Consumes existing `/Game/2DAnim/Flipbook/Idel` and `/Game/2DAnim/Flipbook/01_2`; does not block Plan 29 UI work.
- Writes: `Source/ReEcho/ReEcho.Build.cs`, `Source/ReEcho/{Public,Private}/Presentation/Animation2D/*`, `Player/ReEchoPlayerPawn.*`, `Graybox/ReEchoEnemyActor.*`, `Private/Tests/ReEcho2DAnimationTests.cpp`, `scripts/ue/repair_2d_animation_flipbooks.py`, `Content/2DAnim/**`, this Plan and its live Exchange row.
- Stable Reads: character ID `J_SPADE`, `EReEchoEnemyKind::Grunt`, existing combat/collision/recording and procedural visual contracts.
- Impact mode: C++ `Isolated`; `Content/2DAnim/**` `Exclusive`.
- Compatibility promise / downstream action: Actor/collision gameplay transforms remain authoritative; recordings, GAS, weapons, HUD, save identity and non-target actor presentation remain unchanged.
- Explicit exclusions: non-Moon-Staff attack Flipbooks; Echo animation; Grunt state-specific clips; Shield/Bomber/Boss animation; animation-notify gameplay; CSV/XLSX migration; asset rename.

## Locked goal

Add a reusable Paper2D presentation layer that loops the existing idle Flipbook only for player `J_SPADE` and the existing `01_2` Flipbook only for Grunt enemies, while retaining all current procedural visual feedback and safe static-Billboard fallback.

## Locked acceptance

- [x] `J_SPADE` uses static `/Game/2DAnim/Player/Idel_01` while idle, loops `/Game/2DAnim/Flipbook/Idel` while moving and replays `/Game/2DAnim/Flipbook/s` for each successful Moon Staff attack; all other characters retain their static Billboard.
- [x] Grunt loops `/Game/2DAnim/Flipbook/01_2` in every gameplay state; all other enemy kinds retain their static Billboard.
- [x] Missing Flipbooks safely show the existing static texture, and no actor shows Billboard and Flipbook simultaneously.
- [x] Bob, attack lunge/pulse, hit shake/stretch and death shrink remain visual-only and observable through a shared visual-effect transform layer.
- [x] Movement, collision, combat, GAS, weapons, recording, HUD, selection and save identity semantics remain unchanged.
- [x] Facing, anchor, world height, transparency/sort behavior and debug bounds are safe for both visual implementations.
- [x] UE 5.8 Editor build, focused automation, project validation and asset/cook presence checks pass.
- [x] Human PIE validates final visual quality before closure.

## Step 0 gate

- Baseline branch/commit: original implementation `origin/main@ccebbf2`; combined integration `origin/main@f5051e4`.
- Engine/build availability: UE 5.8 installed build; Editor must be closed before build/commandlet checks.
- Existing focused-test result: prior main evidence only; refresh required after implementation.
- Active exclusive ownership or shared-contract approval: Plan39 owns `Content/2DAnim/**`; UI ownership is disjoint.
- Stop condition if the baseline is broken: stop if assets do not load as PaperFlipbooks or implementation requires gameplay/schema changes.

## Implementation outline

1. Add Paper2D module dependency and reusable animation state/profile/component types.
2. Introduce an actor-owned `VisualEffectRoot`; attach static Billboard and animation component beneath it.
3. Activate the player profile only for `J_SPADE`, otherwise stop animation and show the selected static texture.
4. Activate the enemy profile only for Grunt, otherwise keep the existing Billboard path.
5. Move procedural visual transforms from Billboard to `VisualEffectRoot`; keep frame playback internal to PaperFlipbook.
6. Make facing and debug bounds implementation-agnostic and verify asset tracking/build/automation/cook visibility.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py`; `git diff --check` | Project/source invariants and whitespace pass |
| Build | `scripts/ue/Build-Editor.cmd` | UE 5.8 UHT/UBT exit code 0 |
| Automation | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | Relevant player/enemy/combat/recording tests pass |
| Assets | Editor asset load plus cook/manifest presence for both Flipbooks | Package paths resolve and are retained |
| Human | PIE with J_SPADE, another player, Grunt and non-Grunt enemies | Facing, sizing, alpha, sorting and procedural feedback accepted |

## Execution notes

### Changed

- Added the reusable Paper2D state/profile/component layer under `Presentation/Animation2D`; unresolved states use the default looping Flipbook.
- Added `VisualEffectRoot` to player and enemy presentation hierarchies, moving bob/lunge/hit/death transforms above both static and animated renderers.
- `J_SPADE` exclusively activates `/Game/2DAnim/Flipbook/Idel`; Grunt exclusively activates `/Game/2DAnim/Flipbook/01_2`; every other case retains the existing Billboard.
- Added hard-reference loading, mutually exclusive visibility, static fallback, Paper2D-safe facing and Billboard-debug skipping.
- Added a focused asset/profile automation test and an Unreal Editor repair script. The script generated the twelve missing player PaperSprite dependencies and rebuilt the existing `Idel` Flipbook through Unreal's asset API.
- After the first PIE showed an invisible Spade, corrected the repair path to call `UPaperSprite::RebuildData()` through a development-only native bridge; all twelve player Sprite assets now contain non-empty baked render geometry.
- Made looping explicit on every profile activation, state resolution and Flipbook application so both Spade `Idel` and Grunt `01_2` restart and continue looping after reconfiguration.
- Fixed the PIE playback defect: the custom component had disabled `PrimaryComponentTick`, preventing PaperFlipbook playback time from advancing. Animation activation now enables ticking and deactivation disables it.
- Updated the Spade state policy by human direction: static Billboard while stationary, looping `Idel` only while moving, and `/Game/2DAnim/Flipbook/s` while a successful Moon Staff (`W_J_02`) attack visual is active. Other weapons retain the existing procedural/static presentation.
- Replaced implicit presentation checks with explicit `Idle -> Move/Attack`, `Move -> Idle/Attack`, and `Attack -> Idle/Move` transitions. Idle uses `/Game/2DAnim/Player/Idel_01` as its static texture. Moon Staff gameplay and its attack origin remain intact, while its held `StaffSprite` is always hidden.
- Spade `Idle` renders `Idel_01` at the texture asset's native 1.0 world scale, and Move/Attack use the Flipbook assets' native 1.0 authored scale through `FReEcho2DAnimationProfile::bUseNativeScale`; no player presentation is forcibly normalized to a target world height. Grunt retains its separate enemy sizing rule.
- Repeated Moon Staff attacks restart the one-shot Attack Flipbook from frame zero, so held/automatic continuous attacks visibly replay at the actual successful-attack cadence without converting Attack into an unconditional loop.
- Added the complete existing `Content/2DAnim` resource tree to the Plan delivery without renaming `Idel` or hand-editing asset bytes.

### Evidence

- UE 5.8 `ReEchoEditor` Win64 Development build passed after clang-format.
- `ReEcho.*` automation passed 35/35; the animation asset test loads both Flipbooks, exercises default-state fallback, and the final log contains zero `/Game/2DAnim` load errors.
- The focused test now also rejects a Flipbook whose runtime render bounds are empty, preventing the original loadable-but-invisible false positive.
- `python scripts/validate_project.py` passed all project, CSV/XLSX and workflow checks.
- Clean Win64 Shipping Build/Cook/Stage/Pak/Archive passed. The staged UFS manifest contains both Flipbooks, all twelve player PaperSprites and all twelve Grunt PaperSprites.
- `git diff --check` passed.
- Rebased integration on `origin/main@f5051e4`: UE 5.8 Editor build passed and the expanded current `ReEcho.*` suite passed 64/64 with zero failures, including the held-attack regression coverage from Plan38.

### Remaining risks

- PaperFlipbook pivot/material/import settings and perceived world scale require PIE judgment.
- Player and enemy Flipbook plane orientation, mirroring axis, transparent edges and sort order require the locked human PIE matrix.

### Human validation result

- Passed through iterative PIE feedback: invisible playback, looping, state transitions, held weapon visibility, native player scale and repeated attack playback were corrected from direct human observations. The human then explicitly requested integration.
