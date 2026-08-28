param(
    [string]$SourceDirectory = 'F:\frames',
    [string]$FfmpegPath = 'C:\tmp\ReEcho-plan124-encounter-card-transition\Saved\Plan124Tools\ffmpeg-full\ffmpeg-9.0.1-full_build\bin\ffmpeg.exe'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $FfmpegPath -PathType Leaf)) {
    throw "FFmpeg was not found: $FfmpegPath"
}

$sourceFrames = @{}
for ($frameIndex = 0; $frameIndex -le 120; ++$frameIndex) {
    $frameName = 'frame_{0:D4}.png' -f $frameIndex
    $framePath = Join-Path $SourceDirectory $frameName
    if (-not (Test-Path -LiteralPath $framePath -PathType Leaf)) {
        throw "Required source frame is missing: $framePath"
    }
    $sourceFrames[$frameIndex] = $framePath
}

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$movieDirectory = Join-Path $projectRoot 'Content\Movies\EncounterTransition'
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('ReEchoPlan143Media-' + [guid]::NewGuid().ToString('N'))
$firstStage = Join-Path $temporaryRoot 'EncounterEndToCardChoiceV2'
$secondStage = Join-Path $temporaryRoot 'CardChoiceToShop'
New-Item -ItemType Directory -Force -Path $firstStage, $secondStage | Out-Null

try {
    # 0-57 contains the one-time entrance and the first complete swing. Frame 26 repeats the
    # centered pose at frame 57, so later cycles start at 27 to avoid a one-frame hold at each seam.
    $firstSequence = New-Object System.Collections.Generic.List[int]
    foreach ($frameIndex in 0..57) { $firstSequence.Add($frameIndex) }
    foreach ($repeatIndex in 1..2) {
        foreach ($frameIndex in 27..57) { $firstSequence.Add($frameIndex) }
    }
    for ($outputIndex = 0; $outputIndex -lt $firstSequence.Count; ++$outputIndex) {
        Copy-Item -LiteralPath $sourceFrames[$firstSequence[$outputIndex]] `
            -Destination (Join-Path $firstStage ('frame_{0:D4}.png' -f $outputIndex))
    }
    for ($sourceIndex = 58; $sourceIndex -le 120; ++$sourceIndex) {
        Copy-Item -LiteralPath $sourceFrames[$sourceIndex] `
            -Destination (Join-Path $secondStage ('frame_{0:D4}.png' -f ($sourceIndex - 58)))
    }

    $firstOutput = Join-Path $temporaryRoot 'EncounterEndToCardChoiceV2.mov'
    $secondOutput = Join-Path $temporaryRoot 'CardChoiceToShop.mov'
    & $FfmpegPath -hide_banner -loglevel error -y -framerate 40 `
        -i (Join-Path $firstStage 'frame_%04d.png') -c:v hap -format hap_alpha -compressor snappy $firstOutput
    if ($LASTEXITCODE -ne 0) { throw "FFmpeg failed to encode the card-choice transition." }
    & $FfmpegPath -hide_banner -loglevel error -y -framerate 40 `
        -i (Join-Path $secondStage 'frame_%04d.png') -c:v hap -format hap_alpha -compressor snappy $secondOutput
    if ($LASTEXITCODE -ne 0) { throw "FFmpeg failed to encode the shop transition." }

    New-Item -ItemType Directory -Force -Path $movieDirectory | Out-Null
    Move-Item -LiteralPath $firstOutput -Destination (Join-Path $movieDirectory 'EncounterEndToCardChoiceV2.mov') -Force
    Move-Item -LiteralPath $secondOutput -Destination (Join-Path $movieDirectory 'CardChoiceToShop.mov') -Force
    Write-Output "[PASS] Plan143 media rebuilt from $SourceDirectory"
    Write-Output "[PASS] EncounterEndToCardChoiceV2: $($firstSequence.Count) frames at 40 FPS (entrance + 3 swings)"
    Write-Output '[PASS] CardChoiceToShop: 63 frames at 40 FPS'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
