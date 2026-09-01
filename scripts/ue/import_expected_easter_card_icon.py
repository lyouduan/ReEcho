"""Import and verify the authored icon for the Expected Outcome Easter card."""

import os
import struct

import unreal


PROJECT_DIR = os.path.normpath(unreal.Paths.project_dir())
SOURCE = os.path.join(
    PROJECT_DIR,
    "Content",
    "SourceArt",
    "UI",
    "Cards",
    "Icon",
    "T_UI_CardIcon_G_4_10.png",
)
DESTINATION = "/Game/ReEcho/Textures/UI/Cards/Icon"
ASSET_NAME = "T_UI_CardIcon_G_4_10"


def read_png_size(path):
    with open(path, "rb") as handle:
        header = handle.read(24)
    if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise RuntimeError(f"Source icon is not a valid PNG: {path}")
    return struct.unpack(">II", header[16:24])


def main():
    expected_size = read_png_size(SOURCE)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", ASSET_NAME)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    expected_path = f"{DESTINATION}/{ASSET_NAME}.{ASSET_NAME}"
    imported = list(task.get_editor_property("imported_object_paths"))
    if expected_path not in imported:
        raise RuntimeError(f"Import failed: {imported}")

    texture = unreal.load_asset(expected_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported asset is not Texture2D: {expected_path}")
    actual_size = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
    if actual_size != expected_size:
        raise RuntimeError(f"Size mismatch: {actual_size} != {expected_size}")
    unreal.log(f"[EasterCardIconImport] imported={ASSET_NAME} size={actual_size}")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
