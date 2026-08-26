import unreal


RULES = {
    "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Bow":
        unreal.ReEchoHeldWeaponMirrorRule.WHEN_FACING_LEFT,
    "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Gun":
        unreal.ReEchoHeldWeaponMirrorRule.WHEN_FACING_RIGHT,
}

for asset_path, mirror_rule in RULES.items():
    profile = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not profile:
        raise RuntimeError(f"Missing weapon presentation profile: {asset_path}")
    profile.set_editor_property("held_mirror_rule", mirror_rule)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save weapon presentation profile: {asset_path}")

unreal.log("PLAN100_RANGED_MIRROR bow=left gun=right")
