"""Reimport only the Plan110 loadout-card slot texture.

This intentionally leaves WBP geometry and all shop runtime logic untouched.
"""

from pathlib import Path

import unreal


SOURCE = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InventoryShop"
    / "Plan110"
    / "Elements"
    / "LoadoutCardSlot.png"
)
DESTINATION = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"
ASSET_NAME = "T_UI_Shop110_LoadoutCardSlot"
ASSET_PATH = f"{DESTINATION}/{ASSET_NAME}"

if not SOURCE.is_file():
    raise RuntimeError(f"Missing Plan110 card-slot source: {SOURCE}")

task = unreal.AssetImportTask()
task.automated = True
task.destination_path = DESTINATION
task.destination_name = ASSET_NAME
task.filename = str(SOURCE)
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

texture = unreal.load_asset(ASSET_PATH)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Failed to import Plan110 card-slot texture: {ASSET_PATH}")

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
    raise RuntimeError(f"Failed to save Plan110 card-slot texture: {ASSET_PATH}")

unreal.log(
    "[Plan110CardSlotReimport] reimported "
    f"{ASSET_PATH} from {SOURCE} ({texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()})"
)
