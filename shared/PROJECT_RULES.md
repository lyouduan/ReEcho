# ReEcho project rules

This file contains project-specific additions to the canonical workflow. If it conflicts with a generic placeholder in the imported rules, this file wins.

## Scope and architecture

- Engine: Unreal Engine 5.8 installed/release build, Windows desktop first. Do not build or open this project with the separate source checkout.
- Project descriptor: `ReEcho.uproject`; runtime module: `Source/ReEcho`.
- Design-owned provisional values live in `Content/Data/*.json`. Stable rules and interfaces live in C++. Do not duplicate balance constants in actors or widgets.
- Preserve deterministic semantics: simulation 60 Hz, recording 20 Hz, encounter length 30 seconds, pause advances neither recording nor playback.
- Automatic attacks are never serialized into recordings. Echoes replay only historical position and successful active-skill events; attack targeting and hit resolution use the current world.
- Do not hand-edit `.uasset` or `.umap` binaries outside Unreal Editor. Prefer C++, JSON source data, Editor Utility generation, or Python automation for mergeable assets.

## Ownership and serial resources

- Merge-hostile resources are `.umap`, `.uasset`, Project Settings changed through the editor, and generated DataTables. Claim them in `shared/PLANNER_EXCHANGE.md` before editing.
- Only one agent may own an editor asset at a time. C++ and separate JSON files may proceed independently when file ownership does not overlap.
- Local editor lock: `.locks/unreal-editor.lock`. It is machine-local and ignored by git. Remove it only after confirming no UnrealEditor process for this project is running.

## Branch and handoff

- Implementation branches use `plan/<id>-<short-name>`; Codex-created branches may use `codex/<short-name>` when required by the client.
- Never merge the implementation branch into the default branch. Human approval and planner review are required.
- Stage explicit paths only. Never use `git add .` or `git add -A`.
- Every plan ends with `Execution notes`: changed behavior, evidence, remaining risks, and human-play checks.
- Before every commit, update the Markdown that describes the changed behavior or workflow (plans/<id>-*.md plus the relevant shared/PROJECT_STATE.md, shared/CODEBASE_MAP.md, shared/LESSONS.md, or rules). The staged code/assets and staged documentation must describe the same state; a code-only commit is incomplete unless the change is truly documentation-neutral.

## Unreal C++ coding standard (mandatory for every AI)

- Follow Epic Games' Unreal Engine C++ Coding Standard for every file under `Source/`.
- Never compress functions, conditionals, loops, lambdas, declarations, or multiple statements onto one line.
- Use Allman braces, four-column tab indentation, one statement per line, and spaces around binary operators and after commas.
- Use Unreal naming and type conventions (`A/U/F/E/I/T` prefixes, `b` for booleans, PascalCase symbols) and UE types where applicable.
- Keep each header self-contained: `#pragma once`, matching generated header last, minimal includes, and forward declarations where valid.
- In `.cpp` files, include the matching header first; keep include groups stable and do not auto-sort Unreal include order.
- Prefer early returns, named constants, and small helpers over deeply nested or duplicated logic. Do not hide control flow in dense expressions.
- Run the repository `.clang-format` on every changed `.h`/`.cpp`, inspect the diff, then run UHT/UBT and `git diff --check`.
- Formatting-only cleanup must not alter gameplay semantics. Any deliberate exception requires a nearby explanation and explicit handoff note.

## Build and verification

Use the `.cmd` entry points on Windows; they bypass PowerShell execution policy only for their child process and do not alter machine policy.

1. Run `scripts/ue/Find-UnrealEngine.cmd` to discover the local engine.
2. Close Unreal Editor before validating C++ changes; restarting PIE does not reliably reload a newly built editor DLL.
3. Run `scripts/ue/Build-Editor.cmd`.
4. Run `scripts/ue/Run-Automation.cmd -Filter ReEcho` when the editor build succeeds.
5. For data-only changes, run `python scripts/validate_project.py` before opening the editor.
6. For gameplay changes, add structured evidence. Prefer deterministic automation or UTF-8-without-BOM telemetry files over screen messages.

Pass `-EngineRoot <path>` or set machine-local `RE_ECHO_UE_ROOT`; never commit an absolute engine path. If Unreal Engine is unavailable, report validation as `static only`; never describe the project as compiled or PIE-tested.

## Definition of done

- Relevant automated checks pass, or the exact unavailable prerequisite is recorded.
- `git diff --check` passes, generated/intermediate/package files are not included, and relevant Markdown has been synchronized before staging.
- The plan execution notes and `shared/PLANNER_EXCHANGE.md` ownership row are updated.
- A human performs PIE play-feel validation for movement, readability, pacing, and planning comprehension.


