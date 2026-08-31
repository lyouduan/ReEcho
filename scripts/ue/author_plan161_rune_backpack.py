"""Create rune backpack/entry Blueprints once. Existing user edits are never rebuilt."""

import csv
from pathlib import Path
import unreal


ROOT = "/Game/ReEcho/UI"
TOOLS = unreal.UMGToolSet.get_default_object()
WHITE = unreal.LinearColor(1, 1, 1, 1)
DARK = unreal.LinearColor(0.02, 0.02, 0.02, 0.97)


def require(value, message):
    if not value:
        raise RuntimeError(message)


def add(bp, cls, name, parent=None):
    widget = TOOLS.call_method("AddWidget", args=(bp, cls, name, parent, -1)).widget
    require(widget, f"Cannot add {name}")
    TOOLS.call_method("ToggleWidgetAsVariable", args=(bp, widget, True))
    slot = widget.get_editor_property("slot")
    if isinstance(slot, (unreal.SizeBoxSlot, unreal.VerticalBoxSlot, unreal.ButtonSlot)):
        slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
        slot.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    if isinstance(slot, unreal.VerticalBoxSlot):
        slot.set_editor_property("size", unreal.SlateChildSize(size_rule=unreal.SlateSizeRule.AUTOMATIC))
    return widget


def create(name, parent):
    path = f"{ROOT}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log(f"[Plan161] Preserve existing asset: {path}")
        return unreal.load_asset(path), False
    cls = unreal.load_class(None, f"/Script/ReEcho.{parent}")
    require(cls, f"Build C++ parent first: {parent}")
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", cls)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, unreal.WidgetBlueprint, factory)
    require(bp, f"Cannot create {path}")
    for info in TOOLS.call_method("GetWidgets", args=(bp,)).widgets:
        if info.widget and info.widget.get_parent() is None:
            TOOLS.call_method("RemoveWidget", args=(bp, info.widget))
    return bp, True


def finish(bp, defaults_callback=None):
    require(TOOLS.call_method("CompileWidgetBlueprint", args=(bp,)), f"Compile failed: {bp.get_name()}")
    defaults = unreal.get_default_object(bp.generated_class())
    defaults.call_method("ApplyDesignerPreviewSettings")
    if defaults_callback:
        defaults_callback(defaults)
    bp.modify()
    require(unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False), "Save failed")


def text(bp, name, parent, content, size):
    block = add(bp, unreal.TextBlock, name, parent)
    block.set_text(content)
    font = block.get_editor_property("font")
    font.set_editor_property("font_object", FONT)
    font.set_editor_property("size", size)
    block.set_editor_property("font", font)
    block.set_editor_property("color_and_opacity", unreal.SlateColor(WHITE))
    block.set_editor_property("auto_wrap_text", True)
    return block


def chrome(bp, parent, prefix, inset):
    frame = add(bp, unreal.Border, prefix + "Frame", parent)
    brush = frame.get_editor_property("background")
    brush.set_editor_property("resource_object", FRAME)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
    brush.set_editor_property("margin", unreal.Margin(2 / 230, 2 / 134, 2 / 230, 2 / 134))
    frame.set_editor_property("background", brush)
    frame.set_editor_property("brush_color", WHITE)
    frame.set_editor_property("padding", unreal.Margin(3, 3, 3, 3))
    surface = add(bp, unreal.Border, prefix + "Surface", frame)
    brush = surface.get_editor_property("background")
    brush.set_editor_property("resource_object", WHITE_TEXTURE)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    surface.set_editor_property("background", brush)
    surface.set_editor_property("brush_color", DARK)
    surface.set_editor_property("padding", unreal.Margin(inset, inset, inset, inset))
    return surface


def author_entry(sample):
    bp, new = create("WBP_ReEchoRuneBackpackEntry", "ReEchoRuneBackpackEntryWidget")
    if not new:
        return bp
    root = add(bp, unreal.SizeBox, "EntryRootSizeBox")
    root.set_width_override(286)
    root.set_height_override(96)
    button = add(bp, unreal.ReEchoIndexedButton, "SelectButton", root)
    style = button.get_editor_property("widget_style")
    for state in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(state)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        style.set_editor_property(state, brush)
    style.set_editor_property("normal_padding", unreal.Margin(0, 0, 0, 0))
    style.set_editor_property("pressed_padding", unreal.Margin(0, 0, 0, 0))
    button.set_editor_property("widget_style", style)
    surface = chrome(bp, button, "Entry", 8)
    line = add(bp, unreal.HorizontalBox, "EntryContent", surface)
    icon_size = add(bp, unreal.SizeBox, "RuneIconSize", line)
    icon_size.set_width_override(72)
    icon_size.set_height_override(72)
    icon_size.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 0, 10, 0))
    icon_size.get_editor_property("slot").set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    scale = add(bp, unreal.ScaleBox, "RuneIconScale", icon_size)
    scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    icon = add(bp, unreal.Image, "RuneIcon", scale)
    icon.set_brush_from_texture(sample.icon, True)
    # Persist actual desired dimensions; the authoring bridge can leave ImageSize zero.
    brush = icon.get_editor_property("brush").copy()
    image_size = brush.get_editor_property("image_size").copy()
    image_size.set_editor_property("x", float(sample.icon.blueprint_get_size_x()))
    image_size.set_editor_property("y", float(sample.icon.blueprint_get_size_y()))
    brush.set_editor_property("image_size", image_size)
    icon.set_editor_property("brush", brush)
    icon_slot = icon.get_editor_property("slot")
    icon_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
    icon_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    name = text(bp, "NameText", line, str(sample.name), 17)
    name_slot = name.get_editor_property("slot")
    name_slot.set_editor_property("size", unreal.SlateChildSize(value=1, size_rule=unreal.SlateSizeRule.FILL))
    name_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    count = text(bp, "CountText", line, "×2", 17)
    count.set_editor_property("auto_wrap_text", False)
    count.get_editor_property("slot").set_editor_property("padding", unreal.Margin(6, 0, 0, 0))
    count.get_editor_property("slot").set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    finish(bp, lambda defaults: defaults.set_editor_property("designer_preview", sample))
    return bp


def author_panel(entry_bp, samples):
    bp, new = create("WBP_ReEchoRuneBackpack", "ReEchoRuneBackpackWidget")
    if not new:
        return
    root = add(bp, unreal.SizeBox, "BackpackRootSizeBox")
    root.set_width_override(320)
    root.set_height_override(390)
    surface = chrome(bp, root, "Backpack", 14)
    content = add(bp, unreal.VerticalBox, "BackpackContent", surface)
    title = text(bp, "TitleText", content, "符文背包", 20)
    title.set_editor_property("justification", unreal.TextJustify.CENTER)
    title.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 0, 0, 10))
    scroll = add(bp, unreal.ScrollBox, "EntryScroll", content)
    scroll.set_editor_property("scroll_bar_visibility", unreal.SlateVisibility.VISIBLE)
    scroll.get_editor_property("slot").set_editor_property("size", unreal.SlateChildSize(value=1, size_rule=unreal.SlateSizeRule.FILL))
    entries = add(bp, unreal.VerticalBox, "EntryList", scroll)
    for index, sample in enumerate(samples):
        row = add(bp, entry_bp.generated_class(), f"RuneEntry{index}", entries)
        row.set_editor_property("designer_preview", sample)
        row.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 3, 0, 3))
    finish(bp, lambda defaults: defaults.set_editor_property("entry_widget_class", entry_bp.generated_class()))


FONT = unreal.load_asset("/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Texts/方正黑体简体_Font")
FRAME = unreal.load_asset("/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_DescriptionPanel")
WHITE_TEXTURE = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
require(FONT and FRAME and WHITE_TEXTURE, "Existing font/backplate resources missing")
with (Path(unreal.Paths.project_content_dir()) / "Data/parts.csv").open(encoding="utf-8-sig", newline="") as handle:
    parts = {row["PartId"]: row for row in csv.DictReader(handle)}
samples = []
for part_id, count in (("P_CORE_PRIMORDIAL", 2), ("P_CORE_TIDE", 1), ("P_CORE_FOREST", 3)):
    icon = unreal.load_asset(f"/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_{part_id}")
    require(icon, f"Missing example icon {part_id}")
    samples.append(unreal.ReEchoRuneBackpackEntryView(name=parts[part_id]["DisplayName"], count=count, icon=icon))
entry = author_entry(samples[0])
author_panel(entry, samples)
unreal.log("[Plan161] PASS authored rune backpack and nested entry examples; existing shop untouched")
