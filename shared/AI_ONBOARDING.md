# ReEcho AI onboarding

This file is for first-time onboarding or workflow maintenance. Ordinary tasks follow `AGENTS.md` directly.

## Entry contract

1. `AGENTS.md` is the sole startup and retrieval-order authority.
2. `shared/PROJECT_RULES.md` contains project constraints.
3. Read only the role section, Plan, Exchange blocks and code route required by the current task.
4. Write durable task evidence back to the assigned Plan; shared project memory is updated by the Planner during review unless the task explicitly owns that document.

## Roles

- **Executor**: implements a concrete task on an isolated branch/worktree, runs objective checks and hands off its branch. It does not merge or publish `main`.
- **Planner**: plans, reviews, coordinates ownership/contracts, integrates accepted work and maintains shared state.
- When the user has not assigned planning/review/workflow responsibility, default to Executor scope.

## Two safety lines

- Claim merge-hostile or external resources before writing them. Read-only consumption does not require an exclusive claim.
- Never commit machine-local paths, credentials, `.env`, caches, intermediate products or private tool configuration.

For the rationale and collaboration topology, read only the relevant section of `shared/WORKFLOW.md`.
