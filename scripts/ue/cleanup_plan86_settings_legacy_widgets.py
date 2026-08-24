"""Remove superseded authored nodes from WBP_ReEchoSettings.

The normal Settings presentation now lives in SettingsLayoutCanvas plus the
authored Panel overlays.  These nodes were retained collapsed during earlier
migrations and only make Designer authoring ambiguous.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
LEGACY_ROOTS = (
    "ArtSettingsPanel",
    "RootPanel",
    "CategoryTitleText",
    "DetailText",
    "RestoreDefaultsButtonLabel",
    "ApplyAndReturnButtonLabel",
    "GraphicsPlaceholderNotice",
    "ControlsPlaceholderNotice",
    "Graphic_1",
    "Graphic",
    "Audio_1",
    "Audio",
    "Input_1",
    "Input",
    "GraphicsSettingsButtonLabel",
    "AudioSettingsButtonLabel",
    "ControlsSettingsButtonLabel",
    "HorizontalBox_128",
    "HorizontalBox",
    "HorizontalBox_1",
    "HorizontalBox_2",
    "HorizontalBox_3",
    # Superseded by the compact three-slider audio layout.  Removing the
    # complete rows also removes their English labels, hidden sliders and
    # mute checkboxes without touching the active designer-owned controls.
    "AmbienceAudioRow",
    "UiSfxAudioRow",
    "DiagnosticToneRow",
)
PRESERVED_DESIGNER_BUTTONS = (
    "RestoreDefaultsButton",
    "ApplyAndReturnButton",
)


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget: {ASSET_PATH}")

widgets = widget_map(toolset, blueprint)
button_geometry = {}
for name in PRESERVED_DESIGNER_BUTTONS:
    button = widgets.get(name)
    if not isinstance(button, unreal.Button):
        raise RuntimeError(f"Missing authored Settings button: {name}")
    slot = button.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{name} must remain directly editable in a Canvas slot")
    button_geometry[name] = (slot.get_position(), slot.get_size())

removed = []
for name in LEGACY_ROOTS:
    widget = widgets.get(name)
    if widget is None:
        unreal.log(f"[Plan86Cleanup] already absent: {name}")
        continue
    if not toolset.call_method("RemoveWidget", args=(blueprint, widget)):
        raise RuntimeError(f"Could not remove legacy Settings widget: {name}")
    removed.append(name)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError(f"Failed to compile Settings widget: {ASSET_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Settings widget: {ASSET_PATH}")

widgets = widget_map(toolset, blueprint)
for name in LEGACY_ROOTS:
    if name in widgets:
        raise RuntimeError(f"Legacy Settings widget survived cleanup: {name}")
for name, (expected_position, expected_size) in button_geometry.items():
    button = widgets.get(name)
    slot = button.get_editor_property("slot") if button else None
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Designer button lost its Canvas slot during cleanup: {name}")
    if slot.get_position() != expected_position or slot.get_size() != expected_size:
        raise RuntimeError(f"Designer button geometry changed during cleanup: {name}")

unreal.log(
    f"[Plan86Cleanup] removed {len(removed)} legacy roots; "
    "RestoreDefaultsButton and ApplyAndReturnButton geometry preserved"
)
