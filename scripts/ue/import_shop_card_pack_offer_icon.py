"""Import and author the reviewed populated card-pack offer icon."""

from pathlib import Path

import unreal


SOURCE = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InventoryShop"
    / "Plan110"
    / "Elements"
    / "CardPackOfferIcon.png"
)
DESTINATION = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"
ASSET_NAME = "T_UI_Shop110_CardPackOfferIcon"
WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
LOCK_PATH = f"{DESTINATION}/T_UI_Shop110_EmptyCardSlotIcon"

if not SOURCE.is_file():
    raise RuntimeError(f"Missing reviewed card-pack source: {SOURCE}")

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
    raise RuntimeError(f"Failed to import card-pack texture: {asset_path}")
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
    raise RuntimeError(f"Failed to save card-pack texture: {asset_path}")

blueprint = unreal.load_asset(WIDGET_PATH)
lock_texture = unreal.load_asset(LOCK_PATH)
if blueprint is None or not isinstance(lock_texture, unreal.Texture2D):
    raise RuntimeError(f"Missing shop widget: {WIDGET_PATH}")
toolset = unreal.UMGToolSet.get_default_object()
widgets = {
    str(info.widget_name): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget is not None
}
for index in range(3):
    offer_icon = widgets.get(f"DesignerPackOfferIcon{index}")
    if not isinstance(offer_icon, unreal.Image):
        raise RuntimeError(f"Missing card-pack offer icon {index}")
    offer_icon.set_brush_from_texture(texture if index == 0 else lock_texture, False)
    offer_icon.set_editor_property(
        "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    offer_icon.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save shop widget: {WIDGET_PATH}")
unreal.log(
    f"[ShopCardPackOfferIcon] imported={asset_path} authored_slots=3 widget={WIDGET_PATH}"
)
