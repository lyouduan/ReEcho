"""Author the five real rune slots used after acquiring 双重武器槽.

The group is an independent CanvasPanel in the shop WBP so every slot remains
freely movable and resizable in UMG. Runtime code only switches layouts and
fills each authored slot with its mapped rune.
"""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
CORE_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/InventoryShop/Plan149/"
    "T_UI_Shop149_CoreWeaponLoadoutSlot"
)
SQUARE_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/InventoryShop/Plan110/"
    "T_UI_Shop110_WeaponLoadoutSlot"
)


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


def add_widget(toolset, blueprint, widget_class, name, parent):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, -1)
    )
    if info.widget is None:
        raise RuntimeError(f"Unable to add Plan149 widget: {name}")
    mark_variable(toolset, blueprint, info.widget)
    return info.widget


def set_canvas_layout(widget, x, y, width, height, z_order=0):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a CanvasPanel child")
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


def slate_size(width, height):
    value = unreal.DeprecateSlateVector2D()
    value.set_editor_property("x", width)
    value.set_editor_property("y", height)
    return value


def apply_frame(button, texture, width, height):
    style = button.get_editor_property("widget_style")
    for state in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(state)
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("image_size", slate_size(width, height))
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
    button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
core_texture = unreal.load_asset(CORE_TEXTURE_PATH)
square_texture = unreal.load_asset(SQUARE_TEXTURE_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing shop widget: {WIDGET_PATH}")
if not isinstance(core_texture, unreal.Texture2D):
    raise RuntimeError(f"Missing reviewed core slot texture: {CORE_TEXTURE_PATH}")
if not isinstance(square_texture, unreal.Texture2D):
    raise RuntimeError(f"Missing reviewed square slot texture: {SQUARE_TEXTURE_PATH}")

widgets = widget_map(toolset, blueprint)
parent = widgets.get("DesignerLoadoutCanvas")
if not isinstance(parent, unreal.CanvasPanel):
    raise RuntimeError("Missing DesignerLoadoutCanvas")

old_layout = widgets.get("DesignerDualAttachmentLayout")
if old_layout is not None:
    if not toolset.call_method("RemoveWidget", args=(blueprint, old_layout)):
        raise RuntimeError("Unable to replace DesignerDualAttachmentLayout")
widgets = widget_map(toolset, blueprint)

layout = add_widget(
    toolset,
    blueprint,
    unreal.CanvasPanel,
    "DesignerDualAttachmentLayout",
    parent,
)
layout.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
set_canvas_layout(layout, 0.0, 0.0, 1920.0, 1080.0, 20)

# One core slot plus two real slots for each non-core category.
slot_rects = (
    (904.0, 748.0, 89.0, 175.0),
    (1000.0, 748.0, 89.0, 89.0),
    (1096.0, 748.0, 89.0, 89.0),
    (1000.0, 840.0, 89.0, 89.0),
    (1096.0, 840.0, 89.0, 89.0),
)
for index, rect in enumerate(slot_rects):
    button = add_widget(
        toolset,
        blueprint,
        unreal.Button,
        f"DesignerDualAttachmentSlot{index}",
        layout,
    )
    set_canvas_layout(button, *rect, 1)
    texture = core_texture if index == 0 else square_texture
    apply_frame(button, texture, rect[2], rect[3])

    art = add_widget(
        toolset,
        blueprint,
        unreal.Image,
        f"DesignerDualAttachmentSlotArt{index}",
        button,
    )
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

# The Designer previews the acquired-card state; runtime restores the standard
# three slots until the effective capacities report the G_3_22 expansion.
widgets = widget_map(toolset, blueprint)
for index in range(3):
    standard = widgets.get(f"DesignerAttachmentSlot{index}")
    if isinstance(standard, unreal.Button):
        standard.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(
    blueprint, only_if_is_dirty=False
):
    raise RuntimeError("Failed to save WBP_ReEchoInventoryShopScreen")

unreal.log("[Plan149DualSlotAuthor] authored one core plus four non-core slots")
