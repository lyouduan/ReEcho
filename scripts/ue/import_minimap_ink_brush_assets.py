#!/usr/bin/env python3
"""Import the reviewed minimap ink-brush tip and grain textures."""

from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "CombatHud"
    / "MinimapInkBrush"
)
DESTINATION_PATH = "/Game/ReEcho/Textures/UI/CombatHud/MinimapInkBrush"
SOURCES = {
    "BrushTip_512.png": "T_UI_MinimapInkBrushTip",
    "Grain_900.png": "T_UI_MinimapInkGrain",
}


def fail(message):
    raise RuntimeError(f"[MinimapInkBrushImport] {message}")


unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)
tasks = []
for source_name, asset_name in SOURCES.items():
    source = SOURCE_ROOT / source_name
    if not source.is_file():
        fail(f"Missing reviewed source: {source}")
    asset_path = f"{DESTINATION_PATH}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        continue
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_PATH
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = False
    task.save = True
    tasks.append(task)

if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for asset_name in SOURCES.values():
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
    unreal.log(f"[MinimapInkBrushImport] PASS {asset_path}")
