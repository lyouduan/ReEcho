"""Import and verify the Plan132 two-stage loadout UI textures."""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "LoadoutSelection"
    / "Plan132"
)
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/LoadoutSelection"
ASSET_NAMES = (
    "T_UI_Loadout_Character_J_HEART_Selected",
    "T_UI_Loadout_Character_J_HEART_Unselected",
    "T_UI_Loadout_Character_J_SPADE_Selected",
    "T_UI_Loadout_Character_J_SPADE_Unselected",
    "T_UI_Loadout_Character_J_CLOVER_Selected",
    "T_UI_Loadout_Character_J_CLOVER_Unselected",
    "T_UI_Loadout_Character_J_DIAMOND_Selected",
    "T_UI_Loadout_Character_J_DIAMOND_Unselected",
    "T_UI_Loadout_Weapon_W_J_04_Selected",
    "T_UI_Loadout_Weapon_W_J_04_Unselected",
    "T_UI_Loadout_Weapon_W_J_09_Selected",
    "T_UI_Loadout_Weapon_W_J_09_Unselected",
    "T_UI_Loadout_Weapon_W_J_01_Selected",
    "T_UI_Loadout_Weapon_W_J_01_Unselected",
    "T_UI_Loadout_Weapon_W_J_08_Selected",
    "T_UI_Loadout_Weapon_W_J_08_Unselected",
    "T_UI_Loadout_DescriptionPanel",
    "T_UI_Loadout_SelectionArrow",
)


def import_texture(asset_name):
    source_path = SOURCE_DIR / f"{asset_name}.png"
    if not source_path.is_file():
        raise RuntimeError(f"Missing Plan132 source art: {source_path}")

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_DIR
    task.destination_name = asset_name
    task.filename = str(source_path)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    expected_path = f"{DESTINATION_DIR}/{asset_name}.{asset_name}"
    if expected_path not in list(task.imported_object_paths):
        raise RuntimeError(
            f"Plan132 import path mismatch for {asset_name}: {task.imported_object_paths}"
        )
    texture = unreal.load_asset(expected_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Plan132 asset is not Texture2D: {expected_path}")
    texture.srgb = True
    texture.filter = unreal.TextureFilter.TF_BILINEAR
    texture.mip_gen_settings = unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    texture.compression_settings = unreal.TextureCompressionSettings.TC_EDITOR_ICON
    texture.lod_group = unreal.TextureGroup.TEXTUREGROUP_UI
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Plan132 texture: {expected_path}")
    return texture


def main():
    textures = [import_texture(asset_name) for asset_name in ASSET_NAMES]
    if len(textures) != 18:
        raise RuntimeError(f"Plan132 expected 18 textures, imported {len(textures)}")
    unreal.log("[Plan132LoadoutImport] imported=18 verified=18")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
