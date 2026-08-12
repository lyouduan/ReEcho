# ReEcho planner exchange

Live local coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Plan AI / implementation AI | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|---|
| Plan 29 full UI UMG migration | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | Integrated into local `main` through the Plan31 candidate | UMG assets and typed UI Framework own screen lifecycle; gameplay/data contracts remain separate | Preserve UI Flow ownership and do not restore direct viewport or code-built page lifecycle. |
| Plan 10 player HUD UMG follow-up | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | Accepted implementation lineage included in Plan29 | Isolated C++ and `/Game/ReEcho/UI/WBP_ReEchoPlayerHud.uasset` | Visual/DPI acceptance requires human PIE before closure. |
| Shared settings UI + Plan 18 full-run weapon lock | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Closed` | `PendingFollowUp` | Delivered on current main | Shared settings shell; runtime weapon switching removed while pre-run selection/save compatibility remain | Do not restore switching from historical Plan03/13/24 notes. |
| Plan 25 designer usability follow-up | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Closed` | `PendingFollowUp` | Delivered on current main | Read-only QA unless the human asks to retain workbook edits | Follow the Plan25 checklist; do not hand-edit generated CSV. |
| Plan 26 weapon read-only UI consumers | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | Starts from the Planner-approved current main/code baseline | Read-only provider consumption plus isolated Inventory/Stats consumer work | Preserve Plan29 presentation bindings and do not edit the data provider contract. |
| Plan 28 automatic/manual player attacks | Gavyn-side / Gavyn-side | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | Correction continues on `integration/gavyn-umg-gameplay-20260812` | Player attack mode, deterministic targeting, persistence, P pause and Restart-WBP child panel | Planner review found physical manual input can interfere with synthetic automatic held input; fix and retest before integration. |
| Plan 30 echo storage/save foundation | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Review` | `NotRequired` | Integrated into local `main` with the Plan31 candidate | Storage/save contract, stable GUID identity and transactional storage commands | Build, Blueprint compilation and full automation pass; await remote-baseline reconciliation/publication. |
| Plan 31 specific single/multi replay runtime | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Review` | `Passed` | Integrated into local `main` from candidate `001321b` | Resolver, zero/one/many Echo spawn/resume and all-Echo fog reveal | Human gameplay validation and all objective integration gates pass; await remote-baseline reconciliation/publication. |
| Plan 32 shop echo storage/selection UI | Gavyn-side / Gavyn-side | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | Existing local behavior retained; final UMG adaptation remains on the integration branch | InventoryShop echo management and post-trait intermission UI | Preserve UI Flow ownership; final typed child-panel adaptation is outside this Plan31-only merge. |
| Plan 33 audio runtime-module foundation | Gavyn-side / `Unassigned` | Gavyn-side Planner | `Ready` | `NotRequired` | Waits for the combined gameplay/UI baseline | Separate `ReEchoAudio` runtime module and one-way dependency contract | No audio implementation is part of the current merge. |
| Plans 34-36 authored and integrated audio | Gavyn-side / `Unassigned` | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Depend on Plan33 | Catalog/settings, noncombat and combat/echo audio integration | Wait for Plan33 and the relevant combined UI/gameplay surfaces. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan26 / teammate-side task branch | `ReadOnly` | `Reserved` | Typed weapon UI-consumer behavior and Plan26 tests | Must preserve Plan29 presentation bindings. |
| Gavyn-side Planner | Plans 28/32 / `integration/gavyn-umg-gameplay-20260812` | `SharedContract` + local `Exclusive` | `Active` | Plan28 correction plus typed Restart/shop child-panel adaptations | Do not restore direct viewport ownership; these remain outside the Plan31-only merge boundary. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or remote integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.
- The Plan31-only local merge includes the Plan30 foundation and Plan29 UMG baseline, but excludes Plan28 gameplay and the final Plan32 typed child-panel adaptation.
- Locked product decisions: selecting attack mode does not auto-resume; Continue restores the run-local choice while a new run defaults automatic; every active Echo contributes fog visibility; an empty explicit replay selection produces no Echo.

## Pending human decisions

- None.
