# ReEcho Planner rules

This file contains Planner actions only. Startup order is defined by `AGENTS.md`; project hard rules and verification live in `PROJECT_RULES.md`.

## Draft a Plan

1. Fetch the advertised remote planning baseline and inspect current Exchange announcements before reserving a number. Remote Plan numbers are canonical and never reused.
2. Search the code/data surface before writing. Cite concrete paths and lines instead of relying on memory.
3. Separate the locked goal/acceptance from implementation guidance. The Executor may refine implementation but may not silently change locked scope.
4. Mark human validation `NotRequired` unless subjective feel, readability, visual quality or usability needs a person. Use `PendingBeforeClose` when it gates acceptance or `PendingFollowUp` only when the human explicitly allows deferral. Do not add ceremonial PIE to documentation or pipeline-only work.
5. Add the Plan `Coordination` block from `plans/TEMPLATE.md`: owners, lifecycle, human validation, planning ref/base, dependencies, Writes/Reads, impact mode, compatibility promise and exclusions.
6. Recommend an Executor model only when it helps the human choose; do not spawn one unless the human explicitly asks.

## Publish coordination before implementation

1. Put the new Plan, its Exchange announcement and any active ownership row in one pure-planning commit on `coord/<owner>-<topic>`.
2. Push that ref before an external Executor starts. A local-only Plan is not coordinated in a distributed project.
3. Give the human two short prompts when another teammate is affected:
   - Executor prompt: planning ref/commit, branch/base, goal, acceptance, Writes and red lines.
   - Peer-Planner prompt: fetch the ref, report overlap/dependencies, register its work and publish a response.
4. `Isolated` and `ReadOnly` work may start after the broadcast. `SharedContract` or `Exclusive` overlap waits for the affected Planner/owner to confirm the contract or ownership.
5. If Writes, dependencies or a stable contract must expand, pause only the out-of-scope change, update Plan/Exchange, push a new planning commit, then continue. Unrelated work need not stop.

Planning publication makes scope visible. It does not authorize implementation to enter `main`.

## Executor prompt template

```text
你是 ReEcho 执行者。按 AGENTS.md 读取最小上下文；不要固定加载完整 WORKFLOW 或 §DEBUG。
从 <base-ref/commit> 创建 <branch/worktree>。Plan：plans/<id>-<name>.md。
目标：<一句话>。Writes：<精确范围>。Impact：<mode>。验收按 Plan 执行；Human validation=<state>。
只在任务分支实现、验证并更新本 Plan Execution notes；不要例行修改共享状态文档，不要 merge/push main。
若必须扩大 Writes、改变稳定 ID/schema/save/public API，先停止越界部分并通知 Planner。
完成后 push 任务分支并报告 commit、验证和剩余人验。
```

## Review and local integration

1. Confirm the Plan is in `Review`, inspect the merge base, branch status, Execution notes and current remote announcements.
2. Start with `git diff --stat <merge-base>..<branch>`, then inspect relevant hunks and generated artifacts. Do not use `main..branch` as the implementation diff when `main` has advanced.
3. Check locked acceptance, objective evidence, public-contract compatibility and code health. Rework returns lifecycle to `InProgress`; do not invent new status strings.
4. Small cleanup stays on the task branch. Large unrelated refactors get a separate Plan.
5. Resolve `PendingBeforeClose` with the human before closure. `PendingFollowUp` may remain only under an explicit human deferral; `NotRequired` needs no artificial PIE.
6. After acceptance, merge with `--no-ff` into local `main`, update shared state once, release ownership and set lifecycle `Closed`.
7. Remove a clean merged worktree only after checking its exact path and `git status --short`. Never use `--force`; delete a merged local branch with `git branch -d`. Remote branch deletion requires a human request.

## Remote-main publication

Local acceptance/merge and remote publication are distinct.

1. Fetch immediately before building the candidate. Integrate the current `origin/main`, all named accepted scope and no unapproved WIP.
2. Re-run checks affected by integration, conflicts or base changes.
3. Report included/excluded Plans, candidate commit, verification and remote difference.
4. Authorization is either:
   - **one-candidate**: explicit human approval for that reported candidate; or
   - **standing scoped**: an Exchange rule explicitly granted by the human with allowed content, expiry/revocation condition and required checks.
5. Publish without force. If remote advances or rejects the push, stop, rebuild/revalidate the candidate and renew authorization when its scope or commit changes beyond the standing grant.
6. Fetch and compare local/remote commits after publication. Notify peer Planners to fetch/rebase.

Do not commit a self-referential “current origin/main hash” into live state. Name `origin/main` plus included scope; Git and the publication report provide the exact commit.

## Shared-memory closure

At review/closure, and only where relevant:

- `PROJECT_STATE.md`: current product/progress/toolchain snapshot, never chronology.
- `CODEBASE_MAP.md`: changed retrieval routes/responsibilities only.
- `LESSONS.md`: reusable evidence-backed lessons with source Plan.
- `PLANNER_EXCHANGE.md`: current work/ownership/warnings only; remove closed rows rather than keeping a history table.
- assigned Plan: final lifecycle, changed behavior, evidence, risk and human-validation result/request.

Workflow/rule changes are reviewed repository changes. Update the single authoritative section instead of copying the same mandate into every role document.
