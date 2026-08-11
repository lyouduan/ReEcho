# ReEcho planner exchange

This is a live coordination board, not history or permanent rules. Remove ownership rows when implementation ends; Git and `plans/` retain completed history.

## Planned and active work announcements

| Date | Plan | Planner | Status | Remote ref / base | Writes and shared impact | Dependencies | Other planner action |
|---|---|---|---|---|---|---|---|
| 2026-08-11 | Workflow: remote `main` publication gate | Gavyn-side Planner | Rule published; teammate acknowledgement requested | `origin/coord/remote-main-gate-20260811` | Workflow documentation only: local acceptance/merge and remote publication are separate one-use authorization points; publish candidates must be rebuilt from current `origin/main`, contain only named accepted scope, be revalidated, receive explicit human approval and never force-push. | None; this rule does not authorize the pending Plans 21–24 synchronization | Fetch this ref, read commit announced by Gavyn-side Planner, adopt the same gate in future planning/prompts, and reply through the teammate planning broadcast if any conflict exists. Do not add current WIP to `origin/main`. |

## Active ownership

| Date | Owner | Plan/branch | Files or exclusive resources | Status | Notes |
|---|---|---|---|---|---|
| — | — | — | — | None | No merge-hostile resource is currently claimed |

## Recently closed

| Date | Plan | Result | Human follow-up |
|---|---|---|---|
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
| 2026-08-11 | Local `main` merge and remote `origin/main` publication are separate gates. Every remote publication requires current-remote integration, accepted-scope-only candidate, post-integration verification, exact one-use human authorization, non-force publication and a published resulting commit; planning/task refs never imply permission | Human clarification; `WORKFLOW.md` §3.4 and Planner/Executor rules | Adopted; teammate Planner acknowledgement requested |

## Warnings / blocked items

- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- Decide whether root blueprint documents should remain as provenance after the workflow baseline is fully adopted.
