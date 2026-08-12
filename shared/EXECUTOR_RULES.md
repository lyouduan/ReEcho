# ReEcho Executor rules

This file contains Executor actions only. Follow `AGENTS.md` for minimal startup order and `PROJECT_RULES.md` for hard constraints.

## Start safely

1. Work in an isolated `plan/<id>-<short>` branch/worktree, never in `main`.
2. Confirm the assigned Plan is already published on `origin/main`, or confirm the embedded prompt contract when the task legitimately has no Plan:
   - the declared implementation-authored-by AI side matches this Executor handoff; if missing or wrong, stop before editing and ask the Planner to correct provenance;
   - lifecycle is `Ready`, `InProgress` or `Review` only when review fixes were explicitly assigned;
   - implementation base and branch are named;
   - Writes/Reads, impact mode and exclusions match Exchange;
   - any `SharedContract`/`Exclusive` approval is present.
3. A `Reserved` ownership row announces intent; it does not block disjoint or read-only work. Never write another owner's `Active` `Exclusive` resource.
4. Read only the assigned Plan, matching Exchange blocks, the selected code route and relevant lessons. Read `§DEBUG` only after a failure needs diagnosis.

Same-machine agents use separate worktrees. Teammates on separate machines use separate clones and local task branches; `origin/main` is the only remote branch. Build products are local and are not expected to appear in another worktree/clone.

## Implement within the contract

- Refine implementation details freely while preserving the locked goal, acceptance and protected gates.
- Record meaningful deviations and evidence in the Plan's Execution notes.
- Update documentation tied directly to owned behavior. Do not routinely edit `PROJECT_STATE`, `CODEBASE_MAP`, `LESSONS`, workflow rules or closed Exchange history.
- If implementation must expand Writes or alter stable IDs, schema, save format, generator contract or public API, stop only that boundary-crossing work and notify the Planner. Continue unrelated in-scope work when safe.
- Never weaken acceptance to make a check pass, silently copy a provider's internal data model or treat a WIP branch as accepted functionality.

## Parallel ownership

- `Isolated`: edit only the declared disjoint files.
- `ReadOnly`: consume only the provider's published stable surface.
- `SharedContract`: use the agreed interface; request additions through the Planner rather than editing provider-owned files.
- `Exclusive`: one active writer for merge-hostile artifacts/targets.

For the canonical workbook, only the active `WorkbookWriter` commits `Design/Data/ReEchoData.xlsx` and its generated production CSV package. Other domains submit requested changes through their Plan/handoff; read-only runtime consumers may proceed concurrently. Disposable local QA edits that will not be published do not claim repository ownership.

## Windows UE toolchain

Use repository entry points from the project root:

```powershell
python scripts\validate_project.py
scripts\ue\Build-Editor.cmd -Configuration Development
scripts\ue\Run-Automation.cmd -Filter ReEcho
python scripts\ue\package_windows.py
```

Use only checks required by `PROJECT_RULES.md` and the Plan. Ask the human to save/close an interactive Editor before commands that require it. Never claim PIE or visual acceptance; those results belong to the human.

### Same-clone Unreal lock

The lock is shared through the Git common directory, so all worktrees in the clone see the same file:

```powershell
$gitCommon = (git rev-parse --path-format=absolute --git-common-dir).Trim()
$lockDir = Join-Path $gitCommon 'reecho-locks'
$lockPath = Join-Path $lockDir 'unreal-editor.lock'
New-Item -ItemType Directory -Force -Path $lockDir | Out-Null
$lockAcquired = $false

try {
    $lockStream = [System.IO.File]::Open($lockPath, 'CreateNew', 'Write', 'None')
    $lockAcquired = $true
    $writer = [System.IO.StreamWriter]::new($lockStream)
    $writer.WriteLine("pid=$PID branch=$(git branch --show-current)")
    $writer.Dispose()
    # Run the exclusive Editor/project command here.
}
catch [System.IO.IOException] {
    throw "ReEcho Unreal resource is already locked: $lockPath"
}
finally {
    if ($lockAcquired -and (Test-Path -LiteralPath $lockPath)) {
        Remove-Item -LiteralPath $lockPath
    }
}
```

Do not remove someone else's lock from the catch path. A stale lock may be removed only after confirming no UnrealEditor/commandlet process is using this project and recording the recovery in the handoff.

## Verification and handoff

1. Run focused checks immediately after risky changes and the required final matrix before handoff.
2. Summarize current-commit evidence; do not rely on stale binaries or pre-rebase results.
3. Update the Plan lifecycle to `Review`, preserve the separate human-validation value (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`), and release only resources no longer being edited.
4. Stage explicit files and commit locally with the Plan number and a concrete outcome. Do not push the task branch; hand the local branch/commit to the Planner in the same clone.
5. Report commit, changed surface, passed/failed checks, unavailable prerequisites and remaining human validation.

Executors do not push any remote ref, merge/publish `main`, create a PR/MR, remove their worktree, or delete remote refs.

## Taking over unfinished work

- Identify the last verified commit from Git and Execution notes.
- Separate current unverified changes with `git status` and `git diff --stat`.
- Change and verify one hypothesis at a time; do not stack speculative fixes.
- Preserve another person's work with a branch/stash/patch before any rollback. Never use destructive history or filesystem cleanup without explicit authority.
- Fix sources and rebuild; do not patch `Intermediate`, `Binaries`, `Saved`, package output or other generated products as if they were source.
