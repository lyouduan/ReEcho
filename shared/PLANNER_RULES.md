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

## External-commit integration audit

Whenever `fetch` reveals remote commits that are not already part of the currently approved local baseline, do not change the working tree or remote history yet. This gate applies before `pull`, merge, rebase, cherry-pick and push, even when Git can fast-forward.

1. Identify the incoming commits, authors/owners, Plans, base and changed paths. Treat any unreviewed incoming commit as external to the candidate; do not rely only on Git author identity.
2. Evaluate **Physical/Git conflict**:
   - predict the three-way result from the merge base with read-only `log`, `diff`, `merge-tree` and ancestry checks;
   - report same-line edits, add/add, rename, modify/delete, binary/generated-file collisions and exact files;
   - say explicitly when there is no textual conflict. “Git can merge” is evidence only for this layer.
3. Evaluate **Logical conflict** even if Git reports none:
   - compare locked goals, current human decisions and runtime semantics;
   - detect deletion versus continued extension of old logic, competing state machines/flows, different public API/schema/stable-ID/save/generator contracts, source-of-truth changes and contradictory tests;
   - identify whether one side would silently restore, bypass or overwrite the other side's behavior.
4. Evaluate **Coupling**:
   - list affected Plans, declared Writes/Reads, shared contracts, downstream branches and required migration/rebase order;
   - state which prior build/test/human evidence becomes invalid and what must be rerun.
5. Report the audit before integration: incoming commit range, three conflict results, available choices (`remote`, `local`, `combined adaptation`, `defer/split`), tradeoffs and a recommendation. Do not choose on the human's behalf.
6. Wait for the human to select the behavior/ownership outcome. Record a decision that supersedes an older rule only when there is explicit newer human authority; do not infer supersession from commit order.
7. Integrate only the selected outcome, inspect the resulting diff for unintended deletion/restoration and run proportional verification.
8. Fetch again immediately before push. If the remote advanced after the audit/decision, restart this gate and report the new coupling; do not extend the old authorization silently.

A conflict-free fast-forward can still contain a logical conflict. Conversely, overlapping files do not automatically require choosing one whole side: present a combined adaptation when contracts and behavior can be reconciled safely.

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
