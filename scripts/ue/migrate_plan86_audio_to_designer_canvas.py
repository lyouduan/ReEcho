"""Move visible Settings audio controls onto a designer-owned Canvas.

The legacy VerticalBox/HorizontalBox rows made fine alignment impossible in
the UMG Designer.  This one-time migration preserves every bound widget and
its style, but gives each logical control a CanvasPanelSlot.  Re-running the
script never overwrites positions that a designer has subsequently adjusted.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
CANVAS_NAME = "AudioDesignerCanvas"

# Positions are local to AudioDesignerCanvas. Slider overlays stay grouped so
# track, fill and thumb cannot drift apart; their root is freely draggable.
INITIAL_LAYOUTS = {
    "MasterVolumeLabel": (5.0, 0.0, 185.0, 68.0, 1),
    "MasterVolumeVisualOverlay": (190.0, 0.0, 802.0, 68.0, 2),
    "MasterVolumePercent": (1005.0, 0.0, 92.0, 68.0, 3),
    "MasterMuteCheckBox": (1110.0, 0.0, 60.0, 68.0, 3),
    "MusicVolumeLabel": (5.0, 108.0, 185.0, 68.0, 1),
    "MusicVolumeVisualOverlay": (190.0, 108.0, 802.0, 68.0, 2),
    "MusicVolumePercent": (1005.0, 108.0, 92.0, 68.0, 3),
    "MusicMuteCheckBox": (1110.0, 108.0, 60.0, 68.0, 3),
    "CombatSfxVolumeLabel": (5.0, 216.0, 185.0, 68.0, 1),
    "CombatVolumeVisualOverlay": (190.0, 216.0, 802.0, 68.0, 2),
    "CombatVolumePercent": (1005.0, 216.0, 92.0, 68.0, 3),
    "CombatSfxMuteCheckBox": (1110.0, 216.0, 60.0, 68.0, 3),
    "AudioOutputLabel": (5.0, 324.0, 185.0, 50.0, 1),
    "AudioOutputField": (190.0, 324.0, 802.0, 50.0, 2),
}

LEGACY_VISIBLE_ROWS = (
    "MasterAudioRow",
    "MusicAudioRow",
    "CombatSfxAudioRow",
    "AudioOutputRow",
)


def widget_map(toolset, blueprint):
    return {
        str(info.widget_name): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, child_index)
    )
    if info.widget is None:
        raise RuntimeError(f"Unable to add {name}")
    return info.widget


def set_canvas_layout(widget, x, y, width, height, z_order):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} did not receive a CanvasPanelSlot")
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
    slot.set_editor_property("z_order", z_order)


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget: {ASSET_PATH}")

widgets = widget_map(toolset, blueprint)
audio_panel = widgets.get("AudioPanel")
if not isinstance(audio_panel, unreal.VerticalBox):
    raise RuntimeError("AudioPanel must remain the authored category wrapper")

designer_canvas = widgets.get(CANVAS_NAME)
created_canvas = designer_canvas is None
if created_canvas:
    designer_canvas = add_widget(
        toolset, blueprint, unreal.CanvasPanel, CANVAS_NAME, audio_panel, 0
    )
    toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, designer_canvas, True))
if not isinstance(designer_canvas, unreal.CanvasPanel):
    raise RuntimeError(f"{CANVAS_NAME} must be a CanvasPanel")

designer_canvas.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
canvas_slot = designer_canvas.get_editor_property("slot")
if not isinstance(canvas_slot, unreal.VerticalBoxSlot):
    raise RuntimeError(f"{CANVAS_NAME} must be hosted by AudioPanel")
fill_size = unreal.SlateChildSize()
fill_size.set_editor_property("value", 1.0)
fill_size.set_editor_property("size_rule", unreal.SlateSizeRule.FILL)
canvas_slot.set_editor_property("size", fill_size)
canvas_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
canvas_slot.set_editor_property(
    "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
)
canvas_slot.set_editor_property(
    "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
)

moved_names = []
for name, layout in INITIAL_LAYOUTS.items():
    widget = widgets.get(name)
    if widget is None:
        raise RuntimeError(f"Missing authored audio control: {name}")
    if widget.get_parent() is designer_canvas:
        # A designer may already have moved this after the migration.
        continue
    moved = toolset.call_method("MoveWidget", args=(blueprint, widget, designer_canvas, -1))
    if moved.widget is None:
        raise RuntimeError(f"Unable to move {name} into {CANVAS_NAME}")
    set_canvas_layout(widget, *layout)
    moved_names.append(name)

widgets = widget_map(toolset, blueprint)
for row_name in LEGACY_VISIBLE_ROWS:
    row = widgets.get(row_name)
    if row is None:
        continue
    if row.get_children_count() != 0:
        raise RuntimeError(f"Legacy audio row still owns children: {row_name}")
    if not toolset.call_method("RemoveWidget", args=(blueprint, row)):
        raise RuntimeError(f"Unable to remove empty legacy row: {row_name}")

# The logical controls extend to y=374. The wrapper is nonvisual, so enlarging
# it only prevents Designer/runtime clipping and does not affect panel art.
audio_panel_slot = audio_panel.get_editor_property("slot")
if isinstance(audio_panel_slot, unreal.CanvasPanelSlot) and created_canvas:
    current_size = audio_panel_slot.get_size()
    audio_panel_slot.set_size(unreal.Vector2D(current_size.x, max(current_size.y, 374.0)))

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError(f"Failed to compile Settings widget: {ASSET_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Settings widget: {ASSET_PATH}")

unreal.log(
    f"[Plan86DesignerCanvas] canvas_created={created_canvas} "
    f"moved={','.join(moved_names) if moved_names else '<preserved>'}"
)
