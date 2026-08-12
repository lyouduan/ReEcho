[CmdletBinding()]
param(
    [string]$EngineRoot,
    [ValidateSet('DebugGame','Development','Shipping')][string]$Configuration = 'Development',
    [switch]$FullRebuild
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
$buildArguments = @('ReEchoEditor', 'Win64', $Configuration, $project, '-WaitMutex', '-NoHotReloadFromIDE')
if ($FullRebuild) {
    $buildArguments += '-Rebuild'
}
& $build @buildArguments
$buildExitCode = $LASTEXITCODE
if ($buildExitCode -ne 0) { exit $buildExitCode }

if ($Configuration -eq 'Development') {
    $prebuiltTool = Join-Path $PSScriptRoot 'prebuilt_editor.py'
    & python $prebuiltTool update
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

exit 0
