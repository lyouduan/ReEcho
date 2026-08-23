"""Bind authored enemy Death Flipbooks to the production sparse animation profiles."""

import unreal


PROFILE_DEATH_FLIPBOOKS = {
    "DA_Enemy_Bomber": "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Death",
    "DA_Enemy_Fox": "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/Death",
    "DA_Enemy_GoatPriest": "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Death",
    "DA_Enemy_Grunt": "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Death",
    "DA_Enemy_RabbitDoll": "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Death",
    "DA_Enemy_Shield": "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Death",
    "DA_Enemy_Slime": "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Death",
    "DA_Enemy_TimeGuard": "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Death",
}
PROFILE_ROOT = "/Game/ReEcho/DataAsset/Enemy/Profiles"


def required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Missing or invalid {expected_type.__name__}: {path}")
    return asset


def semantic(name):
    value = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError(f"Unregistered animation semantic: {name}")
    return value


def make_death_clip(flipbook):
    clip = unreal.ReEcho2DAnimationClip()
    clip.set_editor_property("flipbook", flipbook)
    clip.set_editor_property("looping", False)
    clip.set_editor_property("restart_on_request", True)
    clip.set_editor_property("play_rate", 1.0)
    clip.set_editor_property("use_native_scale", False)
    return clip


death_semantic = semantic("Animation.Death")
written = []
for profile_name, flipbook_path in PROFILE_DEATH_FLIPBOOKS.items():
    profile = required(
        f"{PROFILE_ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile
    )
    flipbook = required(flipbook_path, unreal.PaperFlipbook)
    animation_sets = list(profile.get_editor_property("animation_sets"))
    default_index = next(
        (
            index
            for index, animation_set in enumerate(animation_sets)
            if str(animation_set.get_editor_property("weapon_visual_set_id")) in ("None", "")
        ),
        None,
    )
    if default_index is None:
        default_set = unreal.ReEcho2DCompositeAnimationSet()
        default_set.set_editor_property("weapon_visual_set_id", "")
        animation_sets.insert(0, default_set)
        default_index = 0
    default_set = animation_sets[default_index]
    clips = default_set.get_editor_property("clips")
    clips[death_semantic] = make_death_clip(flipbook)
    default_set.set_editor_property("clips", clips)
    animation_sets[default_index] = default_set
    profile.set_editor_property("animation_sets", animation_sets)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save profile: {profile.get_path_name()}")
    written.append(f"{profile_name}={flipbook_path}")

unreal.log("PLAN82_DEATH_PROFILES " + ", ".join(written))
