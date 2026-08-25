"""Read-only Plan102 audit for timer cleanup and minimap profile icons."""

import csv
import hashlib
from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "CombatHud"
    / "Plan102"
)
MANIFEST_PATH = SOURCE_ROOT / "_SourceManifest.csv"
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/CombatHud/Minimap"
PROFILE_ROOT = "/Game/ReEcho/DataAsset/Character/Profiles"
ENCOUNTER_HUD = "/Game/ReEcho/UI/WBP_ReEchoEncounterHud"
EXPECTED_BINDINGS = {
    (character_id, role)
    for character_id in ("J_HEART", "J_SPADE", "J_CLOVER", "J_DIAMOND")
    for role in ("Player", "Echo")
}


def nearly_equal(actual, expected):
    return abs(float(actual) - float(expected)) <= 0.01


def profile_path(character_id, role):
    prefix = "DA_Character" if role == "Player" else "DA_Echo"
    return f"{PROFILE_ROOT}/{prefix}_{character_id}"


with MANIFEST_PATH.open("r", encoding="utf-8-sig", newline="") as handle:
    rows = list(csv.DictReader(handle))
if len(rows) != 8:
    raise RuntimeError(f"Expected 8 Plan102 minimap icon rows, got {len(rows)}")
actual_bindings = {(row["CharacterId"], row["Role"]) for row in rows}
if actual_bindings != EXPECTED_BINDINGS:
    raise RuntimeError(f"Unexpected Plan102 profile mapping: {actual_bindings}")

for row in rows:
    source_path = SOURCE_ROOT / row["SourceFile"]
    if not source_path.is_file():
        raise RuntimeError(f"Missing Plan102 minimap source: {source_path}")
    if source_path.stat().st_size != int(row["Bytes"]):
        raise RuntimeError(f"Source byte count mismatch: {source_path}")
    source_hash = hashlib.sha256(source_path.read_bytes()).hexdigest()
    if source_hash.lower() != row["Sha256"].lower():
        raise RuntimeError(f"Source SHA-256 mismatch: {source_path}")

    texture_path = f"{TEXTURE_ROOT}/{row['RuntimeAsset']}"
    texture = unreal.load_asset(texture_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Missing Plan102 minimap Texture2D: {texture_path}")
    if texture.blueprint_get_size_x() != 512 or texture.blueprint_get_size_y() != 512:
        raise RuntimeError(f"Unexpected minimap texture size: {texture_path}")
    if (
        texture.get_editor_property("mip_gen_settings")
        != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    ):
        raise RuntimeError(f"Minimap texture must disable mipmaps: {texture_path}")
    if not texture.get_editor_property("srgb"):
        raise RuntimeError(f"Minimap texture must use sRGB: {texture_path}")
    if texture.get_editor_property("filter") != unreal.TextureFilter.TF_BILINEAR:
        raise RuntimeError(f"Minimap texture must use bilinear filtering: {texture_path}")
    if (
        texture.get_editor_property("compression_settings")
        != unreal.TextureCompressionSettings.TC_EDITOR_ICON
    ):
        raise RuntimeError(f"Minimap texture must use EditorIcon compression: {texture_path}")
    if texture.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        raise RuntimeError(f"Minimap texture must use UI LOD group: {texture_path}")

    bound_profile_path = profile_path(row["CharacterId"], row["Role"])
    profile = unreal.load_asset(bound_profile_path)
    if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
        raise RuntimeError(f"Missing presentation profile: {bound_profile_path}")
    if profile.get_editor_property("minimap_icon") != texture:
        raise RuntimeError(
            f"Profile minimap icon mismatch: {bound_profile_path} -> {texture_path}"
        )
    unreal.log(
        f"[Plan102Audit] PROFILE role={row['Role']} character={row['CharacterId']} "
        f"profile={bound_profile_path} icon={texture_path}"
    )

blueprint = unreal.load_asset(ENCOUNTER_HUD)
if blueprint is None:
    raise RuntimeError(f"Missing Encounter HUD: {ENCOUNTER_HUD}")
toolset = unreal.UMGToolSet.get_default_object()
infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {
    str(info.widget_name): info.widget for info in infos if info.widget is not None
}
if widgets.get("ArtTimeReadout") is not None:
    raise RuntimeError("Obsolete ArtTimeReadout remains in Encounter HUD")
needle = widgets.get("ArtClockNeedle")
if not isinstance(needle, unreal.Image):
    raise RuntimeError("Encounter HUD is missing ArtClockNeedle Image")
pivot = needle.get_editor_property("render_transform_pivot")
if not (nearly_equal(pivot.x, 0.5) and nearly_equal(pivot.y, 0.12)):
    raise RuntimeError(f"Unexpected clock needle pivot: {pivot}")
if unreal.EditorAssetLibrary.does_asset_exist(
    "/Game/ReEcho/Textures/UI/CombatHud/T_UI_CombatHud_TimeReadout"
):
    raise RuntimeError("Obsolete countdown background Texture2D still exists")

unreal.log(
    f"[Plan102Audit] Encounter HUD widgets={len(infos)} has_readout=False "
    f"needle_pivot={pivot} profiles={len(rows)}"
)
