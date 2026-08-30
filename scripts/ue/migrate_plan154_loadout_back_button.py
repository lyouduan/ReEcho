"""Add a Designer-owned Back button beside the authored Loadout Confirm button.

The migration is intentionally narrow: on first run it splits the existing Confirm
anchor into a centered two-button row, and on later runs it preserves both authored
positions so manual Designer tweaks are never shifted again.
"""

import unreal


SELECTION_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection"
BUTTON_GAP = 28.0


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


def canvas_geometry(widget):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct Canvas child")
    layout = slot.get_editor_property("layout_data")
    offsets = layout.get_editor_property("offsets")
    return slot, layout, offsets


def set_canvas_x(slot, layout, offsets, x):
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, offsets.top, offsets.right, offsets.bottom),
            anchors=layout.get_editor_property("anchors"),
            alignment=layout.get_editor_property("alignment"),
        ),
    )


def copy_button_presentation(source, target):
    for prop in (
        "widget_style",
        "color_and_opacity",
        "background_color",
        "click_method",
        "touch_method",
        "press_method",
        "is_focusable",
    ):
        target.set_editor_property(prop, source.get_editor_property(prop))
    target.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)
    target.set_editor_property("is_enabled", True)


def copy_label_presentation(source, target):
    for prop in (
        "font",
        "color_and_opacity",
        "shadow_offset",
        "shadow_color_and_opacity",
        "justification",
        "margin",
    ):
        target.set_editor_property(prop, source.get_editor_property(prop))
    target.set_editor_property("text", "返回")
    target.set_editor_property(
        "visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE
    )


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(SELECTION_PATH)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Missing Loadout WidgetBlueprint: {SELECTION_PATH}")

    widgets = widget_map(toolset, blueprint)
    canvas = widgets.get("LoadoutDesignCanvas")
    confirm_button = widgets.get("ConfirmButton")
    confirm_label = widgets.get("ConfirmButtonLabel")
    if not isinstance(canvas, unreal.CanvasPanel):
        raise RuntimeError("LoadoutDesignCanvas is missing")
    if not isinstance(confirm_button, unreal.Button) or not isinstance(
        confirm_label, unreal.TextBlock
    ):
        raise RuntimeError("Authored Confirm button contract is missing")

    back_button = widgets.get("BackButton")
    back_label = widgets.get("BackButtonLabel")
    created = back_button is None and back_label is None
    if (back_button is None) != (back_label is None):
        raise RuntimeError("Partial Plan154 Back button hierarchy detected")

    if created:
        confirm_slot, confirm_layout, confirm_offsets = canvas_geometry(confirm_button)
        label_slot, label_layout, label_offsets = canvas_geometry(confirm_label)
        split = (confirm_offsets.right + BUTTON_GAP) * 0.5

        back_button = toolset.call_method(
            "AddWidget",
            args=(blueprint, unreal.Button, "BackButton", canvas, -1),
        ).widget
        back_label = toolset.call_method(
            "AddWidget",
            args=(blueprint, unreal.TextBlock, "BackButtonLabel", canvas, -1),
        ).widget
        if not isinstance(back_button, unreal.Button) or not isinstance(
            back_label, unreal.TextBlock
        ):
            raise RuntimeError("Failed to create Plan154 Back controls")

        back_slot, back_layout, back_offsets = canvas_geometry(back_button)
        back_label_slot, back_label_layout, back_label_offsets = canvas_geometry(
            back_label
        )
        set_canvas_x(
            back_slot,
            confirm_layout,
            confirm_offsets,
            confirm_offsets.left - split,
        )
        set_canvas_x(
            confirm_slot,
            confirm_layout,
            confirm_offsets,
            confirm_offsets.left + split,
        )
        label_split = (label_offsets.right + BUTTON_GAP) * 0.5
        set_canvas_x(
            back_label_slot,
            label_layout,
            label_offsets,
            label_offsets.left - label_split,
        )
        set_canvas_x(
            label_slot,
            label_layout,
            label_offsets,
            label_offsets.left + label_split,
        )

    copy_button_presentation(confirm_button, back_button)
    copy_label_presentation(confirm_label, back_label)
    mark_variable(toolset, blueprint, back_button)
    mark_variable(toolset, blueprint, back_label)

    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan154 Loadout Selection failed to compile")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan154 Loadout Selection failed to save")

    _, _, confirm_offsets = canvas_geometry(confirm_button)
    _, _, back_offsets = canvas_geometry(back_button)
    unreal.log(
        "[Plan154LoadoutBackMigration] "
        f"created={created} back=({back_offsets.left},{back_offsets.top},"
        f"{back_offsets.right},{back_offsets.bottom}) "
        f"confirm=({confirm_offsets.left},{confirm_offsets.top},"
        f"{confirm_offsets.right},{confirm_offsets.bottom}) preserved_manual_layout=True"
    )


if __name__ == "__main__":
    main()
