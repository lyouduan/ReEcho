# ReEcho planner exchange

Live coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|
| Plan 28 full UI UMG migration | Codex | `InProgress` | `PendingBeforeClose` | `plan/28-ui-umg-migration`; approved combined `origin/main` + Plan 10 HUD baseline | Batched isolated UI/GameMode C++ plus Exclusive `/Game/ReEcho/UI/WBP_*.uasset`; gameplay/data contracts are ReadOnly | Plan 26 retains `InventoryShopWidget.*` and `StatsWidget.*`; Plan 28 does not write them until release/coordination. |
| Plan 10 player HUD UMG follow-up | Codex | `Review` | `PendingBeforeClose` | `plan/10-player-hud-umg` / `origin/main` | Isolated C++ writes to `ReEchoPlayerHudWidget.*`, `ReEchoGameMode.*`, Plan 10 notes; Exclusive write to `/Game/ReEcho/UI/WBP_ReEchoPlayerHud.uasset` completed and released | Objective checks pass; visual/DPI acceptance requires human PIE before closure. |
| Shared settings UI + Plan 18 full-run weapon lock | ReEcho teammate-side Planner | `Closed` | `PendingFollowUp` | Local reviewed merge candidate based on current `origin/main`; remote publication awaits renewed candidate approval | Shared settings shell for start/pause menus; removes runtime weapon hotkeys/GAS abilities/run setter/recording and echo switch timeline while retaining pre-run selection, save restore and compatibility `InputSlot` data | Visual/menu PIE remains follow-up. Direct user decision supersedes historical Plan 18 switching. After publication, peer Planners fetch/rebase and must not restore switching from historical Plan 03/13/24 notes. |
| Plan 25 designer usability follow-up | Gavyn-side Planner | `Closed` | `PendingFollowUp` | Delivered on `origin/main`; QA uses a disposable local branch/clone | `ReadOnly` QA unless the human asks to keep workbook edits; no repository ownership is claimed by disposable tests | Designers follow `Design/Data/ReEchoData策划验收清单.md`. Publishable findings go to the current `WorkbookWriter`; do not hand-edit generated CSV. |
| Plan 26 weapon read-only UI consumers | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | New work starts from freshly fetched `origin/main`; an already-started branch from the former coordination ref must integrate current `origin/main` before `Review` | `ReadOnly` provider consumption plus isolated Writes to `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*` and Plan26-only UI tests | Consume typed weapon identity/display/query APIs. Do not edit data registry/reader/schema, canonical workbook or generated CSV. Rebroadcast before requesting a shared-contract change. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| Codex | Plan 28 / `plan/28-ui-umg-migration` | `Isolated` + `Exclusive` | `Active` | UI coordination C++; `ReEchoEncounterHudWidget.*`; new `WBP_ReEchoEncounterHud` and subsequent non-Plan26 WBP assets claimed per batch | Batch A is active; same-clone Unreal lock required for assets. |
| ReEcho teammate-side Planner | Plan 26 / `plan/26-weapon-readonly-ui` | `Isolated` + `ReadOnly` | `Reserved` | Plan26 UI files and Plan26-only tests | Reservation does not block other disjoint/read-only work. It becomes `Active` when implementation starts. |

## Warnings / blocked items

- Fetch `origin/main` before new planning or integration. Historical `coord/...` refs are handoff evidence, not current baselines.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- None.
