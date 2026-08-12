# ReEcho planner exchange

Live local coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|
| Shared settings UI + Plan 18 full-run weapon lock | ReEcho teammate-side Planner | `Closed` | `PendingFollowUp` | Delivered on current main | Shared settings shell for start/pause menus; runtime weapon switching removed while pre-run selection/save compatibility remain | Visual/menu PIE remains follow-up. Do not restore switching from historical Plan03/13/24 notes. |
| Plan 25 designer usability follow-up | Gavyn-side Planner | `Closed` | `PendingFollowUp` | Delivered on current main; QA uses a disposable local branch/clone | `ReadOnly` QA unless the human asks to keep workbook edits | Designers follow the Plan25 checklist; do not hand-edit generated CSV. |
| Plan 26 run-locked weapon read-only UI | Gavyn-side Planner | `Ready` | `PendingBeforeClose` | Local `planning/26-31-local`; create local `plan/26-weapon-readonly-ui` from its current main-integrated commit | `Isolated` + `ReadOnly` Writes to `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*` and optional Plan26-only tests | Current main contains no Plan26 implementation. Show configured run-locked WeaponId/DisplayName through the pinned typed snapshot; do not restore runtime switching or edit providers/gameplay. Plan31 waits for release. |
| Plan 28 player automatic/manual attacks | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | Local `plan/28-player-attack-modes`; integrate current main before Review | `Isolated` Writes to P input, player attack-mode state, GameMode integration, existing pause widget, narrow additive range/target queries if needed and Plan28-only tests | P is the latest explicit human pause/menu decision; the committed remap is intentional. Plan26 may continue; user owns PIE and play-feel validation. |
| Plan 30 specific single/multi replay runtime | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Future local `plan/30-specific-echo-replay-runtime`; final base contains accepted Plans29/28 | Replay resolver plus GameMode zero/one/many Echo spawning/resume and focused tests | Do not start until Plan29 is accepted and Plan28 releases GameMode. Limit 0 defaults to the immediately previous encounter; positive limit uses only selected stored GUIDs. |
| Plan 31 shop echo storage/selection UI | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Future local `plan/31-shop-echo-selection-ui`; final base contains accepted Plans26/29/30 | Existing InventoryShopWidget plus GameMode bridge and cheap UI-state tests | Real write overlap with Plan26. Do not start until all dependencies are accepted/released; user owns visual and interaction validation. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| Gavyn-side Planner | Plan26 / `plan/26-weapon-readonly-ui` | `Isolated` + `ReadOnly` | `Reserved` | `ReEchoInventoryShopWidget.*`, `ReEchoStatsWidget.*` and Plan26-only tests | User-requested takeover of the stale Plan. Plan31 remains Proposed until this ownership is released. |
| Gavyn-side Planner | Plan28 / `plan/28-player-attack-modes` | `Isolated` | `Active` | P input, player attack-mode state, GameMode integration, existing `ReEchoRestartWidget.*` pause UI and Plan28-only tests | P remap is intentional under the latest human decision. Additive target queries require a local scope update before expansion. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- None.
