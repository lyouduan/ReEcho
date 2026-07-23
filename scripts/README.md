# Project validation commands

Windows may block direct `.ps1` execution. Use the committed `.cmd` entry points; they apply `ExecutionPolicy Bypass` only to the child PowerShell process and do not change machine policy.

```powershell
python scripts\validate_project.py
scripts\ue\Find-UnrealEngine.cmd
scripts\ue\Build-Editor.cmd
scripts\ue\Run-Automation.cmd -Filter ReEcho
```

Pass `-EngineRoot D:\UE_5.x` to any UE command, or set the machine-local `RE_ECHO_UE_ROOT` environment variable. Do not commit an absolute engine path.
