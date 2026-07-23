# ReEcho AI workflow

The project uses a planner/executor workflow with file-based memory.

## Start a task

1. Read `AGENTS.md` and the listed `shared/` files.
2. Scan `shared/PLANNER_EXCHANGE.md`; claim any `.umap`, `.uasset`, DataTable, Project Settings, editor instance, or deployment target.
3. Copy `plans/TEMPLATE.md` to a numbered plan. Lock the goal, acceptance, and Step 0 baseline before implementation.
4. Create an isolated branch/worktree. Executors never edit the planner-owned default branch.

## Finish a task

1. Run static validation, build, automation, and human checks in proportion to the change.
2. Record exact evidence and unavailable prerequisites in the plan.
3. Release ownership in the exchange board.
4. Stage explicit files, commit, and hand the branch to the human/planner. Do not merge it yourself.

## Evidence vocabulary

- `static verified`: JSON/config/source checks only.
- `build verified`: UHT and UBT completed successfully.
- `automation verified`: named UE automation tests passed.
- `PIE verified`: a human completed the stated interaction check.
- `shipping verified`: a packaged build was installed/run on the target platform.

Never promote one evidence level into another.
