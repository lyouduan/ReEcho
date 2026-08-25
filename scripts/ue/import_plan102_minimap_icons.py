"""Import Plan102 minimap icons and bind them to Player/Echo presentation profiles.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript`` after rebuilding the
Editor target that exposes ``UReEcho2DCharacterPresentationProfile.MinimapIcon``.
The operation is idempotent. Pass ``-Plan102ReimportExisting`` to replace the
existing Texture2D source data.
"""

import csv
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
DESTINATION_ROOT = "/Game/ReEcho/Textures/UI/CombatHud/Minimap"
PROFILE_ROOT = "/Game/ReEcho/DataAsset/Character/Profiles"
REIMPORT_EXISTING = "-Plan102ReimportExisting" in unreal.SystemLibrary.get_command_line()


def configure_ui_texture(texture):
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


def import_texture(source_path, asset_name):
    expected_path = f"{DESTINATION_ROOT}/{asset_name}"
    existing = unreal.load_asset(expected_path)
    if existing is not None and not REIMPORT_EXISTING:
        if not isinstance(existing, unreal.Texture2D):
            raise RuntimeError(f"Existing minimap asset is not Texture2D: {expected_path}")
        configure_ui_texture(existing)
        return existing

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_ROOT
    task.destination_name = asset_name
    task.filename = str(source_path)
    task.replace_existing = REIMPORT_EXISTING
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(expected_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported minimap asset is not Texture2D: {expected_path}")
    configure_ui_texture(texture)
    return texture


def profile_path(character_id, role):
    prefix = "DA_Character" if role == "Player" else "DA_Echo"
    return f"{PROFILE_ROOT}/{prefix}_{character_id}"


if not MANIFEST_PATH.is_file():
    raise RuntimeError(f"Missing Plan102 source manifest: {MANIFEST_PATH}")

with MANIFEST_PATH.open("r", encoding="utf-8-sig", newline="") as handle:
    rows = list(csv.DictReader(handle))

if len(rows) != 8:
    raise RuntimeError(f"Expected 8 Plan102 minimap icon rows, got {len(rows)}")

saved_profiles = []
for row in rows:
    source_path = SOURCE_ROOT / row["SourceFile"]
    if not source_path.is_file():
        raise RuntimeError(f"Missing Plan102 minimap source: {source_path}")
    texture = import_texture(source_path, row["RuntimeAsset"])
    profile_asset_path = profile_path(row["CharacterId"], row["Role"])
    profile = unreal.load_asset(profile_asset_path)
    if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
        raise RuntimeError(f"Missing character presentation profile: {profile_asset_path}")
    profile.set_editor_property("minimap_icon", texture)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Unable to save minimap icon binding: {profile_asset_path}")
    saved_profiles.append(profile_asset_path)

unreal.log(
    f"[Plan102Import] imported/bound {len(rows)} minimap icons: {saved_profiles}"
)
