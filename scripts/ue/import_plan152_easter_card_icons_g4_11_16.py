"""Import and verify the six new Plan152 Easter-card icons (G_4_11..G_4_16)."""

import os
import struct

import unreal


PROJECT_DIR = os.path.normpath(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "Content", "SourceArt", "UI", "Cards", "Icon")
DESTINATION = "/Game/ReEcho/Textures/UI/Cards/Icon"
CARD_IDS = [f"G_4_{index}" for index in (11, 12, 13, 14, 15, 16)]


def read_png_size(path):
    with open(path, "rb") as handle:
        header = handle.read(24)
    if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise RuntimeError(f"Source icon is not a valid PNG: {path}")
    return struct.unpack(">II", header[16:24])


def main():
    tasks = []
    expected_sizes = {}
    for card_id in CARD_IDS:
        asset_name = f"T_UI_CardIcon_{card_id}"
        source = os.path.join(SOURCE_DIR, f"{asset_name}.png")
        expected_sizes[card_id] = read_png_size(source)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source)
        task.set_editor_property("destination_path", DESTINATION)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for card_id, task in zip(CARD_IDS, tasks):
        asset_name = f"T_UI_CardIcon_{card_id}"
        expected_path = f"{DESTINATION}/{asset_name}.{asset_name}"
        imported = list(task.get_editor_property("imported_object_paths"))
        if expected_path not in imported:
            raise RuntimeError(f"Import failed for {card_id}: {imported}")
        texture = unreal.load_asset(expected_path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Imported asset is not Texture2D: {expected_path}")
        actual = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
        if actual != expected_sizes[card_id]:
            raise RuntimeError(f"Size mismatch for {card_id}: {actual} != {expected_sizes[card_id]}")
    unreal.log(f"[Plan152][EasterCardIconImport] imported={len(tasks)} ids={CARD_IDS}")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
