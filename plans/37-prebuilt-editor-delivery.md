# Plan 37 - Build - clone-and-open Editor delivery

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Codex.
- Plan authored by (AI side): `Gavyn-side AI`.
- Implementation authored by (AI side): `Gavyn-side AI`.
- Task status: `Review` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `NotRequired` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: `origin/main` at `dcd0b050f3cd24c2740d587dfbe2e5ef1025a89a`.
- Implementation branch: local `plan/37-prebuilt-editor-delivery` in a separate worktree.
- Depends on / Blocks: no implementation dependency; every later C++ or module-descriptor publication must refresh the prebuilt Editor bundle.
- Writes: `.gitignore`; curated `Binaries/Win64` Editor runtime files; `scripts/ue` prebuilt tooling and build entry point; project validation; directly related workflow/docs; this Plan and live Exchange row.
- Stable Reads: current UE 5.8 installed-build module descriptor, target rules, runtime source and existing build entry point.
- Impact mode: `SharedContract` because this establishes the repository-wide C++ publication gate consumed by every programmer Plan.
- Compatibility promise / downstream action: a fresh Windows checkout at a valid commit opens with the UE 5.8 release Editor without a local compiler; source builders continue using the existing build command, which refreshes the curated bundle after a successful build.
- Explicit exclusions: no gameplay/content change, no non-Windows precompiled target, no `Intermediate`, PDB, import library, Live Coding patch, generated solution or machine-local absolute path, and no promise across a different Unreal Engine build.

## Locked goal

Make a fresh Windows checkout of ReEcho directly openable through `ReEcho.uproject` with the project-standard UE 5.8 installed/release build, without requiring Visual Studio or a local C++ compile before first launch.

## Locked acceptance

- [ ] `origin/main` contains the minimal Win64 Editor module files required for Unreal to load `ReEcho`.
- [ ] A committed manifest binds the bundle to the UE association/build, target/configuration, source fingerprint and binary hashes.
- [ ] Static validation fails when required prebuilt files are missing, modified, or stale relative to `Source`, target rules or `ReEcho.uproject`.
- [ ] The normal Editor build command refreshes the manifest after a successful full build.
- [ ] A clean checkout containing only tracked files launches `ReEcho.uproject` unattended with UE 5.8 and exits without a missing/incompatible-module error.
- [ ] Generated products outside the explicit prebuilt allowlist remain ignored and uncommitted.
- [ ] Project validation, focused prebuilt tests, Editor build and `git diff --check` pass.

## Step 0 gate

- Baseline branch/commit: `origin/main` at `dcd0b050f3cd24c2740d587dfbe2e5ef1025a89a`.
- Engine/build availability: UE 5.8 installed/release build discoverable through repository scripts; no ReEcho Editor process was active at start.
- Existing focused-test result: current main static project validation passed in the preceding Secretary publication; refresh on this branch.
- Active exclusive ownership or shared-contract approval: no active owner writes the selected files; future C++ Plans consume this new shared contract.
- Stop condition if the baseline is broken: full Editor build fails, required startup files cannot be isolated without machine-local data, or clean-checkout launch reports a missing/incompatible module.

## Implementation outline

1. Define the minimal tracked Editor bundle and retain ignores for all unrelated generated output.
2. Add deterministic source/binary fingerprint generation and check modes.
3. Refresh the fingerprint automatically after a successful repository Editor build.
4. Extend project validation and update the authoritative build-product exception plus concise onboarding pointers.
5. Rebuild, stage only the allowlist, and launch a tracked-files-only checkout for objective proof.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | prebuilt presence, hashes, source fingerprint and workflow invariants pass |
| Focused | prebuilt tool `--check` plus tamper/staleness fixtures | missing, changed and stale bundles fail deterministically |
| Build | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT succeeds and refreshes the manifest |
| Fresh checkout | launch tracked-only copy with UE 5.8 unattended | project module loads; no missing/incompatible-module error |
| Scope | `git diff --check` and explicit tracked-product audit | only allowlisted generated products are published |

## Execution notes

### Changed

- Added a centralized AI commit-identity and Programmer publication-build authority.
- Added a minimal tracked Win64 Editor module bundle with deterministic source and binary fingerprints.
- Made the normal Development Editor build refresh and normalize the bundle automatically.
- Extended static validation and focused tests for missing, tampered and source-stale bundles.

### Evidence

- UE 5.8 `ReEchoEditor Win64 Development` full UHT/UBT build succeeded against Build ID `55116800`.
- `python scripts/ue/prebuilt_editor.py check` passed for one declared module and the current source fingerprint.
- `python scripts/ue/test_prebuilt_editor.py` passed 3/3 focused tests.
- `python scripts/validate_project.py` and `git diff --cached --check` passed.
- A tracked-files-only pre-implementation probe loaded the module and reached `Cmd: QUIT` with no missing/incompatible-module log; repeat against the committed candidate before closure.

### Remaining risks

- The committed candidate still needs the final tracked-files-only launch smoke after its implementation commit exists.
- The bundle is intentionally limited to Win64 and the exact UE 5.8 installed-build ID in its manifest.

### Human validation result/request

`NotRequired`: direct-open behavior is covered by a clean-checkout launch smoke test; visual or gameplay acceptance is outside this Plan.
