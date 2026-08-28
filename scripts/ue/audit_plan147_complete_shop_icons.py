"""Audit the normalized Plan147 icon catalog and its runtime Texture2Ds."""

import csv
import hashlib
from pathlib import Path

import unreal


CONTENT_DIR = Path(unreal.Paths.project_content_dir())
PROJECT_DIR = Path(unreal.Paths.project_dir())
DELIVERY_DIR = PROJECT_DIR / "策划数据源" / "icon"
CARDS_CSV = CONTENT_DIR / "Data" / "cards.csv"
PARTS_CSV = CONTENT_DIR / "Data" / "parts.csv"
CARD_SOURCE_DIR = CONTENT_DIR / "SourceArt" / "UI" / "Cards" / "Icon"
PART_SOURCE_DIR = CONTENT_DIR / "SourceArt" / "UI" / "WeaponParts" / "Icons"
PART_MANIFEST = PART_SOURCE_DIR / "icon_manifest.csv"
CARD_ASSET_DIR = "/Game/ReEcho/Textures/UI/Cards/Icon"
PART_ASSET_DIR = "/Game/ReEcho/Textures/UI/WeaponParts/Icons"
SHOP_SOURCE_DIR = CONTENT_DIR / "SourceArt" / "UI" / "InventoryShop" / "Plan110" / "Elements"
SHOP_ASSET_DIR = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"
CATALOG_SOURCE_DIR = CONTENT_DIR / "SourceArt" / "UI" / "IconCatalog"
CATALOG_ASSET_DIR = "/Game/ReEcho/Textures/UI/IconCatalog"
SHOP_STAT_ICONS = (
    "CriticalChance",
    "CriticalEffect",
    "EchoEfficiency",
    "ElementalAttack",
    "Health",
    "MovementSpeed",
    "PhysicalAttack",
    "ReactionEfficiency",
)
SHARED_ICONS = (
    ("Elements", "T_UI_Element_Flame"),
    ("Elements", "T_UI_Element_Lightning"),
    ("Elements", "T_UI_Element_Grass"),
    ("Elements", "T_UI_Element_Water"),
    ("Characters", "T_UI_CharacterIcon_J_DIAMOND"),
    ("Characters", "T_UI_CharacterIcon_J_SPADE"),
    ("Characters", "T_UI_CharacterIcon_J_HEART"),
    ("Characters", "T_UI_CharacterIcon_J_CLOVER"),
)


def is_true(value: str) -> bool:
    return value.strip().lower() == "true"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def require_ui_texture(asset_path: str) -> unreal.Texture2D:
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Missing Texture2D: {asset_path}")
    if texture.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        raise RuntimeError(f"Texture is not in TEXTUREGROUP_UI: {asset_path}")
    if (
        texture.get_editor_property("mip_gen_settings")
        != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    ):
        raise RuntimeError(f"Texture still has mipmaps: {asset_path}")
    return texture


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


active_cards = [
    row
    for row in read_csv(CARDS_CSV)
    if is_true(row["Enabled"]) and is_true(row["Offerable"])
]
all_parts = read_csv(PARTS_CSV)
active_parts = [
    row
    for row in all_parts
    if is_true(row["Enabled"]) and is_true(row["ShopEnabled"])
]
manifest_rows = read_csv(PART_MANIFEST)
manifest = {row["PartId"]: row for row in manifest_rows}
delivery_files = sorted(DELIVERY_DIR.rglob("*.png")) if DELIVERY_DIR.is_dir() else []
normalized_files = []
for normalized_dir in (
    CARD_SOURCE_DIR,
    PART_SOURCE_DIR,
    SHOP_SOURCE_DIR,
    CATALOG_SOURCE_DIR,
):
    normalized_files.extend(normalized_dir.rglob("*.png"))
normalized_hashes = {file_sha256(path) for path in normalized_files}

if len(active_cards) != 64:
    raise RuntimeError(f"Active card set drifted: expected 64, got {len(active_cards)}")
if len(active_parts) != 45:
    raise RuntimeError(f"Active shop part set drifted: expected 45, got {len(active_parts)}")
if len(manifest) != len(manifest_rows):
    raise RuntimeError("Weapon-part icon manifest contains duplicate PartIds")
if len(manifest) != 48:
    raise RuntimeError(f"Delivered part icon set drifted: expected 48, got {len(manifest)}")
if DELIVERY_DIR.is_dir() and len(delivery_files) != 101:
    raise RuntimeError(
        f"Raw Plan147 icon delivery drifted: expected 101 PNGs, got {len(delivery_files)}"
    )
missing_delivery_files = [
    path for path in delivery_files if file_sha256(path) not in normalized_hashes
]
if missing_delivery_files:
    raise RuntimeError(
        "Raw icon delivery has not been normalized into SourceArt: "
        + ", ".join(str(path.relative_to(DELIVERY_DIR)) for path in missing_delivery_files)
    )
part_rows = {row["PartId"]: row for row in all_parts}
if not {row["PartId"] for row in active_parts}.issubset(manifest):
    raise RuntimeError("An active shop part is missing from the delivered icon manifest")
if not set(manifest).issubset(part_rows):
    raise RuntimeError("Weapon-part icon manifest contains an unknown PartId")

for row in active_cards:
    card_id = row["Id"]
    asset_name = f"T_UI_CardIcon_{card_id}"
    source = CARD_SOURCE_DIR / f"{asset_name}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing normalized card icon source: {source}")
    require_ui_texture(f"{CARD_ASSET_DIR}/{asset_name}")

for part_id, manifest_row in manifest.items():
    row = part_rows[part_id]
    if manifest_row["WorkbookName"] != row["DisplayName"]:
        raise RuntimeError(f"Wrong icon mapping for {part_id} ({row['DisplayName']})")
    if manifest_row["MappingKind"] != "exact":
        raise RuntimeError(f"Non-exact icon mapping remains for active part: {part_id}")
    asset_name = f"T_UI_Part_{part_id}"
    source = PART_SOURCE_DIR / f"{asset_name}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing normalized weapon-part icon source: {source}")
    require_ui_texture(f"{PART_ASSET_DIR}/{asset_name}")

for source_name in SHOP_STAT_ICONS:
    source = SHOP_SOURCE_DIR / f"{source_name}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing normalized shop-stat icon source: {source}")
    require_ui_texture(f"{SHOP_ASSET_DIR}/T_UI_Shop110_{source_name}")

for category, asset_name in SHARED_ICONS:
    source = CATALOG_SOURCE_DIR / category / f"{asset_name}.png"
    if not source.is_file():
        raise RuntimeError(f"Missing normalized shared icon source: {source}")
    require_ui_texture(f"{CATALOG_ASSET_DIR}/{category}/{asset_name}")

unreal.log(
    f"[Plan147ShopIconAudit] PASS cards={len(active_cards)} "
    f"active_shop_parts={len(active_parts)} delivered_parts={len(manifest)} "
    f"shop_stats={len(SHOP_STAT_ICONS)} shared={len(SHARED_ICONS)} "
    f"raw_delivery={len(delivery_files) if DELIVERY_DIR.is_dir() else 'not-present'}"
)
