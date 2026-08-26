#!/usr/bin/env python3
"""Import the reviewed transparent element-reaction labels."""

from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "CombatHud"
    / "ElementReactions"
)
DESTINATION_PATH = "/Game/ReEcho/Textures/UI/CombatHud/ElementReactions"
SOURCES = {
    "Burn.png": "T_UI_Reaction_Burn",
    "Vaporize.png": "T_UI_Reaction_Vaporize",
    "Growth.png": "T_UI_Reaction_Growth",
    "Conduct.png": "T_UI_Reaction_Conduct",
    "Enhance.png": "T_UI_Reaction_Enhance",
}


def fail(message):
    raise RuntimeError(f"[ElementReactionPopupImport] {message}")


unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)
for source_name, asset_name in SOURCES.items():
    source = SOURCE_ROOT / source_name
    if not source.is_file():
        fail(f"Missing reviewed source: {source}")
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_PATH
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = f"{DESTINATION_PATH}/{asset_name}"
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        fail(f"Imported asset is not Texture2D: {asset_path}")
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    unreal.log(f"[ElementReactionPopupImport] PASS {asset_path}")
