import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat"
SETTINGS_ICON_PATH = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/StartMenu/T_UI_Start_SettingsIcon"


def make_slate_size(x, y):
    size = unreal.DeprecateSlateVector2D()
    size.set_editor_property("x", x)
    size.set_editor_property("y", y)
    return size


PAUSE_BUTTON_SIZE = make_slate_size(421.0, 87.0)


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def get_tree(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,))


def widget_map(toolset, blueprint):
    return {str(info.widget_name): info.widget for info in get_tree(toolset, blueprint).widgets if info.widget}


def mark_variable(toolset, blueprint, widget):
    toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method(
        "AddWidget",
        args=(blueprint, widget_class, name, parent, child_index),
    )
    if info.widget is None:
        raise RuntimeError(f"Unable to add widget {name}")
    return info.widget


def wrap_label_in_overlay(toolset, blueprint, label):
    wrapped = toolset.call_method(
        "WrapWidgets",
        args=(blueprint, [label], unreal.Overlay),
    )
    if not wrapped or wrapped[0].widget is None:
        raise RuntimeError(f"Unable to wrap {label.get_name()} in an Overlay")
    return wrapped[0].widget


def set_zero_button_padding(button):
    content = button.get_child_at(0)
    if content is None:
        return
    slot = content.get_editor_property("slot")
    if slot is not None:
        slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)


def add_button_art(toolset, blueprint, widgets, button_name, label_name, art_specs):
    button = widgets[button_name]
    overlay = button.get_child_at(0)
    if overlay is None or not isinstance(overlay, unreal.Overlay):
        overlay = wrap_label_in_overlay(toolset, blueprint, widgets[label_name])
    for art_name, texture_path in art_specs:
        widgets = widget_map(toolset, blueprint)
        image = widgets.get(art_name)
        if image is None:
            image = add_widget(toolset, blueprint, unreal.Image, art_name, overlay)
        mark_variable(toolset, blueprint, image)
        image.set_brush_from_texture(load_required(texture_path), True)
        brush = image.get_editor_property("brush")
        brush.set_editor_property("image_size", PAUSE_BUTTON_SIZE)
        image.set_editor_property("brush", brush)
        image.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
        image_slot = image.get_editor_property("slot")
        image_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
        image_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
    set_zero_button_padding(button)
    return widget_map(toolset, blueprint)


blueprint = load_required(ASSET_PATH)
toolset = unreal.UMGToolSet.get_default_object()
widgets = widget_map(toolset, blueprint)
root_canvas = widgets["CanvasPanel_0"]

if "ArtPauseDimmer" not in widgets:
    dimmer = add_widget(toolset, blueprint, unreal.Image, "ArtPauseDimmer", root_canvas, 0)
else:
    dimmer = widgets["ArtPauseDimmer"]
mark_variable(toolset, blueprint, dimmer)
dimmer.set_brush_from_texture(load_required("/Engine/EngineResources/WhiteSquareTexture"), True)
dimmer.set_editor_property("color_and_opacity", unreal.LinearColor(0.0, 0.0, 0.0, 0.62))
dimmer.set_editor_property("visibility", unreal.SlateVisibility.HIDDEN)
dimmer_slot = dimmer.get_editor_property("slot")
dimmer_slot.set_editor_property(
    "layout_data",
    unreal.AnchorData(
        offsets=unreal.Margin(0.0, 0.0, 0.0, 0.0),
        anchors=unreal.Anchors(
            minimum=unreal.Vector2D(0.0, 0.0),
            maximum=unreal.Vector2D(1.0, 1.0),
        ),
        alignment=unreal.Vector2D(0.0, 0.0),
    ),
)

widgets = add_button_art(
    toolset,
    blueprint,
    widgets,
    "ResumeButton",
    "ResumeButtonLabel",
    [
        ("ArtPauseResume", f"{TEXTURE_ROOT}/T_UI_Pause_Resume"),
        ("ArtPauseSaveAndExit", f"{TEXTURE_ROOT}/T_UI_Pause_SaveAndExit"),
    ],
)
widgets = add_button_art(
    toolset,
    blueprint,
    widgets,
    "RestartButton",
    "RestartButtonLabel",
    [
        ("ArtPauseExitToMenu", f"{TEXTURE_ROOT}/T_UI_Pause_ExitToMenu"),
        ("ArtPauseExitWithoutSave", f"{TEXTURE_ROOT}/T_UI_Pause_ExitWithoutSave"),
    ],
)
widgets = add_button_art(
    toolset,
    blueprint,
    widgets,
    "QuitButton",
    "QuitButtonText",
    [
        ("ArtPauseExitGame", f"{TEXTURE_ROOT}/T_UI_Pause_ExitGame"),
        ("ArtPauseBack", f"{TEXTURE_ROOT}/T_UI_Pause_Back"),
    ],
)

widgets = widget_map(toolset, blueprint)
pause_settings_button = widgets.get("PauseSettingsButton")
if pause_settings_button is None:
    pause_settings_button = add_widget(
        toolset,
        blueprint,
        unreal.Button,
        "PauseSettingsButton",
        root_canvas,
    )
mark_variable(toolset, blueprint, pause_settings_button)
widgets = widget_map(toolset, blueprint)
settings_art = widgets.get("ArtPauseSettings")
if settings_art is None:
    settings_art = add_widget(
        toolset,
        blueprint,
        unreal.Image,
        "ArtPauseSettings",
        pause_settings_button,
    )
mark_variable(toolset, blueprint, settings_art)
settings_art.set_brush_from_texture(load_required(SETTINGS_ICON_PATH), True)
settings_art.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
set_zero_button_padding(pause_settings_button)
pause_settings_button.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
pause_settings_button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
settings_slot = pause_settings_button.get_editor_property("slot")
settings_slot.set_editor_property(
    "layout_data",
    unreal.AnchorData(
        offsets=unreal.Margin(0.0, 0.0, 86.0, 86.0),
        anchors=unreal.Anchors(
            minimum=unreal.Vector2D(0.936, 0.068),
            maximum=unreal.Vector2D(0.936, 0.068),
        ),
        alignment=unreal.Vector2D(0.5, 0.5),
    ),
)

for button_name in ("ResumeButton", "RestartButton", "QuitButton"):
    button = widget_map(toolset, blueprint)[button_name]
    button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
    button_slot = button.get_editor_property("slot")
    button_slot.set_editor_property("padding", unreal.Margin(4.0, 6.0, 4.0, 6.0))

widgets = widget_map(toolset, blueprint)
title = widgets["TitleText"]
title.set_editor_property("justification", unreal.TextJustify.CENTER)
title_font = title.get_editor_property("font")
title_font.size = 52
title.set_editor_property("font", title_font)

message = widgets["MessageText"]
message.set_editor_property("justification", unreal.TextJustify.CENTER)
message_font = message.get_editor_property("font")
message_font.size = 24
message.set_editor_property("font", message_font)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoRestart failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoRestart failed to save")

final_widgets = widget_map(toolset, blueprint)
required = {
    "ArtPauseDimmer",
    "ArtPauseResume",
    "ArtPauseExitToMenu",
    "ArtPauseExitGame",
    "ArtPauseSaveAndExit",
    "ArtPauseExitWithoutSave",
    "ArtPauseBack",
    "PauseSettingsButton",
    "ArtPauseSettings",
}
missing = sorted(required.difference(final_widgets))
if missing:
    raise RuntimeError(f"Pause widget is missing required presentation widgets: {missing}")
for art_name in required.difference({"ArtPauseDimmer", "PauseSettingsButton", "ArtPauseSettings"}):
    art = final_widgets[art_name]
    image_size = art.get_editor_property("brush").get_editor_property("image_size")
    if image_size.get_editor_property("x") < 420.0 or image_size.get_editor_property("y") < 86.0:
        raise RuntimeError(f"{art_name} has collapsed brush size: {image_size}")
unreal.log(f"[Plan45Pause] configured {ASSET_PATH}; widget_count={len(final_widgets)}")
