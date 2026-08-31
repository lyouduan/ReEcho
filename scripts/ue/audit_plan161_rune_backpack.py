"""Read-only check of the saved rune backpack Blueprint contract."""
import unreal

TOOLS = unreal.UMGToolSet.get_default_object()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def inspect(name, required):
    bp = unreal.load_asset(f"/Game/ReEcho/UI/{name}")
    require(isinstance(bp, unreal.WidgetBlueprint), f"Missing {name}")
    infos = TOOLS.call_method("GetWidgets", args=(bp,)).widgets
    widgets = {i.widget.get_name(): i.widget for i in infos if i.widget}
    for key, cls in required.items():
        require(isinstance(widgets.get(key), cls), f"Missing {name}.{key}")
    for widget in widgets.values():
        if isinstance(widget, unreal.Border):
            brush = widget.get_editor_property("background")
            require(brush.get_editor_property("resource_object"), f"Empty backplate {widget.get_name()}")
            require(widget.get_editor_property("brush_color").a > 0, "Transparent backplate")
            if widget.get_name().endswith("Frame"):
                pad = widget.get_editor_property("padding")
                require(all(getattr(pad, side) > 0 for side in ("left", "right", "top", "bottom")), "Frame covered by child")
        if isinstance(widget, unreal.TextBlock):
            require(widget.get_editor_property("font").get_editor_property("font_object"), "Missing CJK font")
    # DesignSizeMode is native editor-only state, not Python-reflected; checked by the C++ render test.
    return bp, widgets


entry, ew = inspect("WBP_ReEchoRuneBackpackEntry", {
    "EntryRootSizeBox": unreal.SizeBox, "SelectButton": unreal.ReEchoIndexedButton,
    "RuneIcon": unreal.Image, "RuneIconScale": unreal.ScaleBox,
    "NameText": unreal.TextBlock, "CountText": unreal.TextBlock,
    "EntryFrame": unreal.Border, "EntrySurface": unreal.Border,
})
require(ew["RuneIconScale"].get_editor_property("stretch") == unreal.Stretch.SCALE_TO_FIT, "Icon aspect not preserved")
brush = ew["RuneIcon"].get_editor_property("brush")
size = brush.get_editor_property("image_size")
require(brush.get_editor_property("resource_object") and size.get_editor_property("x") > 0 and size.get_editor_property("y") > 0,
        "Example icon has no drawable size")
panel, pw = inspect("WBP_ReEchoRuneBackpack", {
    "BackpackRootSizeBox": unreal.SizeBox, "BackpackFrame": unreal.Border, "BackpackSurface": unreal.Border,
    "TitleText": unreal.TextBlock, "EntryScroll": unreal.ScrollBox, "EntryList": unreal.VerticalBox,
})
defaults = unreal.get_default_object(panel.generated_class())
require(defaults.get_editor_property("entry_widget_class") == entry.generated_class(), "Runtime and preview row class differ")
require(pw["EntryList"].get_children_count() >= 3, "Complete examples missing")
for row in pw["EntryList"].get_all_children():
    require(row.get_class() == entry.generated_class(), "Example is not a real nested row")
    sample = row.get_editor_property("designer_preview")
    require(sample.icon and str(sample.name) and sample.count > 0, "Incomplete preview data")
unreal.log("[Plan161Audit] PASS real nested examples, authored sizes, aspect fit, and visible backplates")
