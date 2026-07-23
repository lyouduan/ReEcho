[CmdletBinding()]
param(
    [string]$EngineRoot,
    [string]$Filter = 'ReEcho'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$project = Join-Path $repoRoot 'ReEcho.uproject'
$finder = Join-Path $PSScriptRoot 'Find-UnrealEngine.ps1'
$resolvedEngine = & $finder -EngineRoot $EngineRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$editor = Join-Path $resolvedEngine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $editor $project -unattended -nop4 -nosplash -nullrhi "-ExecCmds=Automation RunTests $Filter;Quit" -TestExit='Automation Test Queue Empty' -log
exit $LASTEXITCODE
