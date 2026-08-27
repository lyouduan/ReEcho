# ReEcho AI workflow pointer

This file is a human-facing directory pointer, not a workflow authority and not a startup checklist.

## Where workflow truth lives

| Question | Authority |
|---|---|
| What an AI reads first | [`AGENTS.md`](../AGENTS.md) |
| Project constraints and verification | [`shared/PROJECT_RULES.md`](../shared/PROJECT_RULES.md) |
| Programmer / designer / artist route | [`shared/PROGRAMMER_RULES.md`](../shared/PROGRAMMER_RULES.md), [`shared/DESIGNER_RULES.md`](../shared/DESIGNER_RULES.md), [`shared/ARTIST_RULES.md`](../shared/ARTIST_RULES.md) |
| Project Secretary coordination duty | [`shared/SECRETARY_RULES.md`](../shared/SECRETARY_RULES.md) |
| Planner / Executor actions | [`shared/PLANNER_RULES.md`](../shared/PLANNER_RULES.md), [`shared/EXECUTOR_RULES.md`](../shared/EXECUTOR_RULES.md) |
| Architecture, module intent and code retrieval routes | [`shared/CODEBASE_MAP/README.md`](../shared/CODEBASE_MAP/README.md) |
| One task's scope and evidence | [`plans/`](../plans/) |
| Designer Bug / request records | [`issues/`](../issues/), routed by [`shared/DESIGNER_RULES.md`](../shared/DESIGNER_RULES.md) |

`shared/` is the collaboration control plane. It defines local autonomy, professional boundaries, Plan requirements and remote integration gates; it does not track live ownership. Delivered behavior is verified from source, tests, Plans and Git; `docs/` is the technical knowledge layer.

Do not copy mandatory branch, Plan, validation or publication rules into this file. If a `docs/` page conflicts with `shared/` on collaboration, `shared/` wins and the stale page should be corrected. Runtime behavior is verified against source and tests.

For the collaboration rationale rather than the mandatory actions, see [`shared/WORKFLOW.md`](../shared/WORKFLOW.md).
