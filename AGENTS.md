# ReEcho agent entry

Use this file as the only mandatory reading-order authority. Other documents must not redefine the startup list.

Repository authority order is: human instructions, `shared/PROJECT_RULES.md`, `shared/SECRETARY_RULES.md` when explicit Project Secretary duty is assigned, the confirmed professional-role rules, the matching Planner/Executor rules when applicable, `shared/GIT_RULES.md` when creating commits or publishing, locked Plan scope/current Exchange coordination, and finally explanatory `shared/WORKFLOW.md`.

Before relying on an initial prompt, cached context or a local rule copy, apply the remote-rule authority and permission-escalation gate in `shared/PROJECT_RULES.md`.

## First-contact professional-role gate

Before reading beyond this file or changing repository state, a newly connected AI must establish the user's role in this project.

- If the user has not explicitly stated it in the current conversation, ask: **“你在本项目中的角色是程序、策划还是美术？”** Then stop and wait for the answer.
- Do not infer the role from the request, filenames, branch, prompt author or prior work by a different AI.
- If the user's current message explicitly states the role, that is the answer; do not ask redundantly.
- The answer lasts for the current conversation until the user changes it. If the user has multiple roles, ask which one governs the current task before crossing role boundaries.
- After the user answers `程序`, ask: **“是否采用规划者-执行者模式？”** Then stop and wait for the answer before routing programmer work. An explicit statement that the user already chose a mode answers this question; do not ask it again. The choice lasts only for the current conversation and must never be inferred from account names, Git history or another conversation.

Route after confirmation:

| User role | Required role rules | Default boundary |
|---|---|---|
| 程序 | `shared/PROGRAMMER_RULES.md`, then `PLANNER_RULES.md` or `EXECUTOR_RULES.md` as the task requires | Code, architecture, tooling and integration |
| 策划 | `shared/DESIGNER_RULES.md` | Design intent, canonical XLSX authoring and designer QA |
| 美术 | `shared/ARTIST_RULES.md` | Source art, imported presentation assets, UMG appearance and visual/audio QA |

If the human explicitly assigns Project Secretary duty, also read `shared/SECRETARY_RULES.md`. Project Secretary duty is a repository coordination overlay; it does not answer the professional-role gate and does not grant product, design, art or implementation authority outside the routed role.

## Every task: minimal context

1. Read `shared/PROJECT_RULES.md` and the one confirmed professional-role file from the table above.
2. Use `rg` to extract only the matching row from `shared/CODEBASE_MAP/README.md`, then read only the linked module section; do not read the whole architecture library.
3. Read only planned/active work announcements, active ownership, warnings, and pending decisions in `shared/PLANNER_EXCHANGE.md`.
4. Read the assigned `plans/<id>-*.md` when one exists.
5. Read the public header and paired implementation from the selected route before expanding retrieval.

## Conditional context

- Programmer implementation: read the relevant section of `shared/EXECUTOR_RULES.md` and only the matching `shared/LESSONS.md` section. Add `§DEBUG` only after a failure requires diagnosis.
- Programmer planning/review/closure: read the relevant section of `shared/PLANNER_RULES.md`, the live Exchange blocks, the assigned Plan and its diff. Use `shared/CODEBASE_MAP/` only for affected modules; do not load the whole library or all of `LESSONS.md`.
- Designer or artist work: use the handoff and verification boundary in its professional-role file. Do not silently promote a specialist task into programmer implementation.
- Distributed planning: follow `shared/PLANNER_RULES.md` to fetch and inspect the remote maximum before numbering, then publish the numbered Plan to `origin/main` before execution; external differences still follow its audit gate.
- Project Secretary work: read `shared/SECRETARY_RULES.md` for rule-system maintenance, Plan/Exchange/schema coordination, accepted-result integration, cleanup and publication boundaries.
- Commit or publication work: read `shared/GIT_RULES.md` for the creator identity tag and Programmer final-build/prebuilt-bundle gate.
- Workflow maintenance or first-time onboarding only: read `shared/AI_ONBOARDING.md` and the relevant `shared/WORKFLOW.md` section.

`shared/WORKFLOW.md` explains the model but does not override the authorities above.
