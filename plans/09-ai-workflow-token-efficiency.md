# Plan 09 — AI workflow token-efficiency cleanup

## Goal

Remove repeated startup instructions, stale state and redundant verification behavior from the ReEcho AI workflow without changing gameplay or the cross-project canonical blueprint.

## Acceptance

- `AGENTS.md` is the only mandatory startup-order authority and uses conditional, task-scoped retrieval.
- Current state and coordination files contain current facts rather than accumulated implementation history.
- Finished Plans 07/08 no longer retain active binary-asset ownership.
- Verification requirements are selected by change surface and successful evidence is not rerun after unrelated edits.
- Static validation detects duplicate startup authorities, stale completed-plan ownership and oversized current-state regressions.
- `python scripts/validate_project.py` and `git diff --check` pass; no Unreal build is required for Markdown/Python workflow-only changes.

## Implementation

- Tighten the project entry and role overlays; leave the imported generic workflow blueprint intact.
- Compact `PROJECT_STATE.md`, archive recent completion summaries in the coordination board, and keep deeper history in plans/Git.
- Extend static validation with inexpensive workflow invariants.

## Execution notes

- Consolidated startup order into `AGENTS.md`; onboarding/workflow documents are now conditional context.
- Replaced duplicated role-specific commit sections with references to project rules.
- Added a change-surface verification matrix and explicit evidence-reuse rule.
- Compacted project state and cleared stale Plan 07/08 ownership while retaining recent human PIE follow-ups.
- Added static guards for document size, duplicate startup phrases, current automation count and stale active ownership.
- Static validation, Python syntax validation, and `git diff --check` pass; Unreal build/automation were intentionally not rerun because no runtime source, config, or assets changed.
