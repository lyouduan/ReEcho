"""Author a fourth Settings tab and its WYSIWYG difficulty page.

The Blueprint owns tab/page geometry, labels, fonts, and visual layers. Runtime
code only switches category visibility, binds clicks, and refreshes selection.
"""

import unreal


SETTINGS_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
RESTART_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
PAUSE_TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat"
BUTTON_LIGHT_PATH = f"{PAUSE_TEXTURE_ROOT}/T_UI_Pause_ButtonLight"
BUTTON_DARK_PATH = f"{PAUSE_TEXTURE_ROOT}/T_UI_Pause_ButtonDark"
SETTINGS_TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings"
TAB_LIGHT_PATH = f"{SETTINGS_TEXTURE_ROOT}/T_UI_Settings_TabLight"
TAB_DARK_PATH = f"{SETTINGS_TEXTURE_ROOT}/T_UI_Settings_TabDark"

TAB_ROOTS = (
    ("Overlay_Graphic", "GraphicsSettingsButton", "TextBlock_253", "画面", 380.0),
    ("Overlay_Audio", "AudioSettingsButton", "TextBlock", "声音", 590.0),
    ("Overlay_Input", "ControlsSettingsButton", "TextBlock_1", "键位", 800.0),
    ("Overlay_Difficulty", "DifficultySettingsButton", "DifficultyTabText", "难度", 1010.0),
)
BUTTONS = (
    ("DifficultyPartyRoot", "DifficultyPartyButton", "DifficultyPartyArt", "DifficultyPartyText", "派对", 50.0),
    (
        "DifficultyStandardRoot",
        "DifficultyStandardButton",
        "DifficultyStandardArt",
        "DifficultyStandardText",
        "常规",
        210.0,
    ),
    (
        "DifficultyNightmareRoot",
        "DifficultyNightmareButton",
        "DifficultyNightmareArt",
        "DifficultyNightmareText",
        "噩梦",
        370.0,
    ),
)


def widget_infos(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(toolset, blueprint):
    return {info.widget.get_name(): info.widget for info in widget_infos(toolset, blueprint) if info.widget}


def mark_variable(toolset, blueprint, widget):
    infos = {info.widget.get_name(): info for info in widget_infos(toolset, blueprint) if info.widget}
    info = infos.get(widget.get_name())
    if info is not None and not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(toolset, blueprint, widget_class, name, parent, index=-1):
    info = toolset.call_method("AddWidget", args=(blueprint, widget_class, name, parent, index))
    if info.widget is None:
        raise RuntimeError(f"Failed to add widget: {name}")
    return info.widget


def set_canvas_geometry(widget, position, size, z_order):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} must have an authored CanvasPanelSlot")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(position[0], position[1], size[0], size[1]),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0),
                maximum=unreal.Vector2D(0.0, 0.0),
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


def set_overlay_fill(widget, horizontal=unreal.HorizontalAlignment.H_ALIGN_FILL, vertical=unreal.VerticalAlignment.V_ALIGN_FILL):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.OverlaySlot):
        raise RuntimeError(f"{widget.get_name()} must have an OverlaySlot")
    slot.set_editor_property("horizontal_alignment", horizontal)
    slot.set_editor_property("vertical_alignment", vertical)
    slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))


def ensure_widget(toolset, blueprint, widgets, widget_class, name, parent):
    widget = widgets.get(name)
    if widget is not None and widget.get_parent() is not parent:
        if not toolset.call_method("RemoveWidget", args=(blueprint, widget)):
            raise RuntimeError(f"Failed to remove misplaced widget: {name}")
        widget = None
    if widget is None:
        widget = add_widget(toolset, blueprint, widget_class, name, parent)
    if not isinstance(widget, widget_class):
        raise RuntimeError(f"{name} has the wrong type")
    return widget


def apply_button_texture(button, texture):
    style = button.get_editor_property("widget_style")
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property(
            "tint_color", unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        )
        style.set_editor_property(brush_name, brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))


toolset = unreal.UMGToolSet.get_default_object()
settings = unreal.load_asset(SETTINGS_PATH)
restart = unreal.load_asset(RESTART_PATH)
button_light = unreal.load_asset(BUTTON_LIGHT_PATH)
button_dark = unreal.load_asset(BUTTON_DARK_PATH)
tab_light = unreal.load_asset(TAB_LIGHT_PATH)
tab_dark = unreal.load_asset(TAB_DARK_PATH)
if settings is None or restart is None:
    raise RuntimeError("Missing Settings or Restart widget Blueprint")
for texture in (button_light, button_dark, tab_light, tab_dark):
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Missing formal Settings difficulty texture")

widgets = widget_map(toolset, settings)
restart_widgets = widget_map(toolset, restart)
panel = widgets.get("Panel")
settings_layout = widgets.get("SettingsLayoutCanvas")
controls_panel = widgets.get("ControlsPanel")
pause_label_style = restart_widgets.get("ResumeButtonLabel")
tab_label_style = widgets.get("TextBlock_1")
if not isinstance(panel, unreal.CanvasPanel) or not isinstance(settings_layout, unreal.CanvasPanel):
    raise RuntimeError("WBP_ReEchoSettings is missing its authored Panel/SettingsLayoutCanvas")
if not isinstance(controls_panel, unreal.CanvasPanel):
    raise RuntimeError("WBP_ReEchoSettings is missing authored ControlsPanel")
if not isinstance(pause_label_style, unreal.TextBlock) or not isinstance(tab_label_style, unreal.TextBlock):
    raise RuntimeError("Missing authored pause/tab label style source")

# The requested “难度” label is a fourth top tab, not a caption inside the
# key-binding page.
legacy_label = widgets.get("DifficultyLabel")
if legacy_label is not None and not toolset.call_method("RemoveWidget", args=(settings, legacy_label)):
    raise RuntimeError("Failed to remove inline DifficultyLabel")

# Fit four independent Canvas tab roots before the authored close button.
for root_name, button_name, text_name, text_value, x_position in TAB_ROOTS:
    widgets = widget_map(toolset, settings)
    root = ensure_widget(toolset, settings, widgets, unreal.Overlay, root_name, panel)
    set_canvas_geometry(root, (x_position, 56.0), (208.0, 68.0), 0)

    widgets = widget_map(toolset, settings)
    button = ensure_widget(toolset, settings, widgets, unreal.ReEchoIndexedButton, button_name, root)
    mark_variable(toolset, settings, button)
    apply_button_texture(button, tab_light if text_value == "难度" else tab_dark)
    button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)
    set_overlay_fill(button)

    widgets = widget_map(toolset, settings)
    text = ensure_widget(toolset, settings, widgets, unreal.TextBlock, text_name, root)
    text.set_editor_property("text", text_value)
    text.set_editor_property("font", tab_label_style.get_editor_property("font"))
    text.set_editor_property(
        "color_and_opacity", tab_label_style.get_editor_property("color_and_opacity")
    )
    text.set_editor_property("justification", unreal.TextJustify.CENTER)
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_overlay_fill(
        text,
        unreal.HorizontalAlignment.H_ALIGN_CENTER,
        unreal.VerticalAlignment.V_ALIGN_CENTER,
    )

widgets = widget_map(toolset, settings)
difficulty_panel = ensure_widget(
    toolset, settings, widgets, unreal.CanvasPanel, "DifficultyPanel", settings_layout
)
mark_variable(toolset, settings, difficulty_panel)
set_canvas_geometry(difficulty_panel, (0.0, 0.0), (1333.0, 734.0), 7)
difficulty_panel.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

# Save the difficulty page as the Designer preview. Runtime still opens on the
# Graphics category and RefreshCategory owns actual page visibility.
for panel_name in ("GraphicsPanel", "AudioPanel", "ControlsPanel"):
    category_panel = widgets.get(panel_name)
    if category_panel is not None:
        category_panel.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

widgets = widget_map(toolset, settings)
for root_name, button_name, art_name, text_name, label_text, y_position in BUTTONS:
    root = ensure_widget(toolset, settings, widgets, unreal.Overlay, root_name, difficulty_panel)
    set_canvas_geometry(root, (456.0, y_position), (421.0, 141.0), 11)

    widgets = widget_map(toolset, settings)
    button = ensure_widget(toolset, settings, widgets, unreal.ReEchoIndexedButton, button_name, root)
    mark_variable(toolset, settings, button)
    button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
    style = button.get_editor_property("widget_style")
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        style.set_editor_property(brush_name, brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)
    set_overlay_fill(button)

    widgets = widget_map(toolset, settings)
    art = ensure_widget(toolset, settings, widgets, unreal.Image, art_name, root)
    mark_variable(toolset, settings, art)
    art.set_brush_from_texture(button_light if label_text == "常规" else button_dark, True)
    art.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_overlay_fill(art)

    widgets = widget_map(toolset, settings)
    text = ensure_widget(toolset, settings, widgets, unreal.TextBlock, text_name, root)
    text.set_editor_property("text", label_text)
    text.set_editor_property("font", pause_label_style.get_editor_property("font"))
    text.set_editor_property(
        "color_and_opacity", pause_label_style.get_editor_property("color_and_opacity")
    )
    text.set_editor_property("justification", unreal.TextJustify.CENTER)
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_overlay_fill(text)

if not toolset.call_method("CompileWidgetBlueprint", args=(settings,)):
    raise RuntimeError("WBP_ReEchoSettings failed to compile after difficulty authoring")
if not unreal.EditorAssetLibrary.save_loaded_asset(settings, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoSettings failed to save after difficulty authoring")
unreal.log("[SettingsDifficulty] authored fourth tab and independent WYSIWYG difficulty page")
