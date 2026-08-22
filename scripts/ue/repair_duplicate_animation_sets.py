"""Remove duplicate semantic entries from authored AnimationSet clip maps."""

from collections import OrderedDict

import unreal


PROFILE_PATHS = (
    "/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_CLOVER",
    "/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_DIAMOND",
    "/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_HEART",
    "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_RabbitDoll",
)


def tag_name(tag):
    try:
        return str(tag.get_editor_property("tag_name"))
    except Exception:
        return unreal.GameplayTagLibrary.get_tag_name(tag).to_string()


def has_flipbook(clip):
    return clip.get_editor_property("flipbook") is not None


for profile_path in PROFILE_PATHS:
    profile = unreal.EditorAssetLibrary.load_asset(profile_path)
    if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
        raise RuntimeError(f"Missing presentation profile: {profile_path}")

    repaired_sets = []
    removed_count = 0
    for source_set in profile.get_editor_property("animation_sets"):
        selected = OrderedDict()
        for tag, clip in source_set.get_editor_property("clips").items():
            semantic = tag_name(tag)
            if semantic in ("None", ""):
                removed_count += 1
                continue
            previous = selected.get(semantic)
            if previous is not None:
                removed_count += 1
                if has_flipbook(previous[1]) and not has_flipbook(clip):
                    continue
            selected[semantic] = (tag, clip)

        repaired_set = unreal.ReEcho2DCompositeAnimationSet()
        repaired_set.set_editor_property(
            "weapon_visual_set_id", source_set.get_editor_property("weapon_visual_set_id")
        )
        repaired_set.set_editor_property("clips", {tag: clip for tag, clip in selected.values()})
        repaired_sets.append(repaired_set)

    profile.set_editor_property("animation_sets", repaired_sets)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save repaired profile: {profile_path}")
    unreal.log(f"ANIMSET_REPAIR {profile_path} removed={removed_count}")

unreal.log("ANIMSET_REPAIR_COMPLETE")
