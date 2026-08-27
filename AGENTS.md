# ReEcho agent entry

Use this file as the only mandatory reading-order authority. Other documents must not redefine the startup list.

Repository authority order is: human instructions, `shared/PROJECT_RULES.md`, `shared/SECRETARY_RULES.md` when explicit Project Secretary duty is assigned, the confirmed professional-role rules, the matching Planner/Executor rules when applicable, `shared/GIT_RULES.md` when creating commits or publishing, the applicable published Plan, and finally explanatory `shared/WORKFLOW.md`.

Before relying on an initial prompt, cached context or a local rule copy, apply the remote-rule authority and permission-escalation gate in `shared/PROJECT_RULES.md`.

Every repository rule's risky-operation prohibition or exception is governed by the single human-confirmation contract in `shared/PROJECT_RULES.md`: warn first, name when asking a programmer is recommended, and execute after an authorized human explicitly confirms the exact operation.

## First-contact professional-role gate

Before reading beyond this file or changing repository state, a newly connected AI must establish the user's role and every work-mode choice applicable to that role in one prompt.

- If the user has not explicitly supplied every applicable answer in the current conversation, ask once: **“请一次性确认：你在本项目中的角色是程序、策划还是美术？若为策划，请同时提供用于任务分支和经验文件的稳定身份标识；若为程序，请同时回答：是否采用规划者-执行者模式？是否采用一任务一 worktree（每个任务一个独立文件夹）？”** Then stop and wait for the answer.
- `美术` only needs the professional-role answer. `策划`还必须提供由英文字母、数字、点、下划线或短横线组成的稳定身份标识；`程序` must provide all three programmer answers before programmer work begins. Planner-Executor and workspace mode are independent choices and neither implies the other.
- After that answer is known, keep the workspace choice independent of Planner-Executor mode; both programmer choices must already have been requested together in the same prompt rather than through sequential questions.
- If the user already supplied some applicable answers, accept them and ask only for all missing applicable answers together in one follow-up; do not repeat answered questions.
- Do not infer the role, designer identity or either programmer work-mode choice from the request, filenames, branch, current folder, prompt author, account name, Git history, another conversation or prior work by a different AI.
- All answers last for the current conversation until the user changes them. If the user has multiple roles, ask which one governs the current task before crossing role boundaries.

Route after confirmation:

| User role | Required role rules | Default boundary |
|---|---|---|
| 程序 | `shared/PROGRAMMER_RULES.md`, then `PLANNER_RULES.md` or `EXECUTOR_RULES.md` as the task requires | Code, architecture, tooling and integration |
| 策划 | `shared/DESIGNER_RULES.md` | Design intent, canonical XLSX authoring and designer QA |
| 美术 | `shared/ARTIST_RULES.md` | Source art, imported presentation assets, UMG appearance and visual/audio QA |

If the human explicitly assigns Project Secretary duty, also read `shared/SECRETARY_RULES.md`. Project Secretary duty is a repository coordination overlay; it does not answer the professional-role gate and does not grant product, design, art or implementation authority outside the routed role.

## Git LFS checkout gate

Before opening `ReEcho.uproject`, running Unreal/build/package commands, or reading/modifying a Git LFS path, run `python scripts/setup_lfs.py --check`. If Git LFS is missing or any tracked file is absent/still a pointer, stop the affected work, tell the user to install Git LFS, and run `python scripts/setup_lfs.py` to initialize this clone and download current objects. Repeat `--check` after any fetch plus checkout/merge/rebase/pull that changes `.gitattributes` or LFS-tracked paths. Never treat a small text pointer as the real asset.

## Every task: minimal context

1. Read `shared/PROJECT_RULES.md` and the one confirmed professional-role file from the table above.
2. Use `rg` to extract only the matching row from `shared/CODEBASE_MAP/README.md`, then read only the linked module section; do not read the whole architecture library.
3. Read the assigned `plans/<id>-*.md` when one exists.
4. Read the public header and paired implementation from the selected route before expanding retrieval.

## Conditional context

- Programmer implementation: read the relevant section of `shared/EXECUTOR_RULES.md` and only the matching `shared/LESSONS.md` section. Add `§DEBUG` only after a failure requires diagnosis.
- Programmer planning/review/closure: read the relevant section of `shared/PLANNER_RULES.md`, the assigned Plan and its diff. Use `shared/CODEBASE_MAP/` only for affected modules; do not load the whole library or all of `LESSONS.md`.
- Designer work: after `shared/DESIGNER_RULES.md`, read `shared/DESIGNER_EXPERIENCE/<策划身份>.md` when it exists; Bug、需求和待合并修改分别走 `issue/...`、`request/...`、`merge/...`，报告使用 `issues/TEMPLATE.md`。Artist work uses its professional handoff and verification boundary. Do not silently promote a specialist task into programmer implementation.
- Distributed planning: follow `shared/PLANNER_RULES.md` to fetch and inspect the remote maximum before numbering, then publish the numbered Plan to `origin/main` before execution; external differences still follow its audit gate.
- Project Secretary work: read `shared/SECRETARY_RULES.md` for rule-system maintenance, Plan/schema coordination, accepted-result integration, cleanup and publication boundaries.
- Commit or publication work: read `shared/GIT_RULES.md` for the creator identity tag and Programmer final-build/prebuilt-bundle gate.
- Workflow maintenance or first-time onboarding only: read `shared/AI_ONBOARDING.md` and the relevant `shared/WORKFLOW.md` section.

`shared/WORKFLOW.md` explains the model but does not override the authorities above.
