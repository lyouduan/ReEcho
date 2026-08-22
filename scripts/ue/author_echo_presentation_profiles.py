"""Create the four data-driven Echo presentation profiles and their isolated catalog."""

import unreal


ROOT = "/Game/ReEcho/DataAsset/Character/Profiles"
CATALOG_PATH = "/Game/ReEcho/DataAsset/Character/Catalogs/DA_EchoPresentationCatalog"
STATE_MACHINE_PATH = "/Game/ReEcho/DataAsset/Common/Animation2D/SM2D_DefaultCharacter"
CHARACTERS = {
    "J_HEART": ("Heart", "walk"),
    "J_SPADE": ("Spade", "walk"),
    "J_CLOVER": ("Clover", "walk"),
    "J_DIAMOND": ("Diamond", "Walk"),
}
RANGED_VISUAL_SETS = ("Staff", "Bow", "Gun")


def load_required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Missing {expected_type.__name__}: {path}")
    return asset


def load_or_create_data_asset(path, data_asset_class):
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        if not isinstance(existing, data_asset_class):
            raise RuntimeError(f"Unexpected asset type at {path}: {type(existing).__name__}")
        return existing
    package_path, asset_name = path.rsplit("/", 1)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", data_asset_class)
    created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, data_asset_class, factory
    )
    if not created:
        raise RuntimeError(f"Could not create data asset: {path}")
    return created


def semantic(name):
    value = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError(f"Unregistered animation semantic: {name}")
    return value


def clip(flipbook, looping, restart=False):
    value = unreal.ReEcho2DAnimationClip()
    value.set_editor_property("flipbook", flipbook)
    value.set_editor_property("looping", looping)
    value.set_editor_property("restart_on_request", restart)
    value.set_editor_property("play_rate", 1.0)
    value.set_editor_property("use_native_scale", False)
    value.set_editor_property("world_height", 100.0)
    value.set_editor_property("translucent_sort_priority", 10)
    return value


state_machine = load_required(STATE_MACHINE_PATH, unreal.ReEcho2DAnimationStateMachineAsset)
profiles = []
entries = []
for character_id, (folder, walk_name) in CHARACTERS.items():
    flipbook_root = f"/Game/ReEcho/Art/Animation2D/Echos/{folder}/Flipbooks"
    walk = load_required(f"{flipbook_root}/{walk_name}", unreal.PaperFlipbook)
    attack = load_required(f"{flipbook_root}/Attack", unreal.PaperFlipbook)
    ranged_attack = load_required(f"{flipbook_root}/Attack_Arrow", unreal.PaperFlipbook)

    profile = load_or_create_data_asset(
        f"{ROOT}/DA_Echo_{character_id}", unreal.ReEcho2DCharacterPresentationProfile
    )
    profile.set_editor_property("appearance_id", f"Echo.{character_id}")
    profile.set_editor_property("world_height", 100.0)
    profile.set_editor_property("auto_align_footpoint", True)
    profile.set_editor_property("footpoint_offset", unreal.Vector(0.0, 0.0, 0.0))
    profile.set_editor_property("state_machine", state_machine)

    default_set = unreal.ReEcho2DCompositeAnimationSet()
    default_set.set_editor_property("weapon_visual_set_id", "")
    default_set.set_editor_property(
        "clips",
        {
            semantic("Animation.Idle"): clip(walk, True),
            semantic("Animation.Move"): clip(walk, True),
            semantic("Animation.Attack.Basic"): clip(attack, False, True),
            semantic("Animation.Hit"): clip(walk, False, True),
        },
    )
    animation_sets = [default_set]
    for visual_set_id in RANGED_VISUAL_SETS:
        ranged_set = unreal.ReEcho2DCompositeAnimationSet()
        ranged_set.set_editor_property("weapon_visual_set_id", visual_set_id)
        ranged_set.set_editor_property(
            "clips", {semantic("Animation.Attack.Basic"): clip(ranged_attack, False, True)}
        )
        animation_sets.append(ranged_set)
    profile.set_editor_property("animation_sets", animation_sets)

    entry = unreal.ReEcho2DPresentationCatalogEntry()
    entry.set_editor_property("presentation_id", character_id)
    entry.set_editor_property("profile", profile)
    profiles.append(profile)
    entries.append(entry)

catalog = load_or_create_data_asset(CATALOG_PATH, unreal.ReEcho2DPresentationCatalog)
catalog.set_editor_property("presentation_entries", entries)
catalog.set_editor_property("character_profiles", profiles)

for asset in [*profiles, catalog]:
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save {asset.get_path_name()}")

reloaded_catalog = load_required(CATALOG_PATH, unreal.ReEcho2DPresentationCatalog)
reloaded_entries = {
    str(entry.get_editor_property("presentation_id")): entry.get_editor_property("profile")
    for entry in reloaded_catalog.get_editor_property("presentation_entries")
}
for character_id in CHARACTERS:
    resolved = reloaded_entries.get(character_id)
    if not resolved or str(resolved.get_editor_property("appearance_id")) != f"Echo.{character_id}":
        raise RuntimeError(f"Catalog mapping failed for {character_id}")
    if len(resolved.get_editor_property("animation_sets")) != 4:
        raise RuntimeError(f"Unexpected animation set count for {character_id}")

unreal.log("[Plan71] Echo presentation profiles and isolated catalog verified")
