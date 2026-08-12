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
| Plan 28 automatic/manual player attacks | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Review` | `PendingBeforeClose` | Delivered on `main` through merge `a931af3` | Player attack mode, deterministic targeting, v6 persistence, mode-gated physical input, P pause and Restart child panel | Input-source defect is corrected. Static/build/Blueprint gates pass; focused 5/5 and full ReEcho 51/51 automation pass. Human PIE/game-feel validation remains pending. |
| Plan 30 echo storage/save foundation | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Review` | `NotRequired` | Integrated into local `main` with the Plan31 candidate | Storage/save contract, stable GUID identity and transactional storage commands | Build, Blueprint compilation and full automation pass; await remote-baseline reconciliation/publication. |
| Plan 31 specific single/multi replay runtime | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Review` | `Passed` | Integrated into local `main` from candidate `001321b` | Resolver, zero/one/many Echo spawn/resume and all-Echo fog reveal | Human gameplay validation and all objective integration gates pass; await remote-baseline reconciliation/publication. |
| Plan 32 shop echo storage/selection UI | Gavyn-side / Gavyn-side | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | Rework from local `main`; `73a8ed6` is behavior reference only | Typed InventoryShop intermission, echo-management child WBP assets and authoritative command/close tests | Planner review requires the missing WBP assets and restored transaction/close coverage. Do not merge the old code-built branch or its deferred acquisition addition. |
| Plan 33 audio runtime-module foundation | Gavyn-side / `Unassigned` | Gavyn-side Planner | `Ready` | `NotRequired` | Waits for the combined gameplay/UI baseline | Separate `ReEchoAudio` runtime module and one-way dependency contract | No audio implementation is part of the current merge. |
| Plans 34-36 authored and integrated audio | Gavyn-side / `Unassigned` | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Depend on Plan33 | Catalog/settings, noncombat and combat/echo audio integration | Wait for Plan33 and the relevant combined UI/gameplay surfaces. |
| Plan 37 clone-and-open Editor delivery | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Ready` | `NotRequired` | Starts after this Plan-only publication on `origin/main` | Curated Win64 Editor module bundle, prebuilt validation and programmer publication gate | Future C++ or module-descriptor publications must rebuild and refresh the bundle before main publication. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan26 / teammate-side task branch | `ReadOnly` | `Reserved` | Typed weapon UI-consumer behavior and Plan26 tests | Must preserve Plan29 presentation bindings. |
| Gavyn-side Planner | Plan32 rework | `SharedContract` + local `Exclusive` | `Active` | Typed shop echo-management child-panel assets and transaction/close tests | Plan28 ownership is released after main integration. Use `73a8ed6` only as Plan32 behavior reference; do not merge the mixed branch. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or remote integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.
- The Plan31-only local merge includes the Plan30 foundation and Plan29 UMG baseline, but excludes Plan28 gameplay and the final Plan32 typed child-panel adaptation.
- Locked product decisions: selecting attack mode does not auto-resume; Continue restores the run-local choice while a new run defaults automatic; every active Echo contributes fog visibility; an empty explicit replay selection produces no Echo.

## Pending human decisions

- None.
