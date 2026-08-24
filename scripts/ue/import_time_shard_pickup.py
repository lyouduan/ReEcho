#!/usr/bin/env python3
"""Import the reviewed time-shard pickup source into the runtime texture path."""

from pathlib import Path

import unreal


SOURCE = Path(unreal.Paths.project_content_dir()) / "SourceArt" / "Pickups" / "TimeShard.png"
DESTINATION_PATH = "/Game/ReEcho/Textures/Pickups"
ASSET_NAME = "T_TimeShard"
ASSET_PATH = f"{DESTINATION_PATH}/{ASSET_NAME}"


if not SOURCE.is_file():
    raise RuntimeError(f"Missing reviewed time-shard source: {SOURCE}")

task = unreal.AssetImportTask()
task.automated = True
task.destination_path = DESTINATION_PATH
task.destination_name = ASSET_NAME
task.filename = str(SOURCE)
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

texture = unreal.load_asset(ASSET_PATH)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Imported asset is not Texture2D: {ASSET_PATH}")

texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
texture.set_editor_property("s_rgb", True)
unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
unreal.log(f"Imported reviewed time-shard pickup texture: {ASSET_PATH}")
