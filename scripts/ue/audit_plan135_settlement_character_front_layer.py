"""Audit that both formal settlement character images remain above every Canvas sibling."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
CHARACTER_IMAGES = (
    ("VictoryCanvas", "ArtVictoryCharacterFormal"),
    ("DefeatCanvas", "ArtDefeatCharacterFormal"),
)
CHARACTER_Z_ORDER = 80


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing settlement widget: {WIDGET_PATH}")

widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}

for canvas_name, character_name in CHARACTER_IMAGES:
    canvas = widgets.get(canvas_name)
    character = widgets.get(character_name)
    if not isinstance(canvas, unreal.CanvasPanel):
        raise RuntimeError(f"Missing settlement canvas: {canvas_name}")
    if not isinstance(character, unreal.Image) or character.get_parent() != canvas:
        raise RuntimeError(f"Missing direct character image: {character_name}")
    character_slot = character.get_editor_property("slot")
    if not isinstance(character_slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Character image has no CanvasPanelSlot: {character_name}")
    if character_slot.get_editor_property("z_order") != CHARACTER_Z_ORDER:
        raise RuntimeError(f"{character_name} must remain at ZOrder {CHARACTER_Z_ORDER}")
    if character.get_editor_property("visibility") != unreal.SlateVisibility.HIT_TEST_INVISIBLE:
        raise RuntimeError(f"{character_name} must remain HitTestInvisible")

    for child_index in range(canvas.get_children_count()):
        sibling = canvas.get_child_at(child_index)
        if sibling == character:
            continue
        sibling_slot = sibling.get_editor_property("slot")
        if isinstance(sibling_slot, unreal.CanvasPanelSlot):
            sibling_z_order = sibling_slot.get_editor_property("z_order")
            if sibling_z_order >= CHARACTER_Z_ORDER:
                raise RuntimeError(
                    f"{character_name} is not front-most: {sibling.get_name()} has ZOrder {sibling_z_order}"
                )

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoRestart failed to compile during Plan135 front-layer audit")
unreal.log("[Plan135Audit] Settlement character images are front-most and non-interactive")
