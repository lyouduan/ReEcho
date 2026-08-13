# ReEcho programmer-user route

This file applies after the user confirms “程序”. It defines the professional boundary; `PLANNER_RULES.md` and `EXECUTOR_RULES.md` still determine the AI's repository authority.

## Planner-Executor mode

This section is the single authority for the Planner-Executor mode choice inside the programmer route.

- Activate Planner-Executor mode only when the current user confirms in the current conversation they want to adopt it. Do not infer, inherit or transfer that choice. A negative answer or no answer keeps the lightweight direct-programmer route active.
- In Planner-Executor mode, the AI follows the formal Plan lifecycle in `PLANNER_RULES.md` / `EXECUTOR_RULES.md`: a numbered Plan published to `origin/main` before execution, ownership/Exchange rows, staged approval waits, prescribed build/readiness gates and pre-publication confirmation. Planner duty covers planning, review, local integration, closure and remote-main publication; Executor duty covers concrete implementation on an assigned local Plan/branch.
- In the lightweight (non-Planner-Executor) route, the AI works directly on the request without the formal Plan/ownership/lifecycle ceremony, but still selects Planner or Executor duty as the task requires and still obeys every non-skippable collaboration and safety check below.
- Neither mode authorizes scope invention, designer/artist product decisions, overwriting uncommitted work, force-pushing, deleting dirty worktrees, concealing conflicts, fabricating evidence or performing an unrequested irreversible action. A destructive or irreversible choice that cannot safely wait still requires confirmation before execution.
- Commit identity is governed only by `shared/GIT_RULES.md`; the chosen mode does not change the commit tag.

## Remote collaboration safety check (applies in every mode)

This is the core of safe collaboration and is NON-SKIPPABLE whether or not Planner-Executor mode is adopted.

- `git fetch` is the only automatic first step whenever remote state may have changed.
- If fetch reveals commits outside the currently approved local baseline (i.e., another person has pushed since the baseline), the AI must stop before `pull`, `merge`, `rebase`, `cherry-pick` or `push`. Report Physical/Git conflict, Logical conflict and Coupling, then wait for the human's explicit choice. A fast-forward or clean auto-merge is not an exemption.
- When the last push to the affected branch was made by another person, the human — not the AI — decides how to handle each Physical conflict and Logical conflict: **adopt (采用)**, **merge (合并)**, or **discard (抛弃)**. The AI presents the options and tradeoffs but does not auto-resolve ownership or behavior.
- This check cannot be waived by any mode, route or session.

## Select the AI duty

- Use Planner duty for planning, review, ownership/contracts, local integration, closure or remote-main publication.
- Use Executor duty for a concrete implementation on an assigned local Plan/branch.
- If neither was assigned, stay within Executor authority. Do not infer permission to merge or publish.

Before committing or publishing, follow the applicable identity and publication rule in `shared/GIT_RULES.md`.

## Programmer scope

- May change C++, Python tooling, schemas, configuration, public contracts and technical documentation when the Plan and ownership allow it.
- Treat gameplay feel, visual quality, wording usability and audio balance as human validation even when objective checks pass.
- Consume designer-authored XLSX and artist-authored assets through their published contracts; do not replace their source of truth with convenience constants or hidden generated files.
- When a request is primarily balance/content authoring or visual/audio production, identify the designer/artist boundary and ask the user before switching routes.

## Cross-role handoff

- Give designers stable fields, units, allowed IDs and validation errors; do not ask them to encode new logic in prose cells.
- Give artists exact asset paths, dimensions/formats, binding names and runtime constraints; do not ask them to modify gameplay state in Blueprint.
- Review specialist changes for runtime contracts and integration safety, but leave subjective acceptance to the corresponding human owner.
