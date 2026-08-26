"""Add the Plan118 Boss health presentation to the existing Encounter HUD.

The operation is idempotent. It reuses named widgets and only authors the
Boss-only panel that replaces the clock during a Boss encounter.
"""

import unreal


ENCOUNTER_HUD = "/Game/ReEcho/UI/WBP_ReEchoEncounterHud"
HEALTH_FRAME = "/Game/ReEcho/Textures/UI/CombatHud/T_UI_CombatHud_HealthFrame"


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Missing Plan118 Boss HUD asset: {path}")
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


def ensure_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    widgets = widget_map(toolset, blueprint)
    widget = widgets.get(name)
    if widget is None:
        result = toolset.call_method(
            "AddWidget", args=(blueprint, widget_class, name, parent, child_index)
        )
        widget = result.widget
        if widget is None:
            raise RuntimeError(f"Unable to add Plan118 Boss HUD widget: {name}")
    if not isinstance(widget, widget_class):
        raise RuntimeError(
            f"Plan118 widget {name} expected {widget_class.__name__}, "
            f"got {widget.get_class().get_name()}"
        )
    if widget.get_parent() != parent:
        result = toolset.call_method(
            "MoveWidget", args=(blueprint, widget, parent, child_index)
        )
        if result.widget is None:
            raise RuntimeError(f"Unable to move Plan118 Boss HUD widget: {name}")
        widget = result.widget
    return widget


def set_canvas_layout(
    widget,
    x,
    y,
    width,
    height,
    anchor_x=0.0,
    anchor_y=0.0,
    alignment_x=0.0,
    alignment_y=0.0,
    z_order=0,
):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct CanvasPanel child")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, y, width, height),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(anchor_x, anchor_y),
                maximum=unreal.Vector2D(anchor_x, anchor_y),
            ),
            alignment=unreal.Vector2D(alignment_x, alignment_y),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


toolset = unreal.UMGToolSet.get_default_object()
encounter = load_required(ENCOUNTER_HUD)
health_frame_texture = load_required(HEALTH_FRAME)
widgets = widget_map(toolset, encounter)
root = widgets.get("RootCanvas")
if not isinstance(root, unreal.CanvasPanel):
    raise RuntimeError("Encounter HUD RootCanvas is missing")

panel = ensure_widget(
    toolset, encounter, unreal.CanvasPanel, "BossHealthPanel", root
)
mark_variable(toolset, encounter, panel)
panel.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
set_canvas_layout(
    panel,
    14.0,
    105.0,
    464.0,
    58.0,
    anchor_x=0.5,
    anchor_y=0.0,
    alignment_x=0.5,
    alignment_y=0.0,
    z_order=8,
)

fill = ensure_widget(
    toolset, encounter, unreal.Border, "BossHealthFill", panel, 0
)
mark_variable(toolset, encounter, fill)
fill.set_brush_color(unreal.LinearColor(0.22, 0.055, 0.32, 1.0))
fill.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
fill.set_editor_property("render_transform_pivot", unreal.Vector2D(0.0, 0.5))
fill.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
set_canvas_layout(fill, 12.0, 8.0, 440.0, 42.0, z_order=0)

frame = ensure_widget(
    toolset, encounter, unreal.Image, "BossHealthFrame", panel, 1
)
mark_variable(toolset, encounter, frame)
frame.set_brush_from_texture(health_frame_texture, False)
frame.set_editor_property(
    "color_and_opacity", unreal.LinearColor(0.55, 0.28, 0.68, 1.0)
)
frame.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
set_canvas_layout(frame, 0.0, 0.0, 464.0, 58.0, z_order=1)

if not toolset.call_method("CompileWidgetBlueprint", args=(encounter,)):
    raise RuntimeError("WBP_ReEchoEncounterHud failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(
    encounter, only_if_is_dirty=False
):
    raise RuntimeError("WBP_ReEchoEncounterHud failed to save")

authored = widget_map(toolset, encounter)
for required_name in ("BossHealthPanel", "BossHealthFill", "BossHealthFrame"):
    if required_name not in authored:
        raise RuntimeError(f"Missing authored Boss HUD widget: {required_name}")

unreal.log(
    "[Plan118BossHudAuthor] PASS "
    "BossHealthPanel=464x58 fill=#381052 frameTint=#8C47AD"
)
