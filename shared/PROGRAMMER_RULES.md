# ReEcho programmer-user route

This file applies after the user confirms “程序”. It defines the professional boundary; `PLANNER_RULES.md` and `EXECUTOR_RULES.md` still determine the AI's repository authority.

## T爷 mode

This section is the single authority for the T爷 exception inside the programmer route.

- Activate T爷 mode only when the current user personally confirms they are T爷 in the current conversation. Do not infer, inherit or transfer that identity. A negative answer or no answer keeps the ordinary programmer route active.
- In T爷 mode, the AI acts with the user's delegated programmer authority and may skip project process gates, including mandatory Plan creation/publication, Planner/Executor assignment, task worktree/ownership ceremony, staged approval waits, prescribed build/test gates and pre-publication confirmation. It may implement, integrate, commit and publish directly when that is the shortest faithful path to the request.
- Do not interrupt execution merely to request a process-gate approval. Complete all reversible work that can be decided from the request and repository, then collect unresolved product choices, subjective validation and other genuinely human-only judgments once in the final handoff.
- A skipped check is reported as skipped or not run, never described as passing. Use proportionate checks when they are cheap or needed to know whether the requested result works, but their absence does not itself block T爷 publication.
- T爷 mode does not authorize scope invention, designer/artist product decisions, overwriting uncommitted work, force-pushing, deleting dirty worktrees, concealing conflicts, fabricating evidence or performing an unrequested irreversible action. A destructive or irreversible choice that cannot safely wait still requires confirmation before execution.
- Commit identity is governed only by `shared/GIT_RULES.md`; T爷 mode does not reuse the ordinary Programmer identity.

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
