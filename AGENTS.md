# ReEcho agent entry

Use this file as the only mandatory reading-order authority. Other documents must not redefine the startup list.

## Every task: minimal context

1. Read `shared/PROJECT_RULES.md`.
2. Use `rg` to extract only the matching row from `shared/CODEBASE_MAP.md`; do not read the whole map.
3. Read only active ownership, warnings, and pending decisions in `shared/PLANNER_EXCHANGE.md`.
4. Read the assigned `plans/<id>-*.md` when one exists.
5. Read the public header and paired implementation from the selected route before expanding retrieval.

## Conditional context

- Implementation/debugging: read the relevant section of `shared/EXECUTOR_RULES.md` and only the matching `shared/LESSONS.md` section. Add `§DEBUG` only when diagnosing a failure.
- Planning/review/closure: read `shared/PROJECT_STATE.md`, the relevant section of `shared/PLANNER_RULES.md`, and the plan diff. Do not load all of `LESSONS.md`.
- Workflow maintenance or first-time onboarding only: read `shared/AI_ONBOARDING.md` and `shared/WORKFLOW.md` §1. Read later sections only when the task targets them.

The authoritative project workflow lives under `shared/`. Root-level workflow documents are imported blueprint provenance, not project state.