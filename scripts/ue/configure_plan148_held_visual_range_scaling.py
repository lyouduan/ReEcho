"""Configure melee held visuals to follow the authoritative attack-range multiplier."""

import unreal


PROFILE_ROOT = "/Game/ReEcho/DataAsset/Weapon/Profiles"


def load_profile(visual_key):
    path = f"{PROFILE_ROOT}/DA_WeaponPresentation_{visual_key}"
    profile = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(profile, unreal.ReEchoWeaponPresentationProfile):
        raise RuntimeError(f"Missing weapon presentation profile: {path}")
    return profile


sword = load_profile("CrescentBlade")
scythe = load_profile("Scythe")

sword_axis = sword.get_editor_property("held_size_axis")
sword_mask = (
    unreal.Vector(1.0, 0.0, 0.0)
    if sword_axis == unreal.ReEchoHeldWeaponSizeAxis.WIDTH
    else unreal.Vector(0.0, 1.0, 0.0)
)
sword.set_editor_property("scale_held_visual_with_attack_range", True)
sword.set_editor_property("held_visual_attack_range_scale_mask", sword_mask)
sword.set_editor_property("min_held_visual_attack_range_multiplier", 0.5)
sword.set_editor_property("max_held_visual_attack_range_multiplier", 2.0)

scythe.set_editor_property("override_held_length", True)
scythe.set_editor_property("held_length_override_cm", 400.0)
scythe.set_editor_property("scale_held_visual_with_attack_range", True)
scythe.set_editor_property(
    "held_visual_attack_range_scale_mask", unreal.Vector(1.0, 1.0, 0.0)
)
scythe.set_editor_property("min_held_visual_attack_range_multiplier", 0.5)
scythe.set_editor_property("max_held_visual_attack_range_multiplier", 2.0)

for profile in (sword, scythe):
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {profile.get_path_name()}")

unreal.log(
    "PLAN148_HELD_RANGE_SCALE "
    f"sword_mask={sword_mask} scythe_length_cm=400 scythe_mask=X,Y limits=0.5..2.0"
)
