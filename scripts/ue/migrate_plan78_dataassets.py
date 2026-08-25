"""Create weapon presentation DA assets and organize combat-subject DataAssets through Unreal APIs."""

import unreal


ROOT = "/Game/ReEcho/DataAsset"
CHARACTER_PROFILE_ROOT = f"{ROOT}/Character/Profiles"
CHARACTER_LEGACY_ROOT = f"{ROOT}/Character/Legacy"
CHARACTER_CATALOG_ROOT = f"{ROOT}/Character/Catalogs"
ENEMY_PROFILE_ROOT = f"{ROOT}/Enemy/Profiles"
ENEMY_CATALOG_ROOT = f"{ROOT}/Enemy/Catalogs"
WEAPON_PROFILE_ROOT = f"{ROOT}/Weapon/Profiles"
WEAPON_CATALOG_ROOT = f"{ROOT}/Weapon/Catalogs"
COMMON_ANIMATION_ROOT = f"{ROOT}/Common/Animation2D"

OLD_ANIMATION_ROOT = "/Game/ReEcho/Animation2D"
OLD_GAMEPLAY_REGISTRY = "/Game/ReEcho/Gameplay/CharacterPrefabs/DA_EnemyGameplayClassRegistry"

CHARACTER_PROFILES = (
    "DA_Character_J_SPADE",
    "DA_Character_J_DIAMOND",
    "DA_Character_J_CLOVER",
    "DA_Character_J_HEART",
    "DA_Echo_J_SPADE",
    "DA_Echo_J_DIAMOND",
    "DA_Echo_J_CLOVER",
    "DA_Echo_J_HEART",
)
ENEMY_PROFILES = (
    "DA_Enemy_Bomber",
    "DA_Enemy_Fox",
    "DA_Enemy_GoatPriest",
    "DA_Enemy_Grunt",
    "DA_Enemy_RabbitDoll",
    "DA_Enemy_Shield",
    "DA_Enemy_Slime",
    "DA_Enemy_TimeGuard",
)

WEAPONS = {
    "CrescentBlade": {
        "held": "/Game/ReEcho/Textures/Effects/CrescentWeapon",
        "motion": unreal.ReEchoWeaponMotionMode.FULL_SPIN,
        "attack": "/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01",
    },
    "Scythe": {
        "held": "/Game/ReEcho/Textures/Effects/Scythe",
        "attack": "/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_01",
    },
    "Bow": {
        "held": "/Game/ReEcho/Textures/Effects/Bow",
        "travel": "/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_01",
        "damage": "/Game/VFX/People/Bow/Particle/NS_People_Bow_Boom",
    },
    "Gun": {
        "held": "/Game/ReEcho/Textures/Effects/Gun",
        "travel": "/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Fly",
        "damage": "/Game/VFX/People/Bullet/Particle/NS_People_Bullet_spark",
    },
    "MoonStaff": {
        "held": "/Game/ReEcho/Textures/Effects/MoonStaff",
        "legacy": "/Game/ReEcho/Textures/Effects/StaffLightWave",
    },
}


def load_required(path, expected_type=None):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset or (expected_type and not isinstance(asset, expected_type)):
        raise RuntimeError(f"Missing or wrong asset type: {path}")
    return asset


def load_optional(path, expected_type=None):
    if not path:
        return None
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_warning(f"PLAN78_OPTIONAL_ASSET_MISSING {path}")
        return None
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset and (not expected_type or isinstance(asset, expected_type)):
        return asset
    unreal.log_warning(f"PLAN78_OPTIONAL_ASSET_MISSING {path}")
    return None


def create_data_asset(path, data_asset_class):
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        if not isinstance(existing, data_asset_class):
            raise RuntimeError(f"Wrong existing type at {path}")
        return existing
    package_path, asset_name = path.rsplit("/", 1)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", data_asset_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, data_asset_class, factory
    )
    if not asset:
        raise RuntimeError(f"Could not create {path}")
    return asset


def move_required(source, destination):
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        return load_required(destination)
    if not unreal.EditorAssetLibrary.does_asset_exist(source):
        raise RuntimeError(f"Missing source for move: {source}")
    if not unreal.EditorAssetLibrary.rename_asset(source, destination):
        raise RuntimeError(f"Could not move {source} -> {destination}")
    return load_required(destination)


def make_slot(system_path, spawn_mode):
    slot = unreal.ReEchoWeaponVfxSlot()
    if system_path:
        slot.set_editor_property("enabled", True)
        slot.set_editor_property("system", load_required(system_path, unreal.NiagaraSystem))
        slot.set_editor_property("spawn_mode", spawn_mode)
        slot.set_editor_property("stop_when_phase_ends", True)
    return slot


for directory in (
    CHARACTER_PROFILE_ROOT,
    CHARACTER_LEGACY_ROOT,
    CHARACTER_CATALOG_ROOT,
    ENEMY_PROFILE_ROOT,
    ENEMY_CATALOG_ROOT,
    WEAPON_PROFILE_ROOT,
    WEAPON_CATALOG_ROOT,
    COMMON_ANIMATION_ROOT,
):
    unreal.EditorAssetLibrary.make_directory(directory)

# Move/create the split catalogs. Their entries are rebuilt from the authoritative
# profiles below so the migration remains idempotent after the old mixed catalog
# has already been renamed.
old_catalog_path = f"{OLD_ANIMATION_ROOT}/DA_PresentationCatalog"
current_character_catalog_path = f"{CHARACTER_CATALOG_ROOT}/DA_CharacterPresentationCatalog"
character_catalog = move_required(
    f"{OLD_ANIMATION_ROOT}/DA_PresentationCatalog",
    f"{CHARACTER_CATALOG_ROOT}/DA_CharacterPresentationCatalog",
)
enemy_catalog = create_data_asset(
    f"{ENEMY_CATALOG_ROOT}/DA_EnemyPresentationCatalog", unreal.ReEcho2DPresentationCatalog
)
for name in CHARACTER_PROFILES:
    move_required(f"{OLD_ANIMATION_ROOT}/{name}", f"{CHARACTER_PROFILE_ROOT}/{name}")
if unreal.EditorAssetLibrary.does_asset_exist(f"{OLD_ANIMATION_ROOT}/DA_Character_J_CAT"):
    move_required(
        f"{OLD_ANIMATION_ROOT}/DA_Character_J_CAT",
        f"{CHARACTER_LEGACY_ROOT}/DA_Character_J_CAT",
    )
for name in ENEMY_PROFILES:
    move_required(f"{OLD_ANIMATION_ROOT}/{name}", f"{ENEMY_PROFILE_ROOT}/{name}")


def make_catalog_entry(profile):
    entry = unreal.ReEcho2DPresentationCatalogEntry()
    entry.set_editor_property("presentation_id", profile.get_editor_property("appearance_id"))
    entry.set_editor_property("profile", profile)
    return entry


character_profile_assets = [
    load_required(f"{CHARACTER_PROFILE_ROOT}/{name}", unreal.ReEcho2DCharacterPresentationProfile)
    for name in CHARACTER_PROFILES
    if name.startswith("DA_Character_")
]
enemy_profile_assets = [
    load_required(f"{ENEMY_PROFILE_ROOT}/{name}", unreal.ReEcho2DCharacterPresentationProfile)
    for name in ENEMY_PROFILES
]
character_catalog.set_editor_property(
    "presentation_entries", [make_catalog_entry(profile) for profile in character_profile_assets]
)
character_catalog.set_editor_property("character_profiles", character_profile_assets)
enemy_catalog.set_editor_property(
    "presentation_entries", [make_catalog_entry(profile) for profile in enemy_profile_assets]
)
enemy_catalog.set_editor_property("character_profiles", [])
move_required(
    f"{OLD_ANIMATION_ROOT}/DA_EchoPresentationCatalog",
    f"{CHARACTER_CATALOG_ROOT}/DA_EchoPresentationCatalog",
)
move_required(
    f"{OLD_ANIMATION_ROOT}/SM2D_DefaultCharacter",
    f"{COMMON_ANIMATION_ROOT}/SM2D_DefaultCharacter",
)
move_required(
    OLD_GAMEPLAY_REGISTRY,
    f"{ENEMY_CATALOG_ROOT}/DA_EnemyGameplayClassRegistry",
)

weapon_catalog = create_data_asset(
    f"{WEAPON_CATALOG_ROOT}/DA_WeaponPresentationCatalog",
    unreal.ReEchoWeaponPresentationCatalog,
)
weapon_profiles = {}
for visual_key, config in WEAPONS.items():
    profile = create_data_asset(
        f"{WEAPON_PROFILE_ROOT}/DA_WeaponPresentation_{visual_key}",
        unreal.ReEchoWeaponPresentationProfile,
    )
    profile.set_editor_property("weapon_visual_key", visual_key)
    profile.set_editor_property("held_texture", load_optional(config["held"], unreal.Texture2D))
    profile.set_editor_property("motion_mode", config.get("motion", unreal.ReEchoWeaponMotionMode.NONE))
    profile.set_editor_property("motion_duration_seconds", 0.18)
    if config.get("legacy"):
        profile.set_editor_property("legacy_attack_texture", load_optional(config["legacy"], unreal.Texture2D))
    profile.set_editor_property(
        "attack_committed",
        make_slot(config.get("attack"), unreal.ReEchoWeaponVfxSpawnMode.ATTACH_TO_ATTACK_ROOT),
    )
    profile.set_editor_property(
        "travel", make_slot(config.get("travel"), unreal.ReEchoWeaponVfxSpawnMode.ATTACH_TO_CARRIER)
    )
    profile.set_editor_property(
        "damage_applied",
        make_slot(config.get("damage"), unreal.ReEchoWeaponVfxSpawnMode.SPAWN_AT_WORLD_LOCATION),
    )
    profile.set_editor_property("charge", unreal.ReEchoWeaponVfxSlot())
    weapon_profiles[visual_key] = profile
    unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False)

weapon_catalog.set_editor_property("profiles", weapon_profiles)
for asset in (character_catalog, enemy_catalog, weapon_catalog):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset.get_path_name()}")

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(
    f"PLAN78_DATAASSET_RESULT character={len(CHARACTER_PROFILES)} enemy={len(ENEMY_PROFILES)} "
    f"weapon={len(weapon_profiles)}"
)
