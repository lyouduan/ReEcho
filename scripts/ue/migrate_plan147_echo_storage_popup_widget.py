"""Extract the authored echo-storage modal into a standalone WYSIWYG WBP.

The first run clones the current Plan147-owned widget subtrees, including
Designer geometry and styles, before replacing them in the shop screen with a
single child UserWidget. Later runs only verify/repair the child host and never
rewrite the standalone popup, so manual Designer edits stay authoritative.
"""

import unreal


SHOP_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
POPUP_DIR = "/Game/ReEcho/UI"
POPUP_NAME = "WBP_ReEchoStoragePopup"
POPUP_PATH = f"{POPUP_DIR}/{POPUP_NAME}"
HOST_NAME = "EchoStoragePopupWidget"
DESIGN_WIDTH = 1920.0
DESIGN_HEIGHT = 1080.0


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


toolset = unreal.UMGToolSet.get_default_object()


def widget_infos(blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in widget_infos(blueprint)
        if info.widget
    }


def mark_variable(blueprint, widget):
    info = next(
        (item for item in widget_infos(blueprint) if item.widget is widget),
        None,
    )
    require(info is not None, f"Missing WidgetInfo for {widget.get_name()}")
    if not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(blueprint, widget_class, name, parent):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, -1)
    )
    require(info.widget is not None, f"Unable to add widget: {name}")
    mark_variable(blueprint, info.widget)
    return info.widget


def copy_property(source, target, property_name):
    try:
        target.set_editor_property(
            property_name, source.get_editor_property(property_name)
        )
    except Exception:
        # UMG property exposure differs slightly between engine versions. The
        # explicit lists below intentionally tolerate unavailable properties.
        pass


def copy_widget_properties(source, target):
    common = (
        "visibility",
        "render_opacity",
        "render_transform",
        "render_transform_pivot",
        "is_enabled",
        "tool_tip_text",
        "cursor",
        "clipping",
        "pixel_snapping",
    )
    for property_name in common:
        copy_property(source, target, property_name)

    typed_properties = []
    if isinstance(source, unreal.TextBlock):
        typed_properties.extend(
            (
                "text",
                "color_and_opacity",
                "font",
                "strike_brush",
                "shadow_offset",
                "shadow_color_and_opacity",
                "min_desired_width",
                "auto_wrap_text",
                "wrap_text_at",
                "wrapping_policy",
                "justification",
                "line_height_percentage",
                "apply_line_height_to_bottom_line",
                "text_transform_policy",
                "text_overflow_policy",
                "simple_text_mode",
            )
        )
    elif isinstance(source, unreal.Image):
        typed_properties.extend(
            ("brush", "color_and_opacity", "flip_for_right_to_left_flow_direction")
        )
    elif isinstance(source, unreal.Button):
        typed_properties.extend(
            (
                "widget_style",
                "color_and_opacity",
                "background_color",
                "click_method",
                "touch_method",
                "press_method",
                "is_focusable",
            )
        )
    elif isinstance(source, unreal.Border):
        typed_properties.extend(
            (
                "horizontal_alignment",
                "vertical_alignment",
                "brush",
                "brush_color",
                "desired_size_scale",
                "show_effect_when_disabled",
                "padding",
            )
        )
    elif isinstance(source, unreal.SizeBox):
        typed_properties.extend(
            (
                "width_override",
                "height_override",
                "min_desired_width",
                "min_desired_height",
                "max_desired_width",
                "max_desired_height",
                "min_aspect_ratio",
                "max_aspect_ratio",
            )
        )
    elif isinstance(source, unreal.ScaleBox):
        typed_properties.extend(
            (
                "stretch",
                "stretch_direction",
                "user_specified_scale",
                "ignore_inherited_scale",
            )
        )
    for property_name in typed_properties:
        copy_property(source, target, property_name)


def copy_slot_properties(source, target):
    source_slot = source.get_editor_property("slot")
    target_slot = target.get_editor_property("slot")
    require(source_slot is not None, f"{source.get_name()} has no source slot")
    require(target_slot is not None, f"{target.get_name()} has no target slot")
    for property_name in (
        "layout_data",
        "auto_size",
        "z_order",
        "padding",
        "horizontal_alignment",
        "vertical_alignment",
        "size",
    ):
        copy_property(source_slot, target_slot, property_name)


def clone_subtree(source, target_blueprint, target_parent):
    target = add_widget(
        target_blueprint, source.get_class(), source.get_name(), target_parent
    )
    copy_widget_properties(source, target)
    copy_slot_properties(source, target)
    if isinstance(source, unreal.PanelWidget):
        for child_index in range(source.get_children_count()):
            clone_subtree(source.get_child_at(child_index), target_blueprint, target)
    return target


def create_popup_from_current_shop(shop_blueprint, shop_widgets):
    echo_root = shop_widgets.get("EchoPanelScale")
    confirm_root = shop_widgets.get("CloseConfirmWidget")
    require(isinstance(echo_root, unreal.ScaleBox), "Missing current EchoPanelScale")
    require(
        isinstance(confirm_root, unreal.VerticalBox),
        "Missing current CloseConfirmWidget",
    )

    parent_class = unreal.load_class(None, "/Script/UMG.UserWidget")
    require(parent_class is not None, "Unable to load UUserWidget parent class")
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    popup_blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        POPUP_NAME, POPUP_DIR, unreal.WidgetBlueprint, factory
    )
    require(
        isinstance(popup_blueprint, unreal.WidgetBlueprint),
        f"Unable to create standalone popup WBP: {POPUP_PATH}",
    )

    popup_root = add_widget(
        popup_blueprint, unreal.CanvasPanel, "EchoStoragePopupRoot", None
    )
    popup_root.set_editor_property(
        "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
    )
    clone_subtree(echo_root, popup_blueprint, popup_root)
    clone_subtree(confirm_root, popup_blueprint, popup_root)

    require(
        toolset.call_method("CompileWidgetBlueprint", args=(popup_blueprint,)),
        "Standalone echo popup failed to compile",
    )
    require(
        unreal.EditorAssetLibrary.save_loaded_asset(
            popup_blueprint, only_if_is_dirty=False
        ),
        "Standalone echo popup failed to save",
    )
    return popup_blueprint


def set_fullscreen_host_layout(host):
    slot = host.get_editor_property("slot")
    require(isinstance(slot, unreal.CanvasPanelSlot), "Popup host needs Canvas slot")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(0.0, 0.0, DESIGN_WIDTH, DESIGN_HEIGHT),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0),
                maximum=unreal.Vector2D(0.0, 0.0),
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", 40)


shop = unreal.load_asset(SHOP_PATH)
require(isinstance(shop, unreal.WidgetBlueprint), f"Missing shop WBP: {SHOP_PATH}")
shop_widgets = widget_map(shop)
popup = (
    unreal.load_asset(POPUP_PATH)
    if unreal.EditorAssetLibrary.does_asset_exist(POPUP_PATH)
    else create_popup_from_current_shop(shop, shop_widgets)
)
require(isinstance(popup, unreal.WidgetBlueprint), f"Invalid popup WBP: {POPUP_PATH}")
require(
    toolset.call_method("CompileWidgetBlueprint", args=(popup,)),
    "Standalone echo popup failed to compile before embedding",
)
popup_class = popup.generated_class()
require(popup_class is not None, "Standalone echo popup has no generated class")

shop_widgets = widget_map(shop)
host = shop_widgets.get(HOST_NAME)
if host is None:
    old_echo = shop_widgets.get("EchoPanelScale")
    old_confirm = shop_widgets.get("CloseConfirmWidget")
    if isinstance(old_echo, unreal.ScaleBox):
        shop_parent = old_echo.get_parent()
        require(isinstance(shop_parent, unreal.CanvasPanel), "Echo roots need Canvas parent")
    else:
        root_canvases = [
            item.widget
            for item in widget_infos(shop)
            if isinstance(item.widget, unreal.CanvasPanel)
            and item.widget.get_parent() is None
        ]
        require(
            len(root_canvases) == 1,
            "Shop without legacy echo roots needs exactly one root CanvasPanel",
        )
        shop_parent = root_canvases[0]

    host = add_widget(shop, popup_class, HOST_NAME, shop_parent)
    set_fullscreen_host_layout(host)
    if isinstance(old_confirm, unreal.VerticalBox):
        require(
            toolset.call_method("RemoveWidget", args=(shop, old_confirm)),
            "Unable to remove embedded close-confirm subtree",
        )
    if isinstance(old_echo, unreal.ScaleBox):
        require(
            toolset.call_method("RemoveWidget", args=(shop, old_echo)),
            "Unable to remove embedded echo-popup subtree",
        )
else:
    require(host.get_class() == popup_class, "Shop popup host uses the wrong WBP")
    set_fullscreen_host_layout(host)

require(
    toolset.call_method("CompileWidgetBlueprint", args=(shop,)),
    "Shop WBP failed to compile after popup extraction",
)
require(
    unreal.EditorAssetLibrary.save_loaded_asset(shop, only_if_is_dirty=False),
    "Shop WBP failed to save after popup extraction",
)
unreal.log(
    "[Plan147EchoWidgetMigration] PASS standalone WYSIWYG popup created and shop reduced to one child host"
)
