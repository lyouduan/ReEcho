"""Author the formal round-victory state in WBP_ReEchoRestart.

Every visual and text element is a direct child of VictoryCanvas so its CanvasPanelSlot
is visible and independently editable in the UMG Designer. Runtime C++ only projects
the current mode and real values into the named optional bindings.
"""

from pathlib import Path

import unreal


SOURCE_DIR = Path(unreal.Paths.project_content_dir()) / "SourceArt" / "UI" / "Formal" / "RoundVictory"
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/Formal/RoundVictory"
WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
TEXTURES = {
    "VictorySummaryPanel.png": "T_UI_Victory_SummaryPanel",
    "VictoryCharacter.png": "T_UI_Victory_Character",
    "VictoryRoseRight.png": "T_UI_Victory_RoseRight",
    "VictoryContinueButton.png": "T_UI_Victory_ContinueButton",
    "VictoryCardSlot.png": "T_UI_Victory_CardSlot",
    "VictoryTimeShard.png": "T_UI_Victory_TimeShard",
}


def import_texture(source_name, asset_name):
    source_path = SOURCE_DIR / source_name
    if not source_path.is_file():
        raise RuntimeError(f"Missing Plan129 source art: {source_path}")
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_DIR
    task.destination_name = asset_name
    task.filename = str(source_path)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f"{DESTINATION_DIR}/{asset_name}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Failed to import Plan129 texture: {asset_name}")
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Plan129 texture: {asset_name}")
    return texture


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method("AddWidget", args=(blueprint, widget_class, name, parent, child_index))
    if info.widget is None:
        raise RuntimeError(f"Unable to add Plan129 widget: {name}")
    toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, info.widget, True))
    return info.widget


def ensure_widget(toolset, blueprint, widgets, widget_class, name, parent):
    widget = widgets.get(name)
    if widget is None:
        widget = add_widget(toolset, blueprint, widget_class, name, parent)
    if not isinstance(widget, widget_class):
        raise RuntimeError(f"Plan129 widget {name} has unexpected type {widget.get_class().get_name()}")
    return widget


def set_canvas_layout(widget, x, y, width, height, z_order):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct CanvasPanel child")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, y, width, height),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0),
                maximum=unreal.Vector2D(0.0, 0.0),
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


def configure_image(image, texture, tint=None):
    image.set_brush_from_texture(texture, False)
    image.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    if tint is not None:
        image.set_editor_property("color_and_opacity", tint)


def configure_text(text, value, size, color, justification=unreal.TextJustify.LEFT):
    text.set_editor_property("text", value)
    text.set_editor_property("justification", justification)
    text.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    text.set_editor_property("shadow_offset", unreal.Vector2D(0.0, 0.0))
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    font = text.get_editor_property("font")
    font.size = size
    text.set_editor_property("font", font)


def configure_transparent_button(button):
    style = button.get_editor_property("widget_style")
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        style.set_editor_property(brush_name, brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
    button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)


textures = {asset_name: import_texture(source_name, asset_name) for source_name, asset_name in TEXTURES.items()}
white_texture = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
if not isinstance(white_texture, unreal.Texture2D):
    raise RuntimeError("Missing engine white-square texture")

toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Plan129 widget blueprint: {WIDGET_PATH}")
widgets = widget_map(toolset, blueprint)
root = widgets.get("CanvasPanel_0")
if not isinstance(root, unreal.CanvasPanel):
    raise RuntimeError("WBP_ReEchoRestart no longer has its stable CanvasPanel_0 root")

victory_canvas = ensure_widget(toolset, blueprint, widgets, unreal.CanvasPanel, "VictoryCanvas", root)
set_canvas_layout(victory_canvas, 0.0, 0.0, 1920.0, 1080.0, 100)
victory_canvas.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

widgets = widget_map(toolset, blueprint)
images = (
    ("ArtVictoryDimmerFormal", white_texture, (0.0, 0.0, 1920.0, 1080.0), 0, unreal.LinearColor(0.0, 0.0, 0.0, 0.66)),
    ("ArtVictorySummaryPanelFormal", textures["T_UI_Victory_SummaryPanel"], (199.0, 341.0, 1405.0, 500.0), 10, None),
    ("ArtVictoryCharacterFormal", textures["T_UI_Victory_Character"], (1195.0, 192.0, 552.0, 695.0), 30, None),
    ("ArtVictoryRoseRightFormal", textures["T_UI_Victory_RoseRight"], (1134.0, 276.0, 216.0, 160.0), 40, None),
    ("ArtVictoryTimeShardFormal", textures["T_UI_Victory_TimeShard"], (1139.0, 400.0, 56.0, 62.0), 45, None),
    ("ArtVictoryContinueButtonFormal", textures["T_UI_Victory_ContinueButton"], (784.0, 816.0, 405.0, 136.0), 50, None),
)
for name, texture, rect, z_order, tint in images:
    image = ensure_widget(toolset, blueprint, widgets, unreal.Image, name, victory_canvas)
    set_canvas_layout(image, *rect, z_order)
    configure_image(image, texture, tint)
    widgets = widget_map(toolset, blueprint)

for index, x in enumerate((465.0, 627.0, 789.0, 951.0, 1113.0)):
    name = f"DesignerVictoryCardSlot{index}"
    image = ensure_widget(toolset, blueprint, widgets, unreal.Image, name, victory_canvas)
    set_canvas_layout(image, x, 595.0, 136.0, 164.0, 45)
    configure_image(image, textures["T_UI_Victory_CardSlot"])
    widgets = widget_map(toolset, blueprint)

cream = unreal.LinearColor(1.0, 0.956, 0.882, 1.0)
white = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
text_specs = (
    ("VictoryTitleText", "本轮胜利", (735.0, 206.0, 613.0, 139.0), 96, cream, unreal.TextJustify.CENTER, 60),
    ("VictoryEncounterLabel", "到达关卡：", (451.0, 414.0, 201.0, 48.0), 36, white, unreal.TextJustify.LEFT, 60),
    ("VictoryEncounterValue", "5", (652.0, 414.0, 201.0, 48.0), 36, white, unreal.TextJustify.LEFT, 60),
    ("VictoryTraitCountLabel", "构筑数量：", (451.0, 482.0, 201.0, 48.0), 36, white, unreal.TextJustify.LEFT, 60),
    ("VictoryTraitCountValue", "5", (652.0, 482.0, 201.0, 48.0), 36, white, unreal.TextJustify.LEFT, 60),
    ("VictoryTimeShardsLabel", "时间碎片：", (863.0, 412.0, 201.0, 48.0), 36, white, unreal.TextJustify.LEFT, 60),
    ("VictoryTimeShardsValue", "126", (1064.0, 412.0, 92.0, 48.0), 36, white, unreal.TextJustify.LEFT, 60),
    ("VictorySelectedCardsTitle", "本轮已选卡牌", (1015.0, 509.0, 267.0, 71.0), 36, white, unreal.TextJustify.CENTER, 60),
    ("VictoryContinueLabel", "继续", (841.0, 850.0, 292.0, 74.0), 42, cream, unreal.TextJustify.CENTER, 70),
)
for name, value, rect, size, color, justification, z_order in text_specs:
    text = ensure_widget(toolset, blueprint, widgets, unreal.TextBlock, name, victory_canvas)
    set_canvas_layout(text, *rect, z_order)
    configure_text(text, value, size, color, justification)
    widgets = widget_map(toolset, blueprint)

continue_button = ensure_widget(
    toolset, blueprint, widgets, unreal.Button, "VictoryContinueButton", victory_canvas
)
set_canvas_layout(continue_button, 784.0, 816.0, 405.0, 136.0, 65)
configure_transparent_button(continue_button)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoRestart failed to compile after Plan129 authoring")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoRestart failed to save after Plan129 authoring")
unreal.log("[Plan129Victory] formal round-victory UI authored successfully")
