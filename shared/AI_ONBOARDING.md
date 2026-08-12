# ReEcho AI onboarding

This file is for first-time onboarding or workflow maintenance. Ordinary tasks follow `AGENTS.md` directly.

## Entry contract

1. `AGENTS.md` is the sole startup, first-contact question and retrieval-order authority.
2. Confirm directly whether the user is 程序、策划 or 美术 before reading the matching route.
3. `shared/PROJECT_RULES.md` contains project constraints.
4. Read only the professional role, duty, Plan, Exchange blocks and code/asset/data route required by the current task.
5. Write durable task evidence back to the assigned Plan or specialist handoff; shared project memory is updated by the Programmer Planner during review unless the task explicitly owns that document.

## Two independent role axes

- **User professional role**: 程序, 策划 or 美术. This selects `PROGRAMMER_RULES.md`, `DESIGNER_RULES.md` or `ARTIST_RULES.md`.
- **AI repository duty**: Planner, Executor or specialist. Only the programmer route uses project Planner/Executor authority; designer and artist AIs are specialists unless a programmer user explicitly reassigns the task.
- A specialist may prepare local changes and evidence but does not gain merge or publication authority from professional expertise.

## Two safety lines

- Claim merge-hostile or external resources before writing them. Read-only consumption does not require an exclusive claim.
- Never commit machine-local paths, credentials, `.env`, caches, intermediate products or private tool configuration.

For the rationale and collaboration topology, read only the relevant section of `shared/WORKFLOW.md`.
