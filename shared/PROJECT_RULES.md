# ReEcho project rules

These are the project-level hard rules. `AGENTS.md` owns startup and professional-role routing; professional boundaries live in `PROGRAMMER_RULES.md`, `DESIGNER_RULES.md` and `ARTIST_RULES.md`; Project Secretary boundaries live in `SECRETARY_RULES.md`; programmer duties live in `PLANNER_RULES.md` and `EXECUTOR_RULES.md`; commit identity and publication completeness live in `GIT_RULES.md`; `WORKFLOW.md` is explanatory.

## Professional-role boundary

- Every newly connected AI must complete the first-contact role gate in `AGENTS.md` before project work.
- The confirmed T爷 exception within the programmer route is governed only by `shared/PROGRAMMER_RULES.md`; its explicit retained safety boundaries continue to apply when ordinary project process gates are skipped.
- The confirmed user role and the AI's repository duty are separate. “策划” does not mean project Planner; “程序” does not automatically authorize main publication.
- Project Secretary duty is separate from the professional role gate and is governed by `shared/SECRETARY_RULES.md`.
- Do not silently cross from designer/art scope into code, schema, gameplay authority or publication. Split the work and hand the boundary to the programmer route, or ask the user to explicitly switch the current task's role.
- A designer or artist AI uses a local specialist branch, records exact changed assets/data and hands the result to a programmer Planner for integration. It never pushes a remote ref or publishes `main`.

## Scope and architecture

- Engine: Unreal Engine 5.8 installed/release build, Windows desktop first. Do not build or open this project with the separate source checkout.
- Project descriptor: `ReEcho.uproject`; runtime module: `Source/ReEcho`.
- `Design/Data/ReEchoData.xlsx` is the canonical designer-editable source for migrated production tables. Generated CSV under `Content/Data/` is the diffable packaged runtime source. Legacy JSON is migration-only for migrated domains.
- Do not hand-edit generated production CSV as a second truth or duplicate migrated balance constants in JSON, C++, DeveloperSettings, actors or widgets.
- Preserve deterministic semantics: simulation 60 Hz, recording 20 Hz, encounter length 30 seconds, and pause advances neither recording nor playback.
- Automatic attacks are never serialized into recordings. Echoes replay historical position and successful active-skill events; targeting and hit resolution use the current world.
- Do not hand-edit `.uasset` or `.umap` outside Unreal Editor. Prefer mergeable C++, data sources and generation tooling.

## Branches and authority

- Executor implementation branches use local `plan/<id>-<short-name>` worktrees; client-required local branches may use `codex/<short-name>`.
- `origin/main` is the only permitted remote branch. Do not push planning, task, handoff, review or PR branches.
- Executors never modify, merge or push `main` and never push any remote ref. Planners may create reviewed local merge commits and may publish only remote `main` under `PLANNER_RULES.md`.
- Before implementation starts, every formally numbered Plan is published to `origin/main` under `PLANNER_RULES.md`; implementation still starts on a local non-`main` branch and no remote task branch is created.
- `git fetch` is the only automatic first step when remote state may have changed. If it reveals commits outside the currently approved local baseline, the Planner must stop before `pull`, merge, rebase, cherry-pick or push; report physical/Git conflicts, logical conflicts and integration coupling, then wait for the human's explicit choice. A fast-forward or clean auto-merge is not an exemption.
- Stage explicit paths only. Never use `git add .` or `git add -A`; never force-push `main`.
- Before creating a commit or publishing, follow the role tag and Programmer final-build/prebuilt-bundle gate in `shared/GIT_RULES.md`.
- Human instructions and Plan locked acceptance define scope. Executors may refine implementation details but must coordinate changes to goals, acceptance, public contracts or declared Writes. Remote main owns published Plan numbers; integration shifts colliding unpublished local Plans as one ordered block.

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
- Executors do not routinely edit `CODEBASE_MAP.md`, `LESSONS.md` or workflow rules. The Planner updates those once during review/closure; an Executor edits them only when the Plan explicitly owns that document.
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
| C++ | `.clang-format`, `Build-Editor.cmd` (refreshes the tracked prebuilt bundle), affected automation, `git diff --check` |
| Texture/import script | import/load and asset-existence check |
| Packaging/cook | applicable checks plus clean package and manifest/smoke evidence |

- Before commands requiring a closed Editor, ask the human to save and close it. Automation must not silently discard an interactive session.
- Reuse evidence only while relevant source/configuration and the integration base remain unchanged. Conflicts or rebases invalidate affected evidence.
- Summarize logs; do not load full Unreal logs into AI context.
- If Unreal is unavailable, label evidence `static only`; never claim compilation, automation or PIE.

## Completion levels

- **Technical delivery**: required objective checks pass or the exact unavailable prerequisite is recorded; `git diff --check` passes; no intermediate/private files or generated products outside the `GIT_RULES.md` prebuilt allowlist are included; Execution notes describe changes, evidence and risks.
- **Human validation**: required only for visual quality, feel, usability or other subjective behavior. `PendingBeforeClose` blocks closure; `PendingFollowUp` records a human-approved deferred confidence check; `NotRequired` and `Passed` are self-explanatory.
- **Plan closure**: required objective checks and every `PendingBeforeClose` item are accepted, the Planner reviews/integrates the task, releases ownership and updates shared state. A `PendingFollowUp` item may remain after closure only when the human explicitly accepted that deferral.
- **Remote publication**: separate from closure/local merge and governed by the scoped authorization gate in `PLANNER_RULES.md`.
