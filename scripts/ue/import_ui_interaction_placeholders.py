"""Import the reviewed Plan 45 UI placeholder slices through Unreal Editor.

Run with UnrealEditor-Cmd and ``-ExecutePythonScript``. Reference screenshots and
pending-license fonts are intentionally absent from this mapping.
"""

from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InteractionPlaceholder"
    / "Elements"
)
DESTINATION_ROOT = "/Game/ReEcho/Textures/UI/InteractionPlaceholder"

# Import only slices that have a current WBP consumer. Extend this mapping one
# reviewed page at a time; do not bulk-import delivery references.
IMPORTS = {
    "StartMenu": {
        "bg.png": "T_UI_Start_Background",
        "主体文字.png": "T_UI_Start_TitleLogo",
        "设置.png": "T_UI_Start_SettingsIcon",
    },
}


def configure_ui_texture(texture: unreal.Texture2D) -> None:
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


def import_texture(category: str, source_name: str, asset_name: str) -> str:
    source = SOURCE_ROOT / category / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing reviewed UI source: {source}")

    destination_path = f"{DESTINATION_ROOT}/{category}"
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = destination_path
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    expected_path = f"{destination_path}/{asset_name}"
    texture = unreal.load_asset(expected_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported asset is not Texture2D: {expected_path}")

    configure_ui_texture(texture)
    return expected_path


imported = []
for page_category, page_imports in IMPORTS.items():
    for filename, runtime_name in page_imports.items():
        imported.append(import_texture(page_category, filename, runtime_name))

unreal.log(f"Plan45 imported {len(imported)} reviewed UI textures: {imported}")
