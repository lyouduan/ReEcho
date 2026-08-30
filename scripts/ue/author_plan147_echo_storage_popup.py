"""Author the formal echo-storage and unresolved-close modals in the shop WBP.

The WBP owns every visual and its geometry. C++ binds the stable widget names
and only projects real echo state, visibility, enabled state and interactions.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
STANDALONE_POPUP_PATH = "/Game/ReEcho/UI/WBP_ReEchoStoragePopup"
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/InventoryShop/EchoStorage"
DESIGN_WIDTH = 1920.0
DESIGN_HEIGHT = 1080.0
FRAME_RECT = (423.0, 229.0, 1073.0, 579.0)


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Missing Plan147 asset: {path}")
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
        raise RuntimeError(f"Unable to add Plan147 widget: {name}")
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
    # ScaleBoxSlot/SizeBoxSlot fill their sole child by construction and expose
    # these inherited properties as protected in UE 5.8 Python. The editable
    # multi-child slots below still need explicit fill alignment.
    if not isinstance(slot, (unreal.VerticalBoxSlot, unreal.ButtonSlot)):
        return
    slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
    )
    slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
    )


def slate_size(width, height):
    size = unreal.DeprecateSlateVector2D()
    size.set_editor_property("x", width)
    size.set_editor_property("y", height)
    return size


def configure_button(button, light_texture, dark_texture, width, height):
    style = button.get_editor_property("widget_style")
    for brush_name, texture in (
        ("normal", light_texture),
        ("hovered", dark_texture),
        ("pressed", dark_texture),
        ("disabled", dark_texture),
    ):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("image_size", slate_size(width, height))
        alpha = 0.55 if brush_name == "disabled" else 1.0
        brush.set_editor_property(
            "tint_color",
            unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, alpha)),
        )
        style.set_editor_property(brush_name, brush)
    style.set_editor_property("normal_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    style.set_editor_property("pressed_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    button.set_editor_property("widget_style", style)
    button.set_editor_property(
        "background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)


def configure_text(text, value, size, color, justification, auto_wrap=False):
    text.set_text(value)
    text.set_editor_property("auto_wrap_text", auto_wrap)
    text.set_editor_property("justification", justification)
    text.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    text.set_editor_property("shadow_offset", unreal.Vector2D(1.5, 1.5))
    text.set_editor_property(
        "shadow_color_and_opacity", unreal.LinearColor(0.0, 0.0, 0.0, 0.75)
    )
    font = text.get_editor_property("font")
    font.size = size
    text.set_editor_property("font", font)
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)


def add_image(toolset, blueprint, parent, name, texture, rect, z_order=0):
    image = add_widget(toolset, blueprint, unreal.Image, name, parent)
    image.set_brush_from_texture(texture, False)
    image.set_editor_property(
        "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    image.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
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
    z_order=4,
    color=unreal.LinearColor(0.10, 0.07, 0.05, 1.0),
    justification=unreal.TextJustify.LEFT,
    auto_wrap=False,
):
    text = add_widget(toolset, blueprint, unreal.TextBlock, name, parent)
    configure_text(text, value, size, color, justification, auto_wrap)
    set_canvas_layout(text, *rect, z_order)
    return text


def add_formal_button(
    toolset,
    blueprint,
    parent,
    name,
    label_name,
    label,
    rect,
    light_texture,
    dark_texture,
    size=18,
    z_order=8,
):
    button = add_widget(toolset, blueprint, unreal.Button, name, parent)
    configure_button(button, light_texture, dark_texture, rect[2], rect[3])
    set_canvas_layout(button, *rect, z_order)
    text = add_widget(toolset, blueprint, unreal.TextBlock, label_name, button)
    configure_text(
        text,
        label,
        size,
        unreal.LinearColor(1.0, 0.97, 0.92, 1.0),
        unreal.TextJustify.CENTER,
    )
    fill_panel_slot(text)
    return button, text


def add_modal_canvas(toolset, blueprint, root, root_name, canvas_name, z_order):
    root_widget = add_widget(
        toolset, blueprint, unreal.VerticalBox, root_name, root
    )
    root_widget.set_editor_property(
        "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
    )
    set_canvas_layout(root_widget, 0.0, 0.0, DESIGN_WIDTH, DESIGN_HEIGHT, z_order)
    canvas = add_widget(
        toolset, blueprint, unreal.CanvasPanel, canvas_name, root_widget
    )
    canvas.set_editor_property(
        "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
    )
    fill_panel_slot(canvas)
    return root_widget, canvas


textures = {
    name: load_required(f"{TEXTURE_ROOT}/T_UI_EchoStorage_{name}")
    for name in ("PopupFrame", "ButtonLight", "ButtonDark")
}
white_texture = load_required("/Engine/EngineResources/WhiteSquareTexture")

toolset = unreal.UMGToolSet.get_default_object()
blueprint = load_required(ASSET_PATH)
if unreal.EditorAssetLibrary.does_asset_exist(STANDALONE_POPUP_PATH):
    raise RuntimeError(
        "Plan147 is now Designer-authored in WBP_ReEchoStoragePopup. "
        "This retired script will not overwrite manual popup edits."
    )
widgets = widget_map(toolset, blueprint)
root = widgets.get("RootPanel")
if not isinstance(root, unreal.CanvasPanel):
    raise RuntimeError("Plan147 requires RootPanel CanvasPanel")

# Rebuild only the Plan147-owned modal surfaces. All shop controls remain intact.
for name in ("EchoPanelScale", "CloseConfirmWidget"):
    old_widget = widgets.get(name)
    if old_widget is not None:
        if not toolset.call_method("RemoveWidget", args=(blueprint, old_widget)):
            raise RuntimeError(f"Unable to replace Plan147 widget: {name}")
        widgets = widget_map(toolset, blueprint)

echo_scale = add_widget(
    toolset, blueprint, unreal.ScaleBox, "EchoPanelScale", root
)
echo_scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
echo_scale.set_editor_property(
    "stretch_direction", unreal.StretchDirection.DOWN_ONLY
)
echo_scale.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
set_canvas_layout(echo_scale, 0.0, 0.0, DESIGN_WIDTH, DESIGN_HEIGHT, 40)

design_size = add_widget(
    toolset, blueprint, unreal.SizeBox, "EchoPopupDesignSize", echo_scale
)
design_size.set_editor_property("width_override", DESIGN_WIDTH)
design_size.set_editor_property("height_override", DESIGN_HEIGHT)
fill_panel_slot(design_size)

echo_panel = add_widget(
    toolset, blueprint, unreal.VerticalBox, "EchoPanel", design_size
)
echo_panel.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
fill_panel_slot(echo_panel)

echo_canvas = add_widget(
    toolset, blueprint, unreal.CanvasPanel, "EchoPopupCanvas", echo_panel
)
echo_canvas.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
fill_panel_slot(echo_canvas)

dimmer = add_widget(toolset, blueprint, unreal.Border, "EchoModalDimmer", echo_canvas)
dimmer.set_editor_property("brush_color", unreal.LinearColor(0.0, 0.0, 0.0, 0.66))
dimmer.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
set_canvas_layout(dimmer, 0.0, 0.0, DESIGN_WIDTH, DESIGN_HEIGHT, 0)
add_image(
    toolset,
    blueprint,
    echo_canvas,
    "EchoPopupFrameArt",
    textures["PopupFrame"],
    FRAME_RECT,
    1,
)
add_text(
    toolset,
    blueprint,
    echo_canvas,
    "EchoPopupTitle",
    "回响存储",
    (740.0, 274.0, 440.0, 70.0),
    44,
    color=unreal.LinearColor(1.0, 0.96, 0.88, 1.0),
    justification=unreal.TextJustify.CENTER,
)
add_formal_button(
    toolset,
    blueprint,
    echo_canvas,
    "EchoPopupCloseButton",
    "EchoPopupCloseLabel",
    "关闭",
    (1270.0, 276.0, 165.0, 56.0),
    textures["ButtonLight"],
    textures["ButtonDark"],
    18,
)
add_text(
    toolset,
    blueprint,
    echo_canvas,
    "EchoCapacityText",
    "当前时间锚点：遭遇 #1 | 角色 J_HEART | 武器 LongSword",
    (530.0, 420.0, 860.0, 38.0),
    24,
    justification=unreal.TextJustify.CENTER,
)
add_text(
    toolset,
    blueprint,
    echo_canvas,
    "EchoReplayModeText",
    "你的回响只会重复时间锚点所在关卡内的行为。",
    (570.0, 365.0, 780.0, 34.0),
    18,
    justification=unreal.TextJustify.CENTER,
)
add_text(
    toolset,
    blueprint,
    echo_canvas,
    "EchoPendingInfoText",
    "本场回响：遭遇 #3 | 角色 J_HEART | 武器 LongSword",
    (530.0, 480.0, 860.0, 38.0),
    18,
    justification=unreal.TextJustify.CENTER,
)

add_formal_button(
    toolset,
    blueprint,
    echo_canvas,
    "EchoStoreButton",
    "EchoStoreLabel",
    "替换为本场时间锚点",
    (670.0, 555.0, 270.0, 75.0),
    textures["ButtonLight"],
    textures["ButtonDark"],
    19,
)
add_formal_button(
    toolset,
    blueprint,
    echo_canvas,
    "EchoSkipButton",
    "EchoSkipLabel",
    "跳过本场回响",
    (980.0, 555.0, 270.0, 75.0),
    textures["ButtonLight"],
    textures["ButtonDark"],
    19,
)
# The unresolved-close confirmation uses the same formal frame and button states.
close_confirm, close_canvas = add_modal_canvas(
    toolset,
    blueprint,
    root,
    "CloseConfirmWidget",
    "EchoCloseConfirmCanvas",
    50,
)
# Keep the primary popup visible in Designer. The confirmation remains fully
# authored and can be previewed by toggling this root to Visible.
close_confirm.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
confirm_dimmer = add_widget(
    toolset, blueprint, unreal.Border, "EchoCloseConfirmDimmer", close_canvas
)
confirm_dimmer.set_editor_property(
    "brush_color", unreal.LinearColor(0.0, 0.0, 0.0, 0.66)
)
confirm_dimmer.set_editor_property(
    "visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE
)
set_canvas_layout(confirm_dimmer, 0.0, 0.0, DESIGN_WIDTH, DESIGN_HEIGHT, 0)
add_image(
    toolset,
    blueprint,
    close_canvas,
    "EchoCloseConfirmFrameArt",
    textures["PopupFrame"],
    FRAME_RECT,
    1,
)
add_text(
    toolset,
    blueprint,
    close_canvas,
    "EchoCloseConfirmTitle",
    "回响存储",
    (740.0, 290.0, 440.0, 70.0),
    44,
    color=unreal.LinearColor(1.0, 0.96, 0.88, 1.0),
    justification=unreal.TextJustify.CENTER,
)
add_text(
    toolset,
    blueprint,
    close_canvas,
    "EchoConfirmText",
    "本场回响尚未处理。\n请选择跳过并继续，或返回回响存储。",
    (635.0, 430.0, 650.0, 105.0),
    28,
    justification=unreal.TextJustify.CENTER,
    auto_wrap=True,
)
add_formal_button(
    toolset,
    blueprint,
    close_canvas,
    "EchoConfirmSkipContinue",
    "EchoConfirmSkipLabel",
    "跳过并继续",
    (560.0, 590.0, 350.0, 119.0),
    textures["ButtonLight"],
    textures["ButtonDark"],
    32,
)
add_formal_button(
    toolset,
    blueprint,
    close_canvas,
    "EchoConfirmReturn",
    "EchoConfirmReturnLabel",
    "返回回响存储",
    (1010.0, 590.0, 350.0, 119.0),
    textures["ButtonLight"],
    textures["ButtonDark"],
    32,
)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(
    blueprint, only_if_is_dirty=False
):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save")

unreal.log("[Plan147EchoAuthor] authored formal echo-storage modal surfaces")
