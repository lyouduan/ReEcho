[CmdletBinding()]
param(
    [string]$EngineRoot,
    [ValidateSet('DebugGame','Development','Shipping')][string]$Configuration = 'Development'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$project = Join-Path $repoRoot 'ReEcho.uproject'
$finder = Join-Path $PSScriptRoot 'Find-UnrealEngine.ps1'
$resolvedEngine = & $finder -EngineRoot $EngineRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$editorProcesses = Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -like "*$project*" }
if ($editorProcesses) {
    Write-Error 'Close the ReEcho Unreal Editor before building C++.'
    exit 3
}

$build = Join-Path $resolvedEngine 'Engine\Build\BatchFiles\Build.bat'
& $build ReEchoEditor Win64 $Configuration $project -WaitMutex -NoHotReloadFromIDE
exit $LASTEXITCODE
