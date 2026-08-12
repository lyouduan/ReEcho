# ReEcho agent entry

Use this file as the only mandatory reading-order authority. Other documents must not redefine the startup list.

Repository authority order is: human instructions, `shared/PROJECT_RULES.md`, the matching role rules, locked Plan scope/current Exchange coordination, and finally explanatory `shared/WORKFLOW.md`.

## Every task: minimal context

1. Read `shared/PROJECT_RULES.md`.
2. Use `rg` to extract only the matching row from `shared/CODEBASE_MAP.md`; do not read the whole map.
3. Read only planned/active work announcements, active ownership, warnings, and pending decisions in `shared/PLANNER_EXCHANGE.md`.
4. Read the assigned `plans/<id>-*.md` when one exists.
5. Read the public header and paired implementation from the selected route before expanding retrieval.

## Conditional context

- Implementation: read the relevant section of `shared/EXECUTOR_RULES.md` and only the matching `shared/LESSONS.md` section. Add `§DEBUG` only after a failure requires diagnosis.
- Planning/review/closure: read `shared/PROJECT_STATE.md`, the relevant section of `shared/PLANNER_RULES.md`, and the plan diff. Do not load all of `LESSONS.md`.
- Distributed planning: before reserving a Plan number or integrating/publishing work, fetch `origin/main`; remote main owns published Plan numbers, and external differences follow the audit gate in `shared/PLANNER_RULES.md`.
- Workflow maintenance or first-time onboarding only: read `shared/AI_ONBOARDING.md` and the relevant `shared/WORKFLOW.md` section.

`shared/WORKFLOW.md` explains the model but does not override the authorities above.
