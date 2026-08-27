param(
    [Parameter(Mandatory = $true)]
    [string]$ScriptPath
)

$ErrorActionPreference = 'Stop'

if (Get-Process -Name 'UnrealEditor', 'UnrealEditor-Cmd' -ErrorAction SilentlyContinue) {
    throw 'Unreal Editor is running. Save and close it before commandlet work.'
}

$projectRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$projectPath = Resolve-Path (Join-Path $projectRoot 'ReEcho.uproject')
$resolvedScript = Resolve-Path $ScriptPath
$engineRoot = (& (Join-Path $PSScriptRoot 'Find-UnrealEngine.cmd') | Select-Object -Last 1).Trim()
$editorPath = Join-Path $engineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editorPath)) {
    throw "UnrealEditor-Cmd.exe not found: $editorPath"
}

$gitCommon = (git -C $projectRoot rev-parse --path-format=absolute --git-common-dir).Trim()
$lockDir = Join-Path $gitCommon 'reecho-locks'
$lockPath = Join-Path $lockDir 'unreal-editor.lock'
New-Item -ItemType Directory -Force -Path $lockDir | Out-Null
$lockAcquired = $false

try {
    $lockStream = [System.IO.File]::Open($lockPath, 'CreateNew', 'Write', 'None')
    $lockAcquired = $true
    $writer = [System.IO.StreamWriter]::new($lockStream)
    $writer.WriteLine("pid=$PID branch=$(git -C $projectRoot branch --show-current)")
    $writer.Dispose()

    & $editorPath $projectPath '-Unattended' '-NoSplash' '-NoSound' '-NullRHI' "-ExecutePythonScript=$resolvedScript" '-log'
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal Python command failed with exit code $LASTEXITCODE"
    }
}
catch [System.IO.IOException] {
    throw "ReEcho Unreal resource is already locked: $lockPath"
}
finally {
    if ($lockAcquired -and (Test-Path -LiteralPath $lockPath)) {
        Remove-Item -LiteralPath $lockPath
    }
}
