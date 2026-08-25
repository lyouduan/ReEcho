"""Import the reviewed Plan93 combat-HUD slices through Unreal Editor.

The 1920x1080 reference image is intentionally not imported. Run with
UnrealEditor-Cmd and ``-ExecutePythonScript``. Existing textures are preserved
unless ``-Plan93ReimportExisting`` is present on the Unreal command line.
"""

from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "CombatHud"
    / "Plan93"
    / "Elements"
)
DESTINATION_ROOT = "/Game/ReEcho/Textures/UI/CombatHud"
REIMPORT_EXISTING = "-Plan93ReimportExisting" in unreal.SystemLibrary.get_command_line()

IMPORTS = {
    "爱心.png": "T_UI_CombatHud_HealthIcon",
    "回响显示框.png": "T_UI_CombatHud_EchoFrame",
    "时间底板.png": "T_UI_CombatHud_ClockFrame",
    "时间碎片.png": "T_UI_CombatHud_TimeShardIcon",
    "时间指针.png": "T_UI_CombatHud_ClockNeedle",
    "血条.png": "T_UI_CombatHud_HealthFill",
    "血条底板.png": "T_UI_CombatHud_HealthFrame",
}


def configure_ui_texture(texture):
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def import_texture(source_name, asset_name):
    source = SOURCE_ROOT / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing reviewed Plan93 UI source: {source}")

    expected_path = f"{DESTINATION_ROOT}/{asset_name}"
    existing = unreal.load_asset(expected_path)
    if existing is not None and not REIMPORT_EXISTING:
        if not isinstance(existing, unreal.Texture2D):
            raise RuntimeError(f"Existing asset is not Texture2D: {expected_path}")
        return expected_path

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_ROOT
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = REIMPORT_EXISTING
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(expected_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported asset is not Texture2D: {expected_path}")
    configure_ui_texture(texture)
    return expected_path


resolved = [import_texture(source, name) for source, name in IMPORTS.items()]
unreal.log(f"[Plan93Import] resolved {len(resolved)} combat HUD textures: {resolved}")
