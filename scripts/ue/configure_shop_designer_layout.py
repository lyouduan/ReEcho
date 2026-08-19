"""Move the shop loadout's spatial authority into the Widget Blueprint.

This is a one-time/idempotent migration.  Existing Designer* controls are never
repositioned so an artist or designer can safely tune them in the UMG Designer.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
ATTACHMENT_TEXTURE = (
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/"
    "T_UI_Shop_AttachmentSlot"
)
CARD_SLOT_TEXTURE = (
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/"
    "T_UI_Shop_HoverCardSlot"
)
CLOCK_TEXTURE = "/Game/ReEcho/Textures/UI/Shop/Loadout/T_UI_Shop_LoadoutClock"
WEAPON_TEXTURE = (
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/"
    "T_UI_Shop_Weapon"
)


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def widget_map(toolset, blueprint):
    result = toolset.call_method("GetWidgets", args=(blueprint,))
    return {str(info.widget_name): info.widget for info in result.widgets if info.widget}


def mark_variable(toolset, blueprint, widget):
    toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method(
        "AddWidget",
        args=(blueprint, widget_class, name, parent, child_index),
    )
    if info.widget is None:
        raise RuntimeError(f"Unable to add widget {name}")
    mark_variable(toolset, blueprint, info.widget)
    return info.widget


def set_canvas_layout(widget, x, y, width, height, z_order):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct CanvasPanel child")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, y, width, height),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0),
                maximum=unreal.Vector2D(0.0, 0.0),
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("z_order", z_order)


def set_button_content_fill(button):
    content = button.get_child_at(0)
    if content is None:
        return
    slot = content.get_editor_property("slot")
    slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
    )
    slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
    )


def ensure_image(toolset, blueprint, widgets, parent, name, texture, rect, z_order):
    image = widgets.get(name)
    if image is None:
        image = add_widget(toolset, blueprint, unreal.Image, name, parent)
        set_canvas_layout(image, *rect, z_order)
    image.set_brush_from_texture(texture, False)
    image.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    return image


def ensure_slot_button(
    toolset, blueprint, widgets, parent, button_name, art_name, texture, rect, z_order
):
    button = widgets.get(button_name)
    if button is None:
        button = add_widget(toolset, blueprint, unreal.Button, button_name, parent)
        set_canvas_layout(button, *rect, z_order)
    button.set_editor_property(
        "background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0)
    )
    button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)

    widgets = widget_map(toolset, blueprint)
    art = widgets.get(art_name)
    if art is None:
        art = add_widget(toolset, blueprint, unreal.Image, art_name, button)
    art.set_brush_from_texture(texture, False)
    art.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_button_content_fill(button)
    return button, art


blueprint = load_required(ASSET_PATH)
toolset = unreal.UMGToolSet.get_default_object()
widgets = widget_map(toolset, blueprint)
loadout_overlay = widgets["Overlay_0"]

designer_canvas = widgets.get("DesignerLoadoutCanvas")
if designer_canvas is None:
    designer_canvas = add_widget(
        toolset,
        blueprint,
        unreal.CanvasPanel,
        "DesignerLoadoutCanvas",
        loadout_overlay,
    )
    overlay_slot = designer_canvas.get_editor_property("slot")
    overlay_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    overlay_slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
    )
    overlay_slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
    )
designer_canvas.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)

widgets = widget_map(toolset, blueprint)
ensure_image(
    toolset,
    blueprint,
    widgets,
    designer_canvas,
    "DesignerWeaponPanel",
    load_required(WEAPON_TEXTURE),
    (110.0, 150.0, 308.0, 531.0),
    1,
)
widgets = widget_map(toolset, blueprint)
ensure_image(
    toolset,
    blueprint,
    widgets,
    designer_canvas,
    "DesignerShopClock",
    load_required(CLOCK_TEXTURE),
    (415.0, 400.0, 162.0, 374.0),
    2,
)

attachment_texture = load_required(ATTACHMENT_TEXTURE)
for index, x in enumerate((28.0, 140.0, 252.0)):
    widgets = widget_map(toolset, blueprint)
    ensure_slot_button(
        toolset,
        blueprint,
        widgets,
        designer_canvas,
        f"DesignerAttachmentSlot{index}",
        f"DesignerAttachmentSlotArt{index}",
        attachment_texture,
        (x, 587.0, 93.0, 93.0),
        10,
    )

# These are the previous runtime positions expressed in Overlay_0-local space.
# They are initial defaults only; rerunning this script does not overwrite them.
card_positions = (
    (580.0, 33.0),
    (580.0, 113.0),
    (580.0, 193.0),
    (863.0, 73.0),
    (863.0, 153.0),
    (863.0, 233.0),
    (617.0, 310.0),
    (617.0, 390.0),
    (863.0, 320.0),
    (863.0, 400.0),
    (727.0, 420.0),
    (727.0, 500.0),
)
card_texture = load_required(CARD_SLOT_TEXTURE)
for index, (x, y) in enumerate(card_positions):
    widgets = widget_map(toolset, blueprint)
    ensure_slot_button(
        toolset,
        blueprint,
        widgets,
        designer_canvas,
        f"DesignerCardSlot{index}",
        f"DesignerCardSlotArt{index}",
        card_texture,
        (x, y, 60.0, 60.0),
        11,
    )

# Retire the Overlay-authored copies.  Their replacements above are direct
# Canvas children, which exposes Position X/Y and Size X/Y in the UMG Designer.
widgets = widget_map(toolset, blueprint)
for legacy_name in (
    "ArtLoadoutWeapon",
    "ArtShopClock",
    "ArtAttachmentSlot0",
    "ArtAttachmentSlot1",
    "ArtAttachmentSlot2",
    "ArtAttachmentSlot3",
):
    legacy = widgets.get(legacy_name)
    if legacy is not None:
        legacy.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save")

final_widgets = widget_map(toolset, blueprint)
required = {
    "DesignerLoadoutCanvas",
    "DesignerWeaponPanel",
    "DesignerShopClock",
    *(f"DesignerAttachmentSlot{index}" for index in range(3)),
    *(f"DesignerAttachmentSlotArt{index}" for index in range(3)),
    *(f"DesignerCardSlot{index}" for index in range(12)),
    *(f"DesignerCardSlotArt{index}" for index in range(12)),
}
missing = sorted(required.difference(final_widgets))
if missing:
    raise RuntimeError(f"Shop designer layout is missing widgets: {missing}")
for name in required.difference({"DesignerLoadoutCanvas"}):
    widget = final_widgets[name]
    if name.endswith("Art0") or name.endswith("Art1") or name.endswith("Art2"):
        continue
    if name.startswith("DesignerCardSlotArt"):
        continue
    slot = widget.get_editor_property("slot")
    if name.startswith("Designer") and "Art" not in name and name != "DesignerLoadoutCanvas":
        if not isinstance(slot, unreal.CanvasPanelSlot):
            raise RuntimeError(f"{name} is not directly editable in DesignerLoadoutCanvas")

unreal.log(
    "[Plan45Shop] Blueprint owns weapon panel, clock, 3 attachment slots and 12 card slots"
)
