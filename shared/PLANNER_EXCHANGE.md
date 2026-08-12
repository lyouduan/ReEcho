# ReEcho planner exchange

Live coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Plan AI / implementation AI | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|---|
| Plan 29 full UI UMG migration | ReEcho teammate-side / ReEcho teammate-side | Codex | `InProgress` | `PendingBeforeClose` | Local `plan/29-ui-umg-migration`, combined with current `origin/main` and the accepted Plan 10 HUD baseline | Batched isolated UI C++ plus Exclusive `/Game/ReEcho/UI/WBP_*.uasset`; gameplay/data contracts are ReadOnly; typed UI Framework now owns screen lifecycle | Human explicitly prioritized Plan 29 over Plan 26 for UI migration; Plan 26 must consume the published presentation bindings without restoring C++-owned layout. |
| Plan 10 player HUD UMG follow-up | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | Accepted implementation lineage included in the Plan 29 local candidate | Isolated C++ and `/Game/ReEcho/UI/WBP_ReEchoPlayerHud.uasset`; objective checks pass | Visual/DPI acceptance requires human PIE before closure. |
| Shared settings UI + Plan 18 full-run weapon lock | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Closed` | `PendingFollowUp` | Local reviewed merge candidate based on current `origin/main`; remote publication awaits renewed candidate approval | Shared settings shell for start/pause menus; removes runtime weapon hotkeys/GAS abilities/run setter/recording and echo switch timeline while retaining pre-run selection, save restore and compatibility `InputSlot` data | Visual/menu PIE remains follow-up. Direct user decision supersedes historical Plan 18 switching. After publication, peer Planners fetch/rebase and must not restore switching from historical Plan 03/13/24 notes. |
| Plan 25 designer usability follow-up | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Closed` | `PendingFollowUp` | Delivered on `origin/main`; QA uses a disposable local branch/clone | `ReadOnly` QA unless the human asks to keep workbook edits; no repository ownership is claimed by disposable tests | Designers follow `Design/Data/ReEchoData策划验收清单.md`. Publishable findings go to the current `WorkbookWriter`; do not hand-edit generated CSV. |
| Plan 26 weapon read-only UI consumers | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | Local task starts from the Planner-approved current main/code baseline | `ReadOnly` provider consumption plus isolated Writes to `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*` and Plan26-only UI tests | Consume typed weapon identity/display/query APIs. Do not edit data registry/reader/schema, canonical workbook or generated CSV. Record a shared-contract request locally before crossing it. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| Codex | Plan 29 / `plan/29-ui-umg-migration` | `Isolated` + `Exclusive` | `Active` | All UI presentation C++, including `ReEchoInventoryShopWidget.*` and `ReEchoStatsWidget.*`; new/modified `WBP_ReEcho*.uasset` assets claimed per batch | Human directed Plan 29 to take priority over Plan 26 UI ownership; preserve Plan 26 data-consumer and canonical Plan 28 attack-mode behavior through combined adaptation. Same-clone Unreal lock required for assets. |
| ReEcho teammate-side Planner | Plan 26 / `plan/26-weapon-readonly-ui` | `ReadOnly` | `Reserved` | Typed weapon UI-consumer behavior and Plan26-only tests; Inventory/Stats presentation files are coordinated to Plan 29 by explicit human priority | Plan 26 must combine-adapt onto Plan 29 presentation bindings and must not restore C++-owned layout. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- None.
