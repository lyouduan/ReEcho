# ReEcho programmer-user route

This file applies after the user confirms “程序”. It defines the professional boundary; `PLANNER_RULES.md` and `EXECUTOR_RULES.md` still determine the AI's repository authority.

## Select the AI duty

- Use Planner duty for planning, review, ownership/contracts, local integration, closure or remote-main publication.
- Use Executor duty for a concrete implementation on an assigned local Plan/branch.
- If neither was assigned, stay within Executor authority. Do not infer permission to merge or publish.

Before committing or publishing, follow the Programmer identity and final integrated build/bundle gate in `shared/GIT_RULES.md`.

## Programmer scope

- May change C++, Python tooling, schemas, configuration, public contracts and technical documentation when the Plan and ownership allow it.
- Treat gameplay feel, visual quality, wording usability and audio balance as human validation even when objective checks pass.
- Consume designer-authored XLSX and artist-authored assets through their published contracts; do not replace their source of truth with convenience constants or hidden generated files.
- When a request is primarily balance/content authoring or visual/audio production, identify the designer/artist boundary and ask the user before switching routes.

## Cross-role handoff

- Give designers stable fields, units, allowed IDs and validation errors; do not ask them to encode new logic in prose cells.
- Give artists exact asset paths, dimensions/formats, binding names and runtime constraints; do not ask them to modify gameplay state in Blueprint.
- Review specialist changes for runtime contracts and integration safety, but leave subjective acceptance to the corresponding human owner.
