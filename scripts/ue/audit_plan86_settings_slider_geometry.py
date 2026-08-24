import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
NAMES = (
    "MasterAudioRow",
    "MasterVolumeVisualOverlay",
    "MasterVolumeTrack",
    "MasterVolumeFill",
    "MasterVolumeSlider",
    "MusicAudioRow",
    "MusicVolumeVisualOverlay",
    "MusicVolumeTrack",
    "MusicVolumeFill",
    "MusicVolumeSlider",
    "CombatSfxAudioRow",
    "CombatVolumeVisualOverlay",
    "CombatVolumeTrack",
    "CombatVolumeFill",
    "CombatSfxVolumeSlider",
    "GraphicsBrightnessTrack",
    "GraphicsBrightnessFill",
    "GraphicsDropdown0",
    "GraphicsDropdown1",
    "AudioOutputRow",
    "AudioOutputLabel",
    "AudioOutputField",
    "AudioOutputFieldBackground",
    "RestoreDefaultsButton",
    "ApplyAndReturnButton",
)


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget: {ASSET_PATH}")
widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}

for name in NAMES:
    widget = widgets.get(name)
    if widget is None:
        unreal.log(f"[Plan86SliderGeometry] {name}: MISSING")
        continue
    parent = widget.get_parent()
    slot = widget.get_editor_property("slot")
    details = [
        f"type={widget.get_class().get_name()}",
        f"parent={parent.get_name() if parent else '<none>'}",
        f"slot={slot.get_class().get_name() if slot else '<none>'}",
        f"render_transform={widget.get_editor_property('render_transform')}",
        f"render_opacity={widget.get_editor_property('render_opacity')}",
    ]
    if isinstance(slot, (unreal.OverlaySlot, unreal.HorizontalBoxSlot, unreal.VerticalBoxSlot)):
        details.extend(
            (
                f"padding={slot.get_editor_property('padding')}",
                f"halign={slot.get_editor_property('horizontal_alignment')}",
                f"valign={slot.get_editor_property('vertical_alignment')}",
            )
        )
    if isinstance(slot, unreal.HorizontalBoxSlot):
        details.append(f"size={slot.get_editor_property('size')}")
    if isinstance(slot, unreal.CanvasPanelSlot):
        details.append(f"layout={slot.get_editor_property('layout_data')}")
    if isinstance(widget, unreal.Image):
        brush = widget.get_editor_property("brush")
        details.extend(
            (
                f"brush_size={brush.get_editor_property('image_size')}",
                f"brush_resource={brush.get_editor_property('resource_object')}",
            )
        )
    if isinstance(widget, unreal.Slider):
        style = widget.get_editor_property("widget_style")
        thumb = style.get_editor_property("normal_thumb_image")
        details.extend(
            (
                f"indent={widget.get_editor_property('indent_handle')}",
                f"thumb_size={thumb.get_editor_property('image_size')}",
                f"thumb_resource={thumb.get_editor_property('resource_object')}",
            )
        )
    if name in ("RestoreDefaultsButton", "ApplyAndReturnButton"):
        details.append(f"lock_api={[item for item in dir(widget) if 'lock' in item.lower()]}")
    unreal.log(f"[Plan86SliderGeometry] {name}: " + " | ".join(details))
