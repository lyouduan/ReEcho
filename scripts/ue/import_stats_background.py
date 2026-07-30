from pathlib import Path
import unreal


SOURCE = Path(unreal.Paths.project_content_dir()) / "SourceArt" / "UI" / "StatsBackgroundBlurred.png"
DESTINATION_PATH = "/Game/ReEcho/Textures/UI"
ASSET_NAME = "StatsBackground"

if not SOURCE.is_file():
    raise RuntimeError(f"Missing stats background source: {SOURCE}")

task = unreal.AssetImportTask()
task.automated = True
task.destination_path = DESTINATION_PATH
task.destination_name = ASSET_NAME
task.filename = str(SOURCE)
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
if len(task.imported_object_paths) != 1:
    raise RuntimeError(f"Expected one imported texture, got: {task.imported_object_paths}")

texture = unreal.load_asset(task.imported_object_paths[0])
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Imported asset is not Texture2D: {task.imported_object_paths[0]}")
texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
unreal.log(f"Imported stats background: {task.imported_object_paths[0]}")
