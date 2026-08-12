# ReEcho multi-agent workflow

This document explains the collaboration model. It is not a second rulebook.

## Authority map

| Concern | Authority |
|---|---|
| Startup and retrieval order | `AGENTS.md` |
| Project constraints, ownership, branches and verification | `shared/PROJECT_RULES.md` |
| Programmer/designer/artist boundaries | `shared/PROGRAMMER_RULES.md`, `shared/DESIGNER_RULES.md`, `shared/ARTIST_RULES.md` |
| Planner actions | `shared/PLANNER_RULES.md` |
| Executor actions | `shared/EXECUTOR_RULES.md` |
| Current local work, ownership and warnings | `shared/PLANNER_EXCHANGE.md` |
| One task's scope and evidence | `plans/<id>-*.md` |
| Current delivered product state | `shared/PROJECT_STATE.md` |

If explanatory text here differs from an authority above, the authority above wins. Workflow changes are made in this repository like any other reviewed change.

## Roles and lifecycle

- First contact selects the user's professional route: programmer, designer or artist. This is distinct from the AI's repository duty.
- The human sets direction, decides subjective validation and chooses how external-main differences are integrated.
- A Planner defines scope, reviews delivery, integrates accepted local work and is the only role that may publish `main`.
- An Executor implements on an isolated local branch/worktree, records evidence and hands the local branch back. Executors never push remote refs.
- Designer and artist AIs operate as local specialists. They hand data/assets and evidence to a programmer Planner rather than publishing remote state themselves.

Task lifecycle is `Proposed -> Ready -> InProgress -> Review -> Closed`; `Blocked` is exceptional and names an unblock condition. Human validation remains separate: `NotRequired`, `PendingBeforeClose`, `PendingFollowUp`, or `Passed`.

## Local coordination contract

Every Plan declares owners, lifecycle, human validation, local/implementation base, dependencies, exact Writes/Reads, impact mode, compatibility promise and exclusions. Ownership is `Reserved`, `Active`, or `Released`.

- `Isolated`: disjoint files and no shared contract change.
- `ReadOnly`: consumes a stable surface without editing provider-owned files.
- `SharedContract`: changes an API, stable ID, schema, save format or generator contract; affected local work agrees before crossing it.
- `Exclusive`: edits a merge-hostile artifact or external target; only one active writer exists.

A reservation announces local intent but does not block unrelated work. Only an `Active Exclusive` row blocks another writer. Scope expansion is recorded locally before the boundary is crossed.

## Local parallelism and cross-machine integration

Same-machine Executors use separate worktrees and local branches. Different teammates use separate clones and local branches. File ownership and contract boundaries determine safe parallelism inside each clone.

The remote repository has exactly one branch: `main`. There are no remote planning, task, handoff or review branches. Planning drafts are not published merely to announce intent; they travel with the accepted implementation when that candidate eventually enters main.

This deliberately moves cross-machine coordination to integration time. A teammate learns about another machine's accepted planning/implementation by fetching `origin/main`. Before pull or push, the Planner reports Physical/Git conflict, Logical conflict and Coupling, plus any Plan-number collision. The human chooses remote, local, combined adaptation or deferral.

Remote main owns Plan numbers. If an unpublished local sequence collides, shift the colliding Plan and every later unpublished local Plan together to the first free ordered range, then update live references. This preserves remote history and local dependency order.

Provider/consumer work may proceed in parallel inside one clone after agreeing on a local contract. Across machines, a consumer cannot rely on unpublished provider work; the provider must first be accepted into main, then the consumer fetches/audits main and integrates it.

Shared documentation stays out of Executor hot paths: Executors update their Plan and implementation-local docs; Planners update shared state/routes/lessons at review. Exchange changes during implementation are limited to local ownership, scope, dependency or contract changes.

## Canonical workbook and exclusive resources

`Design/Data/ReEchoData.xlsx` has one active repository writer (`WorkbookWriter`); generated production CSV follows the same publication unit. Read-only consumers and disposable local QA may proceed concurrently. A publishable QA edit is handed to the current writer.

The Unreal Editor lock is machine-local but shared by all worktrees in one clone through the Git common directory. It serializes only operations that actually require exclusive Editor/project access; commands live in `EXECUTOR_RULES.md`.

## Local and remote lanes

1. **Local planning/execution**: Plans, task branches, worktrees and handoffs remain local. They may be committed locally for safety but are not pushed as side refs.
2. **Local acceptance**: the Planner reviews an Executor commit, resolves human validation and merges accepted scope into local main.
3. **Remote-main integration**: fetch, audit external main, resolve Plan-number and behavioral differences with the human, validate the resulting candidate, then push only main without force.

One-candidate or narrowly documented standing authorization still governs main publication. A local Plan or task commit never inherits that authority.

## Why this shape

- One remote branch removes stale coordination refs and duplicate branch cleanup.
- Local worktrees preserve same-machine parallelism without exposing WIP remotely.
- The external-main audit makes semantic coupling visible before pull/push rather than pretending a clean Git merge is sufficient.
- Remote-canonical numbering resolves independent Planner collisions without rewriting published history.
- The tradeoff is explicit: different machines cannot consume one another's unpublished contracts or review WIP through Git remote refs.
