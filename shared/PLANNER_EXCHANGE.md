# ReEcho planner exchange

Live local coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Plan AI / implementation AI | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|---|
| Plan 29 full UI UMG migration | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | Integrated into local `main` through the Plan31 candidate | UMG assets and typed UI Framework own screen lifecycle; gameplay/data contracts remain separate | Preserve UI Flow ownership and do not restore direct viewport or code-built page lifecycle. |
| Plan 10 player HUD UMG follow-up | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | Accepted implementation lineage included in Plan29 | Isolated C++ and `/Game/ReEcho/UI/WBP_ReEchoPlayerHud.uasset` | Visual/DPI acceptance requires human PIE before closure. |
| Plan 26 weapon read-only UI consumers | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | Starts from the Planner-approved current main/code baseline | Read-only provider consumption plus isolated Inventory/Stats consumer work | Preserve Plan29 presentation bindings and do not edit the data provider contract. |
| Plan 28 automatic/manual player attacks | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Review` | `PendingBeforeClose` | Delivered on `main` through merge `a931af3` | Player attack mode, deterministic targeting, v6 persistence, mode-gated physical input, P pause and Restart child panel | Input-source defect is corrected. Static/build/Blueprint gates pass; focused 5/5 and full ReEcho 51/51 automation pass. Human PIE/game-feel validation remains pending. |
| Plan 34 authored audio catalog and persistent settings | Gavyn-side / Gavyn-side | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | Local `plan/34-audio-catalog-settings@86824a0`, based on `f5051e4`; current main is `ec0f5e3` | AudioEvents XLSX/CSV contract, async catalog/preload, five persisted buses and settings controls | Implementation exists locally but has no current-main build, diagnostic asset or human validation. Adapt onto the current canonical workbook; never merge the old workbook blob wholesale. |
| Plans 35-36 noncombat and combat/echo audio integration | Gavyn-side / `Unassigned` | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Plan33 API is available; audible closure depends on Plan34 catalog/assets | Semantic event hooks only; audio module retains playback ownership | May use stable Plan33 IDs, but do not invent catalog parsing, asset paths or parallel audio services. |
| Plan 40 data-driven 2D character presentation and frame collision | ReEcho teammate-side / `Unassigned` | Codex | `Ready` | `PendingBeforeClose` | `origin/main@ec0f5e3`; continue preserved local animation WIP only after this Plan is published | Shared animation/profile/controller/collision-track contract plus Exclusive `Content/2DAnim/**` and `/Game/ReEcho/Animation2D/**` asset writes | Depends on closed Plan39 and published Plan38; preserve committed-attack retry semantics, existing artist assets and stable Capsule movement authority. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan26 / teammate-side task branch | `ReadOnly` | `Reserved` | Typed weapon UI-consumer behavior and Plan26 tests | Must preserve Plan29 presentation bindings. |
| Gavyn-side Planner | Plan34 / `plan/34-audio-catalog-settings` | `SharedContract` + `Exclusive` | `Active` | `Design/Data/ReEchoData.xlsx`, generated `Content/Data/audio_events.csv`, settings UI and Plan33 catalog/settings provider surface | Branch implementation is local. Current main independently changed the workbook, Build.cs, XLSX generator and validator; Planner adaptation is required before review. |
| Codex | Plan40 / `codex/animation-idle-walk-attack` | `SharedContract` + `Exclusive` | `Reserved` | `Presentation/Animation2D/**`, narrowly required player/enemy presentation call sites, `Content/2DAnim/**`, `/Game/ReEcho/Animation2D/**`, animation/collision tests and tooling | Becomes Active only when an Executor starts. Frame Hurtbox/AttackHitbox are Query-only; do not replace the stable movement Capsule or overwrite pre-existing dirty art. |

## Warnings / blocked items

- Fetch `origin/main` before numbering or remote integration. If it advanced, report physical, logical, coupling and numbering differences before pull/merge/rebase/push.
- Remote side branches are prohibited; Plans, task branches and worktrees stay local until accepted scope enters main.
- Legacy remote `origin/review/gavyn-plan28-attack-modes-20260812` still exists with five commits that are not patch-equivalent to current `main`. Do not merge it; compare its semantics with the closed Plan28/38 result before deciding whether the ref is safely removable.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.
- Plan34 cannot be merged wholesale: the branch and current main both changed `Design/Data/ReEchoData.xlsx`, `Source/ReEcho/ReEcho.Build.cs`, `scripts/data/sync_xlsx_to_csv.py` and `scripts/validate_project.py`. Regenerate the AudioEvents Table on the current canonical workbook, combine the text contracts explicitly and rerun all data/build evidence.
- Locked product decisions: selecting attack mode does not auto-resume; Continue restores the run-local choice while a new run defaults automatic; every active Echo contributes fog visibility; an empty explicit replay selection produces no Echo.

## Pending human decisions

- None.
