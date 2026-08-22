"""Complete the production Plan74 FSM and enemy animation profile contracts.

Run only through UnrealEditor/UnrealEditor-Cmd with ``-ExecutePythonScript``.
The operation is idempotent and saves only the shared FSM plus production enemy
profiles whose serialized contract changes.
"""

import unreal


ROOT = "/Game/ReEcho/Animation2D"
STATE_MACHINE_PATH = f"{ROOT}/SM2D_DefaultCharacter"
PHASE2_SET_ID = "Phase2"

ENEMY_FALLBACKS = {
    "DA_Enemy_Bomber": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Hit",
    ),
    "DA_Enemy_Grunt": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Hit",
    ),
    "DA_Enemy_Slime": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
        "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Hit",
    ),
    "DA_Enemy_RabbitDoll": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Default",
        "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/NRA",
        "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Default",
    ),
    "DA_Enemy_Fox": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/Walk",
        "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/gfa",
        "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/gfar",
    ),
    "DA_Enemy_GoatPriest": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0",
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Attack0",
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0",
    ),
    "DA_Enemy_Shield": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0",
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Attack0",
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0",
    ),
    "DA_Enemy_TimeGuard": (
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0",
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Attack0",
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0",
    ),
}


def required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Expected {expected_type.__name__}: {path}")
    return asset


def semantic(name):
    value = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError(f"Missing semantic GameplayTag: {name}")
    return value


def make_clip(flipbook, looping, restart):
    clip = unreal.ReEcho2DAnimationClip()
    clip.set_editor_property("flipbook", flipbook)
    clip.set_editor_property("looping", looping)
    clip.set_editor_property("restart_on_request", restart)
    clip.set_editor_property("play_rate", 1.0)
    clip.set_editor_property("use_native_scale", False)
    clip.set_editor_property("mirror_horizontally", False)
    clip.set_editor_property("local_offset", unreal.Vector(0.0, 0.0, 0.0))
    clip.set_editor_property("translucent_sort_priority", 10)
    return clip


def make_state(name, priority, lock=False, terminal=False):
    state = unreal.ReEcho2DAnimationStateDefinition()
    tag = semantic(name)
    state.set_editor_property("state_tag", tag)
    state.set_editor_property("semantic_key", tag)
    state.set_editor_property("interrupt_priority", priority)
    state.set_editor_property("lock_until_playback_complete", lock)
    state.set_editor_property("terminal", terminal)
    return state


def normalized_set_id(animation_set):
    value = str(animation_set.get_editor_property("weapon_visual_set_id"))
    return "" if value in ("None", "") else value


def find_set(animation_sets, set_id):
    for index, animation_set in enumerate(animation_sets):
        if normalized_set_id(animation_set) == set_id:
            return index, animation_set
    return None, None


def clip_has_flipbook(clips, tag):
    clip = clips.get(tag)
    return bool(clip and clip.get_editor_property("flipbook"))


def fill_clip(clips, tag_name, flipbook, looping, restart, force=False):
    tag = semantic(tag_name)
    if force or not clip_has_flipbook(clips, tag):
        clips[tag] = make_clip(flipbook, looping, restart)


state_machine = required(STATE_MACHINE_PATH, unreal.ReEcho2DAnimationStateMachineAsset)
state_machine.set_editor_property("initial_state_tag", semantic("Animation.Idle"))
state_machine.set_editor_property(
    "states",
    [
        make_state("Animation.Idle", 0),
        make_state("Animation.Move", 10),
        make_state("Animation.Attack.Charge", 30, lock=True),
        make_state("Animation.Attack.Basic", 40, lock=True),
        make_state("Animation.Hit", 60, lock=True),
        make_state("Animation.Transform.Phase2", 80, lock=True),
        make_state("Animation.Death", 100, lock=True, terminal=True),
    ],
)
unreal.EditorAssetLibrary.save_loaded_asset(state_machine, only_if_is_dirty=False)

changed_profiles = []
for profile_name, fallback_paths in ENEMY_FALLBACKS.items():
    profile = required(f"{ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile)
    profile.set_editor_property("state_machine", state_machine)
    animation_sets = list(profile.get_editor_property("animation_sets"))
    base_index, base_set = find_set(animation_sets, "")
    if base_set is None:
        raise RuntimeError(f"Missing default AnimationSet: {profile.get_path_name()}")
    base, attack, hit = (required(path, unreal.PaperFlipbook) for path in fallback_paths)
    clips = base_set.get_editor_property("clips")
    fill_clip(clips, "Animation.Idle", base, True, False)
    fill_clip(clips, "Animation.Move", base, True, False)
    fill_clip(clips, "Animation.Attack.Charge", base, True, True)
    fill_clip(clips, "Animation.Attack.Basic", attack, False, True)
    fill_clip(clips, "Animation.Hit", hit, False, True)
    fill_clip(clips, "Animation.Death", hit, False, True)
    base_set.set_editor_property("clips", clips)
    animation_sets[base_index] = base_set

    phase2_index, phase2_set = find_set(animation_sets, PHASE2_SET_ID)
    if phase2_set is not None:
        phase2_clips = phase2_set.get_editor_property("clips")
        phase2_idle = phase2_clips.get(semantic("Animation.Idle"))
        phase2_base = phase2_idle.get_editor_property("flipbook") if phase2_idle else None
        if not phase2_base:
            phase2_base = base
        fill_clip(phase2_clips, "Animation.Idle", phase2_base, True, False)
        fill_clip(phase2_clips, "Animation.Move", phase2_base, True, False)
        fill_clip(phase2_clips, "Animation.Attack.Charge", phase2_base, True, True)
        fill_clip(phase2_clips, "Animation.Attack.Basic", phase2_base, False, True)
        fill_clip(phase2_clips, "Animation.Hit", phase2_base, False, True)
        fill_clip(phase2_clips, "Animation.Transform.Phase2", phase2_base, False, True)
        fill_clip(phase2_clips, "Animation.Death", phase2_base, False, True)
        phase2_set.set_editor_property("clips", phase2_clips)
        animation_sets[phase2_index] = phase2_set

    if profile_name == "DA_Enemy_Fox":
        fox_charge = required(
            "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/gfar", unreal.PaperFlipbook
        )
        fill_clip(clips, "Animation.Attack.Charge", fox_charge, True, True, force=True)
        base_set.set_editor_property("clips", clips)
        animation_sets[base_index] = base_set

    if profile_name == "DA_Enemy_TimeGuard":
        fill_clip(clips, "Animation.Attack.Charge", attack, True, True, force=True)
        base_set.set_editor_property("clips", clips)
        animation_sets[base_index] = base_set
        phase2_index, phase2_set = find_set(animation_sets, PHASE2_SET_ID)
        if phase2_set is None:
            raise RuntimeError("TimeGuard requires a Phase2 AnimationSet")
        phase2_clips = phase2_set.get_editor_property("clips")
        dark_walk = required(
            "/Game/ReEcho/Art/Animation2D/Enemies/BadGoat/Flipbooks/Walk1", unreal.PaperFlipbook
        )
        dark_charge = required(
            "/Game/ReEcho/Art/Animation2D/Enemies/BadGoat/Flipbooks/Attack0", unreal.PaperFlipbook
        )
        dark_attack = required(
            "/Game/ReEcho/Art/Animation2D/Enemies/BadGoat/Flipbooks/Attack1", unreal.PaperFlipbook
        )
        fill_clip(phase2_clips, "Animation.Idle", dark_walk, True, False, force=True)
        fill_clip(phase2_clips, "Animation.Move", dark_walk, True, False, force=True)
        fill_clip(phase2_clips, "Animation.Attack.Charge", dark_charge, True, True, force=True)
        fill_clip(phase2_clips, "Animation.Attack.Basic", dark_attack, False, True, force=True)
        fill_clip(phase2_clips, "Animation.Hit", dark_walk, False, True, force=True)
        # No dedicated transform frames exist. Switching immediately to the dark
        # authored form is the explicit presentation fallback until art supplies one.
        fill_clip(phase2_clips, "Animation.Transform.Phase2", dark_walk, False, True, force=True)
        fill_clip(phase2_clips, "Animation.Death", dark_walk, False, True, force=True)
        phase2_set.set_editor_property("clips", phase2_clips)
        animation_sets[phase2_index] = phase2_set

    profile.set_editor_property("animation_sets", animation_sets)
    unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False)
    changed_profiles.append(profile.get_path_name())

unreal.log(
    f"PLAN74_CONFIG_RESULT state_machine={state_machine.get_path_name()} "
    f"profiles={len(changed_profiles)} paths={changed_profiles}"
)
