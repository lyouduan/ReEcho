"""Audit Plan120 Bad enemy transform Paper2D assets and profile bindings.

Use ``-Plan120Baseline`` before authoring to print the existing profile and
neighboring asset conventions without requiring the new Transform assets.
The default mode is a strict post-authoring audit and raises on any mismatch.
"""

import unreal


TRANSFORM_TAG_NAME = "Animation.Transform.Phase2"
SPECS = (
    ("BadRabbit", "DA_Enemy_RabbitDoll", 7),
    ("BadSlime", "DA_Enemy_Slime", 5),
    ("BadFox", "DA_Enemy_Fox", 4),
)
PROFILE_ROOT = "/Game/ReEcho/DataAsset/Enemy/Profiles"
ANIMATION_ROOT = "/Game/ReEcho/Art/Animation2D/Enemies"


def required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Expected {expected_type.__name__}: {path}, got {asset}")
    return asset


def semantic(name):
    value = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError(f"Missing semantic GameplayTag: {name}")
    return value


def set_id(animation_set):
    value = str(animation_set.get_editor_property("weapon_visual_set_id"))
    return "" if value in ("None", "") else value


def asset_path(asset):
    return asset.get_path_name().split(".", 1)[0] if asset else "None"


def profile_snapshot(profile):
    result = []
    for animation_set in profile.get_editor_property("animation_sets"):
        clips = animation_set.get_editor_property("clips")
        clip_rows = []
        for tag, clip in clips.items():
            tag_name = str(tag.get_editor_property("tag_name"))
            clip_rows.append(
                (
                    tag_name,
                    asset_path(clip.get_editor_property("flipbook")),
                    bool(clip.get_editor_property("looping")),
                    bool(clip.get_editor_property("restart_on_request")),
                )
            )
        result.append((set_id(animation_set), sorted(clip_rows)))
    return result


def log_neighbor_conventions(monster):
    root = f"{ANIMATION_ROOT}/{monster}"
    flipbooks = []
    sprites = []
    for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.PaperFlipbook):
            flipbooks.append((path, float(asset.get_editor_property("frames_per_second"))))
        elif isinstance(asset, unreal.PaperSprite):
            sprites.append(
                (
                    path,
                    str(asset.get_editor_property("pivot_mode")),
                    float(asset.get_editor_property("pixels_per_unreal_unit")),
                )
            )
    unreal.log(f"PLAN120_BASELINE_CONVENTIONS monster={monster} flipbooks={flipbooks} sprites={sprites[:8]}")


def baseline():
    for monster, profile_name, _ in SPECS:
        profile = required(
            f"{PROFILE_ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile
        )
        unreal.log(
            f"PLAN120_BASELINE_PROFILE monster={monster} profile={profile.get_path_name()} "
            f"sets={profile_snapshot(profile)}"
        )
        log_neighbor_conventions(monster)
    unreal.log("PLAN120_BASELINE_RESULT profiles=3 read_only=true")


def strict_audit():
    transform_tag = semantic(TRANSFORM_TAG_NAME)
    audited = []
    for monster, profile_name, frame_count in SPECS:
        root = f"{ANIMATION_ROOT}/{monster}"
        sprites = []
        for index in range(1, frame_count + 1):
            texture_path = f"{root}/Transform/Textures/Transform_{index:02d}"
            sprite_path = f"{root}/Transform/Sprites/Transform_{index:02d}_Sprite"
            texture = required(texture_path, unreal.Texture2D)
            sprite = required(sprite_path, unreal.PaperSprite)
            if sprite.get_editor_property("source_texture") != texture:
                raise RuntimeError(f"Sprite source mismatch: {sprite_path}")
            if sprite.get_editor_property("pivot_mode") != unreal.SpritePivotMode.CENTER_CENTER:
                raise RuntimeError(f"Unexpected pivot: {sprite_path}")
            if abs(float(sprite.get_editor_property("pixels_per_unreal_unit")) - 1.0) > 0.0001:
                raise RuntimeError(f"Unexpected pixels_per_unreal_unit: {sprite_path}")
            sprites.append(sprite)

        flipbook_path = f"{root}/Flipbooks/Transform"
        flipbook = required(flipbook_path, unreal.PaperFlipbook)
        if abs(float(flipbook.get_editor_property("frames_per_second")) - frame_count) > 0.0001:
            raise RuntimeError(f"Unexpected FPS: {flipbook_path}")
        key_frames = list(flipbook.get_editor_property("key_frames"))
        if len(key_frames) != frame_count:
            raise RuntimeError(f"Unexpected key frame count: {flipbook_path}")
        for index, (key_frame, sprite) in enumerate(zip(key_frames, sprites), start=1):
            if key_frame.get_editor_property("sprite") != sprite:
                raise RuntimeError(f"Key frame order mismatch: {flipbook_path} frame={index}")
            if int(key_frame.get_editor_property("frame_run")) != 1:
                raise RuntimeError(f"Unexpected frame_run: {flipbook_path} frame={index}")

        profile = required(
            f"{PROFILE_ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile
        )
        animation_sets = list(profile.get_editor_property("animation_sets"))
        if not animation_sets:
            raise RuntimeError(f"Profile has no AnimationSet: {profile.get_path_name()}")
        phase2_sets = []
        for animation_set in animation_sets:
            if set_id(animation_set) == "Phase2":
                phase2_sets.append(animation_set)
        if len(phase2_sets) != 1:
            raise RuntimeError(
                f"Expected one Phase2 AnimationSet: {profile.get_path_name()} count={len(phase2_sets)}"
            )
        clip = phase2_sets[0].get_editor_property("clips").get(transform_tag)
        if not clip or clip.get_editor_property("flipbook") != flipbook:
            raise RuntimeError(f"Transform binding mismatch: {profile.get_path_name()} set=Phase2")
        if bool(clip.get_editor_property("looping")):
            raise RuntimeError(f"Transform clip must be non-looping: {profile.get_path_name()}")
        if not bool(clip.get_editor_property("restart_on_request")):
            raise RuntimeError(f"Transform clip must restart: {profile.get_path_name()}")
        audited.append((monster, frame_count, len(animation_sets)))

    unreal.log(f"PLAN120_AUDIT_RESULT assets_ok=true profiles_ok=true audited={audited}")


if "-Plan120Baseline" in unreal.SystemLibrary.get_command_line():
    baseline()
else:
    strict_audit()
