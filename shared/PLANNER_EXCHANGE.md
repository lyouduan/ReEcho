# ReEcho planner exchange

Live coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|
| Shared settings UI + Plan 18 full-run weapon lock | ReEcho teammate-side Planner | `Closed` | `PendingFollowUp` | Local reviewed merge candidate based on current `origin/main`; remote publication awaits renewed candidate approval | Shared settings shell for start/pause menus; removes runtime weapon hotkeys/GAS abilities/run setter/recording and echo switch timeline while retaining pre-run selection, save restore and compatibility `InputSlot` data | Visual/menu PIE remains follow-up. Direct user decision supersedes historical Plan 18 switching. After publication, peer Planners fetch/rebase and must not restore switching from historical Plan 03/13/24 notes. |
| Plan 25 designer usability follow-up | Gavyn-side Planner | `Closed` | `PendingFollowUp` | Delivered on `origin/main`; QA uses a disposable local branch/clone | `ReadOnly` QA unless the human asks to keep workbook edits; no repository ownership is claimed by disposable tests | Designers follow `Design/Data/ReEchoData策划验收清单.md`. Publishable findings go to the current `WorkbookWriter`; do not hand-edit generated CSV. |
| Plan 26 weapon read-only UI consumers | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | Local task starts from the Planner-approved current main/code baseline | `ReadOnly` provider consumption plus isolated Writes to `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*` and Plan26-only UI tests | Consume typed weapon identity/display/query APIs. Do not edit data registry/reader/schema, canonical workbook or generated CSV. Record a shared-contract request locally before crossing it. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan 26 / `plan/26-weapon-readonly-ui` | `Isolated` + `ReadOnly` | `Reserved` | Plan26 UI files and Plan26-only tests | Reservation does not block other disjoint/read-only work. It becomes `Active` when implementation starts. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- None.
