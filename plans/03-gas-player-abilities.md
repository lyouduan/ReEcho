# Plan 03 - Gameplay - GAS player abilities

## Locked goal

Route player-triggered combat and weapon-selection abilities through Unreal Gameplay Ability System while preserving the current graybox combat, recording, echo, and UI behavior.

## Locked acceptance

- [x] Player owns an initialized `UAbilitySystemComponent` and implements `IAbilitySystemInterface`.
- [x] Basic attack, active attack, and weapon slots 1/2/3 activate dedicated `UGameplayAbility` classes.
- [x] Input functions no longer invoke the weapon actor directly.
- [x] Existing cooldown, damage, weapon recording, and active-skill recording behavior remains intact.
- [x] UE 5.8 installed-build compilation and ReEcho automation pass.
- [x] No generated products or machine-local engine paths are committed.

## Step 0 gate

- Baseline branch/commit: `main` at `cc70122`.
- Engine version and build availability: UE 5.8 installed/release build.
- Existing automated test result: two ReEcho recording tests passed on main.
- Claimed merge-hostile assets/resources: `ReEcho.uproject` plus generated 2D character texture assets. Scope expanded after explicit user requests; no `.umap` was added.
- Compatibility boundary: `UReEchoCombatantComponent` remains the health/stat source. Gameplay Effects and an AttributeSet are deferred to a separate migration.

## Implementation outline

1. Enable GameplayAbilities and add GameplayAbilities/GameplayTags/GameplayTasks module dependencies.
2. Add player ability classes for basic attack, active attack, and three weapon slots.
3. Initialize the player ASC, grant startup abilities, and route input activation through GAS.
4. Preserve weapon execution and recording behind pawn execution methods called by abilities.
5. Add GAS structure automation and preserve recording tests.
6. Integrate the user-requested 2D actor presentation, collision/health-bar fixes, packaged pause menu, and Windows Shipping verification accumulated during implementation.

## Verification matrix

| Layer | Command/check | Evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | JSON/workflow checks pass |
| Build | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT exit code 0 |
| Automation | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | Three tests pass |
| Packaging | clean Windows Shipping `BuildCookRun` | Four 2D actor textures present; EXE launch smoke test passes |
| Human | iterative PIE/package feedback | User approved closing and merging current state |

## Execution notes

### Changed

- Enabled GAS and routed basic attack, active attack, and weapon slots 1/2/3 through dedicated abilities.
- Added 2D player, echo, grunt, and boss Billboard visuals, soft ground shadows, debug-only bounds, and compact anchored health bars.
- Replaced enemy sphere hit volumes with sprite-height capsules and changed projectiles to swept path collision.
- Added an Esc pause menu with resume, full restart, and packaged-build quit; death exposes restart and quit.
- Replaced conditional runtime monster texture loads with Cooker-visible CDO hard references.

### Evidence

- UE 5.8 ReEchoEditor Development and Windows Shipping builds succeeded.
- `ReEcho.GAS.PlayerAbilityStructure`, `ReEcho.Recording.CapturesWeaponTimeline`, and `ReEcho.Recording.InterpolatesAndHolds` passed.
- A clean Windows Cook includes `Player2D`, `Echo2D`, `Grunt2D`, and `Boss2D`.
- The fixed Shipping package remained responsive during a 12-second launch smoke test.
- User iterated on Billboard scale, health-bar anchoring, collision placement, projectile damage, menu behavior and packaging, then requested final merge.

### Remaining risks

- Health, damage, cooldown costs, and gameplay effects are not yet migrated to `UAttributeSet`/`UGameplayEffect`.
- Projectile damage and menu actions still lack deterministic automation; packaged interaction remains a human regression check.

### Human validation

- User approved closing and merging the current playable state after the fixed Windows Shipping build and resource verification.