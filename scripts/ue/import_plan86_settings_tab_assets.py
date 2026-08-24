"""Import Plan86 Settings tab paper-note art and seed the authored default tab state."""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InteractionPlaceholder"
    / "Elements"
    / "Settings"
)
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings"
TEXTURES = {
    "标签浅.png": "T_UI_Settings_TabLight",
    "标签深.png": "T_UI_Settings_TabDark",
    "滑动条把手.png": "T_UI_Settings_SliderThumb",
    "设置按钮浅.png": "T_UI_Settings_ActionButtonLight",
    "设置按钮深.png": "T_UI_Settings_ActionButtonDark",
}
SETTINGS_WIDGET = "/Game/ReEcho/UI/WBP_ReEchoSettings"
TAB_BUTTONS = (
    ("GraphicsSettingsButton", "T_UI_Settings_TabLight"),
    ("AudioSettingsButton", "T_UI_Settings_TabDark"),
    ("ControlsSettingsButton", "T_UI_Settings_TabDark"),
)
ACTION_BUTTONS = (
    ("RestoreDefaultsButton", "T_UI_Settings_ActionButtonLight"),
    ("ApplyAndReturnButton", "T_UI_Settings_ActionButtonDark"),
)
AUTHORED_SLIDERS = (
    "MasterVolumeSlider",
    "MusicVolumeSlider",
    "CombatSfxVolumeSlider",
)
AUDIO_VISUAL_OVERLAYS = (
    "MasterVolumeVisualOverlay",
    "MusicVolumeVisualOverlay",
    "CombatVolumeVisualOverlay",
)


def import_texture(source_name: str, asset_name: str) -> unreal.Texture2D:
    source_path = SOURCE_DIR / source_name
    if not source_path.is_file():
        raise RuntimeError(f"Missing Plan86 Settings tab source: {source_path}")

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_DIR
    task.destination_name = asset_name
    task.filename = str(source_path)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = f"{DESTINATION_DIR}/{asset_name}"
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Failed to import Settings tab texture: {asset_path}")
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Settings tab texture: {asset_path}")
    return texture


def make_slate_size(x: float, y: float):
    size = unreal.DeprecateSlateVector2D()
    size.set_editor_property("x", x)
    size.set_editor_property("y", y)
    return size


textures = {asset_name: import_texture(source_name, asset_name) for source_name, asset_name in TEXTURES.items()}

toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(SETTINGS_WIDGET)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget blueprint: {SETTINGS_WIDGET}")
widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}

for button_name, texture_name in TAB_BUTTONS:
    button = widgets.get(button_name)
    if not isinstance(button, unreal.ReEchoIndexedButton):
        raise RuntimeError(f"Missing Settings tab button: {button_name}")
    style = button.get_editor_property("widget_style")
    texture = textures[texture_name]
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        style.set_editor_property(brush_name, brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    parent = button.get_parent()
    if isinstance(parent, unreal.PanelWidget):
        for child_index in range(parent.get_children_count()):
            child = parent.get_child_at(child_index)
            child_detail = f"{child.get_name()}:{child.get_class().get_name()}"
            if isinstance(child, unreal.Image):
                child_brush = child.get_editor_property("brush")
                child_detail += (
                    f":visibility={child.get_editor_property('visibility')}"
                    f":resource={child_brush.get_editor_property('resource_object')}"
                )
            unreal.log(f"[Plan86][TabAudit] {button_name} child[{child_index}]={child_detail}")

for button_name, texture_name in ACTION_BUTTONS:
    button = widgets.get(button_name)
    if not isinstance(button, unreal.Button):
        raise RuntimeError(f"Missing Settings action button: {button_name}")
    style = button.get_editor_property("widget_style")
    texture = textures[texture_name]
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)))
        style.set_editor_property(brush_name, brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
unreal.log("[Plan86] Applied the delivered light/dark art to the two Settings action buttons")

slider_thumb = textures["T_UI_Settings_SliderThumb"]
for overlay_name in AUDIO_VISUAL_OVERLAYS:
    overlay = widgets.get(overlay_name)
    if not isinstance(overlay, unreal.Overlay):
        raise RuntimeError(f"Missing authored Settings volume overlay: {overlay_name}")
    row_slot = overlay.get_editor_property("slot")
    if not isinstance(row_slot, unreal.HorizontalBoxSlot):
        raise RuntimeError(f"Settings volume overlay is not in a HorizontalBox: {overlay_name}")
    # The graphics brightness track is an authored 802 px Canvas child. These
    # audio overlays previously used Fill, so the row stretched them beyond the
    # 802 px track/fill art and the thumb only happened to meet the fill around
    # the middle of its range. Automatic preserves the art's authored 802 px
    # desired width and therefore gives the audio sliders the same geometry as
    # the already-correct brightness slider.
    child_size = unreal.SlateChildSize()
    child_size.set_editor_property("value", 1.0)
    child_size.set_editor_property("size_rule", unreal.SlateSizeRule.AUTOMATIC)
    row_slot.set_editor_property("size", child_size)

for slider_name in AUTHORED_SLIDERS:
    slider = widgets.get(slider_name)
    if not isinstance(slider, unreal.Slider):
        raise RuntimeError(f"Missing authored Settings slider: {slider_name}")
    style = slider.get_editor_property("widget_style")
    for bar_brush_name in (
        "normal_bar_image",
        "hovered_bar_image",
        "disabled_bar_image",
    ):
        bar_brush = style.get_editor_property(bar_brush_name)
        bar_brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        style.set_editor_property(bar_brush_name, bar_brush)
    for brush_name in (
        "normal_thumb_image",
        "hovered_thumb_image",
        "disabled_thumb_image",
    ):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("resource_object", slider_thumb)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("image_size", make_slate_size(50.0, 68.0))
        style.set_editor_property(brush_name, brush)
    slider.set_editor_property("widget_style", style)
    # These authored sliders were previously invisible input overlays. Restore
    # only the thumb; the separate track/fill Images remain the visual bar.
    slider.set_editor_property("render_opacity", 1.0)
    slider.set_editor_property("slider_handle_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    # UE 5.8 subtracts two thumb widths from the draggable range when this is
    # enabled. With the delivered 50 px thumb that leaves a visible gap at both
    # ends of the 802 px art track. Disabled keeps the thumb wholly inside while
    # allowing its outer edge to touch 0%/100% exactly.
    slider.set_editor_property("indent_handle", False)
    overlay_slot = slider.get_editor_property("slot")
    if not isinstance(overlay_slot, unreal.OverlaySlot):
        raise RuntimeError(f"Settings slider is not in an Overlay: {slider_name}")
    overlay_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    overlay_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    overlay_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
unreal.log(
    "[Plan86] Applied the delivered thumb art and matched three audio tracks to the authored 802 px geometry"
)

brightness_slider = widgets.get("GraphicsBrightnessSlider")
if isinstance(brightness_slider, unreal.Slider):
    brightness_slider.set_editor_property("indent_handle", False)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError(f"Failed to compile Settings widget: {SETTINGS_WIDGET}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Settings widget: {SETTINGS_WIDGET}")
unreal.log("[Plan86] Settings tabs and slider thumbs authored successfully")
