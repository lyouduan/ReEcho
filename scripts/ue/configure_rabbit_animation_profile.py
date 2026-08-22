"""Normalize Rabbit sprites and author every semantic clip in its DataAsset."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_RabbitDoll"
DEFAULT_FLIPBOOK_PATH = "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Default"
ATTACK_FLIPBOOK_PATH = "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/NRA"
PHASE2_FLIPBOOK_PATH = "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/BadRabbitWalk"
PHASE2_SET_ID = "Phase2"
ATTACK_SPRITE_ROOT = "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Attack/Sprites"
ATTACK_UNION_UV = unreal.Vector2D(366.0, 295.0)
ATTACK_UNION_SIZE = unreal.Vector2D(291.0, 434.0)


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


def clip(flipbook, looping, restart, mirror=False):
    value = unreal.ReEcho2DAnimationClip()
    value.set_editor_property("flipbook", flipbook)
    value.set_editor_property("looping", looping)
    value.set_editor_property("restart_on_request", restart)
    value.set_editor_property("play_rate", 1.0)
    value.set_editor_property("use_native_scale", False)
    value.set_editor_property("mirror_horizontally", mirror)
    value.set_editor_property("local_offset", unreal.Vector(0.0, 0.0, 0.0))
    value.set_editor_property("translucent_sort_priority", 10)
    return value


def normalize_attack_sprites():
    for index in range(16):
        sprite_path = f"{ATTACK_SPRITE_ROOT}/NRA_{index:05d}_Sprite"
        sprite = required(sprite_path, unreal.PaperSprite)
        sprite.set_editor_property("source_uv", ATTACK_UNION_UV)
        sprite.set_editor_property("source_dimension", ATTACK_UNION_SIZE)
        sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CENTER_CENTER)
        if not unreal.ReEcho2DAnimationComponent.rebuild_sprite_asset(sprite):
            raise RuntimeError(f"Failed to rebuild Rabbit attack sprite: {sprite_path}")
        unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False)


def configure_profile():
    profile = required(PROFILE_PATH, unreal.ReEcho2DCharacterPresentationProfile)
    default_flipbook = required(DEFAULT_FLIPBOOK_PATH, unreal.PaperFlipbook)
    attack_flipbook = required(ATTACK_FLIPBOOK_PATH, unreal.PaperFlipbook)
    phase2_flipbook = required(PHASE2_FLIPBOOK_PATH, unreal.PaperFlipbook)

    sets = list(profile.get_editor_property("animation_sets"))
    default_set = None
    for candidate in sets:
        if str(candidate.get_editor_property("weapon_visual_set_id")) in ("None", ""):
            default_set = candidate
            break
    if default_set is None:
        default_set = unreal.ReEcho2DCompositeAnimationSet()
        default_set.set_editor_property("weapon_visual_set_id", "")
        sets.insert(0, default_set)

    # Rebuild the map instead of mutating Python-wrapped GameplayTag keys. The wrapper can preserve
    # stale equivalent keys and serialize duplicate semantics into the UE TMap.
    clips = {}
    clips[semantic("Animation.Idle")] = clip(default_flipbook, True, False)
    clips[semantic("Animation.Move")] = clip(default_flipbook, True, False)
    clips[semantic("Animation.Attack.Basic")] = clip(attack_flipbook, False, True)
    clips[semantic("Animation.Hit")] = clip(default_flipbook, False, True)
    default_set.set_editor_property("clips", clips)

    phase2_set = None
    for candidate in sets:
        if str(candidate.get_editor_property("weapon_visual_set_id")) == PHASE2_SET_ID:
            phase2_set = candidate
            break
    if phase2_set is None:
        phase2_set = unreal.ReEcho2DCompositeAnimationSet()
        phase2_set.set_editor_property("weapon_visual_set_id", PHASE2_SET_ID)
        sets.append(phase2_set)

    phase2_clips = {}
    phase2_clips[semantic("Animation.Idle")] = clip(phase2_flipbook, True, False)
    phase2_clips[semantic("Animation.Move")] = clip(phase2_flipbook, True, False)
    phase2_clips[semantic("Animation.Attack.Charge")] = clip(phase2_flipbook, True, True)
    phase2_clips[semantic("Animation.Attack.Basic")] = clip(phase2_flipbook, False, True)
    phase2_clips[semantic("Animation.Hit")] = clip(phase2_flipbook, False, True)
    phase2_clips[semantic("Animation.Transform.Phase2")] = clip(phase2_flipbook, False, True)
    phase2_set.set_editor_property("clips", phase2_clips)
    profile.set_editor_property("animation_sets", sets)
    unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False)


normalize_attack_sprites()
configure_profile()
unreal.log(
    "Rabbit profile configured: Phase2 temporarily uses BadRabbitWalk; "
    "all semantics use Profile.WorldHeight; attack crop normalized"
)
