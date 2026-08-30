"""Reduce the authored scythe slash plane by ten percent without changing range scaling."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Scythe"
TARGET_SCALE = unreal.Vector(0.495, 0.495, 1.0)

profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if not isinstance(profile, unreal.ReEchoWeaponPresentationProfile):
    raise RuntimeError(f"Missing scythe presentation profile: {PROFILE_PATH}")

slot = profile.get_editor_property("attack_committed")
if not slot.get_editor_property("enabled") or not slot.get_editor_property("system"):
    raise RuntimeError("Scythe AttackCommitted Niagara is not configured")
if not slot.get_editor_property("scale_with_attack_range"):
    raise RuntimeError("Scythe AttackCommitted no longer follows attack range")
previous_range_mask = slot.get_editor_property("attack_range_scale_mask")
if abs(previous_range_mask.z) >= 1.0e-6:
    raise RuntimeError(
        "Scythe AttackCommitted unexpectedly scales its thickness: "
        f"({previous_range_mask.x}, {previous_range_mask.y}, {previous_range_mask.z})"
    )

offset = slot.get_editor_property("offset")
offset.set_editor_property("scale3d", TARGET_SCALE)
slot.set_editor_property("offset", offset)
slot.set_editor_property("attack_range_scale_mask", unreal.Vector(1.0, 1.0, 0.0))
profile.set_editor_property("attack_committed", slot)

if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save scythe presentation profile: {PROFILE_PATH}")

unreal.log(
    "PLAN148_SCYTHE_VFX_SIZE "
    f"profile={PROFILE_PATH} base_scale={TARGET_SCALE} "
    f"previous_range_mask={previous_range_mask} range_mask=X,Y size_multiplier=0.90"
)
