"""Layer the reviewed lock icon inside each authored empty shop card slot."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"
FRAME_PATH = f"{TEXTURE_ROOT}/T_UI_Shop110_LoadoutCardSlot"
LOCK_PATH = f"{TEXTURE_ROOT}/T_UI_Shop110_EmptyCardSlotIcon"
PACK_OFFER_PATH = f"{TEXTURE_ROOT}/T_UI_Shop110_CardPackOfferIcon"


def slate_size(width, height):
    value = unreal.DeprecateSlateVector2D()
    value.set_editor_property("x", width)
    value.set_editor_property("y", height)
    return value


def apply_frame(button, texture):
    style = button.get_editor_property("widget_style")
    for state in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(state)
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("image_size", slate_size(66.0, 66.0))
        brush.set_editor_property(
            "tint_color",
            unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
        )
        style.set_editor_property(state, brush)
    style.set_editor_property("normal_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    style.set_editor_property("pressed_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    button.set_editor_property("widget_style", style)
    button.set_editor_property(
        "background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
frame = unreal.load_asset(FRAME_PATH)
lock = unreal.load_asset(LOCK_PATH)
pack_offer = unreal.load_asset(PACK_OFFER_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing shop widget: {WIDGET_PATH}")
if (
    not isinstance(frame, unreal.Texture2D)
    or not isinstance(lock, unreal.Texture2D)
    or not isinstance(pack_offer, unreal.Texture2D)
):
    raise RuntimeError("Missing reviewed card-slot frame, lock, or populated-pack texture")

infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {
    str(info.widget_name): info.widget
    for info in infos
    if info.widget is not None
}
for index in range(12):
    button = widgets.get(f"DesignerCardSlot{index}")
    art = widgets.get(f"DesignerCardSlotArt{index}")
    if not isinstance(button, unreal.Button) or not isinstance(art, unreal.Image):
        raise RuntimeError(f"Missing authored card slot pair at index {index}")
    if art.get_parent() != button:
        raise RuntimeError(f"DesignerCardSlotArt{index} is no longer the slot content")
    apply_frame(button, frame)
    brush = art.get_editor_property("brush")
    brush.set_editor_property("resource_object", None)
    art.set_editor_property("brush", brush)
    art.set_editor_property("visibility", unreal.SlateVisibility.HIDDEN)
    content_slot = art.get_editor_property("slot")
    if isinstance(content_slot, unreal.ButtonSlot):
        content_slot.set_editor_property(
            "padding", unreal.Margin(0.0, 0.0, 0.0, 0.0)
        )
        content_slot.set_editor_property(
            "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
        )
        content_slot.set_editor_property(
            "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
        )

for index in range(3):
    offer_art = widgets.get(f"DesignerPackOfferIcon{index}")
    if not isinstance(offer_art, unreal.Image):
        raise RuntimeError(f"Missing authored card-pack offer icon at index {index}")
    offer_art.set_brush_from_texture(pack_offer if index == 0 else lock, False)
    offer_art.set_editor_property(
        "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    offer_art.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save shop widget: {WIDGET_PATH}")

unreal.log(
    f"[Plan110EmptySlotAuthor] widget={WIDGET_PATH} cleared_loadout_slots=12 pack_offers=3 "
    f"frame={FRAME_PATH} lock={LOCK_PATH} populated_pack={PACK_OFFER_PATH}"
)
