# Plan 40 - Presentation - Data-driven 2D character presentation

## Coordination

- Planner owner: Codex.
- Executor owner: Codex, assigned directly by the programmer human on 2026-08-13.
- Plan authored by (AI side): `ReEcho teammate-side AI`.
- Implementation authored by (AI side): `ReEcho teammate-side AI`.
- Task status: `InProgress`.
- Human validation: `PendingBeforeClose`.
- Local planning / implementation base: `origin/main@ec0f5e3`, with the existing uncommitted animation WIP on `codex/animation-idle-walk-attack` preserved as implementation input rather than overwritten.
- Implementation branch: continue `codex/animation-idle-walk-attack` unless the human explicitly requests a clean worktree after the current WIP is safely committed or handed off.
- Depends on / Blocks: depends on closed Plan 39 and published Plan 38 attack-repeat semantics; blocks bulk authoring of the remaining player animation sets until the profile contract is stable.
- Writes: `Source/ReEcho/{Public,Private}/Presentation/Animation2D/**`, narrowly required player/enemy presentation and collision call sites, animation/collision-focused tests, animation import/repair/collision-generation tooling, `Content/2DAnim/**`, new `/Game/ReEcho/Animation2D/**` profile/catalog/collision-track assets, animation documentation and this Plan's Execution notes.
- Stable Reads: `characters.csv` `AppearanceId`, `FReEchoBuildSnapshot::CharacterId`, authoritative committed-attack callbacks, Plan 38 held-input retry behavior, existing collision/recording/GAS/save contracts and current artist-authored Flipbooks.
- Impact mode: C++ `SharedContract`; `Content/2DAnim/**` and `/Game/ReEcho/Animation2D/**` `Exclusive` while Editor asset writes are active.
- Compatibility promise / downstream action: gameplay actors emit presentation intent only; the authoritative animation clock may select pre-authored per-frame query geometry, but render pixels/alpha and Flipbook physics never directly decide damage, movement, blocking, death, recording or save identity. Existing uncommitted art and Flipbook work must be preserved, and `.uasset` edits occur only through Unreal Editor or reviewed Editor automation.
- Explicit exclusions: producing missing art for all states; changing character balance; changing weapon cadence or Plan 38 retry semantics; Echo animation migration; animation-notify-driven damage; replacing the stable movement Capsule with animated polygons; XLSX/CSV schema migration; renaming existing artist assets; redesigning procedural hit/death feel.

## Locked goal

Replace character-specific Flipbook fields and branches in gameplay actors with a reusable, data-driven 2D presentation layer in which an appearance/weapon profile owns composite character-plus-weapon clips, playback policy and matching per-frame collision tracks; a presentation controller owns visual state priority and authoritative frame selection; a collision driver applies separately authored body Hurtbox and weapon AttackHitbox query geometry; and the existing PaperFlipbook component remains a collision-free renderer.

## Locked acceptance

- [ ] Player animation selection resolves through `CharacterId -> characters.csv AppearanceId -> profile -> WeaponVisualSetId/composite animation set`; no animation resource is selected by a hard-coded `J_SPADE`, `J_HEART`, `J_CLOVER` or `J_DIAMOND` branch.
- [ ] Four production player appearances can each receive distinct Idle, Move, BasicAttack, Hit and Death clip configuration without adding fields, constructor asset finders, functions or branches to `AReEchoPlayerPawn`.
- [ ] A profile clip defines its composite character-plus-weapon Flipbook or static fallback, matching collision track, looping, play rate, restart policy, offset, scale/height policy and sort priority. Missing state clips follow a deterministic documented fallback chain and never make the actor invisible.
- [ ] A reusable presentation controller owns renderer mutual exclusion, facing, base locomotion state, one-shot override state, completion return and priority `Death > Hit > Action > Move > Idle`.
- [ ] `UReEcho2DAnimationComponent` is a pure collision-free renderer: it applies a requested clip faithfully, does not force looping, does not resolve character/weapon/gameplay state and consistently reapplies authored scale and facing after clip changes.
- [ ] The existing root Capsule remains the only movement sweep/blocking authority. Animated polygons never push actors, block navigation, alter arena clamping or change recorded positions.
- [ ] Every active animation frame may select an authored body Hurtbox polygon set and weapon AttackHitbox polygon set from the clip's collision track. Hurtboxes receive enemy attack queries; AttackHitboxes query enemies only during the authoritative attack-active window; both are Query-only.
- [ ] Composite visual alpha is never used directly as runtime collision. A reviewed Editor/import pipeline converts authored body/weapon masks or manual frame annotations into simplified, bounded collision geometry before runtime.
- [ ] Body and weapon regions remain distinct even though they share one rendered texture: weapon pixels cannot enlarge the player's Hurtbox, and body pixels cannot apply weapon damage.
- [ ] Collision-track validation rejects frame-count/key-frame, pivot, PixelsPerUnrealUnit, scale, source-revision or missing-frame mismatches. Facing mirrors visual and collision geometry from the same local origin.
- [ ] Gameplay call sites submit semantic presentation requests only. Held-input retries, cooldown waits and rejected attacks do not play an attack clip or activate AttackHitboxes; each formally committed attack may restart exactly one configured action clip and one attack collision timeline whether or not it ultimately hits.
- [ ] Existing Plan 39 Spade/Grunt behavior is migrated through profiles and remains visually available; Grunt can keep one default looping clip for all missing states.
- [ ] Static Billboard and Flipbook are never visible simultaneously. Unconfigured appearances retain the current static texture.
- [ ] Player/Grunt bob, lunge/pulse, hit shake/stretch and death shrink remain on the shared visual-effect root and remain visual-only.
- [ ] `AReEchoPlayerPawn` no longer owns `Spade*Flipbook`, `UpdateSpadeAnimationState`, `TransitionSpadeAnimationState`, `SequenceAttackRemaining` or per-character animation maps.
- [ ] Focused tests prove profile lookup, priority, fallback, one-shot completion, looping fidelity, facing/scale persistence, renderer mutual exclusion, committed-attack-only triggering, frame/Hurtbox synchronization, AttackHitbox active windows, mirroring and per-attack target deduplication; all Plan 38 held-attack tests remain green.
- [ ] UE 5.8 Editor build, full `ReEcho.*` automation, project validation, asset load/cook presence and `git diff --check` pass on the final source/content candidate.
- [ ] Human PIE validates all four configured players plus Grunt for identity, scale, pivot, facing, alpha/sort, state transitions and retained procedural feedback before closure.
- [ ] No generated UE products outside the curated `GIT_RULES.md` prebuilt allowlist or machine-local paths are committed.

## Public contract

The Executor may refine names, but must preserve these dependency directions and responsibilities.

### Semantic clip identity

- Use presentation semantic keys rather than character-specific enums. Gameplay Tags are preferred because later weapon/skill variants can extend without changing a central enum.
- Minimum keys: `Animation.Idle`, `Animation.Move`, `Animation.Attack.Basic`, `Animation.Hit`, `Animation.Death`.
- More-specific requests may fall back through an explicit chain, for example `Animation.Attack.Weapon.Staff -> Animation.Attack.Basic -> locomotion -> static fallback`.
- Do not register or consume a tag that changes gameplay activation semantics; these tags are presentation keys only.

### Clip, composite animation set and profile

- A clip contains the composite character-plus-weapon renderer asset, collision-track reference and playback/display policy. Looping and restart behavior belong to the clip, not to `ApplyFlipbook()`.
- A composite animation set is selected by `AppearanceId + WeaponVisualSetId` and contains semantic clips for that exact baked visual combination. The code cannot eliminate the artist cost of distinct baked combinations; it must keep that variation data-driven.
- A character profile is an Editor-authored DataAsset keyed by canonical `AppearanceId`; it contains the static fallback and weapon-animation-set map.
- A catalog/resolver maps `AppearanceId` to a profile. One catalog reference at the presentation boundary is acceptable; per-character hard references in gameplay actors are not.
- Profile/catalog lookup failure is non-fatal and preserves the actor's existing static appearance.

### Presentation controller

- Add a reusable component under `Presentation/Animation2D` that owns the static renderer/Flipbook handoff and presentation state arbitration.
- It accepts narrow calls equivalent to `ConfigureAppearance`, `SetMoving`, `PlayAction`, `PlayReaction`, `SetDead` and `SetFacing`; it must not accept or query PlayerPawn, EnemyActor, ASC, WeaponActor, recording or save objects.
- Base locomotion and one-shot overrides are separate. Completion of an action/reaction returns to the latest Idle/Move base state without gameplay polling inside the renderer.
- The component controls the renderer children only. Existing actors retain `VisualEffectRoot` and apply procedural transforms above the controller's renderers.

### Frame collision track and driver

- Add a `UReEcho2DFrameCollisionTrack` DataAsset paired to one Flipbook/source revision. Each key frame stores separate local-space body Hurtbox polygons and weapon AttackHitbox polygons.
- Add a collision driver under `Presentation/Animation2D` that receives the controller's authoritative clip time/frame and applies the corresponding pre-authored query geometry. It must not inspect texture pixels at runtime or enable PaperFlipbook `EachFrameCollision`.
- Keep the root Capsule for movement sweep and blocking. Hurtbox and AttackHitbox geometry use dedicated collision channels/responses and never block movement.
- AttackHitboxes are enabled only while both the committed attack instance and its data-authored active window are valid. Track a stable attack-instance ID and a per-instance hit set so one enemy is damaged at most once unless the attack definition explicitly permits repeated hits.
- Hurtboxes remain queryable during Idle, Move and Attack. Death/untargetable policy follows existing gameplay state rather than inferred visual pixels.
- The renderer, collision track and driver share the same local pivot, authored scale and facing sign. Programmatic bob/lunge/scale lives above them on `VisualEffectRoot` so rendered geometry and query geometry transform together without moving the Actor root.
- A missing/mismatched Hurtbox track falls back visibly and diagnostically to the existing Capsule for receiving hits. A missing/mismatched AttackHitbox track falls back to the existing weapon-range query; it must never silently create collision from the whole composite alpha.

### Authoring contract

- Preferred source is one reviewed collision mask per rendered frame with at least two semantic regions: body Hurtbox and weapon AttackHitbox. A deterministic color contract such as red=body, blue=weapon is acceptable once documented by the Executor.
- Manual polygon correction in an Editor tool is permitted and must save to the collision-track asset; runtime inference is prohibited.
- Import generation performs contour extraction, vertex simplification, convex decomposition where required, vertex/area limits and deterministic source-revision stamping.
- Generated polygons must be previewable over the source frame in Editor/debug PIE. Artists must be able to see body/weapon separation before acceptance.

### Gameplay boundary

- Player configuration passes the resolved `AppearanceId`, not a character-specific animation profile assembled in the Pawn.
- Player configuration also passes the stable weapon visual-set identity required to select the baked character-plus-weapon animation set.
- The attack presentation request is emitted only when the authoritative attack is formally committed. Plan 38 retry/cooldown/rejected paths emit no presentation or collision request. Hit/miss resolution remains downstream and does not control whether the committed animation plays.
- Enemy configuration supplies an appearance/profile key; Grunt's current one-clip behavior is represented as profile fallback rather than an EnemyKind branch inside the animation module.

## Step 0 gate

- Baseline branch/commit: `origin/main@ec0f5e3`; verify a fresh fetch before any later integration or publication.
- Engine/build availability: UE 5.8 installed/release build. Ask the human to save and close the Editor before build, commandlet, or scripted `.uasset` mutation.
- Existing focused-test result: Plan 39 passed on its closed baseline; Plan 38 passed on `origin/main`. Both evidence sets become stale once this refactor starts.
- Active exclusive ownership or shared-contract approval: before editing `.uasset`, confirm no active Exchange owner or Unreal lock conflicts with `Content/2DAnim/**` and `/Game/ReEcho/Animation2D/**`.
- Dirty-worktree preservation: the current branch contains user/artist asset changes, new `walk`/`attack` Flipbooks, animation code WIP and rule edits. Inventory exact paths before implementation; do not restore, delete, rename, stage or absorb unrelated files into the Plan by convenience.
- Prebuilt baseline: keep the remote Plan 38 bundle until the combined source is rebuilt. The pre-integration local bundle is retained in `stash@{0}` only as recovery material and is not validation evidence.
- Stop condition if the baseline is broken: stop if `AppearanceId`/weapon visual identity cannot be resolved without changing a stable data contract, if attack commit cannot be observed without changing combat semantics, if body and weapon cannot be unambiguously separated from available masks/annotations, or if required asset mutation would overwrite another owner's current Editor work.

## Implementation outline

1. **Stabilize and characterize the current WIP.** Inventory dirty animation source/assets, inspect the authoritative committed-attack call path after Plan 38, add regression characterization for current Spade/Grunt behavior, and remove unrelated formatting noise from Plan-owned C++ only.
2. **Repair the pure renderer contract.** Disable PaperFlipbook collision, replace forced `SetLooping(true)` with clip-owned policy, restore deterministic scale/facing application after every Flipbook change, expose playback completion/frame selection without gameplay knowledge, and test one-shot versus looping behavior directly.
3. **Introduce data contracts.** Add semantic presentation tags, composite clip/weapon-set definition, profile DataAsset, catalog/resolver and frame-collision-track types. Keep resource policy in profiles; do not add four sets of fields to the Pawn.
4. **Build collision authoring and validation.** Establish the body/weapon mask or manual-annotation contract, deterministic polygon generation/simplification, source revision checks and Editor/debug overlay before enabling runtime queries.
5. **Add the presentation controller and collision driver.** Centralize static/animated mutual exclusion, priority, base state, one-shot override, completion return, frame selection, facing, safe fallback and Query-only frame geometry. Keep `VisualEffectRoot` above renderer and collision geometry; keep the movement Capsule authoritative.
6. **Migrate Spade first.** Reproduce the current Idle/Walk/Attack result through its `AppearanceId + WeaponVisualSetId` set, bind matching Hurtbox/AttackHitbox tracks and delete Spade-specific state/resource code from the Pawn in the same coherent change.
7. **Migrate Grunt.** Represent its single looping animation and body Hurtbox track as profile fallback without teaching the animation module about `EReEchoEnemyKind::Grunt`; retain its existing gameplay attack query until an authored weapon/contact track exists.
8. **Prove four-character scalability.** Create/configure the four player profiles using available assets and explicit static/collision fallbacks for missing clips. Demonstrate that adding or changing a character/weapon set requires no Pawn C++ edit. Missing artist content remains visibly static and is recorded as art follow-up, not synthesized by the Executor.
9. **Integrate committed attack semantics.** Submit BasicAttack and its collision timeline only on actual attack commit; verify Plan 38 retries/cooldowns do not restart animation or enable AttackHitboxes, misses still animate, and repeated commits receive distinct deduplicated attack-instance IDs.
10. **Remove legacy seams.** Delete character-specific Flipbook fields/functions, duplicated state timers/maps and obsolete tests/docs only after replacement coverage passes.
11. **Verify and hand off.** Format changed C++, build, run focused then full automation, validate collision-track/asset loads, performance and cook inclusion, refresh the allowed prebuilt bundle only on the final integrated candidate, and request the locked PIE matrix.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py`; `git diff --check`; targeted `rg` for `Spade.*Flipbook|UpdateSpadeAnimationState|TransitionSpadeAnimationState` | Project invariants pass and player-specific animation seams are absent |
| Unit/automation | Animation profile/controller/collision-track tests | Appearance/weapon-set lookup, fallback, priority, completion, looping, scale/facing, frame synchronization, polygon validation and visibility pass without gameplay ownership leakage |
| Combat regression | Plan 38 held-repeat tests plus committed-attack presentation/collision test | Temporary lock/cooldown retries continue; rejected/retry attempts emit no attack animation/AttackHitbox; committed misses animate; repeated commits use distinct hit sets |
| Actor integration | Player/Grunt animation and collision integration tests | Four appearances configure without Pawn resource branches; Capsule movement remains stable; frame Hurtboxes follow body only; Grunt/static fallback behave correctly |
| Build | `.clang-format` on changed C++; `scripts/ue/Build-Editor.cmd` | UE 5.8 UHT/UBT exit code 0 |
| Full automation | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | Current full suite passes |
| Assets/cook | Editor load check and staged/cooked manifest inspection | Catalog, profiles, Flipbooks and sprite dependencies load and are retained |
| Publication readiness | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`; manifest/source fingerprint check | Final combined candidate owns a fresh valid curated prebuilt bundle |
| Performance | PIE/automation stress with representative simultaneous animated actors | Frame changes do not recreate blocking physics state; collision updates stay within an agreed frame budget and produce no overlap storm |
| Human | PIE: four players, repeated held attacks, missing-clip/collision fallback and Grunt with collision debug overlay | Identity, timing, size, pivot, facing, body/weapon separation, hit accuracy, alpha/sort and procedural feedback accepted |

## Executor handoff

Start from this Plan and the existing `codex/animation-idle-walk-attack` worktree. Treat every pre-existing dirty path as user work until this Plan explicitly lists it and the diff proves it belongs to the animation change. Do not create four hard-coded character implementations and do not enable PaperFlipbook `EachFrameCollision`. First make the renderer truthful, then add profile/controller/collision-track contracts and authoring preview, then migrate Spade and Grunt, and only then configure the other appearances. If the contract requires a character CSV/schema change, animation-notify-driven damage, ambiguous automatic body/weapon separation or overwriting an artist asset, stop that boundary and report to the Planner.

Report back with:

- exact changed paths and any retained pre-existing dirty paths;
- public contract deviations from this Plan;
- profile/catalog asset paths and `AppearanceId` mappings;
- collision-track paths, source revisions, generation/manual-correction evidence and fallback use;
- focused/full test counts, build/cook evidence and refreshed prebuilt status;
- remaining missing art and the exact static fallback used;
- the human PIE checklist still required.

## Execution notes

### Changed

- Preserved the dirty animation/art input in a recoverable stash and restored only Plan40-owned animation paths into the isolated `plan/40-data-driven-2d-presentation` worktree based on `origin/main@095a133`.
- Stabilized the incoming Idle/Walk/Attack WIP: corrected the Walk finder/member assignment, standardized the authored lowercase `walk.walk` and `attack.attack` package references, restored display-scale application after clip changes, removed an unused Walk texture-frame array and aligned focused tests/documentation with the new assets.
- Retained the explicit presentation decision priority `Attack > Walk > Idle`; Walk loops, Attack remains one-shot and repeated committed attacks restart from frame zero.
- Added the first data-owned renderer contract (`FReEcho2DAnimationClip`) with Flipbook, collision-track reference, looping, restart, play-rate, scale/height, offset and sort policy. `UReEcho2DAnimationComponent::PlayClip` now applies that policy faithfully and the obsolete force-looping helper was removed.
- Added `UReEcho2DFrameCollisionTrack` with separate per-frame body Hurtbox and weapon AttackHitbox polygons, attack-active flags, source revision/pivot/PPUU metadata, bounded polygon validation and Flipbook frame-count matching. Runtime queries remain deliberately disabled at this checkpoint.
- Added native semantic presentation tags plus `AppearanceId`-owned character Profile and project Catalog DataAsset contracts. Composite animation sets select clips by stable weapon `VisualKey`, fall back to the character default set and never resolve gameplay character IDs as asset identities.
- Added the presentation-only `UReEcho2DPresentationController` contract for exclusive static/Flipbook visibility, Idle/Move base state, one-shot action completion, death locking, weapon-set refresh, facing and safe static fallback. Actor migration remains pending, so no runtime behavior changed at this checkpoint.
- Migrated the Player presentation call site to the controller: character setup resolves CSV `AppearanceId`, equipped weapon exposes its data-authored `VisualKey`, movement submits Move/Idle intent, and a formally executed basic attack submits `Animation.Attack.Basic`. Removed the Pawn's Spade Flipbook fields, MoonStaff WeaponId comparison, attack animation timer and Spade-specific transition functions.
- Added a deterministic Editor Python asset-authoring script for five existing player Appearance profiles plus `/Game/ReEcho/Animation2D/DA_PresentationCatalog`. The intended Spade policy is static `Idel_01` for Idle, looping `walk` as the default Move clip, and one-shot `attack` only in the `MoonStaff` composite set; other current players use explicit static fallbacks.
- Migrated Grunt's renderer selection to the same controller/profile contract and removed its Actor-owned Flipbook reference/profile assembly. The Actor supplies the `Enemy.Grunt` profile at its existing enemy-kind configuration boundary; the animation module remains unaware of `EReEchoEnemyKind`, while non-Grunt enemies keep their existing Billboard path.

### Evidence

- UE 5.8 `ReEchoEditor` Win64 Development build passed on the isolated Plan40 worktree.
- `ReEcho.Presentation.Animation2D.AssetProfiles` passed with the new Walk and Attack packages loaded successfully.
- `git diff --check` passed for the stabilized candidate.
- The renderer/collision-contract checkpoint builds successfully; focused automation proves authored one-shot/play-rate fidelity plus matching-track acceptance and frame-count mismatch rejection.
- The Profile/Catalog/Controller checkpoint compiles under UE 5.8 UHT/UBT. Its focused test was extended for exact weapon-set lookup, default-set fallback and strict AppearanceId resolution.
- The sandboxed Editor launch was diagnosed as an execution-boundary issue rather than SDK failure; approved non-sandbox Editor-Cmd runs now enter the project normally. An earlier accidental full-suite invocation (caused by positional binding to `EngineRoot` instead of `Filter`) still exposed a pre-existing `HeldRepeat` access violation in `AReEchoWeaponActor::GetAttackInterval`; full-suite status remains pending.
- The Player/controller migration and its expanded transient controller tests compile under UE 5.8. Targeted source search finds no remaining `Spade.*Flipbook`, `UpdateSpadeAnimationState`, `TransitionSpadeAnimationState`, `SequenceAttackRemaining` or `MoonStaffWeaponId` seam in the Player Pawn.
- The Grunt/controller migration compiles under UE 5.8, and targeted source search finds no remaining `GruntDefaultFlipbook`, `GruntFlipbookFinder` or Actor-built legacy animation profile.
- Generated and saved seven cook-visible assets under `/Game/ReEcho/Animation2D`: five player Appearance profiles, `DA_Enemy_Grunt` and `DA_PresentationCatalog`. The authoring log reports `5 player profiles + Grunt + catalog` with no Python error.
- `ReEcho.Presentation.Animation2D.AssetProfiles` passes after loading the saved assets from disk and verifying catalog resolution, Spade static Idle/default looping Move/MoonStaff one-shot Attack, Grunt looping default, controller renderer exclusivity and one-shot completion return.

### Remaining risks

- This checkpoint still contains the Plan39 actor-specific Spade seams; the profile/catalog/controller migration and frame-collision contract remain pending Plan40 work.
- Profile/Catalog assets now exist and load in automation. Human PIE still must accept visual scale, pivot, facing, alpha/sort and the static/animated transitions.
- Visual identity, pivot, scale and timing for the new artist-authored Walk/Attack assets remain human PIE acceptance items.

### Human validation result/request
