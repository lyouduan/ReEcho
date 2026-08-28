"""Author the formal three-slot save-rollback page in WBP_ReEchoStartMenu.

All presentation widgets are authored into the UMG asset for WYSIWYG editing.
Runtime code only swaps save data, screenshot textures, state and click actions.
"""

from pathlib import Path

import unreal


SOURCE_DIR = Path(unreal.Paths.project_content_dir()) / "SourceArt" / "UI" / "SaveRollback" / "Plan145"
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/SaveRollback/Plan145"
WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoStartMenu"
TEXTURES = {
    "SaveRollbackFrame.png": "T_UI_SaveRollback_Frame",
    "SaveRollbackSurface.png": "T_UI_SaveRollback_Surface",
    "SaveRollbackTitle.png": "T_UI_SaveRollback_Title",
    "SaveSlotOccupied.png": "T_UI_SaveRollback_SlotOccupied",
    "SaveSlotEmpty.png": "T_UI_SaveRollback_SlotEmpty",
    "SaveSlotPlaceholder.png": "T_UI_SaveRollback_Placeholder",
    "SaveSlotNoteStrip.png": "T_UI_SaveRollback_NoteStrip",
    "SaveRollbackClose.png": "T_UI_SaveRollback_Close",
}


def import_texture(source_name, asset_name):
    source_path = SOURCE_DIR / source_name
    if not source_path.is_file():
        raise RuntimeError(f"Missing staged Plan145 source art: {source_path}")
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
        raise RuntimeError(f"Failed to import Plan145 texture: {asset_name}")
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Plan145 texture: {asset_name}")
    return texture


def widget_infos(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in widget_infos(toolset, blueprint)
        if info.widget
    }


def mark_variable(toolset, blueprint, widget):
    infos = {info.widget.get_name(): info for info in widget_infos(toolset, blueprint) if info.widget}
    info = infos.get(widget.get_name())
    if info is not None and not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method("AddWidget", args=(blueprint, widget_class, name, parent, child_index))
    if info.widget is None:
        raise RuntimeError(f"Unable to add Plan145 widget: {name}")
    mark_variable(toolset, blueprint, info.widget)
    return info.widget


def set_canvas_layout(widget, x, y, width, height, z_order=0):
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


def configure_image(image, texture):
    image.set_brush_from_texture(texture, False)
    image.set_editor_property("color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    image.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)


def add_image(toolset, blueprint, parent, name, texture, rect, z_order=0):
    image = add_widget(toolset, blueprint, unreal.Image, name, parent)
    configure_image(image, texture)
    set_canvas_layout(image, *rect, z_order)
    return image


def configure_text(text, value, size, color, justification=unreal.TextJustify.LEFT):
    text.set_editor_property("text", value)
    text.set_editor_property("justification", justification)
    text.set_editor_property("auto_wrap_text", False)
    text.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    text.set_editor_property("shadow_offset", unreal.Vector2D(0.0, 0.0))
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    font = text.get_editor_property("font")
    font.size = size
    text.set_editor_property("font", font)


def add_text(toolset, blueprint, parent, name, value, rect, size, color, justification=unreal.TextJustify.LEFT, z_order=0):
    text = add_widget(toolset, blueprint, unreal.TextBlock, name, parent)
    configure_text(text, value, size, color, justification)
    set_canvas_layout(text, *rect, z_order)
    return text


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
toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Plan145 widget blueprint: {WIDGET_PATH}")
widgets = widget_map(toolset, blueprint)
root = widgets.get("CanvasPanel_0")
if not isinstance(root, unreal.CanvasPanel):
    raise RuntimeError("WBP_ReEchoStartMenu no longer has its stable CanvasPanel_0 root")

# Remove only the Plan145-owned containers so this script can be rerun safely.
for owned_name in ("SaveRollbackPanel", "StartMenuPanel"):
    owned = widgets.get(owned_name)
    if owned is not None:
        if not toolset.call_method("RemoveWidget", args=(blueprint, owned)):
            raise RuntimeError(f"Unable to replace {owned_name}")
        widgets = widget_map(toolset, blueprint)

# Preserve the existing start-menu composition, but give it one switchable parent.
main_panel = add_widget(toolset, blueprint, unreal.CanvasPanel, "StartMenuPanel", root)
set_canvas_layout(main_panel, 0.0, 0.0, 1920.0, 1080.0, 10)
main_panel.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
for name in ("ArtTitleLogo", "ContinueGame", "NewGame", "About", "Settings", "Quit"):
    widget = widgets.get(name)
    if widget is None:
        raise RuntimeError(f"Missing existing start-menu widget: {name}")
    old_slot = widget.get_editor_property("slot")
    if not isinstance(old_slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Existing start-menu widget is not on a CanvasPanel: {name}")
    layout_data = old_slot.get_editor_property("layout_data")
    auto_size = old_slot.get_editor_property("auto_size")
    z_order = old_slot.get_editor_property("z_order")
    if not toolset.call_method("MoveWidget", args=(blueprint, widget, main_panel, -1)):
        raise RuntimeError(f"Unable to move {name} into StartMenuPanel")
    new_slot = widget.get_editor_property("slot")
    new_slot.set_editor_property("layout_data", layout_data)
    new_slot.set_editor_property("auto_size", auto_size)
    new_slot.set_editor_property("z_order", z_order)

save_panel = add_widget(toolset, blueprint, unreal.CanvasPanel, "SaveRollbackPanel", root)
set_canvas_layout(save_panel, 0.0, 0.0, 1920.0, 1080.0, 100)
save_panel.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

# Formal frame and surface.
add_image(toolset, blueprint, save_panel, "ArtSaveRollbackSurface", textures["T_UI_SaveRollback_Surface"], (366.0, 178.0, 1210.0, 701.0), 0)
add_image(toolset, blueprint, save_panel, "ArtSaveRollbackFrame", textures["T_UI_SaveRollback_Frame"], (268.0, 96.0, 1389.0, 879.0), 10)
add_image(toolset, blueprint, save_panel, "ArtSaveRollbackTitle", textures["T_UI_SaveRollback_Title"], (392.0, 138.0, 224.0, 86.0), 20)
add_image(toolset, blueprint, save_panel, "ArtSaveRollbackClose", textures["T_UI_SaveRollback_Close"], (1501.0, 158.0, 122.0, 119.0), 30)

black = unreal.LinearColor(0.02, 0.012, 0.008, 1.0)
cream = unreal.LinearColor(1.0, 0.92, 0.78, 1.0)
for index, y in enumerate((270.0, 453.0, 636.0)):
    occupied = add_image(
        toolset,
        blueprint,
        save_panel,
        f"SaveSlotOccupiedArt{index}",
        textures["T_UI_SaveRollback_SlotOccupied"],
        (416.0, y, 1088.0, 155.0),
        20,
    )
    empty = add_image(
        toolset,
        blueprint,
        save_panel,
        f"SaveSlotEmptyArt{index}",
        textures["T_UI_SaveRollback_SlotEmpty"],
        (416.0, y, 1088.0, 155.0),
        21,
    )
    occupied.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
    empty.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    add_image(
        toolset,
        blueprint,
        save_panel,
        f"SaveSlotPreview{index}",
        textures["T_UI_SaveRollback_Placeholder"],
        (416.0, y, 236.0, 155.0),
        25,
    )
    add_image(
        toolset,
        blueprint,
        save_panel,
        f"DesignerSaveSlotNoteStrip{index}",
        textures["T_UI_SaveRollback_NoteStrip"],
        (669.0, y + 92.0, 445.0, 45.0),
        26,
    )
    add_text(
        toolset,
        blueprint,
        save_panel,
        f"SaveSlotName{index}",
        "新建存档",
        (672.0, y + 23.0, 300.0, 58.0),
        36,
        cream,
        z_order=30,
    )
    add_text(
        toolset,
        blueprint,
        save_panel,
        f"SaveSlotMetadata{index}",
        "创建新存档进入游戏",
        (678.0, y + 96.0, 425.0, 38.0),
        24,
        cream,
        z_order=30,
    )
    add_text(
        toolset,
        blueprint,
        save_panel,
        f"SaveSlotTimestamp{index}",
        "",
        (1212.0, y + 109.0, 275.0, 36.0),
        24,
        black,
        unreal.TextJustify.RIGHT,
        30,
    )
    button = add_widget(toolset, blueprint, unreal.ReEchoIndexedButton, f"SaveSlotButton{index}", save_panel)
    configure_transparent_button(button)
    set_canvas_layout(button, 416.0, y, 1088.0, 155.0, 50)

close_button = add_widget(toolset, blueprint, unreal.ReEchoIndexedButton, "SaveRollbackCloseButton", save_panel)
configure_transparent_button(close_button)
set_canvas_layout(close_button, 1501.0, 158.0, 122.0, 119.0, 50)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoStartMenu failed to compile after Plan145 authoring")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoStartMenu failed to save after Plan145 authoring")
unreal.log("[Plan145SaveRollback] formal three-slot save-rollback UI authored successfully")
