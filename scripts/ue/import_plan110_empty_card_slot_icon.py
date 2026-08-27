"""Import only the reviewed Plan110 empty-card-slot lock icon."""

from pathlib import Path

import unreal


SOURCE = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InventoryShop"
    / "Plan110"
    / "Elements"
    / "EmptyCardSlotIcon.png"
)
DESTINATION = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"
ASSET_NAME = "T_UI_Shop110_EmptyCardSlotIcon"

if not SOURCE.is_file():
    raise RuntimeError(f"Missing reviewed empty-slot source: {SOURCE}")

task = unreal.AssetImportTask()
task.automated = True
task.destination_path = DESTINATION
task.destination_name = ASSET_NAME
task.filename = str(SOURCE)
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

asset_path = f"{DESTINATION}/{ASSET_NAME}"
texture = unreal.load_asset(asset_path)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Failed to import empty-slot texture: {asset_path}")

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
    raise RuntimeError(f"Failed to save empty-slot texture: {asset_path}")

unreal.log(f"[Plan110EmptySlotImport] imported={asset_path} source={SOURCE}")
