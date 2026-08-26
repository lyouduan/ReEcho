"""Configure the longsword to perform three sixty-degree pivot swings."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_CrescentBlade"


profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if profile is None:
    raise RuntimeError(f"Missing longsword presentation profile: {PROFILE_PATH}")

profile.set_editor_property("motion_mode", unreal.ReEchoWeaponMotionMode.TRIPLE_SWING60)

if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save longsword presentation profile: {PROFILE_PATH}")

unreal.log(
    "Plan100 longsword triple swing saved: "
    f"mode={profile.get_editor_property('motion_mode')} "
    f"duration={profile.get_editor_property('motion_duration_seconds')}"
)
