"""Verify the staged new tier-2 card icons and their runtime Texture2D settings."""

from pathlib import Path

import unreal


SOURCE_DIR = Path(unreal.Paths.project_content_dir()) / "SourceArt" / "UI" / "Cards" / "Icon"
RUNTIME_DIR = "/Game/ReEcho/Textures/UI/Cards/Icon"
CARD_IDS = tuple(f"G_2_{index:02d}" for index in range(18, 37))


for card_id in CARD_IDS:
    asset_name = f"T_UI_CardIcon_{card_id}"
    source = SOURCE_DIR / f"{asset_name}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing source icon: {source}")

    asset_path = f"{RUNTIME_DIR}/{asset_name}"
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Missing runtime icon: {asset_path}")
    if (texture.blueprint_get_size_x(), texture.blueprint_get_size_y()) != (512, 512):
        raise RuntimeError(f"Unexpected card icon size: {asset_path}")
    if texture.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        raise RuntimeError(f"Card icon is not in TEXTUREGROUP_UI: {asset_path}")
    if (
        texture.get_editor_property("mip_gen_settings")
        != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    ):
        raise RuntimeError(f"Card icon still has mipmaps: {asset_path}")
    if (
        texture.get_editor_property("compression_settings")
        != unreal.TextureCompressionSettings.TC_EDITOR_ICON
    ):
        raise RuntimeError(f"Card icon has the wrong compression: {asset_path}")

unreal.log(f"[Plan110Tier2CardIconAudit] PASS {len(CARD_IDS)} source/runtime icons")
