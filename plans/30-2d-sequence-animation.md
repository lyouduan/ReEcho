# Plan 30 - Presentation - 2D sequence animation integration

## Coordination

- Planner owner: Codex.
- Executor owner: Codex.
- Plan authored by (AI side): `ReEcho teammate-side AI`.
- Implementation authored by (AI side): `ReEcho teammate-side AI`.
- Task status: `Review`.
- Human validation: `PendingBeforeClose`.
- Local planning / implementation base: `origin/main@ccebbf2`.
- Implementation branch: `plan/30-2d-sequence-animation`.
- Depends on / Blocks: Consumes existing `/Game/2DAnim/Flipbook/Idel` and `/Game/2DAnim/Flipbook/01_2`; does not block Plan 29 UI work.
- Writes: `Source/ReEcho/ReEcho.Build.cs`, `Source/ReEcho/{Public,Private}/Presentation/Animation2D/*`, `Player/ReEchoPlayerPawn.*`, `Graybox/ReEchoEnemyActor.*`, `Private/Tests/ReEcho2DAnimationTests.cpp`, `scripts/ue/repair_2d_animation_flipbooks.py`, `Content/2DAnim/**`, this Plan and its live Exchange row.
- Stable Reads: character ID `J_SPADE`, `EReEchoEnemyKind::Grunt`, existing combat/collision/recording and procedural visual contracts.
- Impact mode: C++ `Isolated`; `Content/2DAnim/**` `Exclusive`.
- Compatibility promise / downstream action: Actor/collision gameplay transforms remain authoritative; recordings, GAS, weapons, HUD, save identity and non-target actor presentation remain unchanged.
- Explicit exclusions: attack Flipbooks; Echo animation; Grunt state-specific clips; Shield/Bomber/Boss animation; animation-notify gameplay; CSV/XLSX migration; asset rename.

## Locked goal

Add a reusable Paper2D presentation layer that loops the existing idle Flipbook only for player `J_SPADE` and the existing `01_2` Flipbook only for Grunt enemies, while retaining all current procedural visual feedback and safe static-Billboard fallback.

## Locked acceptance

- [ ] `J_SPADE` loops `/Game/2DAnim/Flipbook/Idel`; all other characters retain their static Billboard.
- [ ] Grunt loops `/Game/2DAnim/Flipbook/01_2` in every gameplay state; all other enemy kinds retain their static Billboard.
- [ ] Missing Flipbooks safely show the existing static texture, and no actor shows Billboard and Flipbook simultaneously.
- [ ] Bob, attack lunge/pulse, hit shake/stretch and death shrink remain visual-only and observable through a shared visual-effect transform layer.
- [ ] Movement, collision, combat, GAS, weapons, recording, HUD, selection and save identity semantics remain unchanged.
- [ ] Facing, anchor, world height, transparency/sort behavior and debug bounds are safe for both visual implementations.
- [ ] UE 5.8 Editor build, focused automation, project validation and asset/cook presence checks pass.
- [ ] Human PIE validates final visual quality before closure.

## Step 0 gate

- Baseline branch/commit: `origin/main@ccebbf2`.
- Engine/build availability: UE 5.8 installed build; Editor must be closed before build/commandlet checks.
- Existing focused-test result: prior main evidence only; refresh required after implementation.
- Active exclusive ownership or shared-contract approval: Plan30 owns `Content/2DAnim/**`; Plan29 UI ownership is disjoint.
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
- Added the complete existing `Content/2DAnim` resource tree to the Plan delivery without renaming `Idel` or hand-editing asset bytes.

### Evidence

- UE 5.8 `ReEchoEditor` Win64 Development build passed after clang-format.
- `ReEcho.*` automation passed 35/35; the animation asset test loads both Flipbooks, exercises default-state fallback, and the final log contains zero `/Game/2DAnim` load errors.
- The focused test now also rejects a Flipbook whose runtime render bounds are empty, preventing the original loadable-but-invisible false positive.
- `python scripts/validate_project.py` passed all project, CSV/XLSX and workflow checks.
- Clean Win64 Shipping Build/Cook/Stage/Pak/Archive passed. The staged UFS manifest contains both Flipbooks, all twelve player PaperSprites and all twelve Grunt PaperSprites.
- `git diff --check` passed.

### Remaining risks

- PaperFlipbook pivot/material/import settings and perceived world scale require PIE judgment.
- Player and enemy Flipbook plane orientation, mirroring axis, transparent edges and sort order require the locked human PIE matrix.

### Human validation result/request

- PendingBeforeClose: in PIE select J_SPADE and a non-Spade player, then observe Grunt plus Shield/Bomber/Boss through movement, attack, hit and death. Confirm exactly one renderer, looping frames, facing, anchor/height, alpha/sort and preserved procedural feedback.
