"""Repair only the missing Spade MoonStaff attack binding and verify core clips."""

import unreal


PROFILE_PATH = "/Game/ReEcho/Animation2D/DA_Character_J_SPADE"
WALK_PATH = "/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/walk"
ATTACK_PATH = "/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack"


def fail(message):
    raise RuntimeError(f"[SpadeProfileFix] {message}")


def semantic(name):
    tag = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        fail(f"GameplayTag is not registered: {name}")
    return tag


def same_asset(value, expected):
    return value is not None and value.get_path_name().lower() == expected.get_path_name().lower()


def tag_name(tag):
    return str(tag.get_editor_property("tag_name"))


def clip_by_semantic(clips, name):
    return next((clip for tag, clip in clips.items() if tag_name(tag) == name), None)


profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
walk = unreal.EditorAssetLibrary.load_asset(WALK_PATH)
attack = unreal.EditorAssetLibrary.load_asset(ATTACK_PATH)
if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
    fail("Spade profile is unavailable")
if not isinstance(walk, unreal.PaperFlipbook) or not isinstance(attack, unreal.PaperFlipbook):
    fail("Spade Walk or Attack Flipbook is unavailable")
if len(walk.get_editor_property("key_frames")) == 0 or len(attack.get_editor_property("key_frames")) == 0:
    fail("Spade Walk or Attack Flipbook has no frames")

attack_tag = semantic("Animation.Attack.Basic")
rebuilt_sets = []
default_valid = False
moon_staff_found = False

for source_set in profile.get_editor_property("animation_sets"):
    rebuilt = unreal.ReEcho2DCompositeAnimationSet()
    visual_set_id = source_set.get_editor_property("weapon_visual_set_id")
    clips = dict(source_set.get_editor_property("clips"))
    normalized_id = str(visual_set_id)
    if normalized_id in ("", "None"):
        idle_clip = clip_by_semantic(clips, "Animation.Idle")
        move_clip = clip_by_semantic(clips, "Animation.Move")
        default_valid = (
            idle_clip is not None
            and move_clip is not None
            and same_asset(idle_clip.get_editor_property("flipbook"), walk)
            and same_asset(move_clip.get_editor_property("flipbook"), walk)
        )
    elif normalized_id == "MoonStaff":
        clip = clip_by_semantic(clips, "Animation.Attack.Basic") or unreal.ReEcho2DAnimationClip()
        clip.set_editor_property("flipbook", attack)
        clip.set_editor_property("looping", False)
        clip.set_editor_property("restart_on_request", True)
        clip.set_editor_property("play_rate", 1.0)
        clip.set_editor_property("use_native_scale", False)
        clip.set_editor_property("world_height", profile.get_editor_property("world_height"))
        clip.set_editor_property("translucent_sort_priority", 10)
        clips[attack_tag] = clip
        moon_staff_found = True
    rebuilt.set_editor_property("weapon_visual_set_id", visual_set_id)
    rebuilt.set_editor_property("clips", clips)
    rebuilt_sets.append(rebuilt)

if not default_valid:
    fail("Default Spade Idle/Move no longer point to the authored walk Flipbook")
if not moon_staff_found:
    fail("Spade profile has no MoonStaff animation set")

profile.set_editor_property("animation_sets", rebuilt_sets)
if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    fail("Could not save the repaired Spade profile")

saved_sets = list(profile.get_editor_property("animation_sets"))
saved_moon_staff = next(
    (value for value in saved_sets if str(value.get_editor_property("weapon_visual_set_id")) == "MoonStaff"), None
)
saved_clip = (
    clip_by_semantic(saved_moon_staff.get_editor_property("clips"), "Animation.Attack.Basic")
    if saved_moon_staff
    else None
)
if saved_clip is None or not same_asset(saved_clip.get_editor_property("flipbook"), attack):
    fail("MoonStaff attack binding did not persist")

unreal.log(
    "[SpadeProfileFix] Passed: Default Idle/Move preserved and MoonStaff BasicAttack references "
    + attack.get_path_name()
)
