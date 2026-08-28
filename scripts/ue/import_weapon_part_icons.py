"""Import every delivered weapon-part icon using stable PartId asset names."""

import csv
from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "WeaponParts"
    / "Icons"
)
DESTINATION_ROOT = "/Game/ReEcho/Textures/UI/WeaponParts/Icons"
MANIFEST_PATH = SOURCE_ROOT / "icon_manifest.csv"
PARTS_CSV = Path(unreal.Paths.project_content_dir()) / "Data" / "parts.csv"


def configure_ui_texture(texture: unreal.Texture2D) -> None:
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def is_true(value: str) -> bool:
    return value.strip().lower() == "true"


def load_parts() -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    with PARTS_CSV.open(encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    parts = [
        row
        for row in rows
        if is_true(row["Enabled"]) and is_true(row["ShopEnabled"])
    ]
    if len(parts) != 45:
        raise RuntimeError(f"Active shop part set drifted: expected 45, got {len(parts)}")
    return rows, parts


def load_manifest() -> dict[str, dict[str, str]]:
    with MANIFEST_PATH.open(encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    manifest = {row["PartId"]: row for row in rows}
    if len(manifest) != len(rows):
        raise RuntimeError("Weapon-part icon manifest contains duplicate PartIds")
    return manifest


def import_texture(source: Path) -> str:
    asset_name = source.stem
    expected_path = f"{DESTINATION_ROOT}/{asset_name}"
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_ROOT
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(expected_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported asset is not Texture2D: {expected_path}")
    configure_ui_texture(texture)
    return expected_path


all_parts, active_parts = load_parts()
manifest = load_manifest()
active_ids = {row["PartId"] for row in active_parts}
part_rows = {row["PartId"]: row for row in all_parts}
if len(manifest) != 48:
    raise RuntimeError(f"Delivered part icon set drifted: expected 48, got {len(manifest)}")
if not active_ids.issubset(manifest):
    raise RuntimeError(f"Active parts missing icons: {sorted(active_ids - set(manifest))}")
if not set(manifest).issubset(part_rows):
    raise RuntimeError(f"Manifest contains unknown PartIds: {sorted(set(manifest) - set(part_rows))}")

sources = []
for part_id, manifest_row in manifest.items():
    row = part_rows[part_id]
    if manifest_row["WorkbookName"] != row["DisplayName"]:
        raise RuntimeError(
            f"Display-name mismatch for {part_id}: "
            f"parts.csv={row['DisplayName']}, manifest={manifest_row['WorkbookName']}"
        )
    if manifest_row["MappingKind"] != "exact":
        raise RuntimeError(f"Delivered part does not use an exact icon: {part_id}")
    source = SOURCE_ROOT / f"T_UI_Part_{part_id}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing normalized source icon for {part_id}: {source}")
    sources.append(source)

imported = [import_texture(source) for source in sources]
unreal.log(
    f"[WeaponPartIconImport] imported={len(imported)} exact delivered icons "
    f"active_shop={len(active_parts)} dormant={len(imported) - len(active_parts)}"
)
