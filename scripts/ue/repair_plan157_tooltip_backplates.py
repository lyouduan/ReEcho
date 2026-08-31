"""Inspect by default; choose one explicit repair mode after closing the Editor.

REECHO_PLAN157_APPLY=1 repairs backplate/button brushes and button visibility.
REECHO_PLAN157_FRAME_INSETS=1 repairs only the three outer frame content insets.
Both preserve saved text, fonts, dimensions and entry layouts; only the latter
changes frame padding. Run with Run-EditorPythonLocked.ps1. Never rebuilds trees.
"""
import os
import unreal

TOOLSET = unreal.UMGToolSet.get_default_object()
ROOT = "/Game/ReEcho/UI/"
APPLY = os.environ.get("REECHO_PLAN157_APPLY") == "1"
FRAME_INSETS = os.environ.get("REECHO_PLAN157_FRAME_INSETS") == "1"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


require(not (APPLY and FRAME_INSETS), "Choose either brush repair or frame inset repair, not both")


def load(name):
    bp = unreal.load_asset(ROOT + name)
    require(isinstance(bp, unreal.WidgetBlueprint), f"Missing {name}")
    widgets = {i.widget.get_name(): i.widget for i in TOOLSET.call_method("GetWidgets", args=(bp,)).widgets if i.widget}
    return bp, widgets


def protected_snapshot(widgets):
    result = {}
    for name, widget in widgets.items():
        fields = ["render_transform", "render_transform_pivot", "render_opacity"]
        if isinstance(widget, unreal.TextBlock):
            fields += ["font", "color_and_opacity", "justification", "auto_wrap_text", "wrap_text_at", "margin"]
            result[(name, "text")] = str(widget.get_text())
        if isinstance(widget, unreal.SizeBox):
            fields += ["width_override", "height_override", "override_width_override", "override_height_override",
                       "min_desired_width", "min_desired_height"]
        if isinstance(widget, unreal.Border):
            fields += ["padding", "horizontal_alignment", "vertical_alignment", "brush_color"]
        for field in fields:
            value = widget.get_editor_property(field)
            result[(name, field)] = value.export_text() if isinstance(value, unreal.StructBase) else value
        slot = widget.get_editor_property("slot")
        if isinstance(slot, unreal.CanvasPanelSlot):
            result[(name, "canvas")] = slot.get_editor_property("layout_data").export_text()
            result[(name, "auto_size")] = slot.get_editor_property("auto_size")
        elif isinstance(slot, (unreal.VerticalBoxSlot, unreal.HorizontalBoxSlot, unreal.SizeBoxSlot, unreal.BorderSlot)):
            for field in ("padding", "horizontal_alignment", "vertical_alignment"):
                value = slot.get_editor_property(field)
                result[(name, "slot_" + field)] = value.export_text() if isinstance(value, unreal.StructBase) else value
    return result


def save_preserving(bp, widgets, before):
    require(TOOLSET.call_method("CompileWidgetBlueprint", args=(bp,)), f"Compile failed: {bp.get_name()}")
    after = protected_snapshot(widgets)
    changed = [str(key) for key in before if before[key] != after[key]]
    require(not changed, f"Protected styling changed: {changed}")
    bp.modify()
    require(unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False), f"Save failed: {bp.get_name()}")
    unreal.log(f"[Plan157Repair] Preserved {len(before)} text/style/layout fields: {bp.get_name()}")


reference_bp, reference = load("WBP_ReEchoLoadoutTooltip")
reference_brush = reference["TooltipFrame"].get_editor_property("background")
require(reference_brush.get_editor_property("resource_object") is not None, "Reference backdrop has no texture")
require(reference_brush.get_editor_property("draw_as") == unreal.SlateBrushDrawType.BOX, "Reference backdrop is not nine-slice")
white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
require(isinstance(white, unreal.Texture2D), "Missing white surface texture")

for name, borders in (
    ("WBP_ReEchoShopTooltip", ("TooltipFrame", "TooltipSurface", "OutcomeFrame", "OutcomeSurface")),
    ("WBP_ReEchoAttributeTooltip", ("AttributeTooltipFrame", "AttributeTooltipSurface")),
):
    bp, widgets = load(name)
    before = protected_snapshot(widgets)
    for border_name in borders:
        border = widgets[border_name]
        brush = border.get_editor_property("background")
        unreal.log(f"[Plan157Repair] BEFORE {name}.{border_name}: draw={brush.get_editor_property('draw_as')} "
                   f"resource={brush.get_editor_property('resource_object')} tint={brush.get_editor_property('tint_color')} "
                   f"color={border.get_editor_property('brush_color')}")
        child = border.get_child_at(0)
        child_slot = child.get_editor_property("slot") if child else None
        unreal.log(f"[Plan157Repair] GEOMETRY {border_name}: padding={border.get_editor_property('padding')} "
                   f"child_padding={child_slot.get_editor_property('padding') if child_slot else None} "
                   f"margin={brush.get_editor_property('margin')} image_size={brush.get_editor_property('image_size')}")
        if FRAME_INSETS:
            if border_name.endswith("Frame"):
                require(isinstance(child_slot, unreal.BorderSlot), f"Missing frame content slot: {border_name}")
                padding = border.get_editor_property("padding")
                slot_padding = child_slot.get_editor_property("padding")
                inset = unreal.Margin(*(max(3.0, getattr(padding, side), getattr(slot_padding, side))
                                       for side in ("left", "top", "right", "bottom")))
                # These are the only geometry fields intentionally changed.
                before.pop((border_name, "padding"))
                before.pop((child.get_name(), "slot_padding"))
                child_slot.set_padding(inset)
                border.set_padding(inset)
            continue
        if not APPLY:
            continue
        if border_name.endswith("Frame"):
            border.set_editor_property("background", reference_brush)
        else:
            brush.set_editor_property("resource_object", white)
            brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
            brush.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(1, 1, 1, 1)))
            border.set_editor_property("background", brush)
    if APPLY or FRAME_INSETS:
        save_preserving(bp, widgets, before)

bp, widgets = load("WBP_ReEchoLoadoutSelection")
before = protected_snapshot(widgets)
for name in ("ConfirmButton", "BackButton"):
    button = widgets[name]
    style = button.get_editor_property("widget_style")
    for state in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(state)
        unreal.log(f"[Plan157Repair] BEFORE {name}.{state}: resource={brush.get_editor_property('resource_object')} "
                   f"tint={brush.get_editor_property('tint_color')}")
    if APPLY:
        # UE nested structs are views: copy before editing so changing Disabled
        # cannot also mutate Normal. Preserve each state's artwork and geometry.
        for state in ("normal", "hovered", "pressed", "disabled"):
            brush = style.get_editor_property(state).copy()
            tint = 0.5 if state == "disabled" or (name == "BackButton" and state == "normal") else 1.0
            brush.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(tint, tint, tint, 1)))
            style.set_editor_property(state, brush)
        button.set_editor_property("widget_style", style)
        button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)
        widgets[name + "Label"].set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
if APPLY:
    save_preserving(bp, widgets, before)

unreal.log(f"[Plan157Repair] PASS mode={'frame-insets' if FRAME_INSETS else 'repair' if APPLY else 'inspect'}; reference loadout tooltip and attribute row left untouched")
