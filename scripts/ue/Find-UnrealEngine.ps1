[CmdletBinding()]
param([string]$EngineRoot)

$ErrorActionPreference = 'Stop'

function Test-EngineRoot([string]$Candidate) {
    if ([string]::IsNullOrWhiteSpace($Candidate)) { return $null }
    $resolved = [IO.Path]::GetFullPath($Candidate)
    $editor = Join-Path $resolved 'Engine\Binaries\Win64\UnrealEditor.exe'
    $build = Join-Path $resolved 'Engine\Build\BatchFiles\Build.bat'
    if ((Test-Path -LiteralPath $editor) -and (Test-Path -LiteralPath $build)) { return $resolved }
    return $null
}

$candidates = [Collections.Generic.List[string]]::new()
if ($EngineRoot) { $candidates.Add($EngineRoot) }
if ($env:RE_ECHO_UE_ROOT) { $candidates.Add($env:RE_ECHO_UE_ROOT) }

# Prefer Epic Launcher release builds because ReEcho.uproject uses EngineAssociation "5.8".
$launcherManifest = 'C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat'
if (Test-Path -LiteralPath $launcherManifest) {
    $manifest = Get-Content -Raw -LiteralPath $launcherManifest | ConvertFrom-Json
    $manifest.InstallationList |
        Where-Object { $_.ArtifactId -eq 'UE_5.8' -or $_.AppName -eq 'UE_5.8' } |
        ForEach-Object { $candidates.Add($_.InstallLocation) }
}

# Custom/source registrations are fallbacks only.
if (Test-Path 'HKCU:\Software\Epic Games\Unreal Engine\Builds') {
    $item = Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
    foreach ($property in $item.PSObject.Properties) {
        if ($property.Value -is [string]) { $candidates.Add($property.Value) }
    }
}

foreach ($drive in 'C','D','E','F') {
    $epicRoot = "${drive}:\Program Files\Epic Games"
    if (Test-Path $epicRoot) {
        Get-ChildItem -LiteralPath $epicRoot -Directory -Filter 'UE_*' | ForEach-Object { $candidates.Add($_.FullName) }
    }
    Get-ChildItem -LiteralPath "${drive}:\" -Directory -Filter 'UE_*' -ErrorAction SilentlyContinue | ForEach-Object { $candidates.Add($_.FullName) }
}

foreach ($candidate in $candidates) {
    $found = Test-EngineRoot $candidate
    if ($found) { Write-Output $found; exit 0 }
}

Write-Error 'Unreal Engine was not found. Pass -EngineRoot or set RE_ECHO_UE_ROOT.'
exit 2
