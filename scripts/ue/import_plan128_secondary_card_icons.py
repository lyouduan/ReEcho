"""Import and verify the Plan128 secondary-card icon delivery."""

import csv
import os

import unreal


CARD_NAMES = {
    "G_2_18": "数字挑战",
    "G_2_19": "诅咒银行",
    "G_2_20": "德古拉第I课",
    "G_2_21": "喂，打劫！",
    "G_2_22": "重启任务",
    "G_2_23": "孤注二掷",
    "G_2_24": "我来组成头部",
    "G_2_25": "我来组成身体",
    "G_2_26": "我来组成腿部",
    "G_2_27": "损人利己",
    "G_2_28": "就要那个！",
    "G_2_29": "自我赛跑",
    "G_2_30": "连接，连接！",
    "G_2_31": "烈火之心",
    "G_2_32": "水火不容",
    "G_2_33": "雷鸣之心",
    "G_2_34": "生命虹吸",
    "G_2_35": "史莱姆杀手",
    "G_2_36": "兔兔杀手",
}

PROJECT_DIR = os.path.normpath(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(
    PROJECT_DIR, "Content", "SourceArt", "UI", "Cards", "Icon"
)
CARDS_CSV = os.path.join(PROJECT_DIR, "Content", "Data", "cards.csv")
DESTINATION = "/Game/ReEcho/Textures/UI/Cards/Icon"


def is_true(value: str) -> bool:
    return value.strip().lower() == "true"


def load_card_rows():
    with open(CARDS_CSV, encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    return {row["Id"]: row for row in rows}


def import_icons():
    rows = load_card_rows()
    tasks = []
    for card_id, expected_name in CARD_NAMES.items():
        row = rows.get(card_id)
        if row is None:
            raise RuntimeError(f"Card id is absent from cards.csv: {card_id}")
        if row["DisplayName"] != expected_name or row["Tier"] != "2":
            raise RuntimeError(
                f"Card mapping drifted for {card_id}: "
                f"name={row['DisplayName']!r} tier={row['Tier']!r}"
            )
        if not is_true(row["Enabled"]) or not is_true(row["Offerable"]):
            raise RuntimeError(
                f"Delivered secondary card is not active: {card_id} "
                f"enabled={row['Enabled']!r} offerable={row['Offerable']!r}"
            )

        asset_name = f"T_UI_CardIcon_{card_id}"
        source_file = os.path.join(SOURCE_DIR, f"{asset_name}.png")
        if not os.path.isfile(source_file):
            raise RuntimeError(f"Missing source icon: {source_file}")

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_file)
        task.set_editor_property("destination_path", DESTINATION)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for card_id, task in zip(CARD_NAMES, tasks):
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
            raise RuntimeError(f"Imported asset is not a Texture2D: {expected_path}")
        if texture.blueprint_get_size_x() != 512 or texture.blueprint_get_size_y() != 512:
            raise RuntimeError(
                f"Unexpected texture size for {card_id}: "
                f"{texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
            )

    active_tier_two = [
        row
        for row in rows.values()
        if row["Tier"] == "2" and is_true(row["Enabled"]) and is_true(row["Offerable"])
    ]
    missing_active_icons = []
    for row in active_tier_two:
        asset_name = f"T_UI_CardIcon_{row['Id']}"
        asset_path = f"{DESTINATION}/{asset_name}.{asset_name}"
        if not isinstance(unreal.load_asset(asset_path), unreal.Texture2D):
            missing_active_icons.append(row["Id"])
    if missing_active_icons:
        raise RuntimeError(
            "Active secondary cards still missing dedicated icons: "
            + ", ".join(missing_active_icons)
        )

    unreal.log(
        f"[Plan128][CardIconImport] imported={len(tasks)} "
        f"active_tier_two_with_icons={len(active_tier_two)}"
    )


if __name__ == "__main__":
    try:
        import_icons()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
