"""Import the two authored card icons missing from the runtime asset directory."""

import os

import unreal


CARD_IDS = ("G_2_07", "G_2_08")
SOURCE_DIR = os.path.join(
    unreal.Paths.project_dir(), "Content", "SourceArt", "UI", "Cards", "Icon"
)
DESTINATION = "/Game/ReEcho/Textures/UI/Cards/Icon"

tasks = []
for card_id in CARD_IDS:
    asset_name = f"T_UI_CardIcon_{card_id}"
    source_file = os.path.join(SOURCE_DIR, f"{asset_name}.png")
    if not os.path.isfile(source_file):
        raise RuntimeError(f"Missing source icon: {source_file}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source_file)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    if len(task.get_editor_property("imported_object_paths")) != 1:
        raise RuntimeError(f"Card icon import failed: {task.get_editor_property('filename')}")

unreal.log(f"[CardIconImport] imported {len(tasks)} missing card icons")
