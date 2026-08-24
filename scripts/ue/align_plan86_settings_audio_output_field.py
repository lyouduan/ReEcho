"""Match the authored Audio output field to the Graphics dropdown geometry."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
AUDIO_TRACK_TRAILING_RESERVE = 178.0


def make_slate_size(x: float, y: float):
    size = unreal.DeprecateSlateVector2D()
    size.set_editor_property("x", x)
    size.set_editor_property("y", y)
    return size


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget: {ASSET_PATH}")
widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}

graphics_field = widgets.get("GraphicsDropdown0")
audio_panel = widgets.get("AudioPanel")
audio_label = widgets.get("AudioOutputLabel")
audio_field = widgets.get("AudioOutputField")
audio_background = widgets.get("AudioOutputFieldBackground")
if not isinstance(graphics_field, unreal.Image):
    raise RuntimeError("Missing Graphics dropdown geometry source")
if not isinstance(audio_panel, unreal.VerticalBox):
    raise RuntimeError("Missing AudioPanel")
if not isinstance(audio_label, unreal.TextBlock):
    raise RuntimeError("Missing AudioOutputLabel")
if not isinstance(audio_field, unreal.Overlay):
    raise RuntimeError("Missing AudioOutputField")
if not isinstance(audio_background, unreal.Image):
    raise RuntimeError("Missing AudioOutputFieldBackground")

graphics_slot = graphics_field.get_editor_property("slot")
panel_slot = audio_panel.get_editor_property("slot")
label_slot = audio_label.get_editor_property("slot")
field_slot = audio_field.get_editor_property("slot")
if not isinstance(graphics_slot, unreal.CanvasPanelSlot):
    raise RuntimeError("GraphicsDropdown0 must use a Canvas slot")
if not isinstance(panel_slot, unreal.CanvasPanelSlot):
    raise RuntimeError("AudioPanel must use a Canvas slot")
if not isinstance(field_slot, (unreal.HorizontalBoxSlot, unreal.CanvasPanelSlot)):
    raise RuntimeError("AudioOutputField must use a legacy row or Designer Canvas slot")

target_size = graphics_slot.get_size()
if isinstance(field_slot, unreal.HorizontalBoxSlot):
    if not isinstance(label_slot, unreal.HorizontalBoxSlot):
        raise RuntimeError("Legacy AudioOutputLabel must use a HorizontalBox slot")
    # The three audio tracks reserve 178 px for percentage and speaker art.
    trailing_reserve = AUDIO_TRACK_TRAILING_RESERVE
    automatic = unreal.SlateChildSize()
    automatic.set_editor_property("value", 1.0)
    automatic.set_editor_property("size_rule", unreal.SlateSizeRule.AUTOMATIC)
    field_slot.set_editor_property("size", automatic)
    field_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, trailing_reserve, 0.0))
    layout_mode = f"legacy trailing reserve {trailing_reserve:.1f}px"
else:
    # Once migrated, position is designer-owned. Only keep the agreed frame
    # size; never reset the artist's Canvas coordinates.
    field_slot.set_size(unreal.Vector2D(target_size.x, target_size.y))
    layout_mode = "preserved Designer Canvas position"

background_brush = audio_background.get_editor_property("brush")
background_brush.set_editor_property("image_size", make_slate_size(target_size.x, target_size.y))
audio_background.set_editor_property("brush", background_brush)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError(f"Failed to compile Settings widget: {ASSET_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Settings widget: {ASSET_PATH}")
unreal.log(
    f"[Plan86OutputField] matched Audio output to {target_size.x:.1f}x{target_size.y:.1f}; "
    f"{layout_mode}"
)
