# Plan 35 - Audio - music ambience UI and camera integration

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Unassigned.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Unassigned.
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`NotRequired`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 关闭原因：2026-08-17 经程序用户明确决定，本 Plan 的音乐、环境、UI 与相机语义触发范围由扩大的 Plan46 全量吸收；本 Plan 未独立实施，不再形成第二套调用点或验收。
- Local planning / implementation base: accepted Plan33 API, with Plan34 catalog/settings contract available; latest accepted menu/shop/pause integration.
- Implementation branch: local `plan/35-audio-noncombat` in a separate worktree.
- Depends on / Blocks: depends on Plan33. May implement against Plan33 IDs in parallel with Plans34/36, but audible closure requires Plan34 definitions/assets. Wait for Plan32 before editing shared shop/GameMode functions.
- Writes: narrow lifecycle calls in `ReEchoGameMode.*`, weather and affected UI widgets or one shared UI-audio binding helper, Plan35-only tests and notes.
- Stable Reads: Plan33 post/state APIs; Plan34 catalog/buses; current menu, pause, trait, shop, weather and fixed-camera flows.
- Impact mode: `SharedContract` because it overlaps GameMode/UI lifecycle owned by Plans26/28/31; integration waits for their accepted behavior and preserves it.
- Compatibility promise / downstream action: audio is observational. Menu transitions, pause, shop close, purchases, card choices, saving and encounter progression must behave identically with audio disabled or missing.
- Explicit exclusions: no combat/damage/weapon/enemy/echo hooks, no camera-system redesign, no blocking delays to “let a sound finish,” no asset authoring outside Plan34 and no executor aural sign-off.

## Locked goal

Drive background music, environment ambience and interaction sounds from authoritative state transitions:

- music states: start/menu, normal encounter, Boss encounter, shop/intermission and death/victory;
- ambience states: arena/weather, including rain where configured;
- UI events: hover only where reliable, confirm, cancel/back, purchase, card selection and error/rejection;
- camera movement event support only when an actual camera transition exists; the current fixed camera must not emit per-frame noise.

State calls must be idempotent, cross-faded by the audio module and safe while paused. UI SFX may play while paused; encounter-owned ambience/combat buses follow the authored pause policy.

## Locked acceptance

- [ ] Each lifecycle state is emitted once from an authoritative transition, not every tick or Widget refresh.
- [ ] Opening/closing pause, settings, inventory/shop and trait UI does not duplicate music/components or advance gameplay.
- [ ] UI rejection uses a distinct semantic event and never relies on delaying the operation.
- [ ] Restart/load/continue leave one correct music and ambience state with no orphan loops.
- [ ] Camera events are transition-based; fixed-camera updates do not post audio per frame.
- [ ] With audio disabled/catalog missing, all existing flows and tests behave identically.
- [ ] Build, affected menu/shop/weather automation and `git diff --check` pass.
- [ ] User validates transitions, cross-fades, pause policy and UI readability by ear before closure.

## Step 0 gate

- Baseline branch/commit: accepted Plan33 plus latest integrated Plans28/30/31/32; record exact base.
- Engine/build availability: UE 5.8 repository scripts.
- Existing focused-test result: affected menu/shop tests pass before edits.
- Active exclusive ownership or shared-contract approval: do not edit active Plan32 GameMode/shop or Plan28 pause functions until integrated or explicitly combined.
- Stop condition if the baseline is broken: duplicate state ownership, an unaccepted overlapping Widget/GameMode branch, or a request to delay gameplay for audio duration.

## Implementation outline

1. Map actual authoritative transitions to Plan33 state/event IDs before editing code.
2. Prefer one orchestration seam or reusable UI binding helper over asset calls in every Widget.
3. Keep music/ambience state ownership in audio; GameMode only reports the new semantic state.
4. Add transition-count/no-audio regressions without requiring an audio device.
5. Record the shortest human listening route; do not perform subjective sign-off as Executor.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static/build | project validation, `.clang-format`, Editor build | clean integration |
| Automation | lifecycle transition counts plus affected menu/shop/weather tests | no duplicate states or flow regression |
| Disabled-audio | run focused flows with empty catalog/muted master | gameplay behavior unchanged |
| Human | start → encounter → shop → Boss/death plus pause/settings/UI actions | user accepts transitions, duck/pause and feedback by ear |
| Scope | `git diff --check`; exact call-site audit | no combat or asset-authoring scope creep |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request

`PendingBeforeClose`: user performs the named listening route and reports missing, duplicated, abrupt or overly loud events.
