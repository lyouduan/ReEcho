# ReEcho planner exchange

Live local coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Plan AI / implementation AI | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|---|
| Shared settings UI + Plan 18 full-run weapon lock | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Closed` | `PendingFollowUp` | Delivered on current main | Shared settings shell for start/pause menus; runtime weapon switching removed while pre-run selection/save compatibility remain | Visual/menu PIE remains follow-up. Do not restore switching from historical Plan03/13/24 notes. |
| Plan 25 designer usability follow-up | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Closed` | `PendingFollowUp` | Delivered on current main; QA uses a disposable local branch/clone | `ReadOnly` QA unless the human asks to keep workbook edits | Designers follow the Plan25 checklist; do not hand-edit generated CSV. |
| Plan 26 run-locked weapon read-only UI | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | Teammate-side local task base; no delivered implementation on current `origin/main` | `ReadOnly` consumer Writes to `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*` and optional Plan26-only tests | Original Plan provenance is `2054f71`. Gavyn-side local refresh did not transfer ownership. May run beside Plan31; Planner later combines Widget behavior. |
| Plan 28 player automatic/manual attacks | Gavyn-side / Gavyn-side | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | Local `plan/28-player-attack-modes`; integrate current main before Review | `Isolated` Writes to P input, player attack-mode state, GameMode integration, existing pause widget, narrow additive range/target queries if needed and Plan28-only tests | P is the latest explicit human pause/menu decision; the committed remap is intentional. Plan26 may continue; user owns PIE and play-feel validation. |
| Plan 30 specific single/multi replay runtime | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Ready` | `PendingBeforeClose` | Local `plan/30-specific-echo-replay-runtime` from current local main with accepted Plan29 | RunSubsystem resolver plus GameMode zero/one/many Echo spawning/resume and focused tests | Human-approved parallel GameMode overlap with Plan28; Planner combines both behaviors. Limit 0 defaults to previous encounter; positive limit uses only selected stored GUIDs. |
| Plan 31 shop echo storage/selection UI | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Ready` | `PendingBeforeClose` | Local `plan/31-shop-echo-selection-ui` from current local main with accepted Plan29 | Existing InventoryShopWidget plus GameMode bridge and cheap UI/state tests | Human-approved parallel overlap with teammate-side Plan26 Widget and Plan28/30 GameMode. Executor records integration points; Planner combines all locked behaviors. User owns visual/interaction validation. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan26 / teammate-side local task branch | `Isolated` + `ReadOnly` | `Reserved` | `ReEchoInventoryShopWidget.*`, `ReEchoStatsWidget.*` and Plan26-only tests | Original Plan is teammate-side. Parallel overlap with Plan31 is human-approved; Planner later combines both Widget contracts. |
| Gavyn-side Planner | Plan28 / `plan/28-player-attack-modes` | `Isolated` | `Active` | P input, player attack-mode state, GameMode integration, existing `ReEchoRestartWidget.*` pause UI and Plan28-only tests | P remap is intentional under the latest human decision. Additive target queries require a local scope update before expansion. |
| Gavyn-side Planner | Plan30 / `plan/30-specific-echo-replay-runtime` | `SharedContract` | `Reserved` | RunSubsystem replay resolver, GameMode zero/one/many Echo orchestration and Plan30 tests | Parallel overlap with Plan28 GameMode is human-approved; Planner owns semantic integration. Do not edit Plan28 Player/Pause UI files. |
| Gavyn-side Planner | Plan31 / `plan/31-shop-echo-selection-ui` | `SharedContract` | `Active` | InventoryShopWidget echo management, GameMode shop bridge and Plan31 tests | Parallel overlaps with Plan26 Widget and Plan28/30 GameMode are human-approved; record changed integration functions precisely. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- None.
