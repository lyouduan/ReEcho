# ReEcho Project Secretary rules

This file is the single authority for the Project Secretary repository duty. It is a coordination duty, not a fourth professional user role and not a substitute for the programmer, designer or artist routes in `AGENTS.md`.

## When this duty applies

- Use Project Secretary duty only when the human explicitly assigns secretary, workflow-maintenance, cross-project coordination, rule-audit, Plan-schema, integration-publication or repository-cleanup work.
- Read `AGENTS.md`, `shared/PROJECT_RULES.md`, this file, the live coordination sections of `shared/PLANNER_EXCHANGE.md`, and only the other rule/state/index/Plan files needed for the secretary change.
- If the task crosses into gameplay implementation, content/balance authorship, visual/audio production or subjective product choice, route that work to the appropriate Planner/Executor or specialist. The Secretary records the handoff and dependency but does not make the product decision.

## Authority

- May maintain repository-wide rules, Plan templates and numbering, shared coordination documents, workflow/document indexes, validation scripts/tests and Git collaboration state.
- May update `PLANNER_EXCHANGE.md`, `PROJECT_STATE.md`, `CODEBASE_MAP.md`, `LESSONS.md`, `shared/WORKFLOW.md`, `AGENTS.md`, `plans/TEMPLATE.md`, and rule files when the change is coordination/control-plane work.
- May integrate already accepted results, create local merge commits, and push `origin/main` after the current gate passes.
- Must not create or push any remote branch other than `origin/main`; must not force-push.
- Must not override uncommitted work, delete dirty worktrees, bypass validation, expand feature scope, or decide product tradeoffs without human selection.

## No Secretary Plan overhead

- A Project Secretary task or Secretary-created commit does not require a Plan, Plan number, Plan file, Planner/Executor lifecycle, or a Secretary task/ownership row in `PLANNER_EXCHANGE.md`.
- Work directly from the explicit human assignment and record the result in the Secretary commit, validation evidence and publication report.
- Do not reserve a Plan number or add ceremonial Exchange coordination merely to perform rule maintenance, audits, accepted-result integration, cleanup or publication.
- This exception applies only to the Secretary's coordination work. Specialist implementation being coordinated or integrated keeps its own Programmer/Designer/Artist Plan and acceptance history when those rules require one.
- Update `PLANNER_EXCHANGE.md` only when live specialist ownership, dependencies, conflicts, warnings or human decisions actually change; the Secretary does not create a row for itself.

## Secretary responsibilities

- Audit the rule system for ambiguity, duplication, conflict, stale references and rules that unnecessarily block parallel work.
- Keep Plan numbering, task status, human-validation status, AI-side provenance, ownership, dependencies, Writes/Reads and impact mode coherent across Plan files, Exchange and validator checks.
- Audit incoming remote changes for Physical/Git conflict, Logical conflict and Coupling before pull, merge, rebase, cherry-pick or push when `origin/main` advanced outside the approved baseline.
- Maintain `PLANNER_EXCHANGE.md` as live coordination only, `PROJECT_STATE.md` as current snapshot only, and workflow/document indexes as pointers rather than duplicate rulebooks.
- Remove obsolete coordination only after proving it is no longer live. Remove a local worktree only when it is clean, merged and the exact path has been verified; delete local branches only with non-force Git deletion.
- Integrate accepted implementation results without replacing the responsible Planner/Executor's technical review or the human's subjective validation.

## Publication gate

1. Start from current `origin/main`: fetch, confirm ancestry and inspect incoming commits when present.
2. Build a scoped candidate containing only approved secretary/control-plane changes or already accepted integration results.
3. Run the verification required by `shared/PROJECT_RULES.md`; for Markdown/workflow-only changes this is `python scripts/validate_project.py` and `git diff --check`.
4. Fetch again immediately before push. If `origin/main` advanced, repeat the external-change audit and wait for human choice when behavior, ownership or irreversible cleanup is implicated.
5. Push only local `main` to `origin/main`, without force, then fetch and compare local/remote main.

## Commit identity

- Every commit created by the Project Secretary must have a subject that starts with `[SECRETARY]`.
- Non-Secretary roles must not use the `[SECRETARY]` tag.
- Secretary commits should describe the coordination/control-plane outcome, not an implementation feature owned by another role.
