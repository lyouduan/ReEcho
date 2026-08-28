"""Add authored square card-icon overlays to the five formal defeat slots."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
SLOT_X_POSITIONS = (465.0, 627.0, 789.0, 951.0, 1113.0)


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


def add_widget(toolset, blueprint, widget_class, name, parent):
    info = toolset.call_method("AddWidget", args=(blueprint, widget_class, name, parent, -1))
    if info.widget is None:
        raise RuntimeError(f"Unable to add Plan150 defeat-card widget: {name}")
    toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, info.widget, True))
    return info.widget


def ensure_widget(toolset, blueprint, widgets, widget_class, name, parent):
    widget = widgets.get(name)
    if widget is None:
        widget = add_widget(toolset, blueprint, widget_class, name, parent)
    if not isinstance(widget, widget_class):
        raise RuntimeError(f"Plan150 widget {name} has unexpected type")
    if widget.get_parent() != parent:
        raise RuntimeError(f"Plan150 widget {name} is not a direct DefeatCanvas child")
    return widget


def set_canvas_layout(widget, x, y, width, height, z_order):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct CanvasPanel child")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, y, width, height),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(0.0, 0.0)
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Plan150 widget blueprint: {WIDGET_PATH}")
widgets = widget_map(toolset, blueprint)
defeat_canvas = widgets.get("DefeatCanvas")
if not isinstance(defeat_canvas, unreal.CanvasPanel):
    raise RuntimeError("WBP_ReEchoRestart no longer has its formal DefeatCanvas")

for index, x in enumerate(SLOT_X_POSITIONS):
    slot_name = f"DesignerDefeatCardSlot{index}"
    slot_image = widgets.get(slot_name)
    if not isinstance(slot_image, unreal.Image) or slot_image.get_parent() != defeat_canvas:
        raise RuntimeError(f"Plan150 requires the existing authored empty slot: {slot_name}")

    icon = ensure_widget(
        toolset,
        blueprint,
        widgets,
        unreal.Image,
        f"DesignerDefeatCardIcon{index}",
        defeat_canvas,
    )
    set_canvas_layout(icon, x, 609.0, 136.0, 136.0, 50)
    icon.set_editor_property("color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    icon.set_editor_property("visibility", unreal.SlateVisibility.HIDDEN)
    widgets = widget_map(toolset, blueprint)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoRestart failed to compile after Plan150 authoring")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoRestart failed to save after Plan150 authoring")
unreal.log("[Plan150DefeatCards] five square card-icon overlays authored successfully")
