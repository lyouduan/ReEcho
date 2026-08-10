# ReEcho planner exchange

This is a live coordination board, not history or permanent rules. Remove ownership rows when implementation ends; Git and `plans/` retain completed history.

## Active ownership

| Date | Owner | Plan/branch | Files or exclusive resources | Status | Notes |
|---|---|---|---|---|---|
| — | — | — | — | None | No merge-hostile resource is currently claimed |

## Recently closed

| Date | Plan | Result | Human follow-up |
|---|---|---|---|
| 2026-08-10 | Plan 17 guided code reading | Read-only walkthrough of startup, encounter, recording, run-state and echo-playback flow; human accepted | Choose GAS/combat, growth/shop or UI/presentation for a future focused reading plan if needed |
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
| 2026-08-10 | CSV is the target designer-editable runtime source for characters/builds/elements/weapons/slots; migrate through Plans 18–21 and do not keep CSV/JSON/C++ as parallel editable truths | Plans 18–21; update `PROJECT_RULES.md` when Plan 18 lands | Adopted |

## Warnings / blocked items

- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- Decide whether root blueprint documents should remain as provenance after the workflow baseline is fully adopted.
