# Plan 27 - META - workflow parallelism and rule consolidation

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Gavyn-side Planner acting under the user's explicit workflow-maintenance request; no external Executor.
- Task status: `Closed`.
- Human validation: `NotRequired`.
- Planning ref / implementation base: `origin/main` integrated through Plan25; the user directly authorized this rules-only implementation and remote-main publication.
- Implementation branch: `plan/27-workflow-parallelism`.
- Depends on: accepted Plans 21–25 and the existing distributed Planner trial. Blocks: no implementation task after publication; new distributed tasks must consume the updated rules.
- Writes: workflow/rule documents, Plan template, live Exchange/state, stale closed-Plan coordination headers, README workflow statement and workflow static validation.
- Stable Reads: all gameplay/data code and assets remain untouched.
- Impact mode: `Exclusive` for the workflow documents during this edit; otherwise documentation-only.
- Compatibility promise: existing implementation branches remain valid within their published scope, but must integrate current `origin/main` before Review. Old coordination refs remain historical handoff evidence only.
- Explicit exclusions: no gameplay, data values, XLSX bytes, generated CSV bytes, UE assets, Plan26 UI implementation or remote history rewriting.

## Locked goal

Make the repository rules unambiguous, nonredundant and parallel-friendly: one authority per concern, one lifecycle vocabulary, precise ownership modes, fewer shared-document write conflicts, contract-first consumer work and current remote baselines.

## Locked acceptance

- [x] Startup, project, Planner, Executor, workflow explanation and live-state responsibilities are nonoverlapping and explicitly ordered.
- [x] Lifecycle, human validation, impact mode and ownership state use separate fixed vocabularies.
- [x] The Plan template requires complete distributed Coordination.
- [x] Executors no longer update shared state/lessons/routes on every commit; Planner closure owns those writes.
- [x] Canonical XLSX single-writer semantics do not block disposable QA or read-only consumers, and generated CSV is not separately locked.
- [x] Provider/consumer integration and stale coordination-ref retirement are defined.
- [x] Project-specific Windows UE commands and a same-clone Git-common-dir lock replace generic placeholders.
- [x] Live state contains no stale current-main hash or obsolete Plan24/25 active status.
- [x] Static validation detects future placeholder/status/startup-rule drift.
- [x] No gameplay/data/asset bytes change.

## Step 0 gate

- Baseline branch/commit: fetched `origin/main` containing Plan25.
- Engine/build availability: not required for this workflow-only surface.
- Existing focused-test result: Plan25 integrated static/data/build/34-test evidence is unchanged.
- Active exclusive ownership: workflow documents only for this Plan; Plan26 UI reservation is disjoint.
- Stop condition: if `origin/main` advances before publication, integrate it and rerun static verification.

## Implementation outline

1. Replace copied blueprint/provenance language with repository-local authority.
2. Consolidate mandatory rules by concern and keep WORKFLOW explanatory.
3. Normalize live coordination and closed Plan headers.
4. Add validator guards for the new invariants.
5. Validate, integrate current remote and publish without force under the user's explicit authorization.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Workflow invariants and existing project checks pass |
| Whitespace | `git diff --check` | No malformed patch/whitespace |
| Scope | `git diff --stat` and path audit | Markdown plus workflow validator only |
| Human | Not required | No subjective product surface changed |

## Execution notes

### Changed

- Consolidated rule authority, state vocabularies, documentation ownership, planning/publication lanes, consumer integration and workbook ownership.
- Replaced generic placeholders and per-worktree lock guidance with ReEcho Windows commands and a Git-common-dir lock.
- Updated live project state, Exchange, Plan template and closed Plan24/25 headers to stop advertising obsolete refs/status.

### Evidence

- `python scripts/data/sync_xlsx_to_csv.py --check` passed and confirmed all production CSV bytes still match the canonical workbook.
- `python scripts/data/test_sync_xlsx_to_csv.py` passed all 10 transaction/generation tests, including final-project-validator rollback coverage.
- `python scripts/validate_project.py` passed with the new workflow authority, status, live-Exchange, placeholder, lock-location and stale-baseline guards.
- `python -m py_compile scripts/validate_project.py` and `git diff --check` passed.
- Path audit shows only Markdown plus `scripts/validate_project.py`; no gameplay, XLSX, CSV or UE asset bytes changed.

### Remaining risks

- Plan26 may already have started from the historical coordination ref; its branch remains usable but must integrate current `origin/main` before Review.

### Human validation result/request

- `NotRequired`: documentation/workflow-only change.
