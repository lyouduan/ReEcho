"""Move the formal victory/defeat character images above every sibling without changing geometry."""

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
    slot = character.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Character image has no CanvasPanelSlot: {character_name}")
    slot.set_editor_property("z_order", CHARACTER_Z_ORDER)
    character.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)

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
    raise RuntimeError("WBP_ReEchoRestart failed to compile after character layer update")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoRestart failed to save after character layer update")
unreal.log("[Plan135] Settlement character images are front-most at ZOrder 80")
