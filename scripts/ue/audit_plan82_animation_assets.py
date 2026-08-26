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
states = [tag_name(state.get_editor_property("semantic_key")) for state in fsm.get_editor_property("states")]
if tuple(states) != EXPECTED_STATES:
    issues.append(f"FSM states mismatch: {states}")
if "Animation.Idle" in states:
    issues.append("FSM still contains Animation.Idle")

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
if missing_sources:
    issues.append(f"unimported source textures: {missing_sources}")

if issues:
    for issue in issues:
        unreal.log_error(f"PLAN82_AUDIT_ISSUE {issue}")
    raise RuntimeError(f"Plan82 audit failed with {len(issues)} issue(s)")

unreal.log(
    f"PLAN82_AUDIT_RESULT profiles={profile_count} states={len(states)} "
    f"imported_sources={len(imported)} issues=0"
)
