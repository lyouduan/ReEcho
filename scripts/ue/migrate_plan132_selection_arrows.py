"""Add Designer-owned selection arrows without touching existing Plan132 layout edits."""

import unreal


SELECTION_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection"
ARROW_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_SelectionArrow"
)
ARROW_LAYOUTS = (
    ("CharacterSelectionArrow0", 330.0, 822.0),
    ("CharacterSelectionArrow1", 725.0, 822.0),
    ("CharacterSelectionArrow2", 1120.0, 822.0),
    ("WeaponSelectionArrow0", 547.0, 852.0),
    ("WeaponSelectionArrow1", 821.0, 852.0),
    ("WeaponSelectionArrow2", 1095.0, 852.0),
    ("WeaponSelectionArrow3", 1369.0, 852.0),
)


def widget_infos(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in widget_infos(toolset, blueprint)
        if info.widget
    }


def mark_variable(toolset, blueprint, widget):
    info = next(
        candidate
        for candidate in widget_infos(toolset, blueprint)
        if candidate.widget is widget
    )
    if not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def configure_new_arrow(arrow, texture, x, y):
    brush = arrow.get_editor_property("brush")
    brush.set_editor_property("resource_object", texture)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    arrow.set_editor_property("brush", brush)
    arrow.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
    slot = arrow.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Plan132 arrow is not a Canvas child: {arrow.get_name()}")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, y, 60.0, 33.0),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0),
                maximum=unreal.Vector2D(0.0, 0.0),
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", 80)


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(SELECTION_PATH)
    texture = unreal.load_asset(ARROW_TEXTURE_PATH)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Plan132 Selection WBP is missing: {SELECTION_PATH}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Plan132 arrow texture is missing: {ARROW_TEXTURE_PATH}")
    widgets = widget_map(toolset, blueprint)
    design_canvas = widgets.get("LoadoutDesignCanvas")
    existing_arrow = widgets.get("SelectionArrow")
    if not isinstance(design_canvas, unreal.CanvasPanel) or not isinstance(
        existing_arrow, unreal.Image
    ):
        raise RuntimeError("Plan132 Selection lost its authored canvas or SelectionArrow")

    added = []
    for name, x, y in ARROW_LAYOUTS:
        widgets = widget_map(toolset, blueprint)
        arrow = widgets.get(name)
        if arrow is None:
            info = toolset.call_method(
                "AddWidget",
                args=(blueprint, unreal.Image, name, design_canvas, -1),
            )
            arrow = info.widget
            if not isinstance(arrow, unreal.Image):
                raise RuntimeError(f"Failed to add Plan132 selection arrow: {name}")
            configure_new_arrow(arrow, texture, x, y)
            added.append(name)
        elif not isinstance(arrow, unreal.Image):
            raise RuntimeError(f"Plan132 selection arrow has wrong type: {name}")
        mark_variable(toolset, blueprint, arrow)

    mark_variable(toolset, blueprint, existing_arrow)
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Selection WBP failed to compile after arrow migration")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Selection WBP failed to save after arrow migration")
    unreal.log(
        f"[Plan132SelectionArrowMigration] added={added} preserved=SelectionArrow"
    )


if __name__ == "__main__":
    main()
