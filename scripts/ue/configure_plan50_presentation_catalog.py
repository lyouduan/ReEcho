"""Build the Plan50 presentation catalog without encoding gameplay behavior in assets."""

import unreal


CATALOG_PATH = "/Game/ReEcho/Animation2D/DA_PresentationCatalog"
GAMEPLAY_REGISTRY_PATH = "/Game/ReEcho/Gameplay/CharacterPrefabs/DA_EnemyGameplayClassRegistry"
PROFILE_ROOT = "/Game/ReEcho/Animation2D"
PREFAB_ROOT = "/Game/ReEcho/Gameplay/CharacterPrefabs"

PLAYER_PROFILES = {
    "J_SPADE": "DA_Character_J_SPADE",
    "J_DIAMOND": "DA_Character_J_DIAMOND",
    "J_CLOVER": "DA_Character_J_CLOVER",
    "J_HEART": "DA_Character_J_HEART",
}

ENEMY_PRESENTATIONS = {
    "Enemy.Grunt": ("DA_Enemy_Grunt", "BP_EnemyGameplay_Grunt", None, None),
    "Enemy.Shield": (
        "DA_Enemy_Shield",
        "BP_EnemyGameplay_Shield",
        "DA_Enemy_GoatPriest",
        "BP_EnemyGameplay_Goat",
    ),
    "Enemy.Bomber": (
        "DA_Enemy_Bomber",
        "BP_EnemyGameplay_Bomber",
        "DA_Enemy_Grunt",
        "BP_EnemyGameplay_Grunt",
    ),
    "Enemy.Slime": (
        "DA_Enemy_Slime",
        "BP_EnemyGameplay_Slime",
        "DA_Enemy_Grunt",
        "BP_EnemyGameplay_Grunt",
    ),
    "Enemy.Rabbit": ("DA_Enemy_RabbitDoll", "BP_EnemyGameplay_Rabbit", None, None),
    "Enemy.Fox": ("DA_Enemy_Fox", "BP_EnemyGameplay_Fox", None, None),
    "Enemy.TimeGuard": (
        "DA_Enemy_TimeGuard",
        "BP_EnemyGameplay_TimeGuard",
        "DA_Enemy_GoatPriest",
        "BP_EnemyGameplay_Goat",
    ),
}


def load_required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Missing {expected_type.__name__}: {path}")
    return asset


def duplicate_if_missing(destination, source):
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        return unreal.EditorAssetLibrary.load_asset(destination)
    if not unreal.EditorAssetLibrary.duplicate_asset(source, destination):
        raise RuntimeError(f"Could not duplicate {source} to {destination}")
    return unreal.EditorAssetLibrary.load_asset(destination)


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


def normalize_profile(profile, presentation_id):
    profile.set_editor_property("appearance_id", presentation_id)
    normalized_sets = []
    seen_visual_sets = set()
    for source_set in profile.get_editor_property("animation_sets"):
        visual_set_id = str(source_set.get_editor_property("weapon_visual_set_id"))
        if visual_set_id in seen_visual_sets:
            continue
        seen_visual_sets.add(visual_set_id)
        normalized_set = unreal.ReEcho2DCompositeAnimationSet()
        normalized_set.set_editor_property(
            "weapon_visual_set_id", source_set.get_editor_property("weapon_visual_set_id")
        )
        normalized_set.set_editor_property("clips", dict(source_set.get_editor_property("clips")))
        normalized_sets.append(normalized_set)
    profile.set_editor_property("animation_sets", normalized_sets)
    return profile


catalog = load_required(CATALOG_PATH, unreal.ReEcho2DPresentationCatalog)
gameplay_registry = load_or_create_data_asset(
    GAMEPLAY_REGISTRY_PATH, unreal.ReEchoEnemyGameplayClassRegistry
)
entries = []
gameplay_entries = []
player_profiles = []

for presentation_id, asset_name in PLAYER_PROFILES.items():
    profile = normalize_profile(
        load_required(f"{PROFILE_ROOT}/{asset_name}", unreal.ReEcho2DCharacterPresentationProfile),
        presentation_id,
    )
    entry = unreal.ReEcho2DPresentationCatalogEntry()
    entry.set_editor_property("presentation_id", presentation_id)
    entry.set_editor_property("profile", profile)
    entries.append(entry)
    player_profiles.append(profile)

for presentation_id, (profile_name, prefab_name, source_profile, source_prefab) in ENEMY_PRESENTATIONS.items():
    profile_path = f"{PROFILE_ROOT}/{profile_name}"
    if source_profile:
        profile = duplicate_if_missing(profile_path, f"{PROFILE_ROOT}/{source_profile}")
    else:
        profile = load_required(profile_path, unreal.ReEcho2DCharacterPresentationProfile)
    profile = normalize_profile(profile, presentation_id)

    prefab_path = f"{PREFAB_ROOT}/{prefab_name}"
    if source_prefab:
        prefab = duplicate_if_missing(prefab_path, f"{PREFAB_ROOT}/{source_prefab}")
    else:
        prefab = load_required(prefab_path, unreal.Blueprint)
    if not isinstance(prefab, unreal.Blueprint):
        raise RuntimeError(f"Enemy prefab is not a Blueprint: {prefab_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(prefab)
    gameplay_class = unreal.load_class(None, f"{prefab_path}.{prefab_name}_C")
    if not gameplay_class:
        raise RuntimeError(f"Enemy prefab class is unavailable: {prefab_path}")

    entry = unreal.ReEcho2DPresentationCatalogEntry()
    entry.set_editor_property("presentation_id", presentation_id)
    entry.set_editor_property("profile", profile)
    entries.append(entry)

    gameplay_entry = unreal.ReEchoEnemyGameplayClassEntry()
    gameplay_entry.set_editor_property("presentation_id", presentation_id)
    gameplay_entry.set_editor_property("gameplay_class", gameplay_class)
    gameplay_entries.append(gameplay_entry)

catalog.set_editor_property("presentation_entries", entries)
catalog.set_editor_property("character_profiles", player_profiles)
gameplay_registry.set_editor_property("entries", gameplay_entries)

assets_to_save = [catalog, gameplay_registry, *player_profiles]
for _, (profile_name, prefab_name, _, _) in ENEMY_PRESENTATIONS.items():
    assets_to_save.append(load_required(f"{PROFILE_ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile))
    assets_to_save.append(load_required(f"{PREFAB_ROOT}/{prefab_name}", unreal.Blueprint))

for asset in assets_to_save:
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save {asset.get_path_name()}")

unreal.log(
    f"[Plan50] Saved {len(entries)} profile entries and {len(gameplay_entries)} gameplay class entries"
)
