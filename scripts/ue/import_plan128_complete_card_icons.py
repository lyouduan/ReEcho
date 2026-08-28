"""Import and verify the Plan128 complete active-card icon delivery."""

import csv
import os
import struct

import unreal


PROJECT_DIR = os.path.normpath(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(
    PROJECT_DIR, "Content", "SourceArt", "UI", "Cards", "Icon"
)
CARDS_CSV = os.path.join(PROJECT_DIR, "Content", "Data", "cards.csv")
DESTINATION = "/Game/ReEcho/Textures/UI/Cards/Icon"


def is_true(value: str) -> bool:
    return value.strip().lower() == "true"


def load_active_cards():
    with open(CARDS_CSV, encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    active_cards = [
        row
        for row in rows
        if is_true(row["Enabled"]) and is_true(row["Offerable"])
    ]
    if len(active_cards) != 73:
        raise RuntimeError(
            f"Active card set drifted: expected 73 after Plan152 Easter cards, got {len(active_cards)}"
        )
    if len({row["Id"] for row in active_cards}) != len(active_cards):
        raise RuntimeError("Active card ids are not unique")
    if len({row["DisplayName"] for row in active_cards}) != len(active_cards):
        raise RuntimeError("Active card display names are not unique")
    return active_cards


def read_png_size(path):
    with open(path, "rb") as handle:
        header = handle.read(24)
    if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise RuntimeError(f"Source icon is not a valid PNG: {path}")
    return struct.unpack(">II", header[16:24])


def import_icons():
    active_cards = load_active_cards()
    tasks = []
    source_sizes = {}

    for row in active_cards:
        card_id = row["Id"]
        asset_name = f"T_UI_CardIcon_{card_id}"
        source_file = os.path.join(SOURCE_DIR, f"{asset_name}.png")
        if not os.path.isfile(source_file):
            raise RuntimeError(
                f"Missing normalized source icon for {card_id} "
                f"({row['DisplayName']}): {source_file}"
            )
        source_sizes[card_id] = read_png_size(source_file)

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_file)
        task.set_editor_property("destination_path", DESTINATION)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    missing_active_icons = []
    for row, task in zip(active_cards, tasks):
        card_id = row["Id"]
        asset_name = f"T_UI_CardIcon_{card_id}"
        expected_path = f"{DESTINATION}/{asset_name}.{asset_name}"
        imported_paths = list(task.get_editor_property("imported_object_paths"))
        if expected_path not in imported_paths:
            raise RuntimeError(
                f"Card icon import failed for {card_id}: "
                f"expected {expected_path}, got {imported_paths}"
            )

        texture = unreal.load_asset(expected_path)
        if not isinstance(texture, unreal.Texture2D):
            missing_active_icons.append(card_id)
            continue

        texture.set_editor_property("srgb", True)
        texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
        texture.set_editor_property(
            "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
        )
        texture.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
        )
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        if not unreal.EditorAssetLibrary.save_loaded_asset(
            texture, only_if_is_dirty=False
        ):
            raise RuntimeError(f"Failed to save normalized card icon: {expected_path}")

        expected_width, expected_height = source_sizes[card_id]
        actual_width = texture.blueprint_get_size_x()
        actual_height = texture.blueprint_get_size_y()
        if (actual_width, actual_height) != (expected_width, expected_height):
            raise RuntimeError(
                f"Texture size does not match source for {card_id}: "
                f"expected {expected_width}x{expected_height}, "
                f"got {actual_width}x{actual_height}"
            )

    if missing_active_icons:
        raise RuntimeError(
            "Active cards still missing dedicated Texture2D icons: "
            + ", ".join(missing_active_icons)
        )

    unreal.log(
        f"[Plan128][CardIconImport] imported={len(tasks)} "
        f"active_cards_with_icons={len(active_cards)}"
    )


if __name__ == "__main__":
    try:
        import_icons()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
