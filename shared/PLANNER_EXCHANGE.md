# ReEcho planner exchange

This is a live coordination board, not history or permanent rules. Remove ownership rows when implementation ends; Git and `plans/` retain completed history.

## Active ownership

| Date | Owner | Plan/branch | Files or exclusive resources | Status | Notes |
|---|---|---|---|---|---|

## Recently closed

| Date | Plan | Result | Human follow-up |
|---|---|---|---|
| 2026-08-11 | Plan 24 weapon/slot CSV rework | Equipped-part snapshots/API, seven-table weapon-domain revision, pinned run/player/echo weapon snapshots, attack-step runtime semantics and 32 passing tests | Unified Human PIE after Plan 25 |
| 2026-08-10 | Plan 23 element/status/reaction CSV migration (formerly local Plan 20) | Typed element domain reader, six ordered reactions, corrected workbook formulas, deterministic Burn refresh/save continuation and 27 passing tests; planner accepted for local merge | Continue with Plan 24 weapon/slot CSV migration; unified human PIE remains after Plan 25 |
| 2026-08-10 | Plan 22 character/build CSV migration (formerly local Plan 19) | Character/build domain reader, runtime-authoritative character/card tables, hard failure without fallback, atomic effects and 22 passing tests; human accepted and planner merged locally | Continue with Plan 23 element migration |
| 2026-08-10 | Plan 21 CSV runtime foundation (formerly local Plan 18) | Immutable startup-loaded CSV snapshot, strict typed validation, 19 tests and clean Shipping loose-file smoke; human accepted and planner merged locally | Continue serially with Plan 22 |
| 2026-08-10 | Plan 20 guided code reading (formerly local Plan 17) | Read-only walkthrough of startup, encounter, recording, run-state and echo-playback flow; human accepted | Choose a future focused reading plan if needed |
| 2026-08-10 | Plan 19 trait draw/shop flow | Centered animated draw presentation and post-draw shop continuation; integrated with local CSV build flow | Unified PIE after Plan 25 |
| 2026-08-10 | Plan 18 initial loadout selection | Start-flow character and initial-weapon selection retained; the proposed full-run weapon lock was rejected during integration | Unified PIE after Plan 25 |
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
| 2026-08-10 | Loadout choices consume the shared typed data source; missing configured character/weapon IDs fail loudly. Suspended elemental durations use remaining/rebased time rather than persisted absolute world time | Human integration decision; Plans 18, 23 and 24 | Adopted |
| 2026-08-10 | CSV is the target runtime package for characters/builds/elements/weapons/slots; migrate through Plans 21–24 and do not keep CSV/JSON/C++ as parallel editable truths | Plans 21–24 | Adopted |
| 2026-08-10 | Plans 22–24 execute serially (`22 → 23 → 24`); each gets its own local worktree/branch because snapshot, manifest/schema, behavior registration, packaging and validator are shared | Plan 21 final implementation; revised Plans 22–24 | Adopted |
| 2026-08-10 | Plan 25 will make XLSX the designer authoring source and deterministically generate validated CSV runtime packages; unified human PIE follows Plan 25 | Human integration decision | Adopted |

## Warnings / blocked items

- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- Decide whether root blueprint documents should remain as provenance after the workflow baseline is fully adopted.
