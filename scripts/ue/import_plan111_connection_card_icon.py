"""Import the Plan 111 icon for the runtime card G_2_30 (连接，连接！)."""

import os

import unreal


CARD_ID = "G_2_30"
ASSET_NAME = f"T_UI_CardIcon_{CARD_ID}"
SOURCE_FILE = os.path.join(
    unreal.Paths.project_dir(),
    "Content",
    "SourceArt",
    "UI",
    "Cards",
    "Icon",
    f"{ASSET_NAME}.png",
)
DESTINATION = "/Game/ReEcho/Textures/UI/Cards/Icon"

if not os.path.isfile(SOURCE_FILE):
    raise RuntimeError(f"Missing source icon: {SOURCE_FILE}")

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE_FILE)
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("destination_name", ASSET_NAME)
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
imported_paths = task.get_editor_property("imported_object_paths")
expected_path = f"{DESTINATION}/{ASSET_NAME}.{ASSET_NAME}"
if expected_path not in imported_paths:
    raise RuntimeError(
        f"Connection card icon import failed: expected {expected_path}, got {imported_paths}"
    )

unreal.log(f"[Plan111][CardIconImport] imported {expected_path}")
