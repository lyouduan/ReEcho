# Plan 40 - Presentation - Data-driven 2D character presentation

## Coordination

- Planner owner: Codex.
- Executor owner: Codex, assigned directly by the programmer human on 2026-08-13.
- Plan authored by (AI side): `ReEcho teammate-side AI`.
- Implementation authored by (AI side): `ReEcho teammate-side AI`.
- Task status: `Review`.
- Human validation: `PendingBeforeClose`.
- Local planning / implementation base: `origin/main@ec0f5e3`, with the existing uncommitted animation WIP on `codex/animation-idle-walk-attack` preserved as implementation input rather than overwritten.
- Implementation branch: continue `codex/animation-idle-walk-attack` unless the human explicitly requests a clean worktree after the current WIP is safely committed or handed off.
- Depends on / Blocks: depends on closed Plan 39 and published Plan 38 attack-repeat semantics; blocks bulk authoring of the remaining player animation sets until the profile contract is stable.
- Writes: `Source/ReEcho/{Public,Private}/Presentation/Animation2D/**`, narrowly required player/enemy presentation and collision call sites, animation/collision-focused tests, animation import/repair/collision-generation tooling, `/Game/ReEcho/Art/Animation2D/**` runtime Texture/Sprite/Flipbook assets, `/Game/ReEcho/Animation2D/**` profile/catalog/collision-track assets, `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`, animation documentation and this Plan's Execution notes. Legacy runtime copies under `Content/2DAnim/**` and erroneous `.uasset` imports under `Content/SourceArt/**` are removed only after references migrate; PNG source art remains.
- Affected architecture IDs: `MOD-ReEcho` / `AREA-Presentation`; no Runtime Module addition or dependency-topology change.
- Module documentation synchronization: maintain `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`; review `shared/CODEBASE_MAP/ARCHITECTURE.md` and `shared/CODEBASE_MAP/README.md` on the final candidate.
- Stable Reads: `characters.csv` `AppearanceId`, `FReEchoBuildSnapshot::CharacterId`, authoritative committed-attack callbacks, Plan 38 held-input retry behavior, existing collision/recording/GAS/save contracts and current artist-authored Flipbooks.
- Impact mode: C++ `SharedContract`; `/Game/ReEcho/Art/Animation2D/**` and `/Game/ReEcho/Animation2D/**` `Exclusive` while Editor asset writes are active.
- Compatibility promise / downstream action: gameplay actors emit presentation intent only; the authoritative animation clock may select pre-authored per-frame query geometry, but render pixels/alpha and Flipbook physics never directly decide damage, movement, blocking, death, recording or save identity. Existing uncommitted art and Flipbook work must be preserved, and `.uasset` edits occur only through Unreal Editor or reviewed Editor automation.
- Explicit exclusions: producing missing art for all states; changing character balance; changing weapon cadence or Plan 38 retry semantics; Echo animation migration; animation-notify-driven damage; replacing the stable movement Capsule with animated polygons; XLSX/CSV schema migration; renaming existing artist assets; redesigning procedural hit/death feel.

## Locked goal

Replace character-specific Flipbook fields and branches in gameplay actors with a reusable, data-driven 2D presentation layer in which an appearance/weapon profile owns composite character-plus-weapon clips, playback policy and matching per-frame collision tracks; a presentation controller owns visual state priority and authoritative frame selection; a collision driver applies separately authored body Hurtbox and weapon AttackHitbox query geometry; and the existing PaperFlipbook component remains a collision-free renderer.

### Human-approved acceptance amendment (2026-08-13)

The human explicitly replaced the final clause above for the currently matched character/enemy sequences: the active PaperFlipbook may use baked `EachFrameCollision` as `QueryOnly` body contour geometry. The root Capsule remains the sole movement/blocking authority, automatic overlap events stay disabled, and semantic weapon damage still requires the independent Body/Weapon frame-track contract. This amendment supersedes only the original prohibition on PaperFlipbook query collision; every gameplay-authority separation remains locked.

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
- Active exclusive ownership or shared-contract approval: before editing `.uasset`, confirm no active Exchange owner or Unreal lock conflicts with `/Game/ReEcho/Art/Animation2D/**` and `/Game/ReEcho/Animation2D/**`.
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
- Added `UReEcho2DFrameCollisionDriver` as the authoritative Flipbook-frame collision clock. It validates clip/track identity and revision, follows PaperFlipbook key-frame selection, applies authored pivot/PPUU and facing mirroring, and exposes immutable local-space Query-only body/weapon polygon snapshots without creating blocking physics state.
- Wired one Driver into Player and Enemy actors. Player allocates a monotonic presentation attack-instance ID only after the existing weapon execution reports a committed attack; Controller opens authored AttackHitboxes only for that instance on track-authored active frames and closes it on one-shot completion. Missing/mismatched tracks expose a diagnostic empty snapshot so existing Capsule/weapon queries remain the safe fallback.
- Added local point-in-polygon query APIs, finite/non-zero-area polygon validation and stale attack-instance rejection. Hurtboxes remain available independently of attack state; weapon polygons are hidden unless both attack-instance and authored frame gates are true.
- Added the review overlay `reecho.Animation2D.DrawFrameCollision`: mode 1 draws current Body polygons green; mode 2 also draws active Attack polygons red. Overlay vertices use the renderer component transform after authored pivot/PPUU/facing conversion and do not mutate Actor, Capsule or gameplay collision.
- Added deterministic reviewed-JSON-to-DataAsset tooling at `scripts/ue/build_plan40_collision_tracks.py` plus the annotation schema/workflow documentation. The generator requires exact Flipbook frame count, explicit body/weapon polygons and a non-empty source revision; it deliberately rejects missing annotations instead of inferring composite alpha geometry.
- Persisted the human-approved Paper2D `EachFrameCollision` mode for `walk`, `attack` and Grunt `01_2` through a repeatable Unreal Python asset script. The renderer enables that geometry only as `QueryOnly`, retains Pawn query response, disables unused automatic overlap events and keeps the Actor Capsule as blocking authority.
- Replaced the misleading aggregate Paper2D AABB debug view with exact Box/Sphere/Capsule/Convex wireframes and centralized `ReEcho.DebugCollision` into levels 0-3 so root collision, Paper2D geometry and semantic tracks can be inspected independently.
- Added Rabbit Doll and Goat Priest as appearance-owned enemy Profiles rather than new gameplay kinds: each Profile owns its static fallback plus looping Idle/Move clips, while the existing Grunt visual variant selects the matching Profile. The updated player `walk` asset remains behind the existing Spade `Animation.Move` semantic key.
- Human-directed minimal validation simplification: non-Boss enemy presentation now cycles only Grunt/Rabbit/Goat, removes static enemy texture authority, and gives every enemy Profile exactly one looping Idle clip. Gameplay EnemyKind and Boss presentation remain unchanged.
- Added Fox as the fourth minimal-validation appearance: its base semantic resolves to looping Walk, while the existing committed attack gate requests a one-shot Attack and the controller returns to Walk on playback completion. Damage timing remains gameplay-owned and does not use animation notifications.
- Plan42 倾斜正交相机的 PIE 复验暴露了动画渲染器的固定朝向假设：静态 Billboard 会自动面向镜头，但启用 Flipbook 后静态渲染器被隐藏，`UPaperFlipbookComponent` 仍保持写死旋转，导致玩家和非 Boss 敌人只剩血条/VFX。Animation2D 现根据实际 `PlayerCameraManager` 每帧更新 Flipbook 世界旋转，使本地 Y 法线朝向镜头、本地 Z 对齐屏幕 Up；该修复只改变表现朝向，不改变 Actor、碰撞或玩法状态。
- 相机朝向修复后 PIE 暴露白色底块。检查 Goat/Rabbit/Fox/FoxAttack 源 PNG 均为 RGBA 且包含大量透明像素，因此没有破坏性修改源图；根因收敛到 PaperSprite 关键帧材质引用未稳定消费 Alpha。Animation2D Renderer 现在统一覆盖为 Paper2D `TranslucentUnlitSpriteMaterial`，保留真实白色绘制内容，只让源 Alpha 控制透明背景。
- 用户以“80°俯视完整场景・高密度人怪精确比例 v5”图片覆盖上一条口述比例：人物 `100 UU / 1.0`、史莱姆/当前 Grunt `80 UU / 0.8`、Rabbit `140 UU / 1.4`、Fox `200 UU / 2.0`、Goat/Boss `220 UU / 2.2`。源 PNG Alpha 有效高度实测约 `777–829 px`，不能直接套图片中的目标有效高度或重复乘 PPUU；因此 Appearance Profile 直接持有目标 `WorldHeight`，Controller 将同一 Profile 的全部 Clip 归一化到该高度，确保狐狸 Walk/Attack 和羊两种形态同尺寸。图片给出的 `PPUU=2.0` 保留为后续美术规范，不作为第二套运行时倍率。
- 玩家和敌人的接触阴影改挂到表现层 `FootShadowAnchor`，挂点位于当前外观目标高度的下缘，GroundShadow 自身保持零相对位移。外观尺寸变化只更新挂点和阴影宽度，不改变 Actor 根、Capsule、Flipbook 状态或战斗逻辑。
- 为增强脚底压地感，在同一挂点增加约外层 `70%` 尺寸、向上偏移 `0.2 UU` 的 `GroundShadowCore`。内外层复用同一软阴影纹理与材质，透明叠加只加深中心并保留外圈柔边；微小高度差避免共面闪烁。
- 状态选择从 Controller 内部写死规则迁移为 `UReEcho2DAnimationStateMachineAsset`：每个状态以 GameplayTag 声明语义、优先级、单次播放锁、完成目标和 Terminal 规则，Profile 引用状态机资产但继续按 Appearance/WeaponVisualSet 解析同帧人物武器合成 Flipbook。Controller 保留原调用API作为运行时FSM执行器，Death > Hit > Attack > Move > Idle 的中断策略由资产数据决定；状态机不产生伤害、移动或碰撞结果。Editor Preview 改为引用真实 Profile/FSM，单独 Flipbook仅作缺Profile回退。
- 第一阶段生成并绑定 `/Game/ReEcho/Animation2D/SM2D_DefaultCharacter`，覆盖现有5个玩家Profile与Grunt/Rabbit/Goat/Fox Profile；Spade/Grunt Editor Preview改用真实Profile。聚焦自动化 `ReEcho.Presentation.Animation2D.AssetProfiles` 已通过，包含Hit打断Attack、Attack不可反向打断锁定Hit、单次播放返回、目标高度归一化及逐帧Query碰撞。Visual Blueprint Prefab承载完整可扩展组件树仍为下一迁移阶段，本阶段未宣称已把宿主视觉组件物理搬出Player/Enemy。
- Visual Blueprint Prefab 迁移阶段已建立 `AReEcho2DVisualPrefabActor` 及 5 个玩家、4 个敌人 Blueprint 资产，Profile 通过 `VisualPrefabClass` 选择同一套运行时/Editor 表现树；宿主仅转发表现意图，缺少 Prefab 时继续走旧组件安全回退。Prefab 暴露 Body、Foot、CenterFX、HeadFX、Projectile、UI 与 SceneLighting 插槽；`bAutoLayoutAnchors=false` 时保留美术在 Blueprint 中编辑的挂点 Transform。Level00 的 EditorOnly Preview 只保留一个 ChildActor 并直接实例化 Profile 指定 Prefab，不再复制 Flipbook/FSM/阴影；类未变化时 Construction 不重建 ChildActor，避免调整属性后累积重复实例。
- Review 修正：Player/Enemy 在 `EndPlay` 显式销毁 SpawnActor 创建的 Visual Prefab，避免 Enemy `SetLifeSpan` 后遗留孤立表现 Actor；Enemy Host 不再把旧 GroundShadow 比例回写 Prefab，阴影尺寸由各 Blueprint Prefab/美术配置权威。
- 阴影 Editor 调整修正：`bAutoLayoutShadow=true` 时继续通过 `ShadowScale/ShadowCoreRatio` 自动维护双层阴影；关闭后 `GroundShadow/GroundShadowCore` 的位置、旋转与缩放完全采用 Blueprint 组件 Transform，Construction/Configure 不再覆盖美术调整。
- 角色整体尺寸统一由 Visual Prefab Blueprint 的 `VisualScale` 调整：Profile `WorldHeight` 保持外观基础标准高度，`VisualScale` 只缩放 `VisualRoot`，使 Body、全部挂点和双层阴影同步变化；不得再分别缩放 Flipbook 与阴影来修改整体尺寸。该表现倍率不改变 Capsule、移动、攻击范围或伤害。
- 用户批准的最终表现树改造：Visual Prefab 删除 StaticRenderer，只保留 FlipbookRenderer；默认 FSM 收敛为 Idle、Move/Walk、Attack.Basic、Hit 四状态并以 Idle 启动。小怪暂时把同一条整体 Flipbook 显式绑定到四个语义槽，保留未来逐状态替换接口；Fox/Spade 使用已有独立 Attack 资产。
- Visual Prefab 拆分 `CharacterFacingRoot` 与 `GroundVisualRoot`：Flipbook 每帧与当前相机正交，FootAnchor/双层阴影保持水平地面空间；`VisualScale` 仍同步角色与阴影尺寸，不改变玩法碰撞。
- 玩家/敌人根碰撞从代码写死 Capsule 改为 Blueprint 可编辑 Box。新增 `/Game/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay` 与 Grunt/Rabbit/Goat/Fox 四个敌人 Gameplay Blueprint，GameMode 在新遭遇和存档恢复时实际按外观选择这些类；C++ 不再按外观高度/半径覆盖碰撞盒。为使 Blueprint 可保存，宿主阴影动态材质从 CDO 构造阶段迁移到实例 BeginPlay。
- 最终评审推翻了中间态“Gameplay Host 运行时再生成 Visual Prefab Actor”的双 Actor 结构。Player/Enemy 及其 Gameplay Blueprint 直接拥有唯一组件树：`Collision -> PresentationRoot -> { FlipbookRoot -> FlipbookRenderer, GroundRoot -> GroundShadow/GroundShadowCore, EffectsRoot -> VFX }`；当前范围不包含 UI 节点。Profile 只保存 FSM/Clip/比例数据，不再引用 Actor 类，GameMode 与 Editor Preview 也只实例化真实 Gameplay Blueprint。
- `PresentationRoot` 成为 Blueprint/关卡实例的整体表现 Transform 权威，运行时不再重写。bob、攻击前冲、受击和死亡表现只作用 `FlipbookRoot` 与 `EffectsRoot`；`GroundRoot` 使用绝对旋转并不参与这些程序化位移，保证倾斜正交相机下人物面向镜头而脚底阴影仍与水平场地平行。根 Box Collision 始终独立于表现缩放。
- Editor Preview 改为单一 ChildActor 预览真实 `BP_PlayerGameplay` / `BP_EnemyGameplay_Grunt`，避免同一个怪物同时出现 Prefab Preview 与 Flipbook Preview，也避免调整属性时重复生成角色子 Actor。旧 Visual Prefab 类型和资产未做破坏性删除，仅作为无引用恢复材料保留。
- 经程序用户确认清理后，已通过 Unreal 资产系统注销并删除 `/Game/ReEcho/Animation2D/VisualPrefabs/**`，并移除无引用的 `ReEcho2DVisualPrefabActor` C++ 类型。恢复材料不再保留；Gameplay Blueprint 是唯一允许继续维护的角色/怪物表现资产。
- 角色不可见修复：资产审计确认 `BP_PlayerGameplay` 的 `FlipbookRenderer` 被误存为 `Z=520`、`HiddenInGame=true` 且预览为 Spade Attack。生成器现确定性复位 Renderer 为局部原点、单位缩放、可见，并恢复对应 Gameplay Blueprint 的 Idle 预览；美术整体/局部校正只能编辑 `PresentationRoot` / `FlipbookRoot`。Controller 在 Profile/语义暂时解析失败时保留 Blueprint fallback Flipbook，并输出 Actor、语义和资产路径诊断，不再无提示地清空角色。
- 二次不可见修复：`BP_PlayerGameplay` 的继承组件 Override 仍残留 `FlipbookRoot Z=360/Pitch=-45`、Renderer 朝向错误和 Spade Walk。修复脚本通过 Blueprint Subobject 模板而非仅改 CDO，复位 FlipbookRoot、恢复 Cat Idle 与面向相机的 Renderer 朝向；Controller 不再把 Blueprint CDO 构造期（此时继承 Override 尚未应用）误报成运行时缺资源错误。
- PIE 隔离修复：Editor Preview 的 ChildActor 设为 HiddenInGame，并在 PIE/Game 的 Construction 与 BeginPlay 中清空 ChildActorClass、销毁子 Actor 和 Preview 宿主，保证 Editor 可见预览不会出现在真实战斗画面。
- 按验收要求从 Level00 移除 `EditorPreview_Player`，避免默认 Cat 预览与运行时所选角色同时出现；Editor 仅保留一个敌人预览，玩家表现改为直接编辑 `BP_PlayerGameplay` 蓝图。
- 场景相对关系修复：移除玩家 `Z=112`、怪物出生 `Z=50` 等固定世界高度。Arena 将 MapRoot 局部 `GameplayPlaneZ` 转为世界脚底高度，Player/Enemy 中心统一为该高度加 Blueprint Box 半高，恢复存档后也重新贴合当前地图平面。SceneLighting 以同一 Actor 脚点写入动态透明排序；Editor Preview ChildActor 标记为 Visualization 并在 BeginPlay 销毁，禁止预览角色的碰撞/AI/FSM 泄漏进 PIE。

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
- The focused Animation2D automation also passes frame synchronization, pivot/PPUU conversion, facing mirror, Query-only body lookup, committed/stale attack-instance gating, matching-instance closure and diagnostic missing-track fallback. UE 5.8 Editor build passes with Player/Grunt Driver components wired.
- UE 5.8 Editor build passes with the collision debug overlay. The collision generator passes Python bytecode compilation, project static validation and `git diff --check`.
- UE 5.8 Editor build passes after the EachFrame/query-policy and exact-shape debug changes. `ReEcho.Presentation.Animation2D.AssetProfiles` passes after a fresh disk reload and now verifies all three Flipbooks use EachFrame mode, every key frame references a PaperSprite with non-empty BodySetup geometry, static Idle disables Paper2D collision, Walk enables QueryOnly and automatic overlap events remain disabled.
- UE 5.8 Editor build and focused Animation2D automation pass after adding Goat/Rabbit. The disk-reload test verifies `Goat`, `Rabbit` and the updated player `walk` load successfully; Goat/Rabbit use EachFrame mode with non-empty collision geometry on every key frame; their Profiles own matching static fallbacks and looping Idle/Move clips; the original Grunt Profile now maps both Idle and Move to looping `01_2` so state submission does not regress its fixed-loop presentation.
- A fresh full `ReEcho` run discovered 69 tests but again hit the pre-existing `ReEcho.BasicAttack.HeldRepeat` access violation in `AReEchoWeaponActor::GetAttackInterval()` (`ReEchoWeaponActor.cpp:241`, called from `ReEchoBasicAttackLoopTest.cpp:84`) before the suite could complete. The focused Animation2D test passes; full-suite evidence remains blocked by that independent failure.
- Imported the complete authoritative four-enemy asset set from the local main workspace, including Fox Walk/Attack PaperSprite and texture dependencies. Corrected the base asset name from legacy `01_2` to authored `Grount` and removed two erroneous Fox entries that overwrote the Goat Profile.
- UE 5.8 Editor build and `ReEcho.Presentation.Animation2D.AssetProfiles` pass after a fresh disk reload. The focused test verifies Grount/Rabbit/Goat/Fox Profile references, looping base clips, Fox one-shot Attack policy, EachFrame collision mode and non-empty geometry for every Fox Walk/Attack key frame.
- Final local-main integration candidate migrated the authoritative runtime Texture/Sprite/Flipbook graph to `/Game/ReEcho/Art/Animation2D/**`, retained referenced general textures and PNG source art, removed legacy animation copies and erroneous `SourceArt` `.uasset` imports, and synchronized Profile, code, test, tool and documentation paths.
- Architecture review — `shared/CODEBASE_MAP/ARCHITECTURE.md`: reviewed, no update required because Plan40 does not add/remove a Runtime Module, reverse a dependency, or change a cross-module invariant.
- Architecture review — `shared/CODEBASE_MAP/README.md`: reviewed, no update required because `MOD-ReEcho` / `AREA-Presentation` IDs and their indexed code roots remain unchanged.
- Architecture review — `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`: updated with Animation2D purpose, Profile/Catalog/Controller/Driver responsibility, static/Flipbook asset roots, Capsule movement authority, Paper2D `QueryOnly` body contour policy, independent semantic Body/Weapon tracks, extension rules and the focused reading route.
- The final post-merge UE 5.8 Win64 `Development -FullRebuild` passed and refreshed the two-module curated Editor bundle with source fingerprint `104cc032135f`; `python scripts/validate_project.py`, Python authoring-script compilation and `git diff --check` passed. The focused automation launcher did not enter the test queue because UE 5.8 `ValidatePlatforms -AllPlatforms` rejected missing local LinuxArm64/VisionOS SDK `MainVersion`; this is recorded as unavailable fresh automation evidence, not as a passing or failing animation assertion. Static asset review also removed an invalid test dependency on an undelivered `Idle_Legacy` Flipbook; Idle remains the intended static texture fallback.
- Flipbook 相机朝向修复通过 UE 5.8 Win64 Development UHT/UBT，并刷新四模块 Editor 预构建包；`python scripts/validate_project.py` 与 `git diff --check` 通过。聚焦测试源码新增倾斜相机下法线/屏幕 Up 对齐断言并已编译；`Run-Automation.cmd -Filter ReEcho.Presentation.Animation2D` 仍在进入测试队列前被 LinuxArm64/VisionOS SDK `ValidatePlatforms` 拦截，因此本轮断言未运行，不记为通过。
- 单一 Gameplay Blueprint 组件树改造完成后，UE 5.8 `ReEchoEditor` Win64 Development 构建通过并刷新四模块预构建包，source fingerprint 为 `be8c6a3e969b`；`python scripts/validate_project.py`、Plan40/42 Editor 工具 `py_compile` 与 `git diff --check` 通过。`ReEcho.Presentation.Animation2D.AssetProfiles` 聚焦自动化通过，并覆盖 Gameplay BP 的 Collision/Presentation/Flipbook/Ground/Effects 附着关系与 GroundRoot 绝对旋转契约。`scripts/ue/Verify-And-Open-Editor.cmd` 再次通过门禁并已打开 Editor，等待本轮 PIE 视觉验收。

### Remaining risks

- This checkpoint still contains the Plan39 actor-specific Spade seams; the profile/catalog/controller migration and frame-collision contract remain pending Plan40 work.
- Profile/Catalog assets now exist and load in automation. Human PIE still must accept visual scale, pivot, facing, alpha/sort and the static/animated transitions.
- Current saved profiles do not yet reference authored collision-track assets, so runtime safely uses the existing Capsule/weapon-query fallback. Collision mask authoring/generation, preview overlay and actual track assets remain required before frame geometry can affect hit testing.
- No reviewed collision Mask/JSON exists in the current repository. Actual body/weapon Track creation and profile binding are therefore intentionally pending human-authored boundaries; the preview overlay and deterministic generator are ready for that input.
- Human override on 2026-08-13: the three actively played `walk`, `attack` and Grunt `01_2` Flipbooks now use Paper2D `EachFrameCollision`, superseding this Plan's original prohibition. Idle remains the static `Idel_01` texture. Runtime honors the baked per-frame Sprite BodySetup as `QueryOnly + Pawn query response`, with automatic overlap events disabled because no runtime consumer exists, while retaining the root Capsule as movement authority; built-in collision is not treated as semantic weapon damage and does not replace reviewed Body/Weapon Track separation.
- Extended `ReEcho.DebugCollision` into a layered collision-volume view: level 1 draws Actor Capsules, level 2 adds the current Paper2D frame's exact Box/Sphere/Capsule/Convex geometry and diagnostics, and level 3 adds reviewed Body/Attack polygons. The former aggregate AABB visualization was removed because it visually exaggerated authored collision and obscured diagnosis. The command can be used interactively or through `-ExecCmds` without changing collision state.
- Visual identity, pivot, scale and timing for the new artist-authored Walk/Attack assets remain human PIE acceptance items.

### Human validation result/request
- 2026-08-17 固定 45° 构图候选：玩家与怪物新增根碰撞直属 `FootRoot`，`GroundRoot` 改挂脚点并保持水平；`FlipbookRoot` 拥有固定 Editor authored 旋转，Renderer 不再每帧朝向相机。五个 Gameplay Blueprint 已迁移并保留碰撞、表现、阴影和 Flipbook 参数，等待 PIE 验收。
- 2026-08-17 参数化阴影候选：新增 `/Game/ReEcho/Materials/M_GroundShadow`，材质以 `ShadowOpacity` 控制透明强度；玩家与四个怪物 Gameplay Blueprint 分别暴露 `GroundShadowMaterial`、`GroundShadowSize`、`GroundShadowOpacity`、`GroundShadowCoreRatio`、`GroundShadowCoreOpacity`。Construction 与 BeginPlay 使用同一参数刷新双层平面，因此 Editor 与 PIE 保持一致，且不修改 GroundRoot、碰撞或 Flipbook 状态。
- 2026-08-17 BP 可编辑性复验推翻上述 Actor 参数方案：截图确认选中原生 `GroundShadow` 时无法编辑，参数入口与组件入口分离。最终改为单个 `GroundShadow`，设置 `bEditableWhenInherited=true`，删除 `GroundShadowCore` 和全部 Actor 阴影参数/运行时刷新。五个 Gameplay Blueprint 分别绑定 `/Game/ReEcho/Materials/GroundShadows/MI_Shadow_*`；强度在材质实例调整，范围/位置在 GroundShadow 组件 Transform 调整，两者都能从 BP 直接进入且运行时不覆盖。
- 2026-08-17 阴影相对位置修正：用户要求阴影相对 Flipbook 静止。`GroundRoot` 从 `FootRoot` 改挂到 `FlipbookRoot`，继承 Flipbook 的表现位移与缩放；GroundRoot 继续使用绝对旋转，因此不继承角色表现面的倾斜。Construction 以 `FootRoot` 世界位置校准初始脚点，攻击前冲、受击抖动和缩放后阴影与 Flipbook 保持固定相对关系。
- 2026-08-17 运行时复验推翻“直接挂 FlipbookRoot”：PIE 截图显示 Profile 显示缩放同时放大阴影及其相对脚点距离，而 BP 资产预览未执行该缩放，造成巨大且脱离角色的阴影。最终将 GroundRoot 恢复到 FootRoot；新增 `BaseGroundLocation`，表现更新仅同步与 Flipbook 相同的 Offset，不同步 Scale。阴影保持绝对旋转和 BP authored 尺寸，解决预览/PIE 不一致。
- 2026-08-17 整体 BP 比例契约修正：用户确认最终尺寸必须覆盖 Gameplay Blueprint 的全部资源，而不是只缩放 Flipbook 或表现分支。Player/Enemy 新增 Blueprint Class Defaults 可编辑的统一 `CharacterScale`，Construction 将其等比应用到整个 Actor，因此根 Box、Flipbook、水平阴影、特效和未来子节点使用同一倍率；Profile `WorldHeight` 仅保留序列间归一化职责，局部根 Scale 不再作为角色整体尺寸入口。
- 2026-08-17 相对层级收敛：用户确认所有节点均采用父子相对 Transform，阴影不需要独立世界空间规则。Player/Enemy 移除 FlipbookRoot/GroundRoot 的绝对旋转，五个 Gameplay Blueprint 清除其 absolute location/rotation/scale Override；阴影作为普通子节点随整个 BP 统一变换。
- 2026-08-17 阴影跟随收敛：最终采用共享 `PresentationMotionRoot` 而非把阴影挂到倾斜/挤压的 FlipbookRoot。`FlipbookRoot`、`GroundRoot`、`EffectsRoot` 同为 MotionRoot 子节点；平面位移只写 MotionRoot 一次，上下 bob/挤压仅写角色与特效分支。玩家新增独立 `AttackAimDirection`，武器改读该方向，瞄准不再旋转整个 Actor，因此碰撞、阴影和 BP 构图保持稳定。
- 2026-08-17 Blueprint Viewport 修复：Gameplay BP 仅含原生继承组件后被 Unreal 识别为 Data Only Blueprint，默认精简编辑器没有 Viewport。UE 5.8 不向 Python 暴露临时 `ForceFullEditor` 标志，因此五个 Gameplay BP 改为各自保存一个 `PresentationMotionRoot -> ArtAuthoringRoot` Blueprint 组件；它既永久恢复完整 Viewport，也是美术新增装饰/特效子节点的稳定入口。资产校验要求该节点存在。
- 2026-08-17 阴影层级修复：玩家阴影曾把整数 `TranslucencySortPriority` 写成 `-0.8`，实际截断为 `0` 后可能与角色同层。Player/Enemy 原生默认值及五个 Gameplay BP Override 统一改为 `-10`，资产校验要求阴影排序为负，确保其始终位于 Flipbook 下层。
