# ReEcho Git rules

This file is the single authority for commit identity and publication-completeness rules. Role and duty files route here instead of redefining tags or the Programmer publication build gate.

## AI commit identity

Every commit created by an AI must begin its subject with exactly one tag matching the professional route or explicitly assigned repository duty used to create that commit:

| Commit creator | Required subject prefix |
|---|---|
| Programmer route | `[PROGRAMMER]` |
| Designer route | `[DESIGNER]` |
| Artist route | `[ARTIST]` |
| Explicit Project Secretary duty | `[SECRETARY]` |

- A role must not use another role's tag. Project Secretary uses its tag only for coordination/control-plane commits within `SECRETARY_RULES.md`; it does not label specialist implementation.
- A Programmer Planner's integration or merge commit uses `[PROGRAMMER]` because Planner/Executor is a repository duty inside the Programmer route, not a separate professional identity.
- A mixed-scope commit is not an excuse to combine tags. Split it at the professional boundary first.
- This rule applies to commits created after this file takes effect; do not rewrite historical commits solely to add tags.

## Programmer publication build gate

This gate applies to the ordinary Programmer route. It is not waived by any Planner-Executor mode choice; the non-skippable remote collaboration safety check in `shared/PROGRAMMER_RULES.md` still applies in every mode.

Before every ordinary Programmer-route push to `origin/main`, the final integrated candidate must be built in full with the project-standard UE 5.8 installed/release build:

```powershell
scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild
```

The successful command refreshes the curated Win64 Editor prebuilt bundle and its source fingerprint. The same candidate must include all approved source/content/configuration changes plus the matching tracked files declared by `Binaries/Win64/ReEchoEditor.prebuilt.json`.

- The publication gate requires `-FullRebuild`; an incremental build that reports the target up to date is useful during implementation but does not satisfy the final push gate.
- Never publish a source-only Programmer candidate or reuse build evidence from before its final integration/rebase/conflict resolution.
- A failed build, missing binary, binary-hash mismatch, stale source fingerprint, wrong engine Build ID, or dirty prebuilt refresh blocks the push.
- Run `python scripts/validate_project.py` after the build and commit the refreshed allowlisted bundle before the final fetch/push gate.
- Only the manifest, `ReEchoEditor.target`, `UnrealEditor.modules`, and the exact module DLLs named by that manifest are publishable generated Editor products. Other `.target` files, `Intermediate`, PDB/import libraries, Live Coding patches, generated solutions, caches, logs and machine-local paths remain local.
- The bundle guarantees clone-and-open only on Win64 with the exact UE 5.8 installed/release Build ID recorded in the manifest. Other engine builds must compile locally or receive their own reviewed delivery contract.

Designer, Artist and Secretary commits do not rebuild or modify the Programmer-owned prebuilt bundle. Their local commits retain their role tag and are integrated by a Programmer Planner; the later Programmer publication build covers the final combined candidate.
