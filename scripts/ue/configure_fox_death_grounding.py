"""Configure the Fox appearance's visual-only terminal Death grounding inset."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Fox"
USE_AUTHORED_DEATH_PIVOT = True
DEATH_GROUND_SINK = 0.0

profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
    raise RuntimeError(f"Missing Fox presentation profile: {PROFILE_PATH}")

profile.set_editor_property("use_authored_death_pivot", USE_AUTHORED_DEATH_PIVOT)
profile.set_editor_property("death_ground_sink", DEATH_GROUND_SINK)
if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Fox presentation profile: {PROFILE_PATH}")

saved_profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
saved_pivot_policy = saved_profile.get_editor_property("use_authored_death_pivot")
saved_sink = saved_profile.get_editor_property("death_ground_sink")
if saved_pivot_policy is not USE_AUTHORED_DEATH_PIVOT:
    raise RuntimeError(
        f"Fox bUseAuthoredDeathPivot expected {USE_AUTHORED_DEATH_PIVOT}, found {saved_pivot_policy}"
    )
if abs(saved_sink - DEATH_GROUND_SINK) > 0.001:
    raise RuntimeError(f"Fox DeathGroundSink expected {DEATH_GROUND_SINK}, found {saved_sink}")

unreal.log(
    f"FOX_DEATH_GROUNDING_OK profile={PROFILE_PATH} "
    f"authored_pivot={saved_pivot_policy} sink={saved_sink}"
)
