from pathlib import Path

import unreal


FRAME_NAMES = (
    "MushroomGirl_Idle_00",
    "MushroomGirl_Idle_01",
    "MushroomGirl_Idle_02",
    "MushroomGirl_Attack_00",
    "MushroomGirl_Attack_01",
)
DESTINATION_PATH = "/Game/ReEcho/Textures/Characters/MushroomGirl"


def main() -> None:
    source_directory = (
        Path(unreal.Paths.project_content_dir())
        / "SourceArt"
        / "Characters"
        / "MushroomGirl"
    )
    source_files = [source_directory / f"{name}.png" for name in FRAME_NAMES]
    missing_files = [str(path) for path in source_files if not path.is_file()]
    if missing_files:
        raise RuntimeError(f"Missing MushroomGirl source frames: {missing_files}")

    tasks = []
    for source_file in source_files:
        task = unreal.AssetImportTask()
        task.automated = True
        task.destination_path = DESTINATION_PATH
        task.filename = str(source_file)
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported_object_paths = [
        object_path
        for task in tasks
        for object_path in task.imported_object_paths
    ]
    if len(imported_object_paths) != len(FRAME_NAMES):
        raise RuntimeError(
            f"Expected {len(FRAME_NAMES)} imported textures, got "
            f"{len(imported_object_paths)}: {imported_object_paths}"
        )

    for object_path in imported_object_paths:
        texture = unreal.load_asset(object_path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Imported asset is not Texture2D: {object_path}")
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)

    unreal.log(f"Imported MushroomGirl frames: {imported_object_paths}")


main()
