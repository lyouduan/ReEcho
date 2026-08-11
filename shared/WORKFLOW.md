# ReEcho multi-agent workflow

This document explains the collaboration model. It is not a second rulebook.

## Authority map

| Concern | Authority |
|---|---|
| Startup and retrieval order | `AGENTS.md` |
| Project constraints, ownership, branches and verification | `shared/PROJECT_RULES.md` |
| Planner actions | `shared/PLANNER_RULES.md` |
| Executor actions | `shared/EXECUTOR_RULES.md` |
| Current work, ownership and warnings | `shared/PLANNER_EXCHANGE.md` |
| One task's scope and evidence | `plans/<id>-*.md` |
| Current delivered product state | `shared/PROJECT_STATE.md` |

If explanatory text here differs from an authority above, the authority above wins. Workflow changes are made in this repository like any other reviewed change; there is no external master that can silently override the committed project rules.

## Roles and lifecycle

- The human sets direction, decides whether subjective validation is satisfactory, and authorizes remote-main publication.
- A Planner defines scope and acceptance, publishes coordination, reviews delivery, integrates accepted work and maintains shared state.
- An Executor implements on an isolated branch/worktree, records evidence and hands the branch back. Executors do not merge or publish `main`.

The task lifecycle field uses one vocabulary only:

```text
Proposed -> Ready -> InProgress -> Review -> Closed
                         |           |
                         +-----------+  rework returns to InProgress

Blocked is exceptional and must include a concrete unblock condition.
```

Human validation is a separate field: `NotRequired`, `PendingBeforeClose`, `PendingFollowUp`, or `Passed`. `PendingBeforeClose` blocks closure; `PendingFollowUp` is an explicitly accepted deferred confidence check and may coexist with `Closed`. It must never be encoded into lifecycle status.

## Coordination contract

Every new distributed Plan declares:

- Planner and Executor owner;
- lifecycle status and human-validation state;
- published planning ref and implementation base;
- `Depends on` and `Blocks`;
- exact `Writes` and stable `Reads`;
- one impact mode: `Isolated`, `ReadOnly`, `SharedContract`, or `Exclusive`;
- downstream compatibility promise and explicit exclusions.

Ownership has a separate state: `Reserved`, `Active`, or `Released`.

- `Isolated`: disjoint files and no shared contract change; may start after the planning ref is published.
- `ReadOnly`: consumes a published stable surface without editing provider-owned files; may run in parallel.
- `SharedContract`: changes an API, stable ID, schema, save format or generated-data contract; provider and affected Planners agree on the contract before implementation crosses it.
- `Exclusive`: edits a merge-hostile artifact or external target; only one active writer exists.

A reservation announces intent but does not block unrelated work. Only an `Active` `Exclusive` row blocks another writer. Scope expansion is rebroadcast before files or contracts outside the published `Writes` are changed.

## Parallel execution and integration

Same-machine Executors use separate worktrees and branches. Different teammates use separate clones and remote branches. In both cases, file ownership and contract boundaries—not the number of machines—determine safe parallelism.

Use contract-first integration for provider/consumer work:

1. The provider publishes a stable consumer commit/ref and names the supported read-only surface.
2. Consumers branch from that ref or a newer `origin/main`, write only their declared surface and avoid provider internals.
3. Provider changes that break the promise require a new coordination broadcast.
4. Once the provider reaches `main`, each consumer fetches and rebases/merges the current `origin/main` before review, then reruns affected checks.
5. Old `coord/...` refs are handoff evidence, not permanent integration baselines.

Shared documentation is deliberately kept out of Executor hot paths:

- Executors update their Plan's Execution notes and documentation local to their owned implementation.
- Planners update `PROJECT_STATE`, `CODEBASE_MAP`, `LESSONS` and closed Exchange rows once at review/integration.
- Exchange changes during implementation occur only for ownership, scope, dependency or contract changes.

This keeps independent branches from conflicting on the same Markdown files.

External remote commits introduce a separate human decision point. A Planner first fetches without changing the working tree, then reports three independent results: **Physical/Git conflict** (what Git can combine), **Logical conflict** (whether the combined behavior still means the same thing), and **Coupling** (which Plans/contracts/tests move together). A fast-forward proves only ancestry; it does not prove semantic compatibility. Integration waits for the human to choose remote, local, a combined adaptation or deferral, and any further remote advance restarts the audit before push.

## Canonical workbook

`Design/Data/ReEchoData.xlsx` is one binary source and therefore has one active repository writer (`WorkbookWriter`). Generated production CSV files follow that writer and are not separately locked.

- `DomainOwner` describes requested row/domain changes in its Plan or handoff without committing a competing canonical workbook.
- `ReadOnly` runtime/UI consumers may use the generated CSV and typed APIs concurrently.
- A local designer usability test that is not intended for publication does not claim repository ownership.
- If a QA edit should be kept, it is handed to the current `WorkbookWriter` for application and regeneration.

## Local exclusive resources

The Unreal Editor lock is machine-local but shared by all worktrees in the same clone. Its location is derived from `git rev-parse --git-common-dir`, not from a worktree-local `.locks/` directory. Separate clones on separate machines do not share this lock.

The lock only serializes commands that actually require exclusive Editor/project access. Source inspection, independent text edits, Python data checks and other nonexclusive work continue in parallel. Stale-lock handling and Windows commands live in `EXECUTOR_RULES.md`.

## Planning and publication lanes

There are three different remote operations:

1. **Planning broadcast**: a pure Plan/Exchange batch pushed to `coord/<owner>-<topic>`. It lets other Planners and Executors see scope before implementation starts; it does not publish functionality.
2. **Task handoff**: an Executor pushes its `plan/<id>-<short>` implementation branch and evidence. It does not authorize a merge.
3. **Remote-main publication**: a Planner integrates accepted scope from the current `origin/main`, revalidates the candidate and publishes without force under the authorization rules in `PLANNER_RULES.md`.

The human may grant either one-candidate authorization or a documented standing authorization with a narrow scope and expiry/revocation condition. Planning/task refs never inherit remote-main authority.

Committed state documents should not cache a supposedly current `origin/main` hash: the commit containing that hash would immediately make it stale. They name the branch and included scope; Git and the publication report provide the exact commit.

## Why this shape

- Branches/worktrees isolate bytes; ownership and contracts isolate intent.
- A stable read-only surface lets consumers start before the provider's whole feature is complete.
- Separate lifecycle, human-validation and ownership fields prevent free-text status from becoming a hidden lock.
- One authoritative location per rule reduces drift and startup context.
- Planner-owned shared-state updates remove a major merge-conflict hotspot from parallel Executor branches.
