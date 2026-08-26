"""Export UE-decoded formal MP3 sources as WAV for deterministic mono preparation."""

from __future__ import annotations

from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
EXPORTS = {
    "/Game/ReEcho/Audio/Enemy/Enemy_Death.Enemy_Death": (
        ROOT / "Design/Audio/Decoded/Enemy/Enemy_Death.wav"
    ),
    "/Game/ReEcho/Audio/Variants/CombatAttack/Combat_Attack_W_J_01.Combat_Attack_W_J_01": (
        ROOT / "Design/Audio/Decoded/Variants/CombatAttack/W_J_01.wav"
    ),
    "/Game/ReEcho/Audio/Variants/CombatAttack/Combat_Attack_W_J_08.Combat_Attack_W_J_08": (
        ROOT / "Design/Audio/Decoded/Variants/CombatAttack/W_J_08.wav"
    ),
    "/Game/ReEcho/Audio/Variants/CombatHit/Combat_Hit_Flame.Combat_Hit_Flame": (
        ROOT / "Design/Audio/Decoded/Variants/CombatHit/Flame.wav"
    ),
}


for asset_path, output_path in EXPORTS.items():
    sound = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError(f"Expected SoundWave is missing: {asset_path}")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    task = unreal.AssetExportTask()
    task.object = sound
    task.filename = str(output_path)
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.SoundExporterWAV()
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError(f"SoundWave WAV export failed: {asset_path} -> {output_path}")
    unreal.log(f"Plan114 decoded WAV exported: {asset_path} -> {output_path}")

unreal.log(f"Plan114 decoded WAV exports complete: {len(EXPORTS)}")
