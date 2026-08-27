"""Import the reviewed new tier-2 card icons without colliding with live card IDs."""

from pathlib import Path

import unreal


SOURCE_DIR = Path(unreal.Paths.project_content_dir()) / "SourceArt" / "UI" / "Cards" / "Icon"
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/Cards/Icon"
CARD_IDS = tuple(f"G_2_{index:02d}" for index in range(18, 37))


def import_icon(card_id: str) -> unreal.Texture2D:
    asset_name = f"T_UI_CardIcon_{card_id}"
    source = SOURCE_DIR / f"{asset_name}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing staged card icon: {source}")

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_DIR
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = f"{DESTINATION_DIR}/{asset_name}"
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Failed to import card icon: {asset_path}")
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
        raise RuntimeError(f"Failed to save card icon: {asset_path}")
    return texture


for stable_card_id in CARD_IDS:
    import_icon(stable_card_id)

unreal.log(f"[Plan110Tier2CardIconImport] imported {len(CARD_IDS)} reviewed icons")
