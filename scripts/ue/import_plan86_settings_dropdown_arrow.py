"""Import the delivered down-arrow art and apply it to Settings dropdowns.

This script deliberately changes only Image brushes.  Designer-authored slots,
positions, sizes and z-order are left untouched.
"""

from pathlib import Path

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings"
TEXTURE_NAME = "T_UI_Settings_DropdownArrowDown"
SOURCE_PATH = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InteractionPlaceholder"
    / "Elements"
    / "Settings"
    / "下拉框箭头下.png"
)
ARROW_WIDGETS = (
    "GraphicsArrow0",
    "GraphicsArrow1",
    "GraphicsArrow2",
    "GraphicsArrow3",
    "AudioOutputArrow",
)


if not SOURCE_PATH.is_file():
    raise RuntimeError(f"Missing Settings dropdown arrow source: {SOURCE_PATH}")

task = unreal.AssetImportTask()
task.set_editor_property("automated", True)
task.set_editor_property("destination_path", DESTINATION_DIR)
task.set_editor_property("destination_name", TEXTURE_NAME)
task.set_editor_property("filename", str(SOURCE_PATH))
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

texture_path = f"{DESTINATION_DIR}/{TEXTURE_NAME}"
texture = unreal.load_asset(texture_path)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Failed to import Settings dropdown arrow: {texture_path}")
texture.set_editor_property("srgb", True)
texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
texture.set_editor_property(
    "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
)
texture.set_editor_property(
    "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
)
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Settings dropdown arrow: {texture_path}")

toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget: {ASSET_PATH}")
widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}

for widget_name in ARROW_WIDGETS:
    image = widgets.get(widget_name)
    if not isinstance(image, unreal.Image):
        raise RuntimeError(f"Missing Settings dropdown arrow Image: {widget_name}")
    brush = image.get_editor_property("brush")
    brush.set_editor_property("resource_object", texture)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    brush.set_editor_property(
        "tint_color",
        unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
    )
    image.set_editor_property("brush", brush)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError(f"Failed to compile Settings widget: {ASSET_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Settings widget: {ASSET_PATH}")

unreal.log(
    "[Plan86DropdownArrow] imported delivered art and updated "
    + ",".join(ARROW_WIDGETS)
    + "; designer geometry preserved"
)
