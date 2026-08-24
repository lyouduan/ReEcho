"""Read-only widget-tree audit for the Plan93 combat HUD assets.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript``. The script loads
the two widget blueprints and prints their authored widget/slot contracts. It
does not compile, save, import, or modify assets.
"""

import unreal


ASSET_PATHS = {
    "/Game/ReEcho/UI/WBP_ReEchoPlayerHud": "/Script/ReEcho.ReEchoPlayerHudWidget",
    "/Game/ReEcho/UI/WBP_ReEchoEncounterHud": "/Script/ReEcho.ReEchoEncounterHudWidget",
}
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/CombatHud"
TEXTURE_NAMES = (
    "T_UI_CombatHud_HealthIcon",
    "T_UI_CombatHud_EchoFrame",
    "T_UI_CombatHud_ClockFrame",
    "T_UI_CombatHud_TimeShardIcon",
    "T_UI_CombatHud_TimeReadout",
    "T_UI_CombatHud_ClockNeedle",
    "T_UI_CombatHud_HealthFill",
    "T_UI_CombatHud_HealthFrame",
)


def slot_summary(slot):
    if slot is None:
        return "<none>"
    fields = [f"type={slot.get_class().get_name()}"]
    if isinstance(slot, unreal.CanvasPanelSlot):
        fields.extend(
            (
                f"position={slot.get_position()}",
                f"size={slot.get_size()}",
                f"anchors={slot.get_anchors()}",
                f"alignment={slot.get_alignment()}",
                f"auto_size={slot.get_auto_size()}",
                f"z={slot.get_z_order()}",
            )
        )
    return " ".join(fields)


def nearly_equal(actual, expected):
    return abs(float(actual) - float(expected)) <= 0.01


toolset = unreal.UMGToolSet.get_default_object()
for asset_path, expected_parent_path in ASSET_PATHS.items():
    blueprint = unreal.load_asset(asset_path)
    if blueprint is None:
        raise RuntimeError(f"Missing combat HUD widget: {asset_path}")
    infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if any(str(info.widget_name) == "ArtSkillBar" for info in infos):
        raise RuntimeError(f"Obsolete ArtSkillBar is still present in {asset_path}")
    widgets = {
        str(info.widget_name): info.widget for info in infos if info.widget is not None
    }
    if asset_path.endswith("WBP_ReEchoPlayerHud"):
        for text_name in ("PlayerHealthText", "TimeShardText"):
            text_widget = widgets.get(text_name)
            if not isinstance(text_widget, unreal.TextBlock) or not nearly_equal(
                text_widget.get_editor_property("font").size, 25.0
            ):
                raise RuntimeError(f"{text_name} must use the accepted 25px font")
    elif asset_path.endswith("WBP_ReEchoEncounterHud"):
        encounter_text = widgets.get("EncounterText")
        encounter_slot = encounter_text.slot if encounter_text is not None else None
        if not isinstance(encounter_slot, unreal.CanvasPanelSlot) or not nearly_equal(
            encounter_slot.get_position().y, 0.0
        ):
            raise RuntimeError("EncounterText must keep the accepted Y=0 position")
        map_canvas = widgets.get("CanvasPanel_0")
        minimaps = [
            widget
            for widget in widgets.values()
            if isinstance(widget, unreal.ReEchoMinimapCanvasWidget)
        ]
        if len(minimaps) != 1 or minimaps[0].get_parent() != map_canvas:
            raise RuntimeError("Minimap must remain inside CanvasPanel_0")
        minimap_slot = minimaps[0].slot
        if not isinstance(minimap_slot, unreal.CanvasPanelSlot):
            raise RuntimeError("Minimap must use the accepted CanvasPanel slot")
        minimap_position = minimap_slot.get_position()
        minimap_size = minimap_slot.get_size()
        if not (
            nearly_equal(minimap_position.x, 40.0)
            and nearly_equal(minimap_position.y, 32.0)
            and nearly_equal(minimap_size.x, 273.316162)
            and nearly_equal(minimap_size.y, 254.796219)
        ):
            raise RuntimeError("Minimap placement differs from the accepted WBP layout")
    generated_class = blueprint.generated_class()
    expected_parent = unreal.load_class(None, expected_parent_path)
    if not generated_class or not expected_parent or not unreal.MathLibrary.class_is_child_of(
        generated_class, expected_parent
    ):
        raise RuntimeError(
            f"{asset_path} is not derived from expected parent {expected_parent_path}"
        )
    unreal.log(
        f"[Plan93Audit] ASSET {asset_path} widgets={len(infos)} "
        f"class={generated_class.get_path_name()} expected_parent={expected_parent_path}"
    )
    for info in infos:
        widget = info.widget
        if widget is None:
            continue
        parent = widget.get_parent()
        details = []
        if isinstance(widget, unreal.Image):
            brush = widget.get_editor_property("brush")
            resource = brush.get_editor_property("resource_object")
            details.append(
                f"resource={resource.get_path_name() if resource else '<none>'}"
            )
        elif isinstance(widget, unreal.TextBlock):
            details.append(f"font_size={widget.get_editor_property('font').size}")
        unreal.log(
            f"[Plan93Audit] WIDGET name={widget.get_name()} "
            f"type={widget.get_class().get_name()} "
            f"parent={parent.get_name() if parent else '<root>'} "
            f"variable={bool(info.get_editor_property('is_variable'))} "
            f"visibility={widget.get_editor_property('visibility')} "
            f"slot=({slot_summary(widget.slot)}) {' '.join(details)}"
        )

for texture_name in TEXTURE_NAMES:
    texture_path = f"{TEXTURE_ROOT}/{texture_name}"
    texture = unreal.load_asset(texture_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Missing Plan93 Texture2D: {texture_path}")
    unreal.log(
        f"[Plan93Audit] TEXTURE path={texture_path} "
        f"size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()} "
        f"srgb={texture.get_editor_property('srgb')} "
        f"filter={texture.get_editor_property('filter')} "
        f"mips={texture.get_editor_property('mip_gen_settings')} "
        f"compression={texture.get_editor_property('compression_settings')} "
        f"lod_group={texture.get_editor_property('lod_group')}"
    )

obsolete_skill_bar = f"{TEXTURE_ROOT}/T_UI_CombatHud_SkillBar"
if unreal.EditorAssetLibrary.does_asset_exist(obsolete_skill_bar):
    raise RuntimeError(f"Obsolete runtime texture still exists: {obsolete_skill_bar}")
