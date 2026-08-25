"""Retire staff/whip roster assets while preserving the Sage MoonStaff helper."""

import unreal


CATALOG_PATH = "/Game/ReEcho/DataAsset/Weapon/Catalogs/DA_WeaponPresentationCatalog"
PROFILE_ROOT = "/Game/ReEcho/DataAsset/Weapon/Profiles"
OLD_STAFF_PROFILE = f"{PROFILE_ROOT}/DA_WeaponPresentation_Staff"
MOON_STAFF_PROFILE = f"{PROFILE_ROOT}/DA_WeaponPresentation_MoonStaff"
WHIP_PROFILE = f"{PROFILE_ROOT}/DA_WeaponPresentation_Whip"
RETAINED_PROFILES = {
    "CrescentBlade": f"{PROFILE_ROOT}/DA_WeaponPresentation_CrescentBlade",
    "Scythe": f"{PROFILE_ROOT}/DA_WeaponPresentation_Scythe",
    "Bow": f"{PROFILE_ROOT}/DA_WeaponPresentation_Bow",
    "Gun": f"{PROFILE_ROOT}/DA_WeaponPresentation_Gun",
}
RETIRED_RUNE_ICONS = (
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_WHIP_KILLHEAL_WHIPBODY",
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_WHIP_HASTE_WHIPBODY",
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_STAFF_WIDEWEAK_STAFFCRYSTAL",
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_STAFF_SLOWWIDE_STAFFCRYSTAL",
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_STAFF_NARROWSTRONG_STAFFCRYSTAL",
    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_STAFF_DUALSPREAD_STAFFBODY",
)


def load_required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset or not isinstance(asset, expected_type):
        raise RuntimeError(f"Missing or wrong asset type: {path}")
    return asset


if not unreal.EditorAssetLibrary.does_asset_exist(MOON_STAFF_PROFILE):
    if not unreal.EditorAssetLibrary.does_asset_exist(OLD_STAFF_PROFILE):
        raise RuntimeError(f"Missing MoonStaff source profile: {OLD_STAFF_PROFILE}")
    if not unreal.EditorAssetLibrary.rename_asset(OLD_STAFF_PROFILE, MOON_STAFF_PROFILE):
        raise RuntimeError("Could not rename the Staff helper profile to MoonStaff")

moon_staff = load_required(MOON_STAFF_PROFILE, unreal.ReEchoWeaponPresentationProfile)
moon_staff.set_editor_property("weapon_visual_key", "MoonStaff")
if not unreal.EditorAssetLibrary.save_loaded_asset(moon_staff, only_if_is_dirty=False):
    raise RuntimeError("Could not save MoonStaff presentation profile")

catalog = load_required(CATALOG_PATH, unreal.ReEchoWeaponPresentationCatalog)
profiles = {
    key: load_required(path, unreal.ReEchoWeaponPresentationProfile)
    for key, path in RETAINED_PROFILES.items()
}
profiles["MoonStaff"] = moon_staff
catalog.set_editor_property("profiles", profiles)
if not unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False):
    raise RuntimeError("Could not save four-weapon presentation catalog")

for retired_asset in (WHIP_PROFILE, *RETIRED_RUNE_ICONS):
    if not unreal.EditorAssetLibrary.does_asset_exist(retired_asset):
        continue
    referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(
        retired_asset, load_assets_to_confirm=True
    )
    if referencers:
        raise RuntimeError(f"Retired asset still has referencers: {retired_asset} <- {referencers}")
    if not unreal.EditorAssetLibrary.delete_asset(retired_asset):
        raise RuntimeError(f"Could not delete retired asset: {retired_asset}")

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(
    f"PLAN104_ASSET_RESULT roster={sorted(RETAINED_PROFILES)} helper=MoonStaff "
    f"retired={1 + len(RETIRED_RUNE_ICONS)}"
)
