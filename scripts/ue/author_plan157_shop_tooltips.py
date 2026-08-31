"""Create the three Designer-owned tooltips once; never reset existing user edits.

Run in UE using Run-EditorPythonLocked.ps1. Samples are presentation-only and
are overwritten by runtime data. This script does not touch the shop screen.
"""

import csv
from pathlib import Path

import unreal


DIRECTORY = "/Game/ReEcho/UI"
FONT_PATH = "/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Texts/方正黑体简体_Font"
WHITE = unreal.LinearColor(1, 1, 1, 1)
GOLD = unreal.LinearColor(0.96, 0.80, 0.34, 1)
DARK = unreal.LinearColor(0.02, 0.02, 0.02, 0.97)
TOOLSET = unreal.UMGToolSet.get_default_object()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def add(bp, cls, name, parent=None):
    widget = TOOLSET.call_method("AddWidget", args=(bp, cls, name, parent, -1)).widget
    require(widget is not None, f"Failed to create {name}")
    TOOLSET.call_method("ToggleWidgetAsVariable", args=(bp, widget, True))
    slot = widget.get_editor_property("slot")
    # BorderSlot mirrors its owner's Padding; clearing it would erase the frame
    # inset and let the solid child surface paint over the complete border.
    if isinstance(slot, (unreal.VerticalBoxSlot, unreal.SizeBoxSlot)):
        slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
        slot.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    if isinstance(slot, unreal.VerticalBoxSlot):
        slot.set_editor_property("size", unreal.SlateChildSize(size_rule=unreal.SlateSizeRule.AUTOMATIC))
    return widget


def create(name, parent_name):
    path = f"{DIRECTORY}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log(f"[Plan157] Preserve existing Blueprint and manual edits: {path}")
        return unreal.load_asset(path), False
    parent = unreal.load_class(None, f"/Script/ReEcho.{parent_name}")
    require(parent is not None, f"Missing C++ parent {parent_name}; build first")
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DIRECTORY, unreal.WidgetBlueprint, factory)
    require(bp is not None, f"Cannot create {path}")
    # Widget factories may create a Canvas root. Replace only on first creation.
    for info in TOOLSET.call_method("GetWidgets", args=(bp,)).widgets:
        if info.widget and info.widget.get_parent() is None:
            TOOLSET.call_method("RemoveWidget", args=(bp, info.widget))
    return bp, True


def compile_save(bp, configure_defaults=None):
    require(TOOLSET.call_method("CompileWidgetBlueprint", args=(bp,)), f"Compile failed: {bp.get_name()}")
    defaults = unreal.get_default_object(bp.generated_class())
    defaults.call_method("ApplyDesignerPreviewSettings")
    if configure_defaults:
        configure_defaults(defaults)
    bp.modify()
    require(unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False), f"Save failed: {bp.get_name()}")


def text(bp, name, parent, value, size, color=WHITE, center=False):
    block = add(bp, unreal.TextBlock, name, parent)
    block.set_text(value)
    font = block.get_editor_property("font")
    font.set_editor_property("font_object", FONT)
    font.set_editor_property("size", size)
    block.set_editor_property("font", font)
    block.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    block.set_editor_property("justification", unreal.TextJustify.CENTER if center else unreal.TextJustify.LEFT)
    block.set_editor_property("auto_wrap_text", True)
    block.set_editor_property("wrap_text_at", 0.0)
    return block


def panel(bp, parent, prefix, frame_color):
    frame = add(bp, unreal.Border, prefix + "Frame", parent)
    frame_brush = frame.get_editor_property("background")
    frame_brush.set_editor_property("resource_object", FRAME_TEXTURE)
    frame_brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
    frame_brush.set_editor_property("margin", unreal.Margin(2 / 230, 2 / 134, 2 / 230, 2 / 134))
    frame.set_editor_property("background", frame_brush)
    frame.set_editor_property("brush_color", frame_color)
    frame.set_editor_property("padding", unreal.Margin(3, 3, 3, 3))
    surface = add(bp, unreal.Border, prefix + "Surface", frame)
    surface_brush = surface.get_editor_property("background")
    surface_brush.set_editor_property("resource_object", WHITE_TEXTURE)
    surface_brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    surface.set_editor_property("background", surface_brush)
    surface.set_editor_property("brush_color", DARK)
    surface.set_editor_property("padding", unreal.Margin(14, 11, 14, 11))
    return add(bp, unreal.VerticalBox, prefix + "Content", surface)


def author_item():
    bp, new = create("WBP_ReEchoShopTooltip", "ReEchoShopTooltipWidget")
    if not new:
        return bp
    root = add(bp, unreal.SizeBox, "TooltipRootSizeBox")
    root.set_width_override(280.0)
    stack = add(bp, unreal.VerticalBox, "TooltipStack", root)
    content = panel(bp, stack, "Tooltip", WHITE)
    title = text(bp, "TitleText", content, "样样都通（预览示例）", 19, center=True)
    title.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 0, 0, 8))
    text(bp, "DescriptionText", content,
         "生命值、物理攻击力、元素攻击力与多项属性提升。\n这里是长说明示例，用于预览字体、换行和边框；游戏中会填充当前物品的真实说明。", 16, center=True)
    # Padding belongs INSIDE the collapsible container, leaving no gap when it is hidden.
    outcome = add(bp, unreal.VerticalBox, "OutcomePanel", stack)
    outcome_content = panel(bp, outcome, "Outcome", GOLD)
    frame = outcome_content.get_parent().get_parent()
    frame.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 3, 0, 0))
    outcome_title = text(bp, "OutcomeTitleText", outcome_content, "实际效果", 17, GOLD, center=True)
    outcome_title.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 0, 0, 8))
    text(bp, "OutcomeText", outcome_content,
         "【预览示例，非实际游戏状态】\n物理攻击力 +2\n元素攻击力 +2\n移动速度 +5%\n累计触发 3 次", 16, center=True)
    compile_save(bp)
    return bp


def attribute_sample(row, index):
    icon = unreal.load_asset(f"/Game/ReEcho/Textures/UI/Attributes/{row['IconName']}")
    require(isinstance(icon, unreal.Texture2D), f"Missing attribute icon {row['IconName']}")
    values = ("20", "5", "5", "100%", "5%", "150%", "100%", "100%")
    return unreal.ReEchoAttributeRowView(name=row["DisplayName"], value=values[index] if index < len(values) else "0", icon=icon)


def author_row(sample):
    bp, new = create("WBP_ReEchoAttributeRow", "ReEchoAttributeRowWidget")
    if not new:
        return bp
    root = add(bp, unreal.SizeBox, "AttributeRowSizeBox")
    root.set_min_desired_width(286.0)
    line = add(bp, unreal.HorizontalBox, "AttributeRowContent", root)
    icon_size = add(bp, unreal.SizeBox, "AttributeIconSize", line)
    icon_size.set_width_override(28.0)
    icon_size.set_height_override(28.0)
    icon_size.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 0, 8, 0))
    icon_size.get_editor_property("slot").set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    scale = add(bp, unreal.ScaleBox, "AttributeIconScale", icon_size)
    scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    icon = add(bp, unreal.Image, "AttributeIcon", scale)
    icon.set_brush_from_texture(sample.icon, True)
    # Explicit authored desired size: a texture reference alone can serialize a
    # zero-sized Brush, which cannot be scaled into the outer 28px icon slot.
    brush = icon.get_editor_property("brush").copy()
    image_size = brush.get_editor_property("image_size").copy()
    image_size.set_editor_property("x", 28.0)
    image_size.set_editor_property("y", 28.0)
    brush.set_editor_property("image_size", image_size)
    icon.set_editor_property("brush", brush)
    name = text(bp, "AttributeNameText", line, str(sample.name), 16)
    name_slot = name.get_editor_property("slot")
    name_slot.set_editor_property("size", unreal.SlateChildSize(value=1.0, size_rule=unreal.SlateSizeRule.FILL))
    name_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    value = text(bp, "AttributeValueText", line, str(sample.value), 16)
    value.set_editor_property("justification", unreal.TextJustify.RIGHT)
    value.get_editor_property("slot").set_editor_property("padding", unreal.Margin(8, 0, 0, 0))
    value.get_editor_property("slot").set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    compile_save(bp, lambda defaults: defaults.set_editor_property("designer_preview", sample))
    return bp


def author_attributes(row_bp, samples):
    bp, new = create("WBP_ReEchoAttributeTooltip", "ReEchoAttributeTooltipWidget")
    if not new:
        return bp
    root = add(bp, unreal.SizeBox, "AttributeTooltipRootSizeBox")
    root.set_width_override(320.0)
    content = panel(bp, root, "AttributeTooltip", WHITE)
    rows = add(bp, unreal.VerticalBox, "AttributeRows", content)
    for index, sample in enumerate(samples):
        row = add(bp, row_bp.generated_class(), f"AttributeRow{index}", rows)
        row.set_editor_property("designer_preview", sample)
        row.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 2, 0, 2))
    compile_save(bp, lambda defaults: defaults.set_editor_property("attribute_row_widget_class", row_bp.generated_class()))
    return bp


FONT = unreal.load_asset(FONT_PATH)
require(isinstance(FONT, unreal.Font), "Formal CJK font missing")
FRAME_TEXTURE = unreal.load_asset("/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_DescriptionPanel")
WHITE_TEXTURE = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
require(isinstance(FRAME_TEXTURE, unreal.Texture2D) and isinstance(WHITE_TEXTURE, unreal.Texture2D), "Backplate textures missing")
with (Path(unreal.Paths.project_content_dir()) / "Data" / "attributes.csv").open(encoding="utf-8-sig", newline="") as handle:
    definitions = sorted(csv.DictReader(handle), key=lambda row: int(row["DisplayOrder"]))
samples = [attribute_sample(row, index) for index, row in enumerate(definitions)]
require(samples, "Attribute catalog is empty")
author_item()
row_bp = author_row(samples[0])
author_attributes(row_bp, samples)
unreal.log("[Plan157] PASS three authored tooltip Blueprints saved; existing shop and loadout assets untouched")
