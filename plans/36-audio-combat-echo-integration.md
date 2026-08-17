# Plan 36 - Audio - combat monster Boss and echo integration

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Unassigned.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Unassigned.
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`NotRequired`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 关闭原因：2026-08-17 经程序用户明确决定，本 Plan 的 Combat、Enemy、Boss 与 Echo 语义触发范围由扩大的 Plan46 全量吸收；本 Plan 未独立实施，不再形成第二套调用点或验收。
- Local planning / implementation base: accepted Plan33 API and latest accepted attack/replay runtime; Plan34 catalog may land independently.
- Implementation branch: local `plan/36-audio-combat-echo` in a separate worktree.
- Depends on / Blocks: depends on Plan33. May implement in parallel with Plans34/35; audible closure needs Plan34 assets. Wait for Plan28 attack-mode integration before editing Player/Weapon execution paths.
- Writes: narrow semantic-event hooks in weapon/projectile/attack effects, combatant/enemy/player/echo/death transitions as required; Plan36-only tests and notes.
- Stable Reads: Plan33 APIs/event IDs; current authoritative attack success, actual-damage, death, Boss-kind and echo playback transitions.
- Impact mode: `SharedContract` because attack hooks overlap Plan28 and multi-echo behavior consumes Plan31; Planner owns final integration.
- Compatibility promise / downstream action: audio never determines whether an attack, hit, damage, kill, death, revive or echo action succeeds. Automatic attacks remain absent from recordings, and audio events themselves are never serialized/replayed as data.
- Explicit exclusions: no damage/attack timing or targeting changes, no save/record schema change, no automatic-attack serialization, no content licensing/import outside Plan34, no final mix/aesthetic claim and no speculative revive mechanic.

## Locked goal

Emit combat audio from authoritative outcomes, covering:

1. successful weapon/skill attack start, with context for weapon/source rather than asset choice in gameplay;
2. confirmed hit and actual positive damage;
3. player/enemy hurt, blocked/rejected damage, kill and death with kill replacing ordinary hurt where appropriate;
4. monster archetype and Boss spawn/attack/death variants;
5. echo spawn/attack/end variants, preserving each echo's world location and avoiding event storms during multi-echo playback;
6. player death and a reserved revive event that is emitted only if a real revive transition exists later.

Spatial events use authoritative source/impact/target locations. The audio module owns cooldown, concurrency, priority, attenuation and variant selection. Combat code must not throttle sounds or access assets.

## Locked acceptance

- [ ] Failed/cooldown-blocked attacks emit no attack event; each successful execution emits once.
- [ ] Zero/blocked damage does not emit a normal hurt event; actual damage emits once from one authoritative seam.
- [ ] A lethal result emits one kill/death sequence without an extra duplicate ordinary-hurt event.
- [ ] Boss/monster context and player/enemy/echo source context reach audio without audio depending on gameplay classes.
- [ ] Multiple echoes remain independently positioned while module concurrency prevents unbounded sound storms.
- [ ] Pause stops encounter progression and does not cause queued combat audio bursts on resume.
- [ ] Audio output cannot perturb gameplay RNG, fixed-step results, save bytes or recording bytes.
- [ ] Existing attack, damage, reaction, recording, replay and save tests pass with audio present and absent.
- [ ] User validates timing, spatial clarity, mix priority and repeated-combat fatigue before closure.

## Step 0 gate

- Baseline branch/commit: accepted Plan33 plus latest integrated Plan28/31 runtime.
- Engine/build availability: UE 5.8 repository scripts.
- Existing focused-test result: affected attack/damage/replay suites pass before editing.
- Active exclusive ownership or shared-contract approval: Plan28 Player/Weapon changes must be accepted or explicitly combined; multi-echo Plan31 semantics are read-only.
- Stop condition if the baseline is broken: no single authoritative outcome seam can be identified, an audio hook would change attack/damage order, or revive does not exist but implementation is requested speculatively.

## Implementation outline

1. Inventory successful attack, actual-damage, block, death, Boss and echo lifecycle seams; assign one emitter per outcome.
2. Post Plan33 requests with semantic context/location only; keep sound choice and limiting in audio.
3. Ensure echo actions derive audio at playback execution time without adding audio events to recordings.
4. Add count/context/no-audio tests and multi-echo stress coverage using a fake audio sink.
5. Run full affected gameplay automation, then hand the user a concise listening matrix.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static/build | project validation, `.clang-format`, Editor build | clean module/gameplay integration |
| Focused automation | attack, damage, reactions, recording/replay/save plus Plan36 event-count tests | authoritative once-only semantics and no regression |
| Stress | many enemies and up to three echoes using fake sink/counters | bounded requests and correct source/location context |
| Disabled-audio | empty catalog/muted master | identical combat outcomes and serialized data |
| Human | weapon variants, monster/Boss, echo, hit/hurt/kill/death listening route | user accepts timing, spatial clarity, priority and fatigue |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request

`PendingBeforeClose`: user performs the combat listening matrix; no Executor visual/aural sign-off is accepted as human evidence.
