"""Move Plan132 selection arrows into each Entry hover root and enforce aspect-fit art."""

import unreal


ENTRY_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry"
SELECTION_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection"
ARROW_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_SelectionArrow"
)
LEGACY_SELECTION_ARROWS = (
    "CharacterSelectionArrow0",
    "CharacterSelectionArrow1",
    "CharacterSelectionArrow2",
    "SelectionArrow",
    "WeaponSelectionArrow0",
    "WeaponSelectionArrow1",
    "WeaponSelectionArrow2",
    "WeaponSelectionArrow3",
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
        (candidate for candidate in widget_infos(toolset, blueprint) if candidate.widget is widget),
        None,
    )
    if info is None:
        raise RuntimeError(f"Missing WidgetInfo for {widget.get_name()}")
    if not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def set_fill(widget):
    slot = widget.get_editor_property("slot")
    if slot is None:
        raise RuntimeError(f"{widget.get_name()} has no panel slot")
    for property_name, value in (
        ("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0)),
        ("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL),
        ("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL),
    ):
        try:
            slot.set_editor_property(property_name, value)
        except Exception:
            pass


def add_widget(toolset, blueprint, widget_class, name, parent):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, -1)
    )
    if info.widget is None:
        raise RuntimeError(f"Failed to add widget: {name}")
    mark_variable(toolset, blueprint, info.widget)
    return info.widget


def configure_arrow(arrow, texture):
    brush = arrow.get_editor_property("brush")
    brush.set_editor_property("resource_object", texture)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    arrow.set_editor_property("brush", brush)
    arrow.set_editor_property(
        "visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE
    )
    slot = arrow.get_editor_property("slot")
    if not isinstance(slot, unreal.OverlaySlot):
        raise RuntimeError("EntrySelectionArrow does not have an Overlay slot")
    slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER
    )
    slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_BOTTOM
    )
    slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))


def migrate_entry(toolset, blueprint, arrow_texture):
    widgets = widget_map(toolset, blueprint)
    root = widgets.get("EntryRootSizeBox")
    select_button = widgets.get("SelectButton")
    portrait_scale = widgets.get("PortraitScale")
    portrait_image = widgets.get("PortraitImage")
    if not isinstance(root, unreal.SizeBox):
        raise RuntimeError("Plan132 EntryRootSizeBox is missing")
    if not isinstance(select_button, unreal.Button):
        raise RuntimeError("Plan132 SelectButton is missing")
    if not isinstance(portrait_scale, unreal.ScaleBox):
        raise RuntimeError("Plan132 PortraitScale is missing")
    if not isinstance(portrait_image, unreal.Image):
        raise RuntimeError("Plan132 PortraitImage is missing")

    visual_overlay = widgets.get("EntryVisualOverlay")
    if visual_overlay is None:
        if select_button.get_parent() is not root:
            raise RuntimeError("Plan132 SelectButton cannot be wrapped without losing layout")
        wrappers = toolset.call_method(
            "WrapWidgets", args=(blueprint, [select_button], unreal.Overlay)
        )
        if len(wrappers) != 1 or wrappers[0].widget is None:
            raise RuntimeError("Failed to wrap Plan132 SelectButton")
        renamed = toolset.call_method(
            "RenameWidget",
            args=(blueprint, wrappers[0].widget, "EntryVisualOverlay"),
        )
        visual_overlay = renamed.widget
    if not isinstance(visual_overlay, unreal.Overlay):
        raise RuntimeError("Plan132 EntryVisualOverlay has the wrong type")
    if select_button.get_parent() is not visual_overlay:
        raise RuntimeError("Plan132 SelectButton is outside EntryVisualOverlay")

    widgets = widget_map(toolset, blueprint)
    arrow = widgets.get("EntrySelectionArrow")
    if arrow is None:
        arrow = add_widget(
            toolset, blueprint, unreal.Image, "EntrySelectionArrow", visual_overlay
        )
    if not isinstance(arrow, unreal.Image) or arrow.get_parent() is not visual_overlay:
        raise RuntimeError("Plan132 EntrySelectionArrow has the wrong parent or type")

    blueprint.modify()
    mark_variable(toolset, blueprint, visual_overlay)
    mark_variable(toolset, blueprint, select_button)
    mark_variable(toolset, blueprint, arrow)
    set_fill(visual_overlay)
    set_fill(select_button)
    configure_arrow(arrow, arrow_texture)
    portrait_scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    portrait_scale.set_editor_property(
        "stretch_direction", unreal.StretchDirection.BOTH
    )
    portrait_slot = portrait_image.get_editor_property("slot")
    if not isinstance(portrait_slot, unreal.ScaleBoxSlot):
        raise RuntimeError("Plan132 PortraitImage does not have a ScaleBox slot")
    portrait_slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER
    )
    portrait_slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER
    )
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Entry failed to compile after migration")
    generated_class = blueprint.generated_class()
    if generated_class is None:
        raise RuntimeError("Plan132 Entry has no generated class after migration")
    default_object = unreal.get_default_object(generated_class)
    blueprint.modify()
    default_object.call_method("ApplyDesignerPreviewSettings")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Entry failed to save after migration")


def remove_detached_selection_arrows(toolset, blueprint):
    widgets = widget_map(toolset, blueprint)
    removed = []
    for name in LEGACY_SELECTION_ARROWS:
        arrow = widgets.get(name)
        if arrow is None:
            continue
        if not toolset.call_method("RemoveWidget", args=(blueprint, arrow)):
            raise RuntimeError(f"Failed to remove detached selection arrow: {name}")
        removed.append(name)
    blueprint.modify()
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Selection failed to compile after arrow migration")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Selection failed to save after arrow migration")
    return removed


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    entry = unreal.load_asset(ENTRY_PATH)
    selection = unreal.load_asset(SELECTION_PATH)
    arrow_texture = unreal.load_asset(ARROW_TEXTURE_PATH)
    if not isinstance(entry, unreal.WidgetBlueprint):
        raise RuntimeError(f"Missing Plan132 Entry WBP: {ENTRY_PATH}")
    if not isinstance(selection, unreal.WidgetBlueprint):
        raise RuntimeError(f"Missing Plan132 Selection WBP: {SELECTION_PATH}")
    if not isinstance(arrow_texture, unreal.Texture2D):
        raise RuntimeError(f"Missing Plan132 arrow texture: {ARROW_TEXTURE_PATH}")

    migrate_entry(toolset, entry, arrow_texture)
    removed = remove_detached_selection_arrows(toolset, selection)
    unreal.log(
        "[Plan132EntryArrowAspectMigration] "
        f"aspect=ScaleToFit+Center arrow_parent=EntryVisualOverlay removed={removed}"
    )


if __name__ == "__main__":
    main()
