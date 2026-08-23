"""Apply the Plan82 sparse 2D-animation asset contract.

Run through UnrealEditor-Cmd with -ExecutePythonScript. The script only edits
the canonical FSM and production presentation profiles, and imports genuinely
missing Texture2D source files below Animation2D. It deliberately does not
invent sprites, Flipbooks, or semantic mappings for an absent animation.
"""

from pathlib import Path
import os
import re
import subprocess

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
CONTENT_ROOT = PROJECT_ROOT / "Content"
ANIMATION_ROOT = CONTENT_ROOT / "ReEcho" / "Art" / "Animation2D"
STATE_MACHINE_PATH = "/Game/ReEcho/DataAsset/Common/Animation2D/SM2D_DefaultCharacter"
LEGACY_STATE_MACHINE_PATH = "/Game/ReEcho/Animation2D/SM2D_DefaultCharacter"
PROFILE_ROOTS = (
    "/Game/ReEcho/DataAsset/Character/Profiles",
    "/Game/ReEcho/DataAsset/Enemy/Profiles",
)
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".exr"}


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


def make_state(name, priority, lock=False, terminal=False):
    value = semantic(name)
    state = unreal.ReEcho2DAnimationStateDefinition()
    state.set_editor_property("state_tag", value)
    state.set_editor_property("semantic_key", value)
    state.set_editor_property("interrupt_priority", priority)
    state.set_editor_property("lock_until_playback_complete", lock)
    state.set_editor_property("terminal", terminal)
    return state


def configure_contract():
    state_machine = required(STATE_MACHINE_PATH, unreal.ReEcho2DAnimationStateMachineAsset)
    state_machine.set_editor_property("initial_state_tag", semantic("Animation.Move"))
    state_machine.set_editor_property(
        "states",
        [
            make_state("Animation.Move", 10),
            make_state("Animation.Attack.Charge", 30, lock=True),
            make_state("Animation.Attack.Basic", 40, lock=True),
            make_state("Animation.Hit", 60, lock=True),
            make_state("Animation.Transform.Phase2", 80, lock=True),
            make_state("Animation.Death", 100, lock=True, terminal=True),
        ],
    )
    unreal.EditorAssetLibrary.save_loaded_asset(state_machine, only_if_is_dirty=False)

    changed = []
    for root in PROFILE_ROOTS:
        for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
            profile = unreal.EditorAssetLibrary.load_asset(path)
            if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
                continue
            profile.set_editor_property("state_machine", state_machine)
            animation_sets = list(profile.get_editor_property("animation_sets"))
            for index, animation_set in enumerate(animation_sets):
                clips = animation_set.get_editor_property("clips")
                for key, clip in list(clips.items()):
                    if str(key.get_editor_property("tag_name")) == "Animation.Idle":
                        clips.pop(key, None)
                        continue
                    if not isinstance(clip.get_editor_property("flipbook"), unreal.PaperFlipbook):
                        clips.pop(key, None)
                animation_set.set_editor_property("clips", clips)
                animation_sets[index] = animation_set
            profile.set_editor_property("animation_sets", animation_sets)
            unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False)
            changed.append(profile.get_path_name())
    if unreal.EditorAssetLibrary.does_asset_exist(LEGACY_STATE_MACHINE_PATH):
        if not unreal.EditorAssetLibrary.delete_asset(LEGACY_STATE_MACHINE_PATH):
            raise RuntimeError(f"Failed to delete duplicate FSM: {LEGACY_STATE_MACHINE_PATH}")
    return changed


def normalized_filename(value):
    if not value:
        return None
    return os.path.normcase(os.path.normpath(str(Path(value).resolve())))


def imported_sources():
    result = {}
    for path in unreal.EditorAssetLibrary.list_assets(
        "/Game/ReEcho/Art/Animation2D", recursive=True, include_folder=False
    ):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(asset, unreal.Texture2D):
            continue
        import_data = asset.get_editor_property("asset_import_data")
        filename = import_data.get_first_filename() if import_data else ""
        normalized = normalized_filename(filename)
        if normalized:
            result[normalized] = asset.get_path_name()
    return result


def deleted_uassets():
    result = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=D", "--", "Content/ReEcho/Art/Animation2D"],
        cwd=PROJECT_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return {line.strip().replace("\\", "/") for line in result.stdout.splitlines() if line.strip()}


def asset_path_for_source(source):
    relative = source.relative_to(CONTENT_ROOT).with_suffix("")
    clean_name = re.sub(r"[^0-9A-Za-z_\u0080-\uffff]", "_", relative.name)
    return "/Game/" + "/".join((*relative.parts[:-1], clean_name))


def import_missing_textures():
    source_map = imported_sources()
    deleted = deleted_uassets()
    tasks = []
    skipped_deleted = []
    for source in sorted(ANIMATION_ROOT.rglob("*")):
        if not source.is_file() or source.suffix.lower() not in IMAGE_EXTENSIONS:
            continue
        normalized = normalized_filename(source)
        if normalized in source_map:
            continue
        asset_path = asset_path_for_source(source)
        disk_asset = "Content/" + source.relative_to(CONTENT_ROOT).with_suffix(".uasset").as_posix()
        if disk_asset in deleted:
            skipped_deleted.append(disk_asset)
            continue
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", asset_path.rsplit("/", 1)[0])
        task.set_editor_property("destination_name", asset_path.rsplit("/", 1)[1])
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)
        tasks.append(task)

    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    imported = []
    for task in tasks:
        for object_path in task.get_editor_property("imported_object_paths"):
            texture = unreal.EditorAssetLibrary.load_asset(str(object_path))
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError(f"Texture import failed: {object_path}")
            texture.set_editor_property("srgb", True)
            texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
            texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
            unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
            imported.append(texture.get_path_name())
    return imported, skipped_deleted


profiles = configure_contract()
textures, skipped = import_missing_textures()
unreal.log(
    f"PLAN82_ORGANIZE_RESULT profiles={len(profiles)} imported_textures={len(textures)} "
    f"explicitly_deleted_skipped={len(skipped)}"
)
