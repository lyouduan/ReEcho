# ReEcho Planner rules

This file contains Planner actions only. Startup order is defined by `AGENTS.md`; project hard rules and verification live in `PROJECT_RULES.md`.

## Draft a Plan locally

1. Fetch `origin/main` and inspect the Plan numbers and live Exchange rows already published there. Remote main is canonical for numbering; numbers are never reused.
2. If remote main advanced, inspect it read-only. Do not pull, merge, rebase or push until the external-commit integration audit is reported and the human chooses the outcome.
3. Search the code/data surface before writing. Cite concrete paths and lines instead of relying on memory.
4. Separate locked goal/acceptance from implementation guidance. The Executor may refine implementation but may not silently change locked scope.
5. Mark human validation `NotRequired` unless subjective feel, readability, visual quality or usability needs a person. Use `PendingBeforeClose` when it gates acceptance or `PendingFollowUp` only when the human explicitly allows deferral.
6. Add the Plan `Coordination` block from `plans/TEMPLATE.md`: owners, **Plan-authored-by AI side**, **implementation-authored-by AI side**, lifecycle, human validation, local/implementation base, dependencies, Writes/Reads, impact mode, compatibility promise and exclusions. Planning ownership, document authorship and implementation authorship are separate facts; never infer or transfer them merely because another Planner refreshed the local file.
7. Keep planning drafts and Exchange reservations local. They do not require a remote planning publication before an Executor starts.
8. Recommend an Executor model only when it helps the human choose; do not spawn one unless the human explicitly asks.

## Start and coordinate local execution

1. Give the Executor the local Plan path or embed the locked goal, acceptance, base, Writes and exclusions in the prompt.
2. Executors work in local `plan/<id>-<short>` branches/worktrees. Different machines use their own clones and local task branches; no remote task or coordination branch is created.
3. `Isolated` and `ReadOnly` work may start when local Writes are disjoint. `SharedContract` or `Exclusive` overlap waits for the affected local owner/human to confirm the contract or ownership.
4. If Writes, dependencies or a stable contract must expand, pause only the out-of-scope change and update the local Plan/Exchange before continuing. Unrelated local work need not stop.
5. Local planning does not inform another machine. Cross-machine differences are discovered from `origin/main` and resolved at pull/push integration under the audit gate below.

## External-commit integration audit

Whenever `fetch` reveals that `origin/main` contains commits outside the currently approved local baseline, do not change the working tree or remote history yet. This gate applies before `pull`, merge, rebase, cherry-pick and push, even when Git can fast-forward.

1. Identify incoming commits, authors/owners, Plans, base and changed paths. Treat every unreviewed incoming main commit as external to the candidate; do not rely only on Git author identity.
2. Evaluate **Physical/Git conflict**:
   - predict the three-way result from the merge base with read-only `log`, `diff`, `merge-tree` and ancestry checks;
   - report same-line edits, add/add, rename, modify/delete, binary/generated-file collisions and exact files;
   - say explicitly when there is no textual conflict. “Git can merge” is evidence only for this layer.
3. Evaluate **Logical conflict** even if Git reports none:
   - compare locked goals, current human decisions and runtime semantics;
   - detect deletion versus continued extension of old logic, competing flows, different public API/schema/stable-ID/save/generator contracts, source-of-truth changes and contradictory tests;
   - identify whether one side would silently restore, bypass or overwrite the other side's behavior.
4. Evaluate **Coupling**:
   - list affected Plans, declared Writes/Reads, shared contracts, downstream local branches and required integration order;
   - state which prior build/test/human evidence becomes invalid and what must be rerun.
5. Evaluate **Plan-number conflicts** against remote main. Remote numbers win. Renumber a colliding unpublished local Plan and every later unpublished local Plan as one ordered block to the first conflict-free range; update filenames, headings, Exchange rows, dependencies, prompts, test names and branch/worktree names where practical. Do not rewrite existing Git history solely to change old commit messages.
6. Report the incoming range, Physical/Git conflict, Logical conflict, Coupling, number shifts, choices (`remote`, `local`, `combined adaptation`, `defer/split`), tradeoffs and a recommendation. Do not choose behavioral ownership on the human's behalf.
7. Wait for the human to select the behavior/ownership outcome. Commit order alone never supersedes an explicit human decision.
8. Integrate only the selected outcome, inspect the result for unintended deletion/restoration and run proportional verification.
9. Fetch again immediately before push. If main advanced after the audit/decision, restart this gate and report the new coupling.

A conflict-free fast-forward can still contain a logical conflict. Conversely, overlapping files do not automatically require choosing one whole side: present a combined adaptation when contracts and behavior can be reconciled safely.

## Executor prompt template

The Plan is the task specification; the startup prompt is only a neutral routing envelope. It must not assign a new identity,
override the recipient AI's existing local role or restate the other user's authority. A normal prompt contains
the exact Plan path, approved base, branch/worktree, one-line start instruction, exceptional gates that cannot be
discovered from the repository, and the required completion report. Do not duplicate the Plan's locked goal,
acceptance, Writes, exclusions, implementation outline or verification matrix. Embed scope only when the recipient
cannot access the Plan; update the Plan first when a follow-up changes scope. Follow-up prompts state only the delta and
point back to the Plan.

```text
这是 ReEcho `plans/<id>-<name>.md` 的启动引导，不改变你现有的身份或职责。
先按 AGENTS.md 完成或复用当前对话中用户已明确确认的专业角色路由，再读取该 Plan 的最小上下文。
从 `<approved-base>` 创建本地 `<branch>` / `<worktree>`；特殊门禁：<only non-discoverable exceptions or none>。
完全按 Plan 实现、验证并更新其 Execution notes。只在本地任务分支工作；不 merge/push main，不推送远端分支。
若必须改变 Plan 锁定范围或公共契约，停止越界部分并报告。完成后报告 commit、验证、风险和剩余人验。
```

## Review and local integration

### Mandatory forward progress

At every review, integration and closure handoff, advance accepted work to the furthest state currently permitted by the existing project gates. Do not leave work waiting merely because the human did not repeat the next routine instruction.

- **Merge when mergeable**: once the review, objective checks, required human validation or explicit deferral, ownership compatibility and integration-base requirements below are satisfied, merge the accepted scope into local `main` before starting avoidable dependent integration work; release its ownership during the resulting shared-state update.
- **Publish when publishable**: once the Remote-main publication gate below has current authorization, a fresh external-main audit, a clean approved candidate and current verification, push `main` to `origin/main` promptly. Never interpret this default as authority to bypass or invent publication authorization.
- **Delete when removable**: once an obsolete worktree's exact path is verified, its status is clean, every required change is already integrated or deliberately retained elsewhere, no active ownership/process still needs it, and its branch satisfies the non-force deletion rules below, remove the worktree and then the merged local branch promptly.
- “Can” means every applicable existing scope, validation, human-decision, external-change, publication and destructive-action gate passes. If any gate does not pass, do not force progress; report the unmet gate and preserve the state needed to resolve it.

1. Confirm the Plan is in `Review`; inspect the merge base, branch status and Execution notes.
2. Start with `git diff --stat <merge-base>..<branch>`, then inspect relevant hunks and generated artifacts. Do not use `main..branch` when main has advanced.
3. Check locked acceptance, objective evidence, public-contract compatibility and code health. Rework returns lifecycle to `InProgress`.
4. The Planner directly performs small, evident, in-scope review corrections on the task branch, including Plan wording,
   stale coordination cleanup, dead-code removal, formatting and narrow deterministic fixes. Do not bounce these back to
   an Executor merely to preserve role separation. Reassign an Executor when the fix is behaviorally substantial,
   uncertain, broad, independently parallelizable or needs a new implementation/verification pass. Large unrelated
   refactors get a separate local Plan.
5. Resolve `PendingBeforeClose` with the human before closure. `PendingFollowUp` may remain only under an explicit human deferral.
6. After acceptance, merge with `--no-ff` into local `main`, update shared state once, release ownership and set lifecycle `Closed`.
7. Remove a clean merged worktree only after checking its exact path and `git status --short`. Never use `--force`; delete a merged local branch with `git branch -d`.

## Remote-main publication

`origin/main` is the only permitted remote branch. Planning drafts and implementation branches stay local.

Standing scoped authorization: after review and validation, publish completed changes limited to authoritative workflow/rule documents, `plans/TEMPLATE.md`, and the matching validator/tests promptly to `origin/main`. A minimal Exchange schema migration required by that rule change may accompany it, but live local Plan drafts, reservations and implementation are excluded. A fresh external-main audit is still mandatory, and any later human instruction may revoke or narrow this authorization.

1. Fetch immediately before building the candidate. If main advanced, run the full external-commit audit and wait for the human's integration choice.
2. Integrate current `origin/main`, all named accepted scope and no unapproved WIP. Renumber unpublished local Plans when remote main already owns a number.
3. Re-run checks affected by integration, conflicts, renumbering or base changes.
4. Report included/excluded Plans, candidate commit, verification and remote difference.
5. Authorization is either:
   - **one-candidate**: explicit human approval for that reported candidate; or
   - **standing scoped**: an Exchange rule explicitly granted by the human with allowed content, expiry/revocation condition and required checks.
6. Push only `main`, without force. Never push local planning/task branches. If remote advances or rejects the push, stop, rebuild/revalidate the candidate and renew authorization when scope or commit changes.
7. Fetch and compare local/remote main after publication. Notify peer Planners to fetch main and run their own audit before integrating it.

Do not commit a self-referential current-main hash into live state. Git and the publication report provide the exact commit.

## Shared-memory closure

At review/closure, and only where relevant:

- `PROJECT_STATE.md`: current product/progress/toolchain snapshot, never chronology.
- `CODEBASE_MAP.md`: changed retrieval routes/responsibilities only.
- `LESSONS.md`: reusable evidence-backed lessons with source Plan.
- `PLANNER_EXCHANGE.md`: current local work/ownership/warnings only; remove closed rows rather than keeping history.
- assigned Plan: final lifecycle, changed behavior, evidence, risk and human-validation result/request.

Workflow/rule changes are reviewed repository changes. Update the single authoritative section instead of copying the same mandate into every role document.
