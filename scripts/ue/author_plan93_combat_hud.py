"""Author the Plan93 Player/Encounter HUD layout in the two existing WBPs.

The operation is idempotent: named Plan93 widgets are reused and every target
slot is reset to the locked 1920x1080 layout. Run only through Unreal Editor or
UnrealEditor-Cmd with ``-ExecutePythonScript``.
"""

import unreal


PLAYER_HUD = "/Game/ReEcho/UI/WBP_ReEchoPlayerHud"
ENCOUNTER_HUD = "/Game/ReEcho/UI/WBP_ReEchoEncounterHud"
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/CombatHud"
OBSOLETE_SKILL_BAR_TEXTURE = f"{TEXTURE_ROOT}/T_UI_CombatHud_SkillBar"
OBSOLETE_TIME_READOUT_TEXTURE = f"{TEXTURE_ROOT}/T_UI_CombatHud_TimeReadout"


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Missing Plan93 asset: {path}")
    return asset


def widget_infos(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(toolset, blueprint):
    return {
        str(info.widget_name): info.widget
        for info in widget_infos(toolset, blueprint)
        if info.widget
    }


def mark_variable(toolset, blueprint, widget):
    infos = {str(info.widget_name): info for info in widget_infos(toolset, blueprint)}
    info = infos.get(widget.get_name())
    if info is not None and not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, child_index)
    )
    if info.widget is None:
        raise RuntimeError(f"Unable to add Plan93 widget: {name}")
    return info.widget


def ensure_widget(
    toolset, blueprint, widgets, widget_class, name, parent, child_index=-1
):
    widget = widgets.get(name)
    if widget is None:
        widget = add_widget(
            toolset, blueprint, widget_class, name, parent, child_index
        )
    if not isinstance(widget, widget_class):
        raise RuntimeError(
            f"Plan93 widget {name} expected {widget_class.__name__}, "
            f"got {widget.get_class().get_name()}"
        )
    return widget


def set_canvas_layout(
    widget,
    x,
    y,
    width,
    height,
    anchor_x=0.0,
    anchor_y=0.0,
    alignment_x=0.0,
    alignment_y=0.0,
    z_order=0,
):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct CanvasPanel child")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, y, width, height),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(anchor_x, anchor_y),
                maximum=unreal.Vector2D(anchor_x, anchor_y),
            ),
            alignment=unreal.Vector2D(alignment_x, alignment_y),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


def fill_panel_slot(widget, padding=unreal.Margin(0.0, 0.0, 0.0, 0.0)):
    slot = widget.get_editor_property("slot")
    if slot is None:
        return
    slot.set_editor_property("padding", padding)
    slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
    )
    slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
    )


def set_image(image, texture):
    image.set_brush_from_texture(texture, False)
    image.set_editor_property(
        "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    image.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)


def configure_text(text, size):
    text.set_editor_property("justification", unreal.TextJustify.CENTER)
    text.set_editor_property(
        "color_and_opacity",
        unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
    )
    text.set_editor_property("shadow_color_and_opacity", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    text.set_editor_property("shadow_offset", unreal.Vector2D(2.0, 2.0))
    font = text.get_editor_property("font")
    font.size = size
    text.set_editor_property("font", font)
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)


textures = {
    "health_icon": load_required(f"{TEXTURE_ROOT}/T_UI_CombatHud_HealthIcon"),
    "health_frame": load_required(f"{TEXTURE_ROOT}/T_UI_CombatHud_HealthFrame"),
    "health_fill": load_required(f"{TEXTURE_ROOT}/T_UI_CombatHud_HealthFill"),
    "shard_icon": load_required(f"{TEXTURE_ROOT}/T_UI_CombatHud_TimeShardIcon"),
    "clock_frame": load_required(f"{TEXTURE_ROOT}/T_UI_CombatHud_ClockFrame"),
    "clock_needle": load_required(f"{TEXTURE_ROOT}/T_UI_CombatHud_ClockNeedle"),
    "echo_frame": load_required(f"{TEXTURE_ROOT}/T_UI_CombatHud_EchoFrame"),
}

toolset = unreal.UMGToolSet.get_default_object()

# Player HUD: heart + health, then time-shard icon + real balance text.
player = load_required(PLAYER_HUD)
widgets = widget_map(toolset, player)
player_root = widgets["PlayerHudRoot"]
player_root.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
player_size = widgets["PlayerHudSize"]
player_size.set_editor_property("width_override", 330.0)
player_size.set_editor_property("height_override", 132.0)
set_canvas_layout(player_size, 30.0, 24.0, 330.0, 132.0, z_order=5)
player_canvas = widgets["PlayerHudCanvas"]
player_canvas.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)

heart = widgets["ArtHealthIcon"]
mark_variable(toolset, player, heart)
set_image(heart, textures["health_icon"])
set_canvas_layout(heart, 0.0, 0.0, 68.0, 68.0, z_order=5)

health_size = widgets["PlayerHealthSize"]
health_size.set_editor_property("width_override", 232.0)
health_size.set_editor_property("height_override", 29.0)
set_canvas_layout(health_size, 81.0, 19.0, 232.0, 29.0, z_order=5)
health_border = widgets["PlayerHealthFrame"]
health_border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
health_border.set_brush_color(unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
health_overlay = widgets["PlayerHealthOverlay"]
health_overlay.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
health_frame = widgets["ArtHealthFrame"]
mark_variable(toolset, player, health_frame)
set_image(health_frame, textures["health_frame"])
fill_panel_slot(health_frame)

health_fill = ensure_widget(
    toolset,
    player,
    widget_map(toolset, player),
    unreal.Image,
    "PlayerHealthFill",
    health_overlay,
    1,
)
mark_variable(toolset, player, health_fill)
set_image(health_fill, textures["health_fill"])
health_fill.set_editor_property("render_transform_pivot", unreal.Vector2D(0.0, 0.5))
fill_panel_slot(health_fill)

widgets = widget_map(toolset, player)
health_progress = widgets["PlayerHealthProgress"]
mark_variable(toolset, player, health_progress)
health_progress.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
health_text = widgets["PlayerHealthText"]
mark_variable(toolset, player, health_text)
configure_text(health_text, 25)
fill_panel_slot(health_text)

portrait_size = widgets["PlayerPortraitSize"]
portrait_size.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

widgets = widget_map(toolset, player)
shard_icon = ensure_widget(
    toolset, player, widgets, unreal.Image, "ArtTimeShardIcon", player_canvas
)
mark_variable(toolset, player, shard_icon)
set_image(shard_icon, textures["shard_icon"])
set_canvas_layout(shard_icon, 1.0, 61.0, 68.0, 70.0, z_order=5)

widgets = widget_map(toolset, player)
shard_frame = ensure_widget(
    toolset, player, widgets, unreal.Image, "ArtTimeShardFrame", player_canvas
)
mark_variable(toolset, player, shard_frame)
set_image(shard_frame, textures["health_frame"])
set_canvas_layout(shard_frame, 81.0, 79.0, 232.0, 29.0, z_order=5)

widgets = widget_map(toolset, player)
shard_text = ensure_widget(
    toolset, player, widgets, unreal.TextBlock, "TimeShardText", player_canvas
)
mark_variable(toolset, player, shard_text)
configure_text(shard_text, 25)
set_canvas_layout(shard_text, 81.0, 72.0, 232.0, 44.0, z_order=6)

if not toolset.call_method("CompileWidgetBlueprint", args=(player,)):
    raise RuntimeError("WBP_ReEchoPlayerHud failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(player, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoPlayerHud failed to save")

# Encounter HUD: full-width top clock and real minimap in the delivered frame.
encounter = load_required(ENCOUNTER_HUD)
widgets = widget_map(toolset, encounter)
root = widgets["RootCanvas"]
root.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

obsolete_skill_bar = widgets.get("ArtSkillBar")
if obsolete_skill_bar is not None and not toolset.call_method(
    "RemoveWidget", args=(encounter, obsolete_skill_bar)
):
    raise RuntimeError("Unable to remove obsolete ArtSkillBar from Encounter HUD")

obsolete_time_readout = widgets.get("ArtTimeReadout")
if obsolete_time_readout is not None and not toolset.call_method(
    "RemoveWidget", args=(encounter, obsolete_time_readout)
):
    raise RuntimeError("Unable to remove obsolete ArtTimeReadout from Encounter HUD")

clock_frame = widgets["ArtClockFrame"]
mark_variable(toolset, encounter, clock_frame)
set_image(clock_frame, textures["clock_frame"])
set_canvas_layout(
    clock_frame, 13.5, 51.0, 1159.0, 216.0, 0.5, 0.0, 0.5, 0.0, 5
)

encounter_text = widgets["EncounterText"]
mark_variable(toolset, encounter, encounter_text)
configure_text(encounter_text, 42)
set_canvas_layout(
    encounter_text, 14.0, 0.0, 360.0, 58.0, 0.5, 0.0, 0.5, 0.0, 9
)

needle = widgets["ArtClockNeedle"]
mark_variable(toolset, encounter, needle)
set_image(needle, textures["clock_needle"])
needle.set_editor_property("render_transform_pivot", unreal.Vector2D(0.5, 0.12))
set_canvas_layout(needle, 14.0, 87.0, 44.0, 150.0, 0.5, 0.0, 0.5, 0.0, 6)

countdown = widgets["CountdownText"]
mark_variable(toolset, encounter, countdown)
configure_text(countdown, 38)
set_canvas_layout(countdown, 14.0, 116.0, 190.0, 58.0, 0.5, 0.0, 0.5, 0.0, 9)

map_overlay = widgets["Overlay_Map"]
map_overlay.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
set_canvas_layout(map_overlay, 0.0, 23.0, 360.0, 322.0, 1.0, 0.0, 1.0, 0.0, 5)
echo_frame = widgets["ArtTrajectoryMap"]
mark_variable(toolset, encounter, echo_frame)
set_image(echo_frame, textures["echo_frame"])
fill_panel_slot(echo_frame)

minimap_widgets = [
    widget
    for widget in widget_map(toolset, encounter).values()
    if isinstance(widget, unreal.ReEchoMinimapCanvasWidget)
]
if len(minimap_widgets) != 1:
    raise RuntimeError(f"Expected one ReEchoMinimapCanvasWidget, got {len(minimap_widgets)}")
minimap = minimap_widgets[0]
mark_variable(toolset, encounter, minimap)
minimap.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
widgets = widget_map(toolset, encounter)
map_canvas = ensure_widget(
    toolset, encounter, widgets, unreal.CanvasPanel, "CanvasPanel_0", map_overlay
)
map_canvas.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)
fill_panel_slot(map_canvas)
if minimap.get_parent() != map_canvas:
    moved = toolset.call_method(
        "MoveWidget", args=(encounter, minimap, map_canvas, -1)
    )
    if moved.widget is None:
        raise RuntimeError("Unable to move Minimap into CanvasPanel_0")
    minimap = moved.widget
set_canvas_layout(minimap, 40.0, 32.0, 273.316162, 254.796219)

if not toolset.call_method("CompileWidgetBlueprint", args=(encounter,)):
    raise RuntimeError("WBP_ReEchoEncounterHud failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(encounter, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoEncounterHud failed to save")

for obsolete_texture in (
    OBSOLETE_SKILL_BAR_TEXTURE,
    OBSOLETE_TIME_READOUT_TEXTURE,
):
    if not unreal.EditorAssetLibrary.does_asset_exist(obsolete_texture):
        continue
    referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(
        obsolete_texture, load_assets_to_confirm=True
    )
    if referencers:
        raise RuntimeError(
            f"Obsolete combat-HUD texture is still referenced: "
            f"{obsolete_texture} <- {referencers}"
        )
    if not unreal.EditorAssetLibrary.delete_asset(obsolete_texture):
        raise RuntimeError(f"Unable to delete obsolete texture: {obsolete_texture}")

unreal.log(
    f"[Plan93Author] configured PlayerHud widgets={len(widget_map(toolset, player))} "
    f"EncounterHud widgets={len(widget_map(toolset, encounter))}"
)
