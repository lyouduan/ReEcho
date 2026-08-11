# ReEcho AI workflow pointer

This file is a human-facing directory pointer, not a workflow authority and not a startup checklist.

## Where workflow truth lives

| Question | Authority |
|---|---|
| What an AI reads first | [`AGENTS.md`](../AGENTS.md) |
| Project constraints and verification | [`shared/PROJECT_RULES.md`](../shared/PROJECT_RULES.md) |
| Planner / Executor actions | [`shared/PLANNER_RULES.md`](../shared/PLANNER_RULES.md), [`shared/EXECUTOR_RULES.md`](../shared/EXECUTOR_RULES.md) |
| Current work, ownership and warnings | [`shared/PLANNER_EXCHANGE.md`](../shared/PLANNER_EXCHANGE.md) |
| Current delivered state | [`shared/PROJECT_STATE.md`](../shared/PROJECT_STATE.md) |
| Code and document retrieval routes | [`shared/CODEBASE_MAP.md`](../shared/CODEBASE_MAP.md) |
| One task's scope and evidence | [`plans/`](../plans/) |

`shared/` is the collaboration control plane. It answers who owns work, what rules apply, what is currently delivered and how integration is decided. `docs/` is the technical knowledge layer: architecture, subsystem onboarding and tool/command guides.

Do not copy mandatory branch, ownership, status, validation or publication rules into this file. If a `docs/` page conflicts with `shared/` on collaboration or current delivery state, `shared/` wins and the stale `docs/` page should be corrected. Runtime behavior is ultimately verified against source and tests.

For the collaboration rationale rather than the mandatory actions, see [`shared/WORKFLOW.md`](../shared/WORKFLOW.md).
