import unreal


PROFILE_CONFIG = {
    "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_CrescentBlade": unreal.Vector(0.0, 1.0, 0.0),
    "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Scythe": unreal.Vector(1.0, 1.0, 0.0),
}

for profile_path, scale_mask in PROFILE_CONFIG.items():
    profile = unreal.EditorAssetLibrary.load_asset(profile_path)
    if not isinstance(profile, unreal.ReEchoWeaponPresentationProfile):
        raise RuntimeError(f"Missing melee weapon presentation profile: {profile_path}")
    slot = profile.get_editor_property("attack_committed")
    if not slot.get_editor_property("enabled") or not slot.get_editor_property("system"):
        raise RuntimeError(f"AttackCommitted Niagara is not configured: {profile_path}")
    slot.set_editor_property("scale_with_attack_range", True)
    slot.set_editor_property("attack_range_scale_mask", scale_mask)
    slot.set_editor_property("min_attack_range_multiplier", 0.5)
    slot.set_editor_property("max_attack_range_multiplier", 2.0)
    profile.set_editor_property("attack_committed", slot)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save melee weapon presentation profile: {profile_path}")
    unreal.log(
        "PLAN148_MELEE_RANGE_SCALE "
        f"profile={profile.get_path_name()} mask={scale_mask} min=0.5 max=2.0"
    )
