# Project validation commands

Windows may block direct `.ps1` execution. Use the committed `.cmd` entry points; they apply `ExecutionPolicy Bypass` only to the child PowerShell process and do not change machine policy.

```powershell
python scripts\validate_project.py
scripts\ue\Find-UnrealEngine.cmd
scripts\ue\Build-Editor.cmd
scripts\ue\Run-Automation.cmd -Filter ReEcho
```

Pass `-EngineRoot D:\UE_5.x` to any UE command, or set the machine-local `RE_ECHO_UE_ROOT` environment variable. Do not commit an absolute engine path.

## Windows Shipping package

Close/save Unreal Editor work before packaging. The script force-closes `UnrealEditor` and `UnrealEditor-Cmd`, discovers the installed UE 5.8 build, performs a clean Shipping Build/Cook/Pak/Archive, checks `ReEcho.exe`, and runs a 10-second smoke test.

```powershell
python scripts\ue\package_windows.py
```

The default output is `Packages\Windows`. Useful options:

```powershell
# Choose another archive directory.
python scripts\ue\package_windows.py --output D:\Builds\ReEcho

# Use a specific installed engine and skip the launch smoke test.
python scripts\ue\package_windows.py --engine-root D:\Epic\UE_5.8 --no-smoke

# Inspect the generated UAT command without closing UE or building.
python scripts\ue\package_windows.py --dry-run
```

Run `python scripts\ue\package_windows.py --help` for all options. Distribute the entire archived Windows directory, not only `ReEcho.exe`.