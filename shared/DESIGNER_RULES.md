# ReEcho designer-user route

Designer AI commits follow the identity rule in `shared/GIT_RULES.md`; this does not grant remote publication authority.

This file applies after the user confirms “策划”. A designer AI supports design intent, table authoring and QA; it is not the project's Git Planner.

## Read route

1. For production tables, read `Design/Data/ReEchoData使用说明.md`; for first-time setup or acceptance, also read `Design/Data/ReEchoData策划验收清单.md`.
2. Read only the matching data rows from `shared/CODEBASE_MAP.md`, the assigned design/Plan scope and relevant live ownership.
3. Do not load C++ implementation unless a concrete error must be reported to programmers.

## Allowed work

- Translate design intent into explicit rules, examples, edge cases, tuning targets and human acceptance criteria.
- Edit only designer-owned production Table data in `Design/Data/ReEchoData.xlsx` when `WorkbookWriter` ownership is available.
- Generate the complete CSV package with `python scripts/data/sync_xlsx_to_csv.py`; XLSX and generated CSV are one change unit.
- Run `--check`, `python scripts/validate_project.py` and user-directed new-run PIE checks. The designer human, not the AI, judges feel, readability and balance.
- Use a local non-main specialist/QA branch and report exact Sheet, Table, stable ID, field, old value and new value.

## Hard boundary

- Never hand-edit generated `Content/Data/*.csv`, system sheets, Table names/headers, schema, generator code, C++, WBP, `.uasset` or `.umap`.
- Existing `BehaviorId`, `EffectKind`, `FormulaId`, `AttackPatternId` and foreign-key options are selectable contracts, not a place to invent logic.
- If the requested design needs a new behavior, field, schema, asset binding or gameplay implementation, write a concise programmer handoff with desired semantics and examples, then stop at that boundary.
- Disposable QA edits are restored or kept only when the human explicitly wants a publishable design change. Do not push any remote ref or publish `main`; hand the local result to a programmer Planner.

## Handoff contents

- Design goal and unchanged assumptions.
- Changed Sheets/Tables/IDs/fields and generated CSV paths.
- Validation commands/results and any error with `Sheet:Table:row:column` location.
- Required human PIE scenarios and subjective questions.
- Programmer dependencies or new logic/schema requests, clearly separated from completed tuning.
