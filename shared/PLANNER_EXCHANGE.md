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
| Plan 32 shop echo storage/selection UI | Gavyn-side / Gavyn-side | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | Continue existing local rework, then adapt onto the published current main | Typed InventoryShop intermission, echo-management WBP children, authoritative command/close tests and one-time 30-shard `SHOP_REPLAY_UNLOCK` purchase | Latest user decision restores shop acquisition: first purchase unlocks limit 3; insufficient/repeat reject atomically. `73a8ed6`/`e2b1047` are behavior references only, never whole-branch merges. |
| Plan 34 authored audio catalog and persistent settings | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Ready` | `PendingBeforeClose` | Start from published `main` containing closed Plan33 and the Ready Plan34 revision | AudioEvents XLSX/CSV contract, async catalog/preload, five persisted buses, typed settings WBP and technical diagnostic tone | Claim the exact workbook and settings-WBP resources before writing; user performs final listening/UI validation. |
| Plans 35-36 noncombat and combat/echo audio integration | Gavyn-side / `Unassigned` | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Plan33 API is available; audible closure depends on Plan34 catalog/assets | Semantic event hooks only; audio module retains playback ownership | May use stable Plan33 IDs, but do not invent catalog parsing, asset paths or parallel audio services. |
| Plan 38 held basic-attack repeat repair | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Ready` | `PendingBeforeClose` | Start from published current `origin/main` after this Plan appears there | GAS held repeat and focused auto/manual second-hit regression; no GameMode/UI/shop writes | Independent of Plan32. Temporary weapon action lock must retry instead of permanently ending a still-held attack. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan26 / teammate-side task branch | `ReadOnly` | `Reserved` | Typed weapon UI-consumer behavior and Plan26 tests | Must preserve Plan29 presentation bindings. |
| Gavyn-side Planner | Plan32 rework | `SharedContract` + local `Exclusive` | `Active` | Typed shop echo-management child-panel assets and transaction/close tests | Plan28 ownership is released after main integration. Use `73a8ed6` only as Plan32 behavior reference; do not merge the mixed branch. |
| Gavyn-side Planner | Plan34 reservation | `SharedContract` + `Exclusive` | `Reserved` | `Design/Data/ReEchoData.xlsx`, generated `Content/Data/audio_events.csv`, `/Game/ReEcho/UI/WBP_ReEchoSettings.uasset`, Plan33 catalog/settings provider surface | Disjoint from active Plan32 shop assets and Plan38 attack code. Reservation becomes Active only when the Plan34 Executor starts. |
| Gavyn-side Planner | Plan38 / `plan/38-held-basic-attack-repeat` | `Isolated` | `Reserved` | Basic-attack GAS repeat files and Plan38-only attack-loop tests | May run in parallel with Plan32; do not edit GameMode/shop/UI/data/save/recording. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or remote integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.
- The Plan31-only local merge includes the Plan30 foundation and Plan29 UMG baseline, but excludes Plan28 gameplay and the final Plan32 typed child-panel adaptation.
- Locked product decisions: selecting attack mode does not auto-resume; Continue restores the run-local choice while a new run defaults automatic; every active Echo contributes fog visibility; an empty explicit replay selection produces no Echo.

## Pending human decisions

- None.
