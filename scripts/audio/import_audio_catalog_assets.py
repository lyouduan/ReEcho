"""Import and configure the current catalog audio using Unreal Editor Python."""
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
    "UI.Hover": ("Source/Formal/UI/UI_Hover.mp3", "/Game/ReEcho/Audio/UI", "UI_Hover"),
    "UI.Confirm": ("Source/Formal/UI/UI_Confirm.mp3", "/Game/ReEcho/Audio/UI", "UI_Confirm"),
    "UI.Cancel": ("Source/Formal/UI/UI_Cancel.wav", "/Game/ReEcho/Audio/UI", "UI_Cancel"),
    "UI.Error": ("Source/Formal/UI/UI_Cancel.wav", "/Game/ReEcho/Audio/UI", "UI_Error"),
    "UI.Purchase": ("Source/Formal/UI/UI_Purchase.wav", "/Game/ReEcho/Audio/UI", "UI_Purchase"),
    "UI.CardSelect": ("Source/Formal/UI/UI_CardSelect.wav", "/Game/ReEcho/Audio/UI", "UI_CardSelect"),
    "Combat.Attack": ("Generated/Combat/Combat_Attack.wav", "/Game/ReEcho/Audio/Combat", "Combat_Attack"),
    "Combat.Hit": ("Generated/Combat/Combat_Hit.wav", "/Game/ReEcho/Audio/Combat", "Combat_Hit"),
    "Combat.Block": ("Generated/Combat/Combat_Block.wav", "/Game/ReEcho/Audio/Combat", "Combat_Block"),
    "Combat.Hurt": ("Derived/Combat/Combat_Hurt.wav", "/Game/ReEcho/Audio/Combat", "Combat_Hurt"),
    "Combat.Kill": ("Generated/Combat/Combat_Kill.wav", "/Game/ReEcho/Audio/Combat", "Combat_Kill"),
    "Combat.Death": ("Derived/Combat/Combat_Death.wav", "/Game/ReEcho/Audio/Combat", "Combat_Death"),
    "Enemy.Spawn": ("Derived/Enemy/Enemy_Spawn.wav", "/Game/ReEcho/Audio/Enemy", "Enemy_Spawn"),
    "Enemy.Attack": ("Generated/Enemy/Enemy_Attack.wav", "/Game/ReEcho/Audio/Enemy", "Enemy_Attack"),
    "Enemy.Death": ("Derived/Enemy/Enemy_Death.wav", "/Game/ReEcho/Audio/Enemy", "Enemy_Death"),
    "Boss.Spawn": ("Generated/Boss/Boss_Spawn.wav", "/Game/ReEcho/Audio/Boss", "Boss_Spawn"),
    "Boss.Attack": ("Generated/Boss/Boss_Attack.wav", "/Game/ReEcho/Audio/Boss", "Boss_Attack"),
    "Boss.Death": ("Derived/Boss/Boss_Death.wav", "/Game/ReEcho/Audio/Boss", "Boss_Death"),
    "Echo.Spawn": ("Generated/Echo/Echo_Spawn.wav", "/Game/ReEcho/Audio/Echo", "Echo_Spawn"),
    "Echo.Attack": ("Generated/Echo/Echo_Attack.wav", "/Game/ReEcho/Audio/Echo", "Echo_Attack"),
    "Echo.End": ("Generated/Echo/Echo_End.wav", "/Game/ReEcho/Audio/Echo", "Echo_End"),
    "CameraMove": ("Source/Formal/Flow/CameraMove.wav", "/Game/ReEcho/Audio/Flow", "CameraMove"),
    "Revive": ("Source/Formal/UI/UI_Cancel.wav", "/Game/ReEcho/Audio/Flow", "Revive"),
}

PLAN114_DIRECT_EVENT_IDS = {
    "Music.Menu",
    "Music.Shop",
    "Music.Boss",
    "UI.Hover",
    "UI.Confirm",
    "UI.Cancel",
    "UI.Error",
    "UI.Purchase",
    "UI.CardSelect",
    "Combat.Hurt",
    "Combat.Death",
    "Enemy.Spawn",
    "Enemy.Death",
    "Boss.Death",
    "CameraMove",
    "Revive",
}

VARIANT_DECODE_INPUTS = {
    "Enemy.Death": (
        "Source/Formal/Enemy/Enemy_Death.mp3",
        "/Game/ReEcho/Audio/Enemy",
        "Enemy_Death",
    ),
    "Combat.Attack/W_J_01": (
        "Source/Formal/Variants/CombatAttack/W_J_01.mp3",
        "/Game/ReEcho/Audio/Variants/CombatAttack",
        "Combat_Attack_W_J_01",
    ),
    "Combat.Attack/W_J_08": (
        "Source/Formal/Variants/CombatAttack/W_J_08.mp3",
        "/Game/ReEcho/Audio/Variants/CombatAttack",
        "Combat_Attack_W_J_08",
    ),
    "Combat.Hit/Flame": (
        "Source/Formal/Variants/CombatHit/Flame.mp3",
        "/Game/ReEcho/Audio/Variants/CombatHit",
        "Combat_Hit_Flame",
    ),
}

VARIANT_ONE_SHOTS = {
    "Combat.Attack/W_J_01": ("Derived/Variants/CombatAttack/W_J_01.wav", "/Game/ReEcho/Audio/Variants/CombatAttack", "Combat_Attack_W_J_01"),
    "Combat.Attack/W_J_04": ("Derived/Variants/CombatAttack/W_J_04.wav", "/Game/ReEcho/Audio/Variants/CombatAttack", "Combat_Attack_W_J_04"),
    "Combat.Attack/W_J_08": ("Derived/Variants/CombatAttack/W_J_08.wav", "/Game/ReEcho/Audio/Variants/CombatAttack", "Combat_Attack_W_J_08"),
    "Combat.Attack/W_J_09": ("Derived/Variants/CombatAttack/W_J_09.wav", "/Game/ReEcho/Audio/Variants/CombatAttack", "Combat_Attack_W_J_09"),
    "Combat.Hit/Flame": ("Derived/Variants/CombatHit/Flame.wav", "/Game/ReEcho/Audio/Variants/CombatHit", "Combat_Hit_Flame"),
    "Combat.Hit/Lightning": ("Derived/Variants/CombatHit/Lightning.wav", "/Game/ReEcho/Audio/Variants/CombatHit", "Combat_Hit_Lightning"),
    "Combat.Hit/Grass": ("Derived/Variants/CombatHit/Grass.wav", "/Game/ReEcho/Audio/Variants/CombatHit", "Combat_Hit_Grass"),
    "Combat.Hit/Water": ("Derived/Variants/CombatHit/Water.wav", "/Game/ReEcho/Audio/Variants/CombatHit", "Combat_Hit_Water"),
}

VARIANT_MUSIC = {
    "Music.Encounter/Stage.1": ("Source/Formal/Variants/MusicEncounter/Stage_1.mp3", "/Game/ReEcho/Audio/Variants/MusicEncounter", "Music_Encounter_Stage_1"),
    "Music.Encounter/Stage.2": ("Source/Formal/Variants/MusicEncounter/Stage_2.mp3", "/Game/ReEcho/Audio/Variants/MusicEncounter", "Music_Encounter_Stage_2"),
    "Music.Encounter/Stage.3": ("Source/Formal/Variants/MusicEncounter/Stage_3.mp3", "/Game/ReEcho/Audio/Variants/MusicEncounter", "Music_Encounter_Stage_3"),
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


command_line = unreal.SystemLibrary.get_command_line()
direct_only = "-Plan114DirectOnly" in command_line
decode_only = "-Plan114DecodeOnly" in command_line
variants_only = "-Plan114VariantsOnly" in command_line
imported_count = 0

if decode_only:
    for relative_source, destination_path, destination_name in VARIANT_DECODE_INPUTS.values():
        import_asset(ROOT / "Design" / "Audio" / relative_source, destination_path, destination_name, False)
        imported_count += 1
elif variants_only:
    for relative_source, destination_path, destination_name in VARIANT_ONE_SHOTS.values():
        import_asset(ROOT / "Design" / "Audio" / relative_source, destination_path, destination_name, False)
        imported_count += 1
    for relative_source, destination_path, destination_name in VARIANT_MUSIC.values():
        import_asset(ROOT / "Design" / "Audio" / relative_source, destination_path, destination_name, True)
        imported_count += 1
else:
    for event_id, (relative_source, destination_path, destination_name) in LONG_AUDIO.items():
        if direct_only and event_id not in PLAN114_DIRECT_EVENT_IDS:
            continue
        import_asset(ROOT / relative_source, destination_path, destination_name, True)
        imported_count += 1

    for event_id, (relative_source, destination_path, destination_name) in ONE_SHOTS.items():
        if direct_only and event_id not in PLAN114_DIRECT_EVENT_IDS:
            continue
        import_asset(ROOT / "Design" / "Audio" / relative_source, destination_path, destination_name, False)
        imported_count += 1

    if not direct_only:
        existing_encounter = unreal.EditorAssetLibrary.load_asset(
            "/Game/ReEcho/Audio/Music/The_Iron_Waltz.The_Iron_Waltz"
        )
        if not isinstance(existing_encounter, unreal.SoundWave):
            raise RuntimeError("Existing Music.Encounter asset is missing or not a SoundWave")
        existing_encounter.set_editor_property("looping", True)
        unreal.EditorAssetLibrary.save_loaded_asset(existing_encounter, only_if_is_dirty=False)
        imported_count += 1

unreal.log(
    f"Imported/configured {imported_count} catalog assets "
    f"direct_only={direct_only} decode_only={decode_only} variants_only={variants_only}"
)
