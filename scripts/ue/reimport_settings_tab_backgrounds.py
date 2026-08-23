"""Reimport the six text-free Settings tab backgrounds through Unreal Editor."""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InteractionPlaceholder"
    / "Elements"
    / "Settings"
)
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings"
TAB_TEXTURES = {
    "画面激活.png": "T_UI_Settings_GraphicsActive",
    "画面置灰.png": "T_UI_Settings_GraphicsInactive",
    "声音激活.png": "T_UI_Settings_AudioActive",
    "声音置灰.png": "T_UI_Settings_AudioInactive",
    "键位激活.png": "T_UI_Settings_ControlsActive",
    "键位置灰.png": "T_UI_Settings_ControlsInactive",
}


for source_name, asset_name in TAB_TEXTURES.items():
    source_path = SOURCE_DIR / source_name
    asset_path = f"{DESTINATION_DIR}/{asset_name}"
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Settings tab texture is missing or has the wrong type: {asset_path}")
    if not source_path.is_file():
        raise RuntimeError(f"Settings tab source is missing: {source_path}")

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_DIR
    task.destination_name = asset_name
    task.filename = str(source_path)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(asset_path)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Settings tab texture: {asset_path}")
    unreal.log(f"[SettingsTabs] Reimported {asset_path} from {source_path}")

unreal.log(f"[SettingsTabs] Reimported {len(TAB_TEXTURES)} text-free tab backgrounds")
