"""Import the formal pause button art and simplify WBP_ReEchoRestart.

The authored WBP owns layout. C++ only changes state, labels, visibility, and
interaction; it must not reposition the pause composition at runtime.
"""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InteractionPlaceholder"
    / "Elements"
    / "PauseAndCombat"
)
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat"
WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
TEXTURES = {
    "暂停按钮浅.png": "T_UI_Pause_ButtonLight",
    "暂停按钮深.png": "T_UI_Pause_ButtonDark",
}
BUTTON_ART = (
    ("ResumeButton", "Overlay_0", "ArtPausePrimaryButton", "T_UI_Pause_ButtonLight"),
    ("RestartButton", "Overlay_1", "ArtPauseSecondaryButton", "T_UI_Pause_ButtonDark"),
    ("QuitButton", "Overlay_2", "ArtPauseTertiaryButton", "T_UI_Pause_ButtonDark"),
)
LEGACY_ART = (
    "ArtPauseResume",
    "ArtPauseSaveAndExit",
    "ArtPauseExitToMenu",
    "ArtPauseExitWithoutSave",
    "ArtPauseExitGame",
    "ArtPauseBack",
)


def slate_size(x: float, y: float):
    size = unreal.DeprecateSlateVector2D()
    size.set_editor_property("x", x)
    size.set_editor_property("y", y)
    return size


def import_texture(source_name: str, asset_name: str) -> unreal.Texture2D:
    source_path = SOURCE_DIR / source_name
    if not source_path.is_file():
        raise RuntimeError(f"Missing formal pause source art: {source_path}")
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_DIR
    task.destination_name = asset_name
    task.filename = str(source_path)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f"{DESTINATION_DIR}/{asset_name}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Failed to import pause texture: {asset_name}")
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save pause texture: {asset_name}")
    return texture


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


textures = {name: import_texture(source, name) for source, name in TEXTURES.items()}
toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing pause widget: {WIDGET_PATH}")

widgets = widget_map(toolset, blueprint)
for legacy_name in LEGACY_ART:
    legacy = widgets.get(legacy_name)
    if legacy is not None and not toolset.call_method("RemoveWidget", args=(blueprint, legacy)):
        raise RuntimeError(f"Failed to remove legacy pause art: {legacy_name}")

widgets = widget_map(toolset, blueprint)
for button_name, overlay_name, art_name, texture_name in BUTTON_ART:
    button = widgets.get(button_name)
    overlay = widgets.get(overlay_name)
    if not isinstance(button, unreal.Button) or not isinstance(overlay, unreal.Overlay):
        raise RuntimeError(f"Missing authored pause button structure: {button_name}/{overlay_name}")

    art = widgets.get(art_name)
    if art is None:
        info = toolset.call_method(
            "AddWidget", args=(blueprint, unreal.Image, art_name, overlay, 0)
        )
        art = info.widget
    if not isinstance(art, unreal.Image):
        raise RuntimeError(f"Failed to create pause art: {art_name}")
    toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, art, True))
    art.set_brush_from_texture(textures[texture_name], True)
    brush = art.get_editor_property("brush")
    brush.set_editor_property("image_size", slate_size(421.0, 141.0))
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    art.set_editor_property("brush", brush)
    art.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    art_slot = art.get_editor_property("slot")
    art_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    art_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)

    style = button.get_editor_property("widget_style")
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        state_brush = style.get_editor_property(brush_name)
        state_brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        style.set_editor_property(brush_name, state_brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0))

widgets = widget_map(toolset, blueprint)
default_labels = {
    "ResumeButtonLabel": "继续游戏",
    "RestartButtonLabel": "退出至主菜单",
    "QuitButtonText": "退出游戏",
}
for label_name, default_text in default_labels.items():
    label = widgets.get(label_name)
    if not isinstance(label, unreal.TextBlock):
        raise RuntimeError(f"Missing pause button label: {label_name}")
    label.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    label.set_editor_property("text", default_text)
    label.set_editor_property("justification", unreal.TextJustify.CENTER)
    label.set_editor_property(
        "color_and_opacity", unreal.SlateColor(unreal.LinearColor(0.12, 0.055, 0.02, 1.0))
    )
    label.set_editor_property("shadow_offset", unreal.Vector2D(0.0, 0.0))
    font = label.get_editor_property("font")
    font.size = 34
    label.set_editor_property("font", font)
    slot = label.get_editor_property("slot")
    slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
    slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)

# Move the authored composition to the former runtime position. This keeps the
# exact target placement visible and editable in Designer without C++ offsets.
root_panel = widgets.get("RootPanel")
if not isinstance(root_panel, unreal.VerticalBox):
    raise RuntimeError("Missing authored pause RootPanel")
root_slot = root_panel.get_editor_property("slot")
root_slot.set_editor_property(
    "layout_data",
    unreal.AnchorData(
        offsets=unreal.Margin(0.0, 0.0, 421.0, 610.0),
        anchors=unreal.Anchors(
            minimum=unreal.Vector2D(0.5, 0.493),
            maximum=unreal.Vector2D(0.5, 0.493),
        ),
        alignment=unreal.Vector2D(0.5, 0.5),
    ),
)

# Seed Designer with the normal pause state. The native fallback SettingsButton
# remains in the class for a root-less widget, but the authored WBP uses the
# independently positioned top-right PauseSettingsButton.
message = widgets.get("MessageText")
if isinstance(message, unreal.TextBlock):
    message.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
fallback_settings = widgets.get("SettingsButton")
if isinstance(fallback_settings, unreal.Button):
    fallback_settings.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
pause_settings = widgets.get("PauseSettingsButton")
if isinstance(pause_settings, unreal.Button):
    pause_settings.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoRestart failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoRestart failed to save")
unreal.log("[Plan86Pause] formal pause visuals authored successfully")
