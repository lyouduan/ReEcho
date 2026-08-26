"""Configure independent Boss-facing offsets on the MoonStaff presentation profile."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_MoonStaff"
BOSS_REFERENCE_HEIGHT_CM = 220.0
DEFAULT_HORIZONTAL_OFFSET_CM = 20.0


profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if profile is None:
    raise RuntimeError(f"Missing MoonStaff presentation profile: {PROFILE_PATH}")

offset_ratio = DEFAULT_HORIZONTAL_OFFSET_CM / BOSS_REFERENCE_HEIGHT_CM
profile.set_editor_property(
    "held_right_facing_offset_ratio", unreal.Vector(0.0, offset_ratio, 0.0)
)
profile.set_editor_property(
    "held_left_facing_offset_ratio", unreal.Vector(0.0, -offset_ratio, 0.0)
)

if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save MoonStaff presentation profile: {PROFILE_PATH}")

unreal.log(
    "Plan100 Boss staff facing offsets saved: "
    f"right={profile.get_editor_property('held_right_facing_offset_ratio')} "
    f"left={profile.get_editor_property('held_left_facing_offset_ratio')}"
)
