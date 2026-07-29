from pathlib import Path
import unreal

ASSET_NAMES = (
    "Enemy_Slime", "Enemy_ThornSlime", "Enemy_RabbitDoll",
    "Enemy_RabbitBeast", "Enemy_GoatPriest", "Enemy_DarkPriest",
    "Merchant_ClockKeeper",
    "Player_Cat", "Player_Heart", "Player_Spade", "Player_Clover", "Player_Diamond",
    "Echo_Cat", "Echo_Heart", "Echo_Spade", "Echo_Clover", "Echo_Diamond",
)
DESTINATION_PATH = "/Game/ReEcho/Textures/Characters/NewCast"

def main() -> None:
    source = Path(unreal.Paths.project_content_dir()) / "SourceArt" / "Characters" / "NewCast" / "Sprites"
    tasks = []
    for name in ASSET_NAMES:
        path = source / f"{name}.png"
        if not path.is_file():
            raise RuntimeError(f"Missing sprite source: {path}")
        task = unreal.AssetImportTask()
        task.automated = True
        task.destination_path = DESTINATION_PATH
        task.destination_name = name
        task.filename = str(path)
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = [path for task in tasks for path in task.imported_object_paths]
    if len(imported) != len(ASSET_NAMES):
        raise RuntimeError(f"Expected {len(ASSET_NAMES)} textures, got {len(imported)}: {imported}")
    for object_path in imported:
        texture = unreal.load_asset(object_path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Imported asset is not Texture2D: {object_path}")
        texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    legacy_asset = f"{DESTINATION_PATH}/MiniBoss_ClockKeeper"
    if unreal.EditorAssetLibrary.does_asset_exist(legacy_asset):
        if not unreal.EditorAssetLibrary.delete_asset(legacy_asset):
            raise RuntimeError(f"Failed to remove legacy enemy asset: {legacy_asset}")
    unreal.log(f"Imported NewCast textures: {imported}")

main()
