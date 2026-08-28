"""Author the formal Plan110 shop composition in the existing shop WBP.

The authored controls own geometry and sample content. C++ only replaces data,
enabled state, textures and click indices at runtime.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Missing Plan110 asset: {path}")
    return asset


def widget_infos(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(toolset, blueprint):
    return {
        str(info.widget_name): info.widget
        for info in widget_infos(toolset, blueprint)
        if info.widget
    }


def mark_variable(toolset, blueprint, widget):
    infos = {str(info.widget_name): info for info in widget_infos(toolset, blueprint)}
    info = infos.get(widget.get_name())
    if info is not None and not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, child_index)
    )
    if info.widget is None:
        raise RuntimeError(f"Unable to add Plan110 widget: {name}")
    mark_variable(toolset, blueprint, info.widget)
    return info.widget


def set_canvas_layout(widget, x, y, width, height, z_order=0):
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
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


def fill_panel_slot(widget):
    slot = widget.get_editor_property("slot")
    if slot is None:
        return
    slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
    )
    slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
    )


def set_image(image, texture, hit_test=False):
    image.set_brush_from_texture(texture, False)
    image.set_editor_property(
        "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    image.set_editor_property(
        "visibility",
        unreal.SlateVisibility.VISIBLE
        if hit_test
        else unreal.SlateVisibility.HIT_TEST_INVISIBLE,
    )


def configure_text(text, value, size, color=None, justification=None):
    text.set_text(value)
    text.set_editor_property("auto_wrap_text", True)
    text.set_editor_property(
        "color_and_opacity",
        unreal.SlateColor(
            color or unreal.LinearColor(0.12, 0.07, 0.035, 1.0)
        ),
    )
    font = text.get_editor_property("font")
    font.size = size
    text.set_editor_property("font", font)
    if justification is not None:
        text.set_editor_property("justification", justification)
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)


def transparent_button(button):
    button.set_editor_property(
        "background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0)
    )
    button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)


def add_image(toolset, blueprint, parent, name, texture, rect, z_order=0):
    image = add_widget(toolset, blueprint, unreal.Image, name, parent)
    set_image(image, texture)
    set_canvas_layout(image, *rect, z_order)
    return image


def add_text(
    toolset,
    blueprint,
    parent,
    name,
    value,
    rect,
    size,
    z_order=0,
    color=None,
    justification=None,
):
    text = add_widget(toolset, blueprint, unreal.TextBlock, name, parent)
    configure_text(text, value, size, color, justification)
    set_canvas_layout(text, *rect, z_order)
    return text


def add_button_visual(toolset, blueprint, button, stem, texture, label_text=""):
    overlay = add_widget(
        toolset, blueprint, unreal.Overlay, f"{stem}VisualRoot", button
    )
    fill_panel_slot(overlay)
    art = add_widget(toolset, blueprint, unreal.Image, f"{stem}Art", overlay)
    set_image(art, texture)
    fill_panel_slot(art)
    label = add_widget(toolset, blueprint, unreal.TextBlock, f"{stem}Label", overlay)
    configure_text(
        label,
        label_text,
        16,
        unreal.LinearColor(0.12, 0.07, 0.035, 1.0),
        unreal.TextJustify.CENTER,
    )
    fill_panel_slot(label)
    return art, label


textures = {
    name: load_required(f"{TEXTURE_ROOT}/T_UI_Shop110_{name}")
    for name in (
        "SaveAndLeave",
        "BuyButton",
        "PageArrow",
        "OfferCard",
        "ShopFrame",
        "CurrencyFrame",
        "RefreshButton",
        "WeaponLoadoutSlot",
        "LoadoutCardSlot",
        "LoadoutTreePanel",
        "WeaponPaper",
        "AssemblyPanel",
        "ShopSurface",
        "ShopSectionPaper",
    )
}

toolset = unreal.UMGToolSet.get_default_object()
blueprint = load_required(ASSET_PATH)
widgets = widget_map(toolset, blueprint)
root = widgets.get("RootPanel")
if not isinstance(root, unreal.CanvasPanel):
    raise RuntimeError("Plan110 requires RootPanel CanvasPanel")

# Rebuild only the Plan110-owned surface. Existing data bindings remain intact.
legacy_formal = widgets.get("DesignerShopPresentationCanvas")
if legacy_formal is not None:
    if not toolset.call_method("RemoveWidget", args=(blueprint, legacy_formal)):
        raise RuntimeError("Unable to replace DesignerShopPresentationCanvas")
widgets = widget_map(toolset, blueprint)

formal = add_widget(
    toolset,
    blueprint,
    unreal.CanvasPanel,
    "DesignerShopPresentationCanvas",
    root,
)
formal.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
set_canvas_layout(formal, 0.0, 0.0, 1920.0, 1080.0, 5)

# Left time-shop frame and two offer shelves.
add_image(toolset, blueprint, formal, "ArtFormalShopFrame", textures["ShopFrame"], (151.0, 2.0, 701.0, 1039.0), 0)
add_image(toolset, blueprint, formal, "ArtFormalShopSurface", textures["ShopSurface"], (196.0, 139.0, 610.0, 854.0), 1)
add_image(toolset, blueprint, formal, "ArtFormalPartPaper", textures["ShopSectionPaper"], (240.0, 262.0, 530.0, 320.0), 2)
add_image(toolset, blueprint, formal, "ArtFormalPackPaper", textures["ShopSectionPaper"], (240.0, 636.0, 530.0, 320.0), 2)
add_text(toolset, blueprint, formal, "DesignerPartSectionTitle", "配件", (241.0, 225.0, 130.0, 38.0), 25, 4)
add_text(toolset, blueprint, formal, "DesignerPackSectionTitle", "卡牌", (241.0, 599.0, 130.0, 38.0), 25, 4)
add_image(toolset, blueprint, formal, "ArtFormalCurrencyFrame", textures["CurrencyFrame"], (336.0, 181.0, 260.0, 38.0), 3)
currency_text = add_text(
    toolset,
    blueprint,
    formal,
    "DesignerCurrencyText",
    "时间碎片：162",
    (366.0, 184.0, 210.0, 30.0),
    20,
    4,
    unreal.LinearColor(0.95, 0.90, 0.78, 1.0),
    unreal.TextJustify.CENTER,
)
currency_text.set_editor_property("auto_wrap_text", False)

refresh = add_widget(
    toolset, blueprint, unreal.Button, "ShopRefreshButton", formal
)
transparent_button(refresh)
set_canvas_layout(refresh, 611.0, 181.0, 162.0, 41.0, 8)
refresh_art = add_widget(
    toolset, blueprint, unreal.Image, "DesignerRefreshArt", refresh
)
set_image(refresh_art, textures["RefreshButton"])
fill_panel_slot(refresh_art)
def add_offer_card(index, is_pack, x, y):
    prefix = "DesignerPackOffer" if is_pack else "DesignerPartOffer"
    card = add_widget(
        toolset, blueprint, unreal.CanvasPanel, f"{prefix}Card{index}", formal
    )
    card.set_editor_property(
        "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
    )
    set_canvas_layout(card, x, y, 161.0, 296.0, 6)
    add_image(
        toolset,
        blueprint,
        card,
        f"{prefix}Base{index}",
        textures["OfferCard"],
        (0.0, 0.0, 161.0, 296.0),
        0,
    )
    icon_scale = add_widget(
        toolset, blueprint, unreal.ScaleBox, f"{prefix}IconScale{index}", card
    )
    icon_scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    icon_scale.set_editor_property(
        "stretch_direction", unreal.StretchDirection.BOTH
    )
    set_canvas_layout(icon_scale, 27.0, 13.0, 108.0, 108.0, 2)
    icon = add_widget(
        toolset, blueprint, unreal.Image, f"{prefix}Icon{index}", icon_scale
    )
    if is_pack:
        icon.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    else:
        set_image(icon, textures["WeaponLoadoutSlot"])
    icon_slot = icon.get_editor_property("slot")
    if isinstance(icon_slot, unreal.ScaleBoxSlot):
        icon_slot.set_editor_property(
            "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER
        )
        icon_slot.set_editor_property(
            "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER
        )
    main_text = add_text(
        toolset,
        blueprint,
        card,
        f"{prefix}Description{index}",
        "三级卡组\n剩余候选 3" if is_pack else "配件名称\n效果说明",
        (8.0, 184.0, 145.0, 52.0),
        12 if is_pack else 10,
        3,
        justification=unreal.TextJustify.CENTER,
    )
    if is_pack:
        configure_text(
            main_text,
            "三级卡组\n剩余候选 3",
            16,
            unreal.LinearColor(0.12, 0.07, 0.035, 1.0),
            unreal.TextJustify.CENTER,
        )
    add_text(
        toolset,
        blueprint,
        card,
        f"{prefix}Cost{index}",
        "10",
        (104.0, 147.0, 48.0, 25.0),
        17,
        4,
        justification=unreal.TextJustify.RIGHT,
    )
    button = add_widget(
        toolset,
        blueprint,
        unreal.ReEchoIndexedButton,
        f"{prefix}Buy{index}",
        card,
    )
    transparent_button(button)
    set_canvas_layout(button, 12.0, 240.0, 134.0, 46.0, 8)
    add_button_visual(
        toolset,
        blueprint,
        button,
        f"{prefix}Buy{index}",
        textures["BuyButton"],
    )


for index, x in enumerate((257.0, 427.0, 596.0)):
    add_offer_card(index, False, x, 276.0)
    add_offer_card(index, True, x, 650.0)

# Middle assembly-room art and right loadout tree.
add_image(toolset, blueprint, formal, "ArtFormalAssemblyPanel", textures["AssemblyPanel"], (841.0, 0.0, 380.0, 966.0), 1)
add_image(toolset, blueprint, formal, "ArtFormalLoadoutTree", textures["LoadoutTreePanel"], (1221.0, 0.0, 585.0, 1039.0), 1)
add_text(toolset, blueprint, formal, "DesignerPageCounter", "1/2", (1715.0, 680.0, 66.0, 42.0), 28, 4, unreal.LinearColor(0.95, 0.90, 0.78, 1.0), unreal.TextJustify.CENTER)
add_image(toolset, blueprint, formal, "ArtFormalPageLeft", textures["PageArrow"], (1712.0, 722.0, 18.0, 34.0), 4)
page_right = add_image(toolset, blueprint, formal, "ArtFormalPageRight", textures["PageArrow"], (1762.0, 722.0, 18.0, 34.0), 4)
page_right.set_render_transform_angle(180.0)
add_image(toolset, blueprint, formal, "ArtFormalSaveAndLeave", textures["SaveAndLeave"], (1263.0, 916.0, 410.0, 104.0), 4)

# Move the editable equipment controls to the 1920x1080 design surface.
widgets = widget_map(toolset, blueprint)
designer = widgets.get("DesignerLoadoutCanvas")
if not isinstance(designer, unreal.CanvasPanel):
    raise RuntimeError("Missing DesignerLoadoutCanvas")
if designer.get_parent() != root:
    if not toolset.call_method("MoveWidget", args=(blueprint, designer, root, -1)):
        raise RuntimeError("Unable to move DesignerLoadoutCanvas to RootPanel")
designer.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
set_canvas_layout(designer, 0.0, 0.0, 1920.0, 1080.0, 10)
widgets = widget_map(toolset, blueprint)

weapon_panel = widgets["DesignerWeaponPanel"]
set_image(weapon_panel, textures["WeaponPaper"])
set_canvas_layout(weapon_panel, 909.0, 327.0, 251.0, 424.0, 2)

equipped_button = widgets.get("DesignerWeaponInteractionButton")
if equipped_button is None:
    equipped_button = add_widget(
        toolset,
        blueprint,
        unreal.Button,
        "DesignerWeaponInteractionButton",
        designer,
    )
    transparent_button(equipped_button)
    scale = add_widget(
        toolset,
        blueprint,
        unreal.ScaleBox,
        "DesignerWeaponInteractionScale",
        equipped_button,
    )
    scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    scale.set_editor_property(
        "stretch_direction", unreal.StretchDirection.BOTH
    )
    fill_panel_slot(scale)
    weapon_art = add_widget(
        toolset,
        blueprint,
        unreal.Image,
        "DesignerWeaponInteractionArt",
        scale,
    )
    weapon_art.set_editor_property(
        "visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE
    )
else:
    weapon_art = widgets.get("DesignerWeaponInteractionArt")
set_canvas_layout(equipped_button, 922.0, 338.0, 225.0, 402.0, 7)

# The tree art already contains the clock; this transparent image is its hover target.
clock = widgets["DesignerShopClock"]
clock.set_brush_from_texture(textures["LoadoutCardSlot"], False)
clock.set_editor_property(
    "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 0.0)
)
clock.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)
set_canvas_layout(clock, 1390.0, 285.0, 230.0, 350.0, 5)

for index, x in enumerate((905.0, 988.0, 1071.0)):
    button = widgets[f"DesignerAttachmentSlot{index}"]
    art = widgets[f"DesignerAttachmentSlotArt{index}"]
    set_canvas_layout(button, x, 774.0, 89.0, 89.0, 12)
    set_image(art, textures["WeaponLoadoutSlot"])
    fill_panel_slot(art)

card_positions = (
    (1309.0, 275.0),
    (1309.0, 349.0),
    (1309.0, 423.0),
    (1587.0, 275.0),
    (1587.0, 349.0),
    (1587.0, 423.0),
    (1587.0, 496.0),
    (1587.0, 570.0),
    (1456.0, 547.0),
    (1456.0, 621.0),
    (1456.0, 695.0),
    # The runtime contract exposes twelve owned-card bindings. The formal
    # composition has eleven illustrated sockets, so keep the twelfth binding
    # editable and available directly below the illustrated stack.
    (1456.0, 769.0),
)
for index, (x, y) in enumerate(card_positions):
    button = widgets.get(f"DesignerCardSlot{index}")
    art = widgets.get(f"DesignerCardSlotArt{index}")
    if button is None:
        button = add_widget(
            toolset,
            blueprint,
            unreal.Button,
            f"DesignerCardSlot{index}",
            designer,
        )
        transparent_button(button)
    if art is None:
        art = add_widget(
            toolset,
            blueprint,
            unreal.Image,
            f"DesignerCardSlotArt{index}",
            button,
        )
        art.set_editor_property(
            "visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE
        )
    set_canvas_layout(button, x, y, 66.0, 66.0, 12)
    set_image(art, textures["LoadoutCardSlot"])
    fill_panel_slot(art)

# Retire old placeholder presentation without deleting required bindings.
widgets = widget_map(toolset, blueprint)
for name in ("Overlay_0", "ArtShopKeeper", "Overlay_1"):
    widget = widgets.get(name)
    if widget is not None:
        widget.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

close_button = widgets.get("CloseButton")
if not isinstance(close_button, unreal.Button):
    raise RuntimeError("Missing required CloseButton")
transparent_button(close_button)
set_canvas_layout(close_button, 1263.0, 916.0, 410.0, 104.0, 20)
if close_button.get_children_count() > 0:
    close_button.get_child_at(0).set_editor_property(
        "visibility", unreal.SlateVisibility.COLLAPSED
    )

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(
    blueprint, only_if_is_dirty=False
):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save")

unreal.log("[Plan110ShopAuthor] authored formal 1920x1080 shop presentation")
