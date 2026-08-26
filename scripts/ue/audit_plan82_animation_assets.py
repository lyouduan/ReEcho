"""Read-only audit for the Plan82 sparse 2D-animation asset contract."""

from pathlib import Path
import os
import subprocess

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
ANIMATION_ROOT = PROJECT_ROOT / "Content" / "ReEcho" / "Art" / "Animation2D"
CANONICAL_FSM = "/Game/ReEcho/DataAsset/Common/Animation2D/SM2D_DefaultCharacter"
LEGACY_FSM = "/Game/ReEcho/Animation2D/SM2D_DefaultCharacter"
PROFILE_ROOTS = (
    "/Game/ReEcho/DataAsset/Character/Profiles",
    "/Game/ReEcho/DataAsset/Enemy/Profiles",
)
EXPECTED_STATES = (
    "Animation.Move",
    "Animation.Born",
    "Animation.Attack.Charge",
    "Animation.Attack.Basic",
    "Animation.Hit",
    "Animation.Transform.Phase2",
    "Animation.Death",
)
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".exr"}
PLAN121_ONLY = os.environ.get("REECHO_PLAN121_ONLY") == "1"
BORN_FLIPBOOKS = {
    "Rabbit": ("/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Born", 6, 12.0),
    "Slime": ("/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Born", 5, 10.0),
    "Goat": ("/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Born", 5, 10.0),
    "Fox": ("/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/Born", 5, 10.0),
}
BORN_PROFILES = {
    "Rabbit": "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_RabbitDoll",
    "Slime": "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Slime",
    "Goat": "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_GoatPriest",
    "Fox": "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Fox",
}


def tag_name(value):
    return str(value.get_editor_property("tag_name"))


def normalized(value):
    return os.path.normcase(os.path.normpath(str(Path(value).resolve()))) if value else None


issues = []
fsm = unreal.EditorAssetLibrary.load_asset(CANONICAL_FSM)
if not isinstance(fsm, unreal.ReEcho2DAnimationStateMachineAsset):
    raise RuntimeError(f"Missing canonical FSM: {CANONICAL_FSM}")
if unreal.EditorAssetLibrary.does_asset_exist(LEGACY_FSM):
    issues.append(f"duplicate FSM remains: {LEGACY_FSM}")

initial = tag_name(fsm.get_editor_property("initial_state_tag"))
if initial != "Animation.Move":
    issues.append(f"FSM initial state is {initial}, expected Animation.Move")
state_definitions = list(fsm.get_editor_property("states"))
states = [tag_name(state.get_editor_property("semantic_key")) for state in state_definitions]
if tuple(states) != EXPECTED_STATES:
    issues.append(f"FSM states mismatch: {states}")
if "Animation.Idle" in states:
    issues.append("FSM still contains Animation.Idle")
born_states = [state for state in state_definitions if tag_name(state.get_editor_property("semantic_key")) == "Animation.Born"]
if len(born_states) != 1:
    issues.append(f"FSM Born state count is {len(born_states)}, expected 1")
else:
    born_state = born_states[0]
    if born_state.get_editor_property("interrupt_priority") != 90:
        issues.append("FSM Born interrupt priority is not 90")
    if not born_state.get_editor_property("lock_until_playback_complete"):
        issues.append("FSM Born must lock until playback completes")
    if born_state.get_editor_property("terminal"):
        issues.append("FSM Born must remain non-terminal")

born_issue_start = len(issues)
born_results = []
for enemy, (path, expected_frames, expected_fps) in BORN_FLIPBOOKS.items():
    flipbook = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(flipbook, unreal.PaperFlipbook):
        issues.append(f"{enemy} Born Flipbook missing: {path}")
        continue
    frames_per_second = float(flipbook.get_editor_property("frames_per_second"))
    frame_count = int(flipbook.get_num_frames())
    duration = frame_count / frames_per_second if frames_per_second > 0.0 else 0.0
    key_frames = list(flipbook.get_editor_property("key_frames"))
    sprite_paths = [key.get_editor_property("sprite").get_path_name() for key in key_frames]
    expected_sprite_paths = [
        f"/Game/ReEcho/Art/Animation2D/Enemies/{enemy}/Born/Sprites/{index}_Sprite.{index}_Sprite"
        for index in range(1, expected_frames + 1)
    ]
    if frame_count != expected_frames or len(key_frames) != expected_frames:
        issues.append(
            f"{enemy} Born frame count is {frame_count} with {len(key_frames)} keys, expected {expected_frames}"
        )
    if abs(frames_per_second - expected_fps) > 0.0001:
        issues.append(f"{enemy} Born FPS is {frames_per_second}, expected {expected_fps}")
    if abs(duration - 0.5) > 0.0001:
        issues.append(f"{enemy} Born duration is {duration}, expected 0.5")
    if sprite_paths != expected_sprite_paths:
        issues.append(f"{enemy} Born sprite order mismatch: {sprite_paths}")
    born_results.append(f"{enemy}:{frame_count}@{frames_per_second:g}={duration:.3f}s")

for enemy, profile_path in BORN_PROFILES.items():
    profile = unreal.EditorAssetLibrary.load_asset(profile_path)
    if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
        issues.append(f"{enemy} production profile missing: {profile_path}")
        continue
    born_clips = []
    for animation_set in profile.get_editor_property("animation_sets"):
        for semantic, clip in animation_set.get_editor_property("clips").items():
            if tag_name(semantic) == "Animation.Born":
                born_clips.append(clip)
    if not born_clips:
        issues.append(f"{enemy} production profile has no Animation.Born clip")
        continue
    expected_flipbook_path = BORN_FLIPBOOKS[enemy][0]
    for clip in born_clips:
        flipbook = clip.get_editor_property("flipbook")
        if not isinstance(flipbook, unreal.PaperFlipbook) or not flipbook.get_path_name().startswith(
            expected_flipbook_path + "."
        ):
            issues.append(f"{enemy} profile Born clip references the wrong Flipbook")
        if clip.get_editor_property("looping"):
            issues.append(f"{enemy} profile Born clip must remain non-looping")

born_issues = issues[born_issue_start:]
if born_issues:
    for issue in born_issues:
        unreal.log_error(f"PLAN121_BORN_AUDIT_ISSUE {issue}")
else:
    unreal.log(f"PLAN121_BORN_AUDIT_RESULT {'|'.join(born_results)} profiles=4 non_looping=4 issues=0")

profile_count = 0
for root in PROFILE_ROOTS:
    for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
        profile = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
            continue
        profile_count += 1
        if profile.get_editor_property("state_machine") != fsm:
            issues.append(f"non-canonical FSM reference: {profile.get_path_name()}")
        owns_death = False
        for animation_set in profile.get_editor_property("animation_sets"):
            for semantic, clip in animation_set.get_editor_property("clips").items():
                name = tag_name(semantic)
                if name == "Animation.Idle":
                    issues.append(f"Idle clip remains: {profile.get_path_name()}")
                flipbook = clip.get_editor_property("flipbook")
                if not isinstance(flipbook, unreal.PaperFlipbook):
                    issues.append(f"invalid Flipbook: {profile.get_path_name()}:{name}")
                if root == "/Game/ReEcho/DataAsset/Enemy/Profiles" and name == "Animation.Death":
                    owns_death = isinstance(flipbook, unreal.PaperFlipbook)
                    if owns_death and flipbook.get_num_frames() <= 0:
                        issues.append(f"empty Death Flipbook: {profile.get_path_name()}")
        if root == "/Game/ReEcho/DataAsset/Enemy/Profiles" and not owns_death:
            issues.append(f"enemy profile missing Death: {profile.get_path_name()}")

imported = set()
for path in unreal.EditorAssetLibrary.list_assets(
    "/Game/ReEcho/Art/Animation2D", recursive=True, include_folder=False
):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(asset, unreal.Texture2D):
        data = asset.get_editor_property("asset_import_data")
        filename = data.get_first_filename() if data else ""
        value = normalized(filename)
        if value:
            imported.add(value)

deleted_result = subprocess.run(
    ["git", "diff", "--name-only", "--diff-filter=D", "--", "Content/ReEcho/Art/Animation2D"],
    cwd=PROJECT_ROOT,
    check=True,
    capture_output=True,
    text=True,
)
deleted_assets = {
    line.strip().replace("\\", "/") for line in deleted_result.stdout.splitlines() if line.strip()
}
missing_sources = []
for source in ANIMATION_ROOT.rglob("*"):
    if source.is_file() and source.suffix.lower() in IMAGE_EXTENSIONS and normalized(source) not in imported:
        same_stem = source.with_suffix(".uasset")
        disk_asset = "Content/" + same_stem.relative_to(PROJECT_ROOT / "Content").as_posix()
        if same_stem.exists() or disk_asset in deleted_assets:
            continue
        missing_sources.append(source.relative_to(PROJECT_ROOT).as_posix())
if missing_sources and not PLAN121_ONLY:
    issues.append(f"unimported source textures: {missing_sources}")

if issues:
    for issue in issues:
        unreal.log_error(f"PLAN82_AUDIT_ISSUE {issue}")
    raise RuntimeError(f"Plan82 audit failed with {len(issues)} issue(s)")

unreal.log(
    f"PLAN82_AUDIT_RESULT profiles={profile_count} states={len(states)} "
    f"imported_sources={len(imported)} born={'|'.join(born_results)} issues=0"
)
