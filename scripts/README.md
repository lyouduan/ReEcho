# Project validation commands

Windows may block direct `.ps1` execution. Use the committed `.cmd` entry points; they apply `ExecutionPolicy Bypass` only to the child PowerShell process and do not change machine policy.

```powershell
python scripts\validate_project.py
scripts\ue\Find-UnrealEngine.cmd
scripts\ue\Build-Editor.cmd
scripts\ue\Run-Automation.cmd -Filter ReEcho
```

`Build-Editor.cmd -Configuration Development` performs a full Editor build and refreshes the tracked clone-and-open bundle. `python scripts\ue\prebuilt_editor.py check` verifies its source fingerprint and binary hashes without invoking Unreal.

`python scripts\setup_lfs.py` initializes Git LFS for the current clone and downloads objects for the checked-out revision. Use `python scripts\setup_lfs.py --check` before opening Unreal, building or packaging to reject missing objects and unresolved LFS pointer files.

Pass `-EngineRoot D:\UE_5.x` to any UE command, or set the machine-local `RE_ECHO_UE_ROOT` environment variable. Do not commit an absolute engine path.

## One-command Windows package

Save all Unreal assets and close Unreal Editor, then run the script directly. With no options it initializes/checks Git LFS, packages the current checkout as a clean Win64 Development test build, runs a 12-second smoke test, and writes a timestamped directory to the Desktop. The script never force-closes Unreal Editor and never commits or pushes Git changes.

```powershell
python scripts\ue\package_windows.py
```

Every run includes `ReEchoPackageSource.txt` and `PackagingEvidence\BuildCookRun.log`. Smoke evidence is copied into `PackagingEvidence\RuntimeLogs`; failures also produce `PackagingEvidence\FailureSummary.txt`.

Useful options:

```powershell
# Fetch and package exact origin/main in an isolated temporary worktree.
python scripts\ue\package_windows.py --remote-main

# Build a formal Shipping package. Shipping does not promise full UE logs.
python scripts\ue\package_windows.py --formal

# Choose an empty archive directory.
python scripts\ue\package_windows.py --output D:\Builds\ReEcho

# Use a specific installed engine and skip the launch smoke test.
python scripts\ue\package_windows.py --engine-root D:\Epic\UE_5.8 --no-smoke

# Inspect the generated UAT command without building or launching the game.
python scripts\ue\package_windows.py --dry-run
```

`--remote-main --formal` can be combined. Run `python scripts\ue\package_windows.py --help` for all options. Distribute the entire output directory, not only `ReEcho.exe`.
