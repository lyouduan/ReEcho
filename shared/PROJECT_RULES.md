# ReEcho project rules

These are the project-level hard rules. `AGENTS.md` owns startup order; role actions live in `PLANNER_RULES.md` and `EXECUTOR_RULES.md`; `WORKFLOW.md` is explanatory.

## Scope and architecture

- Engine: Unreal Engine 5.8 installed/release build, Windows desktop first. Do not build or open this project with the separate source checkout.
- Project descriptor: `ReEcho.uproject`; runtime module: `Source/ReEcho`.
- `Design/Data/ReEchoData.xlsx` is the canonical designer-editable source for migrated production tables. Generated CSV under `Content/Data/` is the diffable packaged runtime source. Legacy JSON is migration-only for migrated domains.
- Do not hand-edit generated production CSV as a second truth or duplicate migrated balance constants in JSON, C++, DeveloperSettings, actors or widgets.
- Preserve deterministic semantics: simulation 60 Hz, recording 20 Hz, encounter length 30 seconds, and pause advances neither recording nor playback.
- Automatic attacks are never serialized into recordings. Echoes replay historical position and successful active-skill events; targeting and hit resolution use the current world.
- Do not hand-edit `.uasset` or `.umap` outside Unreal Editor. Prefer mergeable C++, data sources and generation tooling.

## Branches and authority

- Executor implementation branches use `plan/<id>-<short-name>`; client-required branches may use `codex/<short-name>`.
- Executors never modify, merge or push `main`. Planners may create reviewed local merge commits and may publish remote `main` only under `PLANNER_RULES.md`.
- Task implementation and planning batches start on a non-`main` branch. A Planner may resolve integration conflicts and create reviewed merge commits on local `main`. Planning broadcasts normally use `coord/<owner>-<topic>` until their reviewed integration/publication.
- `git fetch` is the only automatic first step when remote state may have changed. If it reveals commits outside the currently approved local baseline, the Planner must stop before `pull`, merge, rebase, cherry-pick or push; report physical/Git conflicts, logical conflicts and integration coupling, then wait for the human's explicit choice. A fast-forward or clean auto-merge is not an exemption.
- Stage explicit paths only. Never use `git add .` or `git add -A`; never force-push `main`.
- Human instructions and Plan locked acceptance define scope. Executors may refine implementation details but must coordinate changes to goals, acceptance, public contracts or declared Writes.

## Parallel ownership

- Impact modes are `Isolated`, `ReadOnly`, `SharedContract`, and `Exclusive`; ownership states are `Reserved`, `Active`, and `Released`.
- Only an `Active` `Exclusive` row blocks another writer. Reservations do not block disjoint or read-only work.
- `.umap`, `.uasset`, editor Project Settings, the canonical XLSX, deployment targets and other merge-hostile artifacts require an Exchange ownership row before publication-intended edits.
- C++, separate text files and read-only API/data consumers may proceed independently when declared Writes do not overlap.
- `SharedContract` includes stable IDs, schemas, save formats, public APIs and generator contracts. Affected Planners agree on the consumer surface before breaking changes proceed.
- The canonical XLSX has one repository `WorkbookWriter`. Generated CSV belongs to the same publication unit and is not separately claimed. Disposable local designer QA does not claim repository ownership.
- The same-clone Unreal lock lives under the Git common directory at `reecho-locks/unreal-editor.lock`. Remove a stale lock only after confirming no process is using this project.

## Documentation ownership

- Executors update their assigned Plan's Execution notes and documentation directly tied to their owned implementation.
- Executors do not routinely edit `PROJECT_STATE.md`, `CODEBASE_MAP.md`, `LESSONS.md` or workflow rules. The Planner updates those once during review/closure; an Executor edits them only when the Plan explicitly owns that document.
- Update Exchange during implementation only when ownership, Writes, dependencies, lifecycle or a shared contract changes.
- Update `CODEBASE_MAP.md` only when a retrieval route or responsibility moves. Add to `LESSONS.md` only for reusable evidence-backed lessons, not a task diary.
- Staged implementation and its local documentation must agree. Documentation-neutral commits do not require a ceremonial shared-file edit.

## Unreal C++ standard

- Follow Epic's Unreal Engine C++ Coding Standard under `Source/`.
- Use Allman braces, four-column tab indentation, one statement per line, spaces around binary operators and Unreal naming/type conventions.
- Keep headers self-contained with the matching generated header last; in `.cpp`, include the matching header first and keep include groups stable.
- Prefer early returns, named constants and small helpers over dense expressions or duplicated logic.
- Run repository `.clang-format` on changed `.h`/`.cpp`, inspect the diff, then run UHT/UBT and `git diff --check`.
- Formatting-only cleanup must not change gameplay semantics; exceptions require a nearby explanation and handoff note.

## Verification matrix

Use Windows `.cmd` entry points. Run checks required by the changed surface, not unrelated suites.

| Change surface | Required checks |
|---|---|
| Markdown/workflow only | `python scripts/validate_project.py`, `git diff --check` |
| Python data tooling/XLSX contract | focused Python tests, canonical `--check`, project validation, `git diff --check` |
| JSON/config/CSV only | project validation plus affected runtime automation when behavior changes |
| C++ | `.clang-format`, `Build-Editor.cmd`, affected automation, `git diff --check` |
| Texture/import script | import/load and asset-existence check |
| Packaging/cook | applicable checks plus clean package and manifest/smoke evidence |

- Before commands requiring a closed Editor, ask the human to save and close it. Automation must not silently discard an interactive session.
- Reuse evidence only while relevant source/configuration and the integration base remain unchanged. Conflicts or rebases invalidate affected evidence.
- Summarize logs; do not load full Unreal logs into AI context.
- If Unreal is unavailable, label evidence `static only`; never claim compilation, automation or PIE.

## Completion levels

- **Technical delivery**: required objective checks pass or the exact unavailable prerequisite is recorded; `git diff --check` passes; no intermediate/private files are included; Execution notes describe changes, evidence and risks.
- **Human validation**: required only for visual quality, feel, usability or other subjective behavior. `PendingBeforeClose` blocks closure; `PendingFollowUp` records a human-approved deferred confidence check; `NotRequired` and `Passed` are self-explanatory.
- **Plan closure**: required objective checks and every `PendingBeforeClose` item are accepted, the Planner reviews/integrates the task, releases ownership and updates shared state. A `PendingFollowUp` item may remain after closure only when the human explicitly accepted that deferral.
- **Remote publication**: separate from closure/local merge and governed by the scoped authorization gate in `PLANNER_RULES.md`.
