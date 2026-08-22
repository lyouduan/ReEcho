"""Read-only production coverage audit for the Plan74 2D animation contract."""

import unreal


ROOT = "/Game/ReEcho/Animation2D"
ENEMY_PROFILE_NAMES = (
    "DA_Enemy_Bomber",
    "DA_Enemy_Fox",
    "DA_Enemy_GoatPriest",
    "DA_Enemy_Grunt",
    "DA_Enemy_RabbitDoll",
    "DA_Enemy_Shield",
    "DA_Enemy_Slime",
    "DA_Enemy_TimeGuard",
)
CHARACTER_PROFILE_NAMES = (
    "DA_Character_J_CLOVER",
    "DA_Character_J_DIAMOND",
    "DA_Character_J_HEART",
    "DA_Character_J_SPADE",
    "DA_Echo_J_CLOVER",
    "DA_Echo_J_DIAMOND",
    "DA_Echo_J_HEART",
    "DA_Echo_J_SPADE",
)
FSM_EXPECTATIONS = (
    ("Animation.Idle", 0, False, False),
    ("Animation.Move", 10, False, False),
    ("Animation.Attack.Charge", 30, True, False),
    ("Animation.Attack.Basic", 40, True, False),
    ("Animation.Hit", 60, True, False),
    ("Animation.Transform.Phase2", 80, True, False),
    ("Animation.Death", 100, True, True),
)
BASE_SEMANTICS = (
    "Animation.Idle",
    "Animation.Move",
    "Animation.Attack.Charge",
    "Animation.Attack.Basic",
    "Animation.Hit",
    "Animation.Death",
)
PHASE2_SEMANTICS = BASE_SEMANTICS + ("Animation.Transform.Phase2",)


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


def normalized_set_id(animation_set):
    value = str(animation_set.get_editor_property("weapon_visual_set_id"))
    return "" if value in ("None", "") else value


def find_set(profile, set_id):
    return next(
        (
            animation_set
            for animation_set in profile.get_editor_property("animation_sets")
            if normalized_set_id(animation_set) == set_id
        ),
        None,
    )


issues = []
state_machine = required(
    f"{ROOT}/SM2D_DefaultCharacter", unreal.ReEcho2DAnimationStateMachineAsset
)
states_by_semantic = {
    str(state.get_editor_property("semantic_key").get_editor_property("tag_name")): state
    for state in state_machine.get_editor_property("states")
}
for name, priority, lock, terminal in FSM_EXPECTATIONS:
    definition = states_by_semantic.get(name)
    if not definition:
        issues.append(f"FSM missing {name}")
        continue
    if int(definition.get_editor_property("interrupt_priority")) != priority:
        issues.append(f"FSM priority mismatch {name}")
    if bool(definition.get_editor_property("lock_until_playback_complete")) != lock:
        issues.append(f"FSM lock mismatch {name}")
    if bool(definition.get_editor_property("terminal")) != terminal:
        issues.append(f"FSM terminal mismatch {name}")

for profile_name in ENEMY_PROFILE_NAMES:
    profile = required(f"{ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile)
    if profile.get_editor_property("state_machine") != state_machine:
        issues.append(f"{profile_name} does not reference the production FSM")
    for set_id, required_semantics in (("", BASE_SEMANTICS), ("Phase2", PHASE2_SEMANTICS)):
        animation_set = find_set(profile, set_id)
        if animation_set is None:
            if set_id == "Phase2":
                continue
            issues.append(f"{profile_name} missing default AnimationSet")
            continue
        clips = animation_set.get_editor_property("clips")
        for semantic_name in required_semantics:
            clip = clips.get(semantic(semantic_name))
            flipbook = clip.get_editor_property("flipbook") if clip else None
            if not flipbook:
                issues.append(f"{profile_name}:{set_id or 'Base'} missing {semantic_name}")
                continue
            looping = bool(clip.get_editor_property("looping"))
            expected_looping = semantic_name in (
                "Animation.Idle",
                "Animation.Move",
                "Animation.Attack.Charge",
            )
            if looping != expected_looping:
                issues.append(
                    f"{profile_name}:{set_id or 'Base'} looping mismatch {semantic_name}"
                )
            unreal.log(
                f"PLAN74_COVERAGE profile={profile_name} set={set_id or 'Base'} "
                f"semantic={semantic_name} looping={looping} flipbook={flipbook.get_path_name()}"
            )

for profile_name in CHARACTER_PROFILE_NAMES:
    profile = required(f"{ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile)
    if profile.get_editor_property("state_machine") != state_machine:
        issues.append(f"{profile_name} does not reference the production FSM")
    animation_set = find_set(profile, "")
    if animation_set is None:
        issues.append(f"{profile_name} missing default AnimationSet")
        continue
    clips = animation_set.get_editor_property("clips")
    for semantic_name in (
        "Animation.Idle",
        "Animation.Move",
        "Animation.Attack.Basic",
        "Animation.Hit",
    ):
        clip = clips.get(semantic(semantic_name))
        if not clip or not clip.get_editor_property("flipbook"):
            issues.append(f"{profile_name}:Base missing {semantic_name}")

if issues:
    for issue in issues:
        unreal.log_error(f"PLAN74_CONTRACT_ISSUE {issue}")
    raise RuntimeError(f"Plan74 animation contract audit failed with {len(issues)} issue(s)")

unreal.log(
    "PLAN74_CONTRACT_RESULT "
    f"enemy_profiles={len(ENEMY_PROFILE_NAMES)} "
    f"character_profiles={len(CHARACTER_PROFILE_NAMES)} issues=0"
)
