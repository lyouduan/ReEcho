# ReEcho planner exchange

This is a live coordination board, not history or permanent rules. Remove ownership rows when implementation ends; Git and `plans/` retain completed history.

## Planned and active work announcements

| Date | Plan | Planner | Status | Remote ref / base | Writes and shared impact | Dependencies | Other planner action |
|---|---|---|---|---|---|---|---|
| 2026-08-11 | Shared settings UI and full-run weapon lock | ReEcho teammate-side Planner | Human-authorized publication candidate `420e5bb`; Editor Development, static validation and 34/34 `ReEcho.*` automation pass | Rebuilt from `origin/main@fc5048f`; candidate `420e5bb` | Shared settings shell used by start/pause menus; fixes settings color symbol collision; removes runtime weapon hotkeys/GAS abilities/run setter/recording and echo switch timeline while retaining pre-run selection, save restore and compatibility `InputSlot` data | Accepted Plans 24/25 data contracts; direct user decision supersedes the earlier Plan 18 runtime-switch decision | Fetch the resulting `origin/main`; do not restore runtime switching from historical Plan 03/13/24 notes. Plan26 may continue read-only UI work after rebasing and checking its reserved UI files. |
| 2026-08-11 | Workflow: remote `main` publication gate | Gavyn-side Planner | Published to `origin/main` at `df6e60a`; teammate rebase/acknowledgement requested | `origin/main` at `df6e60a`; coordination detail on `origin/coord/remote-main-gate-20260811` | Workflow documentation only: local acceptance/merge and remote publication are separate one-use authorization points; publish candidates must be rebuilt from current `origin/main`, contain only named accepted scope, be revalidated, receive explicit human approval and never force-push. The published commit contains only six shared workflow documents and no Plans 21–26 implementation. | None; this rules-only publication does not authorize or include the pending Plans 21–24 implementation synchronization | Fetch `origin/main` at `df6e60a`, adopt the same gate in future planning/prompts, and reply through the teammate planning broadcast if any conflict exists. Do not add current WIP to `origin/main`. |
| 2026-08-11 | Plan 25 XLSX authoring and CSV generation | Gavyn-side Planner | Remote-main publication explicitly authorized; designer QA follows publication | `origin/plan/25-xlsx-authoring` at `0bce447`; publication candidate rebuilt with current `origin/main` | Exactly one canonical `Design/Data/ReEchoData.xlsx`; seven familiar Chinese production Sheets plus protected export metadata; 16 manifest data CSVs are deterministic generated outputs; repository-pinned Python XLSX dependency; transactional publish/rollback/recovery through `scripts/data/sync_xlsx_to_csv.py`, `--check`, and optional `--sheet`. | Accepted Plans 21–24 | Fetch the resulting `origin/main` before further work. Designer QA uses `Design/Data/ReEchoData策划验收清单.md`; do not create a competing workbook/schema or hand-edit generated CSV as another truth. |
| 2026-08-11 | Plan 26 weapon read-only UI consumers | ReEcho teammate-side Planner | Ready; executor may start after fetching integration ref | `origin/coord/accepted-plan24-20260811`; teammate planning response `origin/coord/teammate-planning-broadcast-20260811` | Writes only `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*` and Plan26-only UI tests; consumes stable weapon identity/display/query APIs and does not mutate registry/reader/schema. | Accepted Plan24 consumer surface on the integrated UI/data lineage | Reservation is approved. Create/rebase Plan26 from the advertised integration ref; do not add missing Registry/Reader APIs or edit Plan24/25-owned files. |

## Active ownership

| Date | Owner | Plan/branch | Files or exclusive resources | Status | Notes |
|---|---|---|---|---|---|
| 2026-08-11 | Gavyn-side Plan25 designer QA | Plan 25 / resulting `origin/main` | `Design/Data/ReEchoData.xlsx` and its generated 16 production CSVs | Human designer QA after publication | Do not make parallel canonical workbook edits while the designer usability test is active. Plan26 UI remains outside this ownership and may only consume typed APIs/data. |
| 2026-08-11 | ReEcho teammate-side Planner | Plan 26 / future `plan/26-weapon-readonly-ui` | `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*`, Plan26-only UI tests | Reserved, not active | Reservation acknowledged; excludes every Plan24/25-owned data, registry, reader, run and weapon file. |

## Recently closed

| Date | Plan | Result | Human follow-up |
|---|---|---|---|
| 2026-08-11 | Plan 24 weapon/slot CSV migration | Planner accepted executor `3c514bb`; seven typed weapon tables, idempotent equipment derivation, atomic compatible-part switch policy, seven-table revision, pinned Run/player/echo snapshots, attack-step runtime; Editor Development, 7/7 focused and 34/34 full automation pass | Unified Human PIE and XLSX value-edit verification after Plan 25 |
| 2026-08-10 | Plan 23 element/status/reaction CSV migration (formerly local Plan 20) | Typed element domain reader, six ordered reactions, corrected workbook formulas, deterministic Burn refresh/save continuation and 27 passing tests; planner accepted for local merge | Continue with Plan 24 weapon/slot CSV migration; unified human PIE remains after Plan 25 |
| 2026-08-10 | Plan 22 character/build CSV migration (formerly local Plan 19) | Character/build domain reader, runtime-authoritative character/card tables, hard failure without fallback, atomic effects and 22 passing tests; human accepted and planner merged locally | Continue with Plan 23 element migration |
| 2026-08-10 | Plan 21 CSV runtime foundation (formerly local Plan 18) | Immutable startup-loaded CSV snapshot, strict typed validation, 19 tests and clean Shipping loose-file smoke; human accepted and planner merged locally | Continue serially with Plan 22 |
| 2026-08-10 | Plan 20 guided code reading (formerly local Plan 17) | Read-only walkthrough of startup, encounter, recording, run-state and echo-playback flow; human accepted | Choose a future focused reading plan if needed |
| 2026-08-10 | Plan 19 trait draw/shop flow | Centered animated draw presentation and post-draw shop continuation; integrated with local CSV build flow | Unified PIE after Plan 25 |
| 2026-08-10 | Plan 18 initial loadout selection | Start-flow character and initial-weapon selection retained; a later direct user decision now locks that selected weapon for the full run and supersedes the earlier switching decision | Unified PIE: confirm 1/2/3 do not switch and continue/echo retain the selected WeaponId |
| 2026-08-10 | Plan 17 start/continue save | Save-aware startup, confirmed save-and-quit, suspended encounter restore; build and 16 tests pass | PIE real quit/relaunch state, glyph/DPI and transient projectile behavior |
| 2026-08-05 | Plan 16 bomber ranges | Configurable trigger/damage radii and fuse behavior; build and 15 tests pass | PIE trigger readability and tuning |
| 2026-08-05 | Plan 15 character promotion | Four deterministic roles, forge/Sage cadence, runtime health correction; build and 15 tests pass | PIE role readability and pacing |
| 2026-08-05 | Plan 14 element reactions | Deterministic attachments/reactions and persistent feedback; build and 15 tests pass | PIE colors, pacing and damage readability |
| 2026-07-30 | Plan 11 time trait draw | Deterministic least-owned offers, pending validation and two new tests; build/eight tests pass | Compare visual draw pacing with supplied video |
| 2026-07-30 | Plan 10 player health HUD | Top-left portrait/live health HUD; player overhead bar removed; build and six tests pass | PIE safe-area, crop, glyph and DPI review |
| 2026-07-30 | Plan 09 AI workflow token efficiency | Startup context consolidated; state compacted; static drift guards pass | Monitor future task startup size |
| 2026-07-30 | Plan 08 player/echo stats | Commit `3b7c4a2`; build, six tests and static checks pass | PIE layout, glyph, DPI and blur review |
| 2026-07-30 | Plan 07 inventory/shop | Commit `8fdf51e`; build, six tests and static checks pass | PIE purchase flow and supplied-art layout |
| 2026-07-29 | Plan 06 weather | Commit `ce2d63f`; build, five tests and static checks pass | PIE rain/fog tuning |
| 2026-07-23 | Plan 04/05 presentation cleanup | Merged to main as `0a97541` | Ongoing play-feel tuning |

## Decisions

| Date | Decision | Authority | Status |
|---|---|---|---|
| 2026-07-21 | `shared/` is the project workflow authority | `AGENTS.md`, `PROJECT_RULES.md` | Adopted |
| 2026-07-21 | UE binary assets are serially owned | `PROJECT_RULES.md` | Adopted |
| 2026-07-21 | Evidence levels must remain explicit | `PROJECT_RULES.md`, `PROJECT_STATE.md` | Adopted |
| 2026-07-30 | `AGENTS.md` is the only startup-order list; verification is change-surface based | `AGENTS.md`, `PROJECT_RULES.md` | Adopted |
| 2026-08-10 | After human acceptance and planner merge, the planner safely removes the clean executor worktree and its merged local branch; remote branch deletion remains a human decision | `WORKFLOW.md`, `PLANNER_RULES.md`, `EXECUTOR_RULES.md` | Adopted |
| 2026-08-10 | Remote task numbers are canonical. Local plans move as one block: `17→20`, `18→21`, `19→22`, `20→23`, `21→24`; the planned XLSX authoring migration becomes Plan 25 | Human integration decision; `plans/` and downstream references | Adopted |
| 2026-08-10 | Remote wins only for start/continue, save-and-quit, initial loadout UI, trait presentation and post-draw shop. Unchanged or conflicting gameplay remains local-authoritative | Human integration decision; integration branch | Adopted |
| 2026-08-10 | Plan 18 keeps initial character/weapon selection but rejects full-run weapon locking. Runtime weapon switching, recording and echo playback semantics remain supported | Human integration decision; Plans 18 and 24 | Adopted |
| 2026-08-11 | Supersede the prior Plan 18 switching decision: character and weapon are chosen before the run, and the weapon is locked for the full run. Remove runtime input/GAS switching, mutable run setter, weapon-change recording and echo replay switching | Direct user decision; Plan 18 runtime contract | Adopted |
| 2026-08-10 | Loadout choices consume the shared typed data source; missing configured character/weapon IDs fail loudly. Suspended elemental durations use remaining/rebased time rather than persisted absolute world time | Human integration decision; Plans 18, 23 and 24 | Adopted |
| 2026-08-10 | CSV is the target runtime package for characters/builds/elements/weapons/slots; migrate through Plans 21–24 and do not keep CSV/JSON/C++ as parallel editable truths | Plans 21–24 | Adopted |
| 2026-08-10 | Plans 22–24 execute serially (`22 → 23 → 24`); each gets its own local worktree/branch because snapshot, manifest/schema, behavior registration, packaging and validator are shared | Plan 21 final implementation; revised Plans 22–24 | Adopted |
| 2026-08-10 | Plan 25 will make XLSX the designer authoring source and deterministically generate validated CSV runtime packages; unified human PIE follows Plan 25 | Human integration decision | Adopted |
| 2026-08-11 | In distributed multi-Planner work, Plans and impact announcements are committed and pushed to a shared planning ref before Executors start; scope expansion is rebroadcast before crossing ownership boundaries | Human workflow decision; `WORKFLOW.md` §3.9 and Planner/Executor rules | Trial adopted with Plans 24–25 |
| 2026-08-11 | The first trial uses `coord/planning-broadcast-20260811` because `origin/main` does not yet contain the accepted local Plans 21–23 baseline; publishing planning refs and WIP task branches does not authorize merging unfinished implementation to main | Human workflow decision; current Git topology | Active trial |
| 2026-08-11 | Plan25 uses exactly one canonical workbook with familiar Chinese Sheets; same-Sheet named Tables and protected `_ExportMap` may fan out to multiple generated CSVs. Production export never infers logic from prose or depends on Excel formula caches | Plan25 calibration after inspecting the source workbook | Adopted for Plan25 |
| 2026-08-11 | Local `main` merge and remote `origin/main` publication are separate gates. Every remote publication requires current-remote integration, accepted-scope-only candidate, post-integration verification, exact one-use human authorization, non-force publication and a published resulting commit; planning/task refs never imply permission | Human clarification; `WORKFLOW.md` §3.4 and Planner/Executor rules | Adopted and published to `origin/main` at `df6e60a`; teammate acknowledgement requested |

## Warnings / blocked items

- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.
- The authorized Plan25 publication candidate integrates the current rules-only `origin/main` with the accepted Plans 18–24/UI lineage and Plan25. After publication, all planners must fetch the resulting `origin/main` before reserving or rebasing further work.
- Plan25 designer QA owns the canonical workbook during the usability test; other work may consume generated CSV/API data but must not edit the workbook or generator contract in parallel.

## Pending human decisions

- Decide whether root blueprint documents should remain as provenance after the workflow baseline is fully adopted.
