"""Ensure every enemy presentation profile has one editable Phase2 animation set."""

import unreal


PROFILE_NAMES = (
    "DA_Enemy_Bomber",
    "DA_Enemy_Fox",
    "DA_Enemy_GoatPriest",
    "DA_Enemy_Grunt",
    "DA_Enemy_RabbitDoll",
    "DA_Enemy_Shield",
    "DA_Enemy_Slime",
    "DA_Enemy_TimeGuard",
)
PHASE2_SET_ID = "Phase2"
VARIANTS = {
    "DA_Enemy_RabbitDoll": {
        "base": "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/BadRabbitWalk",
    },
    "DA_Enemy_Fox": {
        "base": "/Game/ReEcho/Art/Animation2D/Enemies/BadFox/Flipbooks/bfw01",
        "charge": "/Game/ReEcho/Art/Animation2D/Enemies/BadFox/Flipbooks/bfr",
        "attack": "/Game/ReEcho/Art/Animation2D/Enemies/BadFox/Flipbooks/bfr_2",
        "hit": "/Game/ReEcho/Art/Animation2D/Enemies/BadFox/Flipbooks/bfr",
    },
    "DA_Enemy_Slime": {
        "base": "/Game/ReEcho/Art/Animation2D/Enemies/BadSlime/Flipbooks/walk",
        "hit": "/Game/ReEcho/Art/Animation2D/Enemies/BadSlime/Flipbooks/Hitted",
    },
}
FALLBACKS = {
    "DA_Enemy_Bomber": "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
    "DA_Enemy_Grunt": "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
    "DA_Enemy_Shield": "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Default",
    "DA_Enemy_TimeGuard": "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Default",
}


def required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Expected {expected_type.__name__}: {path}")
    return asset


def semantic(name):
    tag = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        raise RuntimeError(f"Missing semantic tag: {name}")
    return tag


def make_clip(flipbook, looping, restart):
    value = unreal.ReEcho2DAnimationClip()
    value.set_editor_property("flipbook", flipbook)
    value.set_editor_property("looping", looping)
    value.set_editor_property("restart_on_request", restart)
    value.set_editor_property("play_rate", 1.0)
    value.set_editor_property("use_native_scale", False)
    value.set_editor_property("mirror_horizontally", False)
    value.set_editor_property("local_offset", unreal.Vector(0.0, 0.0, 0.0))
    value.set_editor_property("translucent_sort_priority", 10)
    return value


def find_default_flipbook(profile, profile_name):
    for animation_set in profile.get_editor_property("animation_sets"):
        set_id = str(animation_set.get_editor_property("weapon_visual_set_id"))
        if set_id not in ("None", ""):
            continue
        clips = animation_set.get_editor_property("clips")
        idle = clips.get(semantic("Animation.Idle"))
        if idle and idle.get_editor_property("flipbook"):
            return idle.get_editor_property("flipbook")
        for candidate in clips.values():
            flipbook = candidate.get_editor_property("flipbook")
            if flipbook:
                return flipbook
    fallback_path = FALLBACKS.get(profile_name)
    if fallback_path:
        return required(fallback_path, unreal.PaperFlipbook)
    raise RuntimeError(f"No default Flipbook or explicit fallback in {profile.get_path_name()}")


def configure_profile(profile_name):
    profile = required(
        f"/Game/ReEcho/DataAsset/Enemy/Profiles/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile
    )
    default_flipbook = find_default_flipbook(profile, profile_name)
    variant = VARIANTS.get(profile_name, {})
    base = required(variant["base"], unreal.PaperFlipbook) if "base" in variant else default_flipbook
    charge = required(variant["charge"], unreal.PaperFlipbook) if "charge" in variant else base
    attack = required(variant["attack"], unreal.PaperFlipbook) if "attack" in variant else base
    hit = required(variant["hit"], unreal.PaperFlipbook) if "hit" in variant else base

    sets = [
        value
        for value in profile.get_editor_property("animation_sets")
        if str(value.get_editor_property("weapon_visual_set_id")) != PHASE2_SET_ID
    ]
    phase2 = unreal.ReEcho2DCompositeAnimationSet()
    phase2.set_editor_property("weapon_visual_set_id", PHASE2_SET_ID)
    clips = {
        semantic("Animation.Idle"): make_clip(base, True, False),
        semantic("Animation.Move"): make_clip(base, True, False),
        semantic("Animation.Attack.Charge"): make_clip(charge, True, True),
        semantic("Animation.Attack.Basic"): make_clip(attack, False, True),
        semantic("Animation.Hit"): make_clip(hit, False, True),
        semantic("Animation.Transform.Phase2"): make_clip(base, False, True),
    }
    phase2.set_editor_property("clips", clips)
    sets.append(phase2)
    profile.set_editor_property("animation_sets", sets)
    unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False)
    unreal.log(
        f"PHASE2_PROFILE_CONFIGURED profile={profile.get_path_name()} "
        f"world_height={profile.get_editor_property('world_height')} base={base.get_path_name()}"
    )


for name in PROFILE_NAMES:
    configure_profile(name)

unreal.log(f"PHASE2_PROFILE_RESULT configured={len(PROFILE_NAMES)}")
