"""Import reviewed Plan110 shop slices as runtime UI textures."""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InventoryShop"
    / "Plan110"
    / "Elements"
)
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"
TEXTURES = {
    "SaveAndLeave.png": "T_UI_Shop110_SaveAndLeave",
    "CriticalChance.png": "T_UI_Shop110_CriticalChance",
    "CriticalEffect.png": "T_UI_Shop110_CriticalEffect",
    "CurrentStatsPanel.png": "T_UI_Shop110_CurrentStatsPanel",
    "BuyButton.png": "T_UI_Shop110_BuyButton",
    "EchoEfficiency.png": "T_UI_Shop110_EchoEfficiency",
    "PageArrow.png": "T_UI_Shop110_PageArrow",
    "AttachmentTooltip.png": "T_UI_Shop110_AttachmentTooltip",
    "OfferCard.png": "T_UI_Shop110_OfferCard",
    "Health.png": "T_UI_Shop110_Health",
    "ShopFrame.png": "T_UI_Shop110_ShopFrame",
    "CurrencyFrame.png": "T_UI_Shop110_CurrencyFrame",
    "RefreshButton.png": "T_UI_Shop110_RefreshButton",
    "WeaponLoadoutSlot.png": "T_UI_Shop110_WeaponLoadoutSlot",
    "PhysicalAttack.png": "T_UI_Shop110_PhysicalAttack",
    "MovementSpeed.png": "T_UI_Shop110_MovementSpeed",
    "LoadoutCardSlot.png": "T_UI_Shop110_LoadoutCardSlot",
    "EmptyCardSlotIcon.png": "T_UI_Shop110_EmptyCardSlotIcon",
    "CardPackOfferIcon.png": "T_UI_Shop110_CardPackOfferIcon",
    "LoadoutTreePanel.png": "T_UI_Shop110_LoadoutTreePanel",
    "ReactionEfficiency.png": "T_UI_Shop110_ReactionEfficiency",
    "ElementalAttack.png": "T_UI_Shop110_ElementalAttack",
    "WeaponPaper.png": "T_UI_Shop110_WeaponPaper",
    "AssemblyPanel.png": "T_UI_Shop110_AssemblyPanel",
    "ShopSurface.png": "T_UI_Shop110_ShopSurface",
    "ShopSectionPaper.png": "T_UI_Shop110_ShopSectionPaper",
}


def import_texture(source_name: str, asset_name: str) -> unreal.Texture2D:
    source = SOURCE_DIR / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing Plan110 source: {source}")

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
        raise RuntimeError(f"Failed to import Plan110 texture: {asset_path}")
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        texture, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Failed to save Plan110 texture: {asset_path}")
    return texture


for source_name, asset_name in TEXTURES.items():
    import_texture(source_name, asset_name)

unreal.log(f"[Plan110ShopImport] imported {len(TEXTURES)} reviewed UI slices")
