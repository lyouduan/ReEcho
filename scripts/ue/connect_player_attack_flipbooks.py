"""Connect each authored player Attack Flipbook to its matching character profile."""

import unreal


ATTACK_BINDINGS = {
    "J_CLOVER": "/Game/ReEcho/Art/Animation2D/Players/Clover/Flipbooks/Attack",
    "J_DIAMOND": "/Game/ReEcho/Art/Animation2D/Players/Diamond/Flipbooks/Attack",
    "J_HEART": "/Game/ReEcho/Art/Animation2D/Players/Heart/Flipbooks/Attack",
    "J_SPADE": "/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack",
}
PROFILE_ROOT = "/Game/ReEcho/DataAsset/Character/Profiles"


def load_required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Required {expected_type.__name__} is missing: {path}")
    return asset


def semantic_tag(name):
    value = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError(f"GameplayTag is not registered: {name}")
    return value


def make_attack_clip(flipbook, world_height):
    if len(flipbook.get_editor_property("key_frames")) == 0:
        raise RuntimeError(f"Attack Flipbook has no frames: {flipbook.get_path_name()}")
    clip = unreal.ReEcho2DAnimationClip()
    clip.set_editor_property("flipbook", flipbook)
    clip.set_editor_property("looping", False)
    clip.set_editor_property("restart_on_request", True)
    clip.set_editor_property("play_rate", 1.0)
    clip.set_editor_property("use_native_scale", False)
    clip.set_editor_property("world_height", world_height)
    clip.set_editor_property("translucent_sort_priority", 10)
    return clip


attack_tag = semantic_tag("Animation.Attack.Basic")
updated = []

for appearance_id, flipbook_path in ATTACK_BINDINGS.items():
    profile_path = f"{PROFILE_ROOT}/DA_Character_{appearance_id}"
    profile = load_required(profile_path, unreal.ReEcho2DCharacterPresentationProfile)
    if str(profile.get_editor_property("appearance_id")) != appearance_id:
        raise RuntimeError(f"Profile identity mismatch: {profile_path}")
    flipbook = load_required(flipbook_path, unreal.PaperFlipbook)
    flipbook.set_editor_property("collision_source", unreal.FlipbookCollisionMode.EACH_FRAME_COLLISION)
    if not unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save attack Flipbook: {flipbook_path}")
    attack_clip = make_attack_clip(flipbook, profile.get_editor_property("world_height"))

    rebuilt_sets = []
    default_found = False
    for source_set in profile.get_editor_property("animation_sets"):
        rebuilt_set = unreal.ReEcho2DCompositeAnimationSet()
        visual_set_id = source_set.get_editor_property("weapon_visual_set_id")
        clips = dict(source_set.get_editor_property("clips"))
        if str(visual_set_id) in ("", "None"):
            clips[attack_tag] = attack_clip
            default_found = True
        rebuilt_set.set_editor_property("weapon_visual_set_id", visual_set_id)
        rebuilt_set.set_editor_property("clips", clips)
        rebuilt_sets.append(rebuilt_set)

    if not default_found:
        default_set = unreal.ReEcho2DCompositeAnimationSet()
        default_set.set_editor_property("weapon_visual_set_id", "")
        default_set.set_editor_property("clips", {attack_tag: attack_clip})
        rebuilt_sets.insert(0, default_set)

    profile.set_editor_property("animation_sets", rebuilt_sets)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save profile: {profile_path}")
    updated.append(f"{appearance_id}={flipbook.get_path_name()}")

unreal.log("[PlayerAttackFlipbooks] " + ", ".join(updated))
