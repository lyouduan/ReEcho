"""Read-only audit of the saved Blueprint samples and editable tooltip contract."""

import unreal


TOOLSET = unreal.UMGToolSet.get_default_object()
ROOT = "/Game/ReEcho/UI/"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def inspect(name, required):
    bp = unreal.load_asset(ROOT + name)
    require(isinstance(bp, unreal.WidgetBlueprint), f"Missing {name}")
    infos = TOOLSET.call_method("GetWidgets", args=(bp,)).widgets
    widgets = {info.widget.get_name(): info.widget for info in infos if info.widget}
    variables = {info.widget.get_name() for info in infos if info.widget and info.get_editor_property("is_variable")}
    for widget_name, expected_type in required.items():
        require(isinstance(widgets.get(widget_name), expected_type), f"Missing/wrong {name}.{widget_name}")
        require(widget_name in variables, f"Binding not marked variable: {widget_name}")
    for widget in widgets.values():
        if isinstance(widget, unreal.Border):
            brush = widget.get_editor_property("background")
            require(brush.get_editor_property("resource_object") is not None, f"Empty backplate resource: {name}.{widget.get_name()}")
            require(brush.get_editor_property("draw_as") != unreal.SlateBrushDrawType.NO_DRAW_TYPE,
                    f"Backplate draw disabled: {name}.{widget.get_name()}")
            require(widget.get_editor_property("brush_color").a > 0, f"Transparent backplate: {name}.{widget.get_name()}")
            if widget.get_name().endswith("Frame"):
                padding = widget.get_editor_property("padding")
                require(all(getattr(padding, side) > 0 for side in ("left", "top", "right", "bottom")),
                        f"Solid child covers frame (zero inset): {name}.{widget.get_name()}")
                child_padding = widget.get_child_at(0).get_editor_property("slot").get_editor_property("padding")
                require(child_padding.export_text() == padding.export_text(),
                        f"Frame/content slot padding disagree: {name}.{widget.get_name()}")
        if isinstance(widget, unreal.TextBlock):
            require(bool(str(widget.get_text()).strip()), f"Empty Designer sample: {name}.{widget.get_name()}")
            require(widget.get_editor_property("font").get_editor_property("font_object") is not None,
                    f"Missing cooked font: {widget.get_name()}")
    unreal.log(f"[Plan157Audit] {name}: {len(widgets)} real editable widgets")
    return bp, widgets


shop, w = inspect("WBP_ReEchoShopTooltip", {
    "TooltipRootSizeBox": unreal.SizeBox, "TooltipFrame": unreal.Border,
    "TooltipSurface": unreal.Border, "TitleText": unreal.TextBlock,
    "DescriptionText": unreal.TextBlock, "OutcomePanel": unreal.VerticalBox,
    "OutcomeFrame": unreal.Border, "OutcomeSurface": unreal.Border,
    "OutcomeTitleText": unreal.TextBlock, "OutcomeText": unreal.TextBlock,
})
require(w["OutcomePanel"].get_editor_property("visibility") not in (
    unreal.SlateVisibility.HIDDEN, unreal.SlateVisibility.COLLAPSED), "Outcome must be visible in Designer")
require(w["OutcomeFrame"].get_parent() == w["OutcomePanel"], "Outcome spacing must be inside collapsed container")
require("\n" in str(w["OutcomeText"].get_text()), "Outcome preview must show multiple results")
for name in ("DescriptionText", "OutcomeText"):
    require(w[name].get_editor_property("auto_wrap_text"), f"{name} must wrap")
require(not w["TooltipRootSizeBox"].get_editor_property("override_height_override"), "Fixed tooltip height clips long descriptions")

row_bp, row = inspect("WBP_ReEchoAttributeRow", {
    "AttributeRowSizeBox": unreal.SizeBox, "AttributeIconSize": unreal.SizeBox,
    "AttributeIconScale": unreal.ScaleBox, "AttributeIcon": unreal.Image,
    "AttributeNameText": unreal.TextBlock, "AttributeValueText": unreal.TextBlock,
})
require(row["AttributeIconScale"].get_editor_property("stretch") == unreal.Stretch.SCALE_TO_FIT, "Attribute icon must preserve aspect")
icon_brush = row["AttributeIcon"].get_editor_property("brush")
icon_size = icon_brush.get_editor_property("image_size")
require(icon_size.get_editor_property("x") > 0 and icon_size.get_editor_property("y") > 0,
        "AttributeIcon Brush Image Size is zero: texture exists but ScaleBox has nothing to draw")
require(isinstance(icon_brush.get_editor_property("resource_object"), unreal.Texture2D), "Attribute preview icon missing")
attributes_bp, attributes = inspect("WBP_ReEchoAttributeTooltip", {
    "AttributeTooltipRootSizeBox": unreal.SizeBox,
    "AttributeTooltipFrame": unreal.Border, "AttributeTooltipSurface": unreal.Border,
    "AttributeRows": unreal.VerticalBox,
})
require(not attributes["AttributeTooltipRootSizeBox"].get_editor_property("override_height_override"), "Fixed attribute panel height")
rows = attributes["AttributeRows"]
require(rows.get_children_count() >= 8, "Complete eight-row attribute preview is missing")
for index in range(rows.get_children_count()):
    widget = rows.get_child_at(index)
    require(widget.get_class() == row_bp.generated_class(), "Preview and runtime must use same row Blueprint")
    preview = widget.get_editor_property("designer_preview")
    require(str(preview.name).strip() and str(preview.value).strip() and preview.icon, f"Missing preview data for row {index}")
defaults = unreal.get_default_object(attributes_bp.generated_class())
require(defaults.get_editor_property("attribute_row_widget_class") == row_bp.generated_class(), "Missing dynamic row class reference")
unreal.log("[Plan157Audit] PASS item + optional outcome + attribute panel/rows all have authored samples")
