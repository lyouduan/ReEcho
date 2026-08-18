"""Import weapon-part UI icons using stable PartId asset names.

Source PNGs live under SourceArt and are named ``T_UI_Part_<PartId>.png``.
The adjacent manifest records the workbook-name mapping and the handful of
semantic fallbacks required by source rows that do not have a delivered icon.
"""

from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "WeaponParts"
    / "Icons"
)
DESTINATION_ROOT = "/Game/ReEcho/Textures/UI/WeaponParts/Icons"
REIMPORT_EXISTING = "-ReimportExisting" in unreal.SystemLibrary.get_command_line()


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


def import_texture(source: Path) -> str:
    asset_name = source.stem
    expected_path = f"{DESTINATION_ROOT}/{asset_name}"
    existing_texture = unreal.load_asset(expected_path)
    if existing_texture is not None and not REIMPORT_EXISTING:
        if not isinstance(existing_texture, unreal.Texture2D):
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


sources = sorted(SOURCE_ROOT.glob("T_UI_Part_*.png"))
if not sources:
    raise RuntimeError(f"No weapon-part icon sources found under {SOURCE_ROOT}")

imported = [import_texture(source) for source in sources]
unreal.log(f"Imported {len(imported)} weapon-part UI icons: {imported}")
