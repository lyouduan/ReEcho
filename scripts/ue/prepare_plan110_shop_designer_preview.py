"""Populate the existing Plan110 WBP with editable Designer-only samples.

Runtime code continues to replace text, brushes and state. This script does
not create commands, bind delegates, or alter shop data/gameplay logic.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
CARD_SAMPLE = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/T_UI_Shop_CardIcon"
WEAPON_SAMPLE = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/T_UI_Shop_Weapon"
ATTACHMENT_SLOT_SAMPLE = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110/T_UI_Shop110_WeaponLoadoutSlot"
PART_SAMPLES = (
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_FLAME",
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_THUNDER",
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_FOREST",
)


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Missing Designer preview asset: {path}")
    return asset


blueprint = load_required(ASSET_PATH)
toolset = unreal.UMGToolSet.get_default_object()
infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {str(info.widget_name): info.widget for info in infos if info.widget}


def required(name):
    widget = widgets.get(name)
    if widget is None:
        raise RuntimeError(f"Missing authored Designer widget: {name}")
    return widget


def ensure_pack_description_overlay(index, pack_card, target):
    """Give the pack label the same editable Overlay contract as rune names."""
    overlay_name = f"DesignerPackOfferDescriptionOverlay{index}"
    existing = widgets.get(overlay_name)
    if isinstance(existing, unreal.Overlay):
        overlay = existing
    else:
        source_slot = target.get_editor_property("slot")
        if not isinstance(source_slot, unreal.CanvasPanelSlot):
            raise RuntimeError(
                f"{target.get_name()} must start as a direct pack-card Canvas child"
            )
        layout_data = source_slot.get_editor_property("layout_data")
        auto_size = source_slot.get_editor_property("auto_size")
        z_order = source_slot.get_editor_property("z_order")
        info = toolset.call_method(
            "AddWidget",
            args=(blueprint, unreal.Overlay, overlay_name, pack_card, -1),
        )
        overlay = info.widget
        if not isinstance(overlay, unreal.Overlay):
            raise RuntimeError(f"Unable to create {overlay_name}")
        widgets[overlay_name] = overlay
        overlay_slot = overlay.get_editor_property("slot")
        if not isinstance(overlay_slot, unreal.CanvasPanelSlot):
            raise RuntimeError(f"{overlay_name} did not receive a CanvasPanelSlot")
        overlay_slot.set_editor_property("layout_data", layout_data)
        overlay_slot.set_editor_property("auto_size", auto_size)
        overlay_slot.set_editor_property("z_order", z_order)

    if target.get_parent() is not overlay:
        old_parent = target.get_parent()
        if old_parent is None or not old_parent.remove_child(target):
            raise RuntimeError(f"Could not detach {target.get_name()} from its old parent")
        overlay.add_child_to_overlay(target)
    return overlay


def copy_text_appearance_and_slot_alignment(source, target):
    """Match the lower pack label to the authored upper rune-name label.

    Geometry remains owned by each Designer overlay. Only text appearance and
    the alignment/padding contract relative to its current parent are copied.
    """
    for property_name in (
        "font",
        "color_and_opacity",
        "shadow_offset",
        "shadow_color_and_opacity",
        "justification",
        "auto_wrap_text",
        "wrapping_policy",
    ):
        try:
            target.set_editor_property(
                property_name, source.get_editor_property(property_name)
            )
        except Exception:
            unreal.log_warning(
                f"[Plan110DesignerPreview] skipped unsupported text property {property_name}"
            )

    source_slot = source.get_editor_property("slot")
    target_slot = target.get_editor_property("slot")
    if not isinstance(source_slot, unreal.OverlaySlot) or not isinstance(
        target_slot, unreal.OverlaySlot
    ):
        raise RuntimeError("Offer name labels must both be children of editable overlays")
    target_slot.set_horizontal_alignment(source_slot.horizontal_alignment)
    target_slot.set_vertical_alignment(source_slot.vertical_alignment)
    target_slot.set_padding(source_slot.padding)


required("DesignerPageCounter").set_text("1/2")

part_names = ("焚绝之晶", "鸣雷之晶", "森林之晶")
pack_names = ("一级卡组", "二级卡组", "三级卡组")
part_textures = tuple(load_required(path) for path in PART_SAMPLES)
card_texture = load_required(CARD_SAMPLE)
weapon_texture = load_required(WEAPON_SAMPLE)
attachment_slot_texture = load_required(ATTACHMENT_SLOT_SAMPLE)

currency_text = required("DesignerCurrencyText")
currency_text.set_text("时间碎片：99999")
currency_text.set_editor_property("auto_wrap_text", False)

weapon_art = required("DesignerWeaponInteractionArt")
weapon_art.set_brush_from_texture(weapon_texture, False)
weapon_art.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)

for index in range(3):
    attachment_button = required(f"DesignerAttachmentSlot{index}")
    attachment_art = required(f"DesignerAttachmentSlotArt{index}")
    # Runtime keeps this frame as the Button background and layers the equipped
    # rune in AttachmentArt. The Designer preview keeps the same empty-socket look.
    style = attachment_button.get_editor_property("widget_style")
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("resource_object", attachment_slot_texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property(
            "tint_color",
            unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
        )
        style.set_editor_property(brush_name, brush)
    style.set_editor_property("normal_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    style.set_editor_property("pressed_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    attachment_button.set_editor_property("widget_style", style)
    attachment_art.set_brush_from_texture(attachment_slot_texture, False)
    attachment_art.set_editor_property("visibility", unreal.SlateVisibility.HIDDEN)

for index in range(3):
    part_card = required(f"DesignerPartOfferCard{index}")
    pack_card = required(f"DesignerPackOfferCard{index}")
    part_card.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    pack_card.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

    part_icon = required(f"DesignerPartOfferIcon{index}")
    part_icon.set_brush_from_texture(part_textures[index], False)
    part_icon.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    part_description = required(f"DesignerPartOfferDescription{index}")
    part_description.set_text(part_names[index])
    required(f"DesignerPartOfferCost{index}").set_text(str((index + 1) * 10))

    pack_icon = required(f"DesignerPackOfferIcon{index}")
    pack_icon.set_brush_from_texture(card_texture, False)
    pack_icon.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    pack_description = required(f"DesignerPackOfferDescription{index}")
    ensure_pack_description_overlay(index, pack_card, pack_description)
    pack_description.set_text(pack_names[index])
    copy_text_appearance_and_slot_alignment(part_description, pack_description)
    required(f"DesignerPackOfferCost{index}").set_text(str((index + 1) * 10))

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save")

unreal.log("[Plan110DesignerPreview] populated editable WYSIWYG samples without logic changes")
