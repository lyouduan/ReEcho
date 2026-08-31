"""One-shot authoring of the shop refresh label, without rebuilding the shop.

Use Run-EditorPythonLocked.ps1 after saving and closing Unreal. Subsequent runs
verify the authored controls without resetting user edits or saving the asset.
"""

import unreal


ASSET = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


def add(toolset, blueprint, kind, name, parent):
    info = toolset.call_method("AddWidget", args=(blueprint, kind, name, parent, -1))
    if not info.widget:
        raise RuntimeError(f"Unable to create {name}")
    toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, info.widget, True))
    return info.widget


def fill(widget):
    slot = widget.get_editor_property("slot")
    slot.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)


def snapshot(widget):
    """Protect unrelated parents, Canvas geometry, text and image presentation."""
    parent = widget.get_parent()
    state = [parent.get_name() if parent else None]
    props = ["visibility", "render_transform", "render_opacity"]
    if isinstance(widget, unreal.TextBlock):
        props += ["text", "font", "color_and_opacity", "justification", "auto_wrap_text"]
    if isinstance(widget, unreal.Image):
        props += ["brush", "color_and_opacity"]
    for prop in props:
        value = widget.get_editor_property(prop)
        state.append(value.export_text() if isinstance(value, unreal.StructBase) else str(value))
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.CanvasPanelSlot):
        state += [slot.get_editor_property("layout_data").export_text(),
                  slot.get_editor_property("auto_size"), slot.get_editor_property("z_order")]
    return tuple(state)


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(ASSET)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Missing WidgetBlueprint: {ASSET}")
    widgets = widget_map(toolset, blueprint)
    button, art = widgets["ShopRefreshButton"], widgets["DesignerRefreshArt"]
    if not isinstance(button, unreal.Button) or not isinstance(art, unreal.Image):
        raise RuntimeError("Unexpected refresh button/art types")

    if "ShopRefreshCountText" in widgets:
        if not isinstance(widgets["ShopRefreshCountText"], unreal.TextBlock):
            raise RuntimeError("ShopRefreshCountText must be a TextBlock")
        unreal.log("[ShopRefreshAuthoring] Already authored; preserved edits, no save")
        return

    if button.get_content() != art:
        raise RuntimeError("Unexpected refresh content; refuse to replace authored controls")
    protected = {name: snapshot(widget) for name, widget in widgets.items()
                 if name != "DesignerRefreshArt"}
    art_brush = art.get_editor_property("brush").export_text()
    # Wrap atomically: detaching a variable before AddWidget triggers an
    # intermediate compile and invalidates the old widget's GUID/outer.
    wrapped = toolset.call_method("WrapWidgets", args=(blueprint, [art], unreal.Overlay))
    if len(wrapped) != 1 or not wrapped[0].widget:
        raise RuntimeError("Unable to wrap the existing refresh artwork")
    renamed = toolset.call_method("RenameWidget", args=(
        blueprint, wrapped[0].widget, "DesignerRefreshOverlay"))
    if not renamed.widget:
        raise RuntimeError("Unable to name the refresh overlay")
    current = widget_map(toolset, blueprint)
    overlay = current["DesignerRefreshOverlay"]
    button, art = current["ShopRefreshButton"], current["DesignerRefreshArt"]
    fill(overlay)
    fill(art)
    canvas = add(toolset, blueprint, unreal.CanvasPanel, "DesignerRefreshTextCanvas", overlay)
    fill(canvas)
    canvas.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    box = add(toolset, blueprint, unreal.Overlay, "DesignerRefreshTextBox", canvas)
    box_slot = box.get_editor_property("slot")
    box_slot.set_editor_property("layout_data", unreal.AnchorData(
        offsets=unreal.Margin(6, 0, 6, 0),
        anchors=unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)),
        alignment=unreal.Vector2D(0, 0),
    ))
    box_slot.set_editor_property("auto_size", False)
    box_slot.set_editor_property("z_order", 1)
    label = add(toolset, blueprint, unreal.TextBlock, "ShopRefreshCountText", box)
    label.set_text("刷新|2次·5碎片")
    font = label.get_editor_property("font")
    font.size = 9
    label.set_editor_property("font", font)
    label.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(0, 0, 0, 1)))
    label.set_editor_property("auto_wrap_text", False)
    label.set_editor_property("justification", unreal.TextJustify.CENTER)
    label.set_editor_property("clipping", unreal.WidgetClipping.CLIP_TO_BOUNDS)
    label.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    label_slot = label.get_editor_property("slot")
    label_slot.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    label_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
    label_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    # Bake the previous runtime padding into the WBP so preview and play agree.
    style = button.get_editor_property("widget_style")
    style.set_editor_property("normal_padding", unreal.Margin(0, 0, 0, 0))
    style.set_editor_property("pressed_padding", unreal.Margin(0, 0, 0, 0))
    button.set_editor_property("widget_style", style)

    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Shop blueprint compilation failed")
    after = widget_map(toolset, blueprint)
    expected = set(widgets) | {"DesignerRefreshOverlay", "DesignerRefreshTextCanvas",
                               "DesignerRefreshTextBox", "ShopRefreshCountText"}
    if set(after) != expected:
        raise RuntimeError(f"Unexpected widget changes: missing={expected - set(after)}, extra={set(after) - expected}")
    for name, before in protected.items():
        if snapshot(after[name]) != before:
            raise RuntimeError(f"Unrelated presentation changed: {name}")
    if after["DesignerRefreshArt"].get_editor_property("brush").export_text() != art_brush:
        raise RuntimeError("Refresh art brush changed")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError("Shop blueprint save failed")
    unreal.log(f"[ShopRefreshAuthoring] SUCCESS protected={len(protected)} new_controls=4 sample={label.get_text()}")


if __name__ == "__main__":
    main()
