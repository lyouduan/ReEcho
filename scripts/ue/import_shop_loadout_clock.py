"""Import the final shop loadout clock as a UI texture."""

from pathlib import Path

import unreal


SOURCE = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "Shop"
    / "Loadout"
    / "T_UI_Shop_LoadoutClock.jpg"
)
DESTINATION_PATH = "/Game/ReEcho/Textures/UI/Shop/Loadout"
ASSET_NAME = "T_UI_Shop_LoadoutClock"
EXPECTED_PATH = f"{DESTINATION_PATH}/{ASSET_NAME}"

if not SOURCE.is_file():
    raise RuntimeError(f"Missing shop loadout clock source: {SOURCE}")

task = unreal.AssetImportTask()
task.automated = True
task.destination_path = DESTINATION_PATH
task.destination_name = ASSET_NAME
task.filename = str(SOURCE)
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

texture = unreal.load_asset(EXPECTED_PATH)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Imported asset is not Texture2D: {EXPECTED_PATH}")

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
unreal.log(f"Imported shop loadout clock: {EXPECTED_PATH}")
