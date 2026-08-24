"""Configure the Fox appearance's visual-only terminal Death grounding inset."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Fox"
DEATH_GROUND_SINK = 0.0

profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
    raise RuntimeError(f"Missing Fox presentation profile: {PROFILE_PATH}")

profile.set_editor_property("death_ground_sink", DEATH_GROUND_SINK)
if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Fox presentation profile: {PROFILE_PATH}")

saved_profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
saved_sink = saved_profile.get_editor_property("death_ground_sink")
if abs(saved_sink - DEATH_GROUND_SINK) > 0.001:
    raise RuntimeError(f"Fox DeathGroundSink expected {DEATH_GROUND_SINK}, found {saved_sink}")

unreal.log(f"FOX_DEATH_GROUNDING_OK profile={PROFILE_PATH} sink={saved_sink}")
