"""Import and configure all Plan46 catalog audio using Unreal Editor Python."""
from __future__ import annotations

from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()

LONG_AUDIO = {
    "Music.Menu": ("Design/Audio/Source/Music/Music_Menu.mp3", "/Game/ReEcho/Audio/Music", "Music_Menu"),
    "Music.Shop": ("Design/Audio/Source/Music/Music_Shop.mp3", "/Game/ReEcho/Audio/Music", "Music_Shop"),
    "Music.Boss": ("Design/Audio/Source/Music/Music_Boss.mp3", "/Game/ReEcho/Audio/Music", "Music_Boss"),
    "Music.Death": ("Design/Audio/Source/Music/Music_Death.mp3", "/Game/ReEcho/Audio/Music", "Music_Death"),
    "Music.Victory": ("Design/Audio/Source/Music/Music_Victory.mp3", "/Game/ReEcho/Audio/Music", "Music_Victory"),
    "Ambience.Arena": ("Design/Audio/Source/Ambience/Ambience_Arena.mp3", "/Game/ReEcho/Audio/Ambience", "Ambience_Arena"),
    "Ambience.Rain": ("Design/Audio/Source/Ambience/Ambience_Rain.mp3", "/Game/ReEcho/Audio/Ambience", "Ambience_Rain"),
}

ONE_SHOTS = {
    "UI.Hover": ("UI/UI_Hover.wav", "/Game/ReEcho/Audio/UI", "UI_Hover"),
    "UI.Confirm": ("UI/UI_Confirm.wav", "/Game/ReEcho/Audio/UI", "UI_Confirm"),
    "UI.Cancel": ("UI/UI_Cancel.wav", "/Game/ReEcho/Audio/UI", "UI_Cancel"),
    "UI.Error": ("UI/UI_Error.wav", "/Game/ReEcho/Audio/UI", "UI_Error"),
    "UI.Purchase": ("UI/UI_Purchase.wav", "/Game/ReEcho/Audio/UI", "UI_Purchase"),
    "UI.CardSelect": ("UI/UI_CardSelect.wav", "/Game/ReEcho/Audio/UI", "UI_CardSelect"),
    "Combat.Attack": ("Combat/Combat_Attack.wav", "/Game/ReEcho/Audio/Combat", "Combat_Attack"),
    "Combat.Hit": ("Combat/Combat_Hit.wav", "/Game/ReEcho/Audio/Combat", "Combat_Hit"),
    "Combat.Block": ("Combat/Combat_Block.wav", "/Game/ReEcho/Audio/Combat", "Combat_Block"),
    "Combat.Hurt": ("Combat/Combat_Hurt.wav", "/Game/ReEcho/Audio/Combat", "Combat_Hurt"),
    "Combat.Kill": ("Combat/Combat_Kill.wav", "/Game/ReEcho/Audio/Combat", "Combat_Kill"),
    "Combat.Death": ("Combat/Combat_Death.wav", "/Game/ReEcho/Audio/Combat", "Combat_Death"),
    "Enemy.Spawn": ("Enemy/Enemy_Spawn.wav", "/Game/ReEcho/Audio/Enemy", "Enemy_Spawn"),
    "Enemy.Attack": ("Enemy/Enemy_Attack.wav", "/Game/ReEcho/Audio/Enemy", "Enemy_Attack"),
    "Enemy.Death": ("Enemy/Enemy_Death.wav", "/Game/ReEcho/Audio/Enemy", "Enemy_Death"),
    "Boss.Spawn": ("Boss/Boss_Spawn.wav", "/Game/ReEcho/Audio/Boss", "Boss_Spawn"),
    "Boss.Attack": ("Boss/Boss_Attack.wav", "/Game/ReEcho/Audio/Boss", "Boss_Attack"),
    "Boss.Death": ("Boss/Boss_Death.wav", "/Game/ReEcho/Audio/Boss", "Boss_Death"),
    "Echo.Spawn": ("Echo/Echo_Spawn.wav", "/Game/ReEcho/Audio/Echo", "Echo_Spawn"),
    "Echo.Attack": ("Echo/Echo_Attack.wav", "/Game/ReEcho/Audio/Echo", "Echo_Attack"),
    "Echo.End": ("Echo/Echo_End.wav", "/Game/ReEcho/Audio/Echo", "Echo_End"),
    "CameraMove": ("Flow/CameraMove.wav", "/Game/ReEcho/Audio/Flow", "CameraMove"),
    "Revive": ("Flow/Revive.wav", "/Game/ReEcho/Audio/Flow", "Revive"),
}


def import_asset(source: Path, destination_path: str, destination_name: str, looping: bool) -> str:
    if not source.is_file():
        raise RuntimeError(f"Missing audio source: {source}")
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = destination_path
    task.destination_name = destination_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset_path = f"{destination_path}/{destination_name}.{destination_name}"
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(asset, unreal.SoundWave):
        raise RuntimeError(f"Import did not produce SoundWave: {asset_path}")
    asset.set_editor_property("looping", looping)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    unreal.log(f"Plan46 audio imported: {asset_path} looping={looping}")
    return asset_path


for event_id, (relative_source, destination_path, destination_name) in LONG_AUDIO.items():
    import_asset(ROOT / relative_source, destination_path, destination_name, True)

for event_id, (relative_source, destination_path, destination_name) in ONE_SHOTS.items():
    import_asset(ROOT / "Design" / "Audio" / "Generated" / relative_source, destination_path, destination_name, False)

existing_encounter = unreal.EditorAssetLibrary.load_asset(
    "/Game/ReEcho/Audio/Music/The_Iron_Waltz.The_Iron_Waltz"
)
if not isinstance(existing_encounter, unreal.SoundWave):
    raise RuntimeError("Existing Music.Encounter asset is missing or not a SoundWave")
existing_encounter.set_editor_property("looping", True)
unreal.EditorAssetLibrary.save_loaded_asset(existing_encounter, only_if_is_dirty=False)
unreal.log(f"Plan46 imported/configured {len(LONG_AUDIO) + len(ONE_SHOTS) + 1} catalog assets")
