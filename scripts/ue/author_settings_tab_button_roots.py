"""Make the authored Settings tab overlays own background, label, and input together."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
TAB_ROOTS = (
    ("GraphicsSettingsButton", "Overlay_Graphic", "TextBlock_253"),
    ("AudioSettingsButton", "Overlay_Audio", "TextBlock"),
    ("ControlsSettingsButton", "Overlay_Input", "TextBlock_1"),
)
LEGACY_UNSCOPED_GRAPHICS_ROWS = (
    "HorizontalBox_128",
    "HorizontalBox",
    "HorizontalBox_1",
    "HorizontalBox_2",
    "HorizontalBox_3",
)
CONTENT_LABELS = (
    "GraphicsLabel0", "GraphicsLabel1", "GraphicsLabel2", "GraphicsLabel3", "GraphicsLabel4",
    "MasterVolumeLabel", "MusicVolumeLabel", "CombatSfxVolumeLabel", "AudioOutputLabel",
    "ControlsLabel0", "ControlsLabel1", "ControlsLabel2", "ControlsLabel3", "ControlsLabel4", "ControlsLabel5",
)
CONTENT_VALUES = (
    "GraphicsValue0", "GraphicsValue1", "GraphicsValue2", "GraphicsValue3", "GraphicsValue4",
    "MasterVolumePercent", "MusicVolumePercent", "CombatVolumePercent", "AudioOutputValue",
    "ControlsValue0", "ControlsValue1", "ControlsValue2", "ControlsValue3", "ControlsValue4", "ControlsValue5",
)


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget blueprint: {ASSET_PATH}")

infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {info.widget.get_name(): info.widget for info in infos if info.widget}

# The authored WBP owns the title and two-layer panel composition. Legacy
# compatibility title/panel nodes were deleted from the normal authoring tree;
# C++ creates its own minimum fallback only when the asset has no root.
authored_title = widgets.get("Settings")
panel_light = widgets.get("PanelLight")
panel_edge = widgets.get("PanelEdge")
if not isinstance(authored_title, unreal.TextBlock):
    raise RuntimeError("Missing authored Settings title: Settings")
if not isinstance(panel_light, unreal.Image) or not isinstance(panel_edge, unreal.Image):
    raise RuntimeError("Missing authored Settings panel layers")

authored_title.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
panel_light.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
panel_edge.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
unreal.log(
    "[SettingsTabs] authored Settings title and PanelLight/PanelEdge retained"
)

close_overlay = widgets.get("Overlay_Close")
close_icon = widgets.get("CloseIcon")
bound_close_button = widgets.get("SettingsCloseButton")
if not isinstance(close_overlay, unreal.Overlay):
    raise RuntimeError("Missing authored Settings close root: Overlay_Close")
if not isinstance(close_icon, unreal.Image):
    raise RuntimeError("Missing authored Settings close icon: CloseIcon")
if not isinstance(bound_close_button, unreal.Button):
    raise RuntimeError("Missing bound Settings close button: SettingsCloseButton")

bound_close_button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
old_close_parent = bound_close_button.get_parent()
if old_close_parent is not close_overlay:
    if old_close_parent is None or not old_close_parent.remove_child(bound_close_button):
        raise RuntimeError("Could not detach SettingsCloseButton from its old parent")

if close_icon.get_parent() is close_overlay and not close_overlay.remove_child(close_icon):
    raise RuntimeError("Could not reorder CloseIcon")
if bound_close_button.get_parent() is not close_overlay:
    close_button_slot = close_overlay.add_child_to_overlay(bound_close_button)
else:
    close_button_slot = bound_close_button.get_editor_property("slot")
close_button_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
close_button_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
close_button_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
bound_close_button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)

close_icon_slot = close_overlay.add_child_to_overlay(close_icon)
close_icon_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
close_icon_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
close_icon_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
close_icon.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
unreal.log(
    "[SettingsTabs] SettingsCloseButton is the input layer in Overlay_Close; "
    "CloseIcon remains the authored visual"
)

audio_panel = widgets.get("AudioPanel")
if not isinstance(audio_panel, unreal.VerticalBox):
    raise RuntimeError("Missing authored Settings audio root: AudioPanel")
audio_panel_slot = audio_panel.get_editor_property("slot")
if not isinstance(audio_panel_slot, unreal.CanvasPanelSlot):
    raise RuntimeError("AudioPanel must remain authored in SettingsLayoutCanvas")
audio_panel_slot.set_position(unreal.Vector2D(111.0, 74.0))
audio_designer_canvas = widgets.get("AudioDesignerCanvas")
if isinstance(audio_designer_canvas, unreal.CanvasPanel):
    # Plan86 migrated the visible controls to CanvasPanelSlots. Their positions
    # now belong to the artist; this general authoring script must not reset
    # them on a later run.
    audio_panel_slot.set_size(unreal.Vector2D(1170.0, 374.0))
    unreal.log("[SettingsTabs] preserved designer-owned AudioDesignerCanvas positions")
else:
    audio_panel_slot.set_size(unreal.Vector2D(1170.0, 335.0))
    audio_rows = ("MasterAudioRow", "MusicAudioRow", "CombatSfxAudioRow", "AudioOutputRow")
    audio_labels = ("MasterVolumeLabel", "MusicVolumeLabel", "CombatSfxVolumeLabel", "AudioOutputLabel")
    for row_index, row_name in enumerate(audio_rows):
        row = widgets.get(row_name)
        if not isinstance(row, unreal.HorizontalBox):
            raise RuntimeError(f"Missing authored Settings audio row: {row_name}")
        row_slot = row.get_editor_property("slot")
        if not isinstance(row_slot, unreal.VerticalBoxSlot):
            raise RuntimeError(f"Unexpected slot for Settings audio row: {row_name}")
        row_slot.set_editor_property(
            "padding", unreal.Margin(0.0, 0.0, 0.0, 0.0 if row_index == len(audio_rows) - 1 else 40.0)
        )

    for label_name in audio_labels:
        label = widgets.get(label_name)
        if not isinstance(label, unreal.TextBlock):
            raise RuntimeError(f"Missing authored Settings audio label: {label_name}")
        label_slot = label.get_editor_property("slot")
        if not isinstance(label_slot, unreal.HorizontalBoxSlot):
            raise RuntimeError(f"Unexpected slot for Settings audio label: {label_name}")
        label_slot.set_editor_property("padding", unreal.Margin(5.0, 0.0, 0.0, 0.0))

    audio_output_field = widgets.get("AudioOutputField")
    if isinstance(audio_output_field, unreal.Overlay):
        audio_output_slot = audio_output_field.get_editor_property("slot")
        if isinstance(audio_output_slot, unreal.HorizontalBoxSlot):
            fixed_output_size = unreal.SlateChildSize()
            fixed_output_size.set_editor_property("value", 1.0)
            fixed_output_size.set_editor_property("size_rule", unreal.SlateSizeRule.AUTOMATIC)
            audio_output_slot.set_editor_property("size", fixed_output_size)
            audio_output_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 178.0, 0.0))
    unreal.log("[SettingsTabs] aligned legacy AudioPanel rows to the GraphicsPanel grid")

# These five original mock-up rows sit directly under Panel, outside every
# category container. Leaving them visible means they remain behind AudioPanel
# and ControlsPanel after a tab switch. The named GraphicsPanel/AudioPanel/
# ControlsPanel nodes are the authored, interactive category roots.
for row_name in LEGACY_UNSCOPED_GRAPHICS_ROWS:
    row = widgets.get(row_name)
    if row is None:
        continue
    if not isinstance(row, unreal.HorizontalBox):
        raise RuntimeError(f"Unexpected legacy Settings row type: {row_name}")
    if row.get_parent() is not widgets.get("Panel"):
        raise RuntimeError(f"Unexpected parent for legacy Settings row: {row_name}")
    row.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
unreal.log("[SettingsTabs] collapsed five unscoped legacy graphics rows")

content_style_source = widgets.get("GraphicsLabel0")
if not isinstance(content_style_source, unreal.TextBlock):
    raise RuntimeError("Missing authored Settings content style source: GraphicsLabel0")
authored_font = content_style_source.get_editor_property("font")
authored_color = content_style_source.get_editor_property("color_and_opacity")
for text_name in CONTENT_LABELS + CONTENT_VALUES:
    text_widget = widgets.get(text_name)
    if not isinstance(text_widget, unreal.TextBlock):
        raise RuntimeError(f"Missing authored Settings content text: {text_name}")
    text_widget.set_editor_property("font", authored_font)
    text_widget.set_editor_property("color_and_opacity", authored_color)
for text_name in CONTENT_VALUES:
    widgets[text_name].set_render_translation(unreal.Vector2D(0.0, -8.0))
unreal.log(
    "[SettingsTabs] three category pages now inherit the authored WBP content font and color; "
    "existing WBP slots remain the layout authority and field values use the authored vertical offset"
)

for button_name, overlay_name, authored_label_name in TAB_ROOTS:
    button = widgets.get(button_name)
    overlay = widgets.get(overlay_name)
    authored_label = widgets.get(authored_label_name)
    if not isinstance(button, unreal.ReEchoIndexedButton):
        raise RuntimeError(f"Missing Settings tab button: {button_name}")
    if not isinstance(overlay, unreal.Overlay):
        raise RuntimeError(f"Missing Settings tab visual root: {overlay_name}")
    if not isinstance(authored_label, unreal.TextBlock):
        raise RuntimeError(f"Missing Settings authored tab label: {authored_label_name}")

    old_parent = button.get_parent()
    if old_parent is not overlay:
        if old_parent is None or not old_parent.remove_child(button):
            raise RuntimeError(f"Could not detach {button_name} from its old parent")

    # Re-add the authored label after the button so it is always painted above
    # the button background while remaining outside the input path.
    if authored_label.get_parent() is overlay:
        if not overlay.remove_child(authored_label):
            raise RuntimeError(f"Could not reorder {authored_label_name}")

    if button.get_parent() is not overlay:
        button_slot = overlay.add_child_to_overlay(button)
    else:
        button_slot = button.get_editor_property("slot")
    button_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    button_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
    button_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))

    label_slot = overlay.add_child_to_overlay(authored_label)
    label_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
    label_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    label_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    authored_label.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    unreal.log(
        f"[SettingsTabs] {button_name} moved into {overlay_name}; "
        f"authored label {authored_label_name} is the top visual"
    )

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError(f"Failed to compile Settings widget: {ASSET_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Settings widget: {ASSET_PATH}")
unreal.log("[SettingsTabs] WBP_ReEchoSettings authored tab roots compiled and saved")
