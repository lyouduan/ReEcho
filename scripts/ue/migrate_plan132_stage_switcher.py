"""Wrap the existing Plan132 stage panels in a Designer-native WidgetSwitcher."""

import unreal


SELECTION_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection"


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


def set_canvas_fill(widget):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError("Plan132 StageSwitcher is not a direct design-canvas child")
    slot.set_editor_property(
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
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", 10)


def set_switcher_slot_fill(stage):
    slot = stage.get_editor_property("slot")
    for property_name, value in (
        ("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0)),
        ("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL),
        ("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL),
    ):
        slot.set_editor_property(property_name, value)
    stage.set_editor_property(
        "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
    )


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(SELECTION_PATH)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Plan132 Selection WBP is missing: {SELECTION_PATH}")
    widgets = widget_map(toolset, blueprint)
    design_canvas = widgets.get("LoadoutDesignCanvas")
    character_stage = widgets.get("CharacterStagePanel")
    weapon_stage = widgets.get("WeaponStagePanel")
    if not isinstance(design_canvas, unreal.CanvasPanel):
        raise RuntimeError("Plan132 LoadoutDesignCanvas is missing")
    if not isinstance(character_stage, unreal.CanvasPanel) or not isinstance(
        weapon_stage, unreal.CanvasPanel
    ):
        raise RuntimeError("Plan132 authored stage panels are missing")

    stage_switcher = widgets.get("StageSwitcher")
    if stage_switcher is None:
        result = toolset.call_method(
            "AddWidget",
            args=(blueprint, unreal.WidgetSwitcher, "StageSwitcher", design_canvas, -1),
        )
        stage_switcher = result.widget
    if not isinstance(stage_switcher, unreal.WidgetSwitcher):
        raise RuntimeError("Plan132 StageSwitcher could not be created")
    if stage_switcher.get_parent() is not design_canvas:
        result = toolset.call_method(
            "MoveWidget", args=(blueprint, stage_switcher, design_canvas, -1)
        )
        stage_switcher = result.widget
    set_canvas_fill(stage_switcher)
    mark_variable(toolset, blueprint, stage_switcher)

    for index, stage in enumerate((character_stage, weapon_stage)):
        if (
            stage.get_parent() is not stage_switcher
            or stage_switcher.get_child_index(stage) != index
        ):
            result = toolset.call_method(
                "MoveWidget", args=(blueprint, stage, stage_switcher, index)
            )
            stage = result.widget
        if stage is None:
            raise RuntimeError("Plan132 failed to move a stage into StageSwitcher")
        set_switcher_slot_fill(stage)

    # Leave the weapon page open so its runtime layout is immediately editable.
    stage_switcher.set_active_widget_index(1)
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Selection failed to compile after stage migration")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Selection failed to save after stage migration")
    unreal.log(
        "[Plan132StageSwitcherMigration] pages=Character,Weapon active=Weapon preserved=stage-contents"
    )


if __name__ == "__main__":
    main()
