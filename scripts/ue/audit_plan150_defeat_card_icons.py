"""Audit the five independently editable Plan150 defeat-card icon overlays."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
SLOT_X_POSITIONS = (465.0, 627.0, 789.0, 951.0, 1113.0)


def canvas_rect(widget):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Plan150 widget has no CanvasPanelSlot: {widget.get_name()}")
    offsets = slot.get_editor_property("layout_data").offsets
    return (offsets.left, offsets.top, offsets.right, offsets.bottom), slot


def assert_rect(name, actual, expected):
    if any(abs(value - target) > 0.1 for value, target in zip(actual, expected)):
        raise RuntimeError(f"Plan150 layout drift for {name}: {actual} != {expected}")


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Plan150 widget: {WIDGET_PATH}")
widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}
defeat_canvas = widgets.get("DefeatCanvas")
if not isinstance(defeat_canvas, unreal.CanvasPanel):
    raise RuntimeError("Plan150 formal DefeatCanvas is missing")

for index, x in enumerate(SLOT_X_POSITIONS):
    slot_name = f"DesignerDefeatCardSlot{index}"
    slot_image = widgets.get(slot_name)
    if not isinstance(slot_image, unreal.Image) or slot_image.get_parent() != defeat_canvas:
        raise RuntimeError(f"Plan150 empty slot is missing: {slot_name}")
    slot_rect, slot = canvas_rect(slot_image)
    assert_rect(slot_name, slot_rect, (x, 595.0, 136.0, 164.0))
    if slot.get_editor_property("z_order") != 45:
        raise RuntimeError(f"Plan150 empty slot z-order changed: {slot_name}")

    icon_name = f"DesignerDefeatCardIcon{index}"
    icon = widgets.get(icon_name)
    if not isinstance(icon, unreal.Image) or icon.get_parent() != defeat_canvas:
        raise RuntimeError(f"Plan150 card icon overlay is missing: {icon_name}")
    icon_rect, icon_slot = canvas_rect(icon)
    assert_rect(icon_name, icon_rect, (x, 609.0, 136.0, 136.0))
    if icon_slot.get_editor_property("z_order") != 50:
        raise RuntimeError(f"Plan150 icon z-order is not above its empty slot: {icon_name}")
    if icon.get_editor_property("visibility") != unreal.SlateVisibility.HIDDEN:
        raise RuntimeError(f"Plan150 icon must default to Hidden: {icon_name}")

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("Plan150 defeat-card WBP compile audit failed")
unreal.log("[Plan150DefeatCardsAudit] five square card-icon overlays are valid")
