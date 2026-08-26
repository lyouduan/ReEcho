import unreal


PROFILE_PATH = (
    "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_CrescentBlade."
    "DA_WeaponPresentation_CrescentBlade"
)

profile = unreal.load_object(None, PROFILE_PATH)
if not profile:
    raise RuntimeError(f"Missing sword profile: {PROFILE_PATH}")

profile.set_editor_property("held_planar_angle_offset_degrees", 90.0)
slot = profile.get_editor_property("attack_committed")
offset = slot.get_editor_property("offset")
translation = offset.get_editor_property("translation")
rotation = offset.get_editor_property("rotation").rotator()
scale = offset.get_editor_property("scale3d")
rotation.yaw = 180.0
slot.set_editor_property("offset", unreal.Transform(location=translation, rotation=rotation, scale=scale))
profile.set_editor_property("attack_committed", slot)
if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save {PROFILE_PATH}")

unreal.log("PLAN100_SWORD_HOLD_DIRECTION planar_angle_degrees=90 slash_local_yaw=180")
