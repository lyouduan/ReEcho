# Plan 13 — GAS architecture migration

## Goal 🔒

Migrate ReEcho from a thin `UGameplayAbility` activation wrapper to an authoritative GAS combat path based on ASC, AttributeSet, GameplayEffect, Gameplay Tags and Ability Tasks, while preserving the deterministic recording/echo contract and current single-player vertical-slice behavior.

## Current gap

- `AReEchoPlayerPawn` owns and initializes an ASC, implements `IAbilitySystemInterface`, and grants five abilities, but activation is still by concrete class.
- Every ability immediately calls a Pawn method and ends; none calls `CommitAbility`, owns cooldown/cost policy, uses tags, applies effects, or launches an Ability Task.
- Health, block, attack stats and death remain authoritative in `UReEchoCombatantComponent`; weapons apply damage and cooldown outside GAS.
- Basic attack hold retries activation every Pawn tick.
- The current GAS test only proves inheritance/ASC presence; it does not verify effects, attributes, cooldowns, tags or lifecycle.

## Step 0 gates 🔒

1. Confirm network target before moving ASC ownership:
   - Current target: single-player vertical slice, keep ASC on Pawn.
   - If persistent multiplayer possession/respawn becomes a requirement, move the player ASC to PlayerState in a separate migration and initialize ActorInfo from possession/replication callbacks.
2. Preserve `FReEchoBuildSnapshot` as run-persistent design state. Runtime combat values must be initialized from it into GAS once per encounter; do not keep two independently writable runtime stat authorities.
3. Preserve deterministic recording: record an active-skill event only after a successful ability commit/execution. Never serialize automatic basic attacks.

## Acceptance 🔒

### 🤖 Automated

- [x] Player ASC has a registered `UReEchoCombatAttributeSet` containing Health, MaxHealth, Block, PhysicalAttack, ElementalAttack, AttackSpeed, MovementSpeed and EchoEfficiency.
- [x] Encounter initialization applies build values through one initialization GameplayEffect/spec path.
- [x] Damage and healing use GameplayEffects; Health clamps to `[0, MaxHealth]`, Block is consumed before Health, and death fires exactly once.
- [x] Basic and active attack abilities call `CommitAbility`, end on every success/failure/cancel path, and reject activation during their cooldown or blocked states.
- [x] Input activates abilities by Gameplay Tags/spec handles rather than concrete ability classes.
- [x] Held basic attack uses an Ability Task/timer owned by the ability, not Pawn Tick retries.
- [x] Existing weapon selection, recording timeline, echo playback, trait/shop build mutation and HUD tests remain green.
- [x] Add automation covering attribute clamping, damage/block/death, heal, cooldown rejection, blocked tags, input-tag lookup, cancellation cleanup and successful-recording semantics.
- [x] Editor build, all `ReEcho.*` automation, static validation and `git diff --check` pass.

### 🎮 Human PIE

- [ ] Basic attack hold cadence matches or improves the current feel at several AttackSpeed values.
- [ ] Active skill cooldown feedback and blocked input feel responsive.
- [ ] Health HUD, damage numbers, death screen and encounter transitions show no duplicate or stale events.
- [ ] Weapon switching and echo playback remain visually and temporally consistent.

## Target architecture 🔧

```text
Input action
  -> Input Gameplay Tag
  -> ASC ability spec lookup / pressed-released notification
  -> GameplayAbility::CanActivateAbility
  -> ActivateAbility
       -> CommitAbility (cost + cooldown)
       -> AbilityTask / weapon execution service
       -> GameplayEffectSpec to target ASC
  -> AttributeSet::PostGameplayEffectExecute
       -> clamp / block / damage / death event
  -> attribute-change delegates
       -> HUD, movement speed, death flow, recorder
```

### Ownership and runtime truth

- Keep `UAbilitySystemComponent` on `AReEchoPlayerPawn` for this single-player phase.
- Add `UReEchoCombatAttributeSet` beside the ASC and expose read-only getters.
- `FReEchoBuildSnapshot` remains persistent run data. `GE_InitializeCombatStats` copies it into GAS at encounter start.
- Convert `UReEchoCombatantComponent` into a temporary compatibility adapter that reads/delegates to ASC attributes and forwards existing `OnHealthChanged`/`OnDeath`. Remove its writable `Stats`/`CurrentHealth` authority after callers migrate.
- Enemies need an ASC + the same combat AttributeSet before they can receive authoritative GAS damage. Echoes can initially keep their playback orchestration but should read the same build-derived attributes; a full echo-ability migration is a later step.

### AttributeSet

Add `Source/ReEcho/Public/AbilitySystem/ReEchoCombatAttributeSet.h` and paired implementation:

- Persistent/current: `Health`, `MaxHealth`, `Block`, `PhysicalAttack`, `ElementalAttack`, `AttackSpeed`, `MovementSpeed`, `EchoEfficiency`.
- Meta attributes: `IncomingDamage`, optionally `IncomingHealing`.
- Use `GAMEPLAYATTRIBUTE_*` accessors.
- Clamp in `PreAttributeChange` for MaxHealth-dependent current Health and in `PostGameplayEffectExecute` for final Health/Block results.
- Consume Block before applying IncomingDamage; broadcast one project-level death event only on the alive-to-dead transition.
- Subscribe UI/gameplay through `GetGameplayAttributeValueChangeDelegate`, not direct property mutation.

### Gameplay Tags

Define native tags in `ReEchoGameplayTags.h/.cpp` (or `Config/Tags/ReEchoAbilityTags.ini` if designers must edit them):

- `Ability.Attack.Basic`, `Ability.Attack.Active`, `Ability.Weapon.Switch`
- `Input.Attack.Basic`, `Input.Attack.Active`, `Input.Weapon.1/2/3`
- `State.Dead`, `State.Attacking`, `State.Stunned`, `State.Menu`
- `Cooldown.Attack.Basic`, `Cooldown.Attack.Active`
- `Damage.Type.Physical`, `Damage.Type.Elemental`
- `Data.Damage`, `Data.Heal`
- Reserve `GameplayCue.Attack.*` and `GameplayCue.Hit.*` for later cosmetic routing.

Use activation blocked tags so Dead/Stunned/Menu gates are declarative rather than scattered conditionals.

### GameplayEffects

Prefer native GE classes/spec builders first to avoid merge-hostile `.uasset` work; switch to Blueprint GE assets only when designers need tuning.

- `GE_InitializeCombatStats`: instant override from `FReEchoBuildSnapshot` SetByCaller magnitudes.
- `GE_Damage`: instant effect carrying `Data.Damage`; AttributeSet resolves Block and Health.
- `GE_Heal`: instant additive healing with clamping.
- `GE_BasicAttackCooldown`, `GE_ActiveAttackCooldown`: duration effects granting matching cooldown tags.
- Trait/shop changes: rebuild/reapply the initialization effect between encounters, or apply explicit infinite modifier effects if mid-encounter mutation is later required.
- Use `UGameplayEffectExecutionCalculation` only when physical/elemental/defense formulas become multi-input; do not add it for a single SetByCaller subtraction.

### Abilities and input

- Add an ability-set descriptor mapping Ability class + level + input tag; grant it once on authority and retain handles for cleanup.
- Replace `TryActivateAbilityByClass` with input-tag lookup and ASC `AbilitySpecInputPressed/Released` plus activation.
- Base ability lifecycle:
  1. validate actor/weapon/state;
  2. `CommitAbility` before irreversible gameplay;
  3. execute or launch task;
  4. call `EndAbility` on every terminal path;
  5. cancel safely and clear callbacks/tasks.
- Basic hold: activate once on press, use `UAbilityTask_WaitInputRelease` plus a cancellable attack-loop task/timer based on AttackSpeed. Remove Tick activation retry.
- Active attack: commit, call the existing weapon execution service, broadcast/record only on success, then end.
- Weapon switching may remain instant abilities without cooldown, but still use input tags and blocked-state tags.

### Damage target integration

- Projectile/melee hit code resolves `IAbilitySystemInterface` on the target and applies a damage spec to its ASC.
- Keep collision, projectile travel and hit detection in actors/weapons; GAS owns permission, cooldown, attributes and effect settlement.
- Damage numbers and hit effects subscribe to the applied result/delegate so blocked or zero damage remains visually silent.

## Implementation order 🔧

1. Add native tags, AttributeSet and initialization/damage/heal GE classes; no caller migration yet.
2. Register AttributeSet on player ASC and initialize ActorInfo reliably; apply build snapshot at encounter start.
3. Turn `UReEchoCombatantComponent` into a GAS-backed compatibility adapter and keep existing HUD/death delegates stable.
4. Give enemies ASC/AttributeSet and route projectile/melee damage through GE specs.
5. Add ability-set/input-tag granting and replace by-class activation.
6. Add `CommitAbility`, cooldown effects and blocked/owned tags to attack abilities.
7. Replace Pawn Tick hold-fire with Ability Tasks and verify cancellation/pause/death behavior.
8. Route successful active ability commit back to recorder; verify echo playback contract.
9. Expand automation, run Editor build/tests, then perform human PIE tuning.
10. After all callers migrate, remove duplicate writable combat state and document the GAS runtime contract.

## Files expected to change 🔧

- `Source/ReEcho/AbilitySystem/ReEchoPlayerAbilities.*`
- `Source/ReEcho/AbilitySystem/ReEchoCombatAttributeSet.*` (new)
- `Source/ReEcho/AbilitySystem/ReEchoGameplayTags.*` (new)
- `Source/ReEcho/AbilitySystem/ReEchoGameplayEffects.*` (new, if native GE classes are selected)
- `Source/ReEcho/Player/ReEchoPlayerPawn.*`
- `Source/ReEcho/Combat/ReEchoCombatantComponent.*`
- `Source/ReEcho/Graybox/ReEchoEnemyActor.*`
- Projectile/weapon damage callers
- `Source/ReEcho/Private/Tests/ReEchoAbilitySystemTests.cpp` (new)
- `shared/CODEBASE_MAP.md`, `shared/PROJECT_STATE.md`, this plan execution notes

## Risks and rollback points

- Highest risk is dual authority between CombatantComponent and AttributeSet. Migrate through one adapter and forbid direct writes once the adapter is active.
- Cooldown migration can change weapon feel. Capture current cadence first and use it as an automated baseline.
- Local prediction is not useful unless effects and ActorInfo lifecycle are prediction-safe. For the current single-player target it may remain configured, but do not claim multiplayer readiness until authority/prediction tests exist.
- Moving ASC to PlayerState is intentionally excluded until persistent possession/respawn or multiplayer is a confirmed requirement.
- Keep each numbered implementation step buildable so the branch can roll back at phase boundaries.

## Execution notes

- Runtime implementation completed through Step 9 at source level: native tags, AttributeSet, initialization/damage/heal/cooldown effects, GAS-backed Combatant adapter, player/enemy ASC ownership, source-aware damage, input-tag ability specs, CommitAbility lifecycle, menu/death blocking tags, and AbilityTask-held basic attacks.
- Echoes intentionally remain on the documented compatibility path so deterministic playback is not expanded into a second ability-authority migration.
- Weapon execution was split from its legacy timer: player abilities use GAS cooldowns while echoes retain the old timer until their later full GAS migration.
- Added focused GAS tests for initialization, clamping, block, damage, healing, death-state idempotency and ability tags.
- Verification: ReEchoEditor Win64 Development build succeeded after UE 5.8 signature/component-construction corrections; all ten `ReEcho.*` tests, `scripts/validate_project.py`, and `git diff --check` pass.
- Added a real temporary-world GAS fixture because ASC requires registered component lifecycle and valid ActorInfo; the fixture destroys its WorldContext after each test.
- Remaining validation is human PIE for attack cadence, cooldown feel, HUD/death flow and echo timing.
- Based on Epic UE 5.8 GAS documentation and the current ReEcho source audit.