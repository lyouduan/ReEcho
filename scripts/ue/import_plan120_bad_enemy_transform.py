"""Import and bind Plan120 Bad enemy transform Paper2D animations.

This script is idempotent. It copies only the user-approved source PNG frames,
authors Texture2D/PaperSprite/PaperFlipbook assets through Unreal APIs, and
updates only Animation.Transform.Phase2 in each profile's Phase2 AnimationSet.
"""

from pathlib import Path
import shutil

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
CONTENT_ROOT = PROJECT_ROOT / "Content"
ANIMATION_ROOT = "/Game/ReEcho/Art/Animation2D/Enemies"
PROFILE_ROOT = "/Game/ReEcho/DataAsset/Enemy/Profiles"
TRANSFORM_TAG_NAME = "Animation.Transform.Phase2"
SPECS = (
    ("BadRabbit", "DA_Enemy_RabbitDoll", Path(r"F:\MiniGame\兔子变形序列帧"), 7),
    ("BadSlime", "DA_Enemy_Slime", Path(r"F:\MiniGame\史莱姆变形"), 5),
    ("BadFox", "DA_Enemy_Fox", Path(r"F:\MiniGame\狐狸变形关键帧"), 4),
)


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


def normalized_set_id(animation_set):
    value = str(animation_set.get_editor_property("weapon_visual_set_id"))
    return "" if value in ("None", "") else value


def make_clip(flipbook):
    clip = unreal.ReEcho2DAnimationClip()
    clip.set_editor_property("flipbook", flipbook)
    clip.set_editor_property("looping", False)
    clip.set_editor_property("restart_on_request", True)
    clip.set_editor_property("play_rate", 1.0)
    clip.set_editor_property("use_native_scale", False)
    clip.set_editor_property("mirror_horizontally", False)
    clip.set_editor_property("local_offset", unreal.Vector(0.0, 0.0, 0.0))
    clip.set_editor_property("translucent_sort_priority", 10)
    return clip


def validate_sources(source_dir, frame_count):
    expected = [source_dir / f"{index}.png" for index in range(1, frame_count + 1)]
    actual = sorted(source_dir.glob("*.png"), key=lambda path: int(path.stem))
    if actual != expected:
        raise RuntimeError(f"Unexpected source frame set: {source_dir} actual={actual}")
    for source in expected:
        if not source.is_file() or source.stat().st_size <= 0:
            raise RuntimeError(f"Missing or empty source frame: {source}")
    return expected


def copy_and_import_textures(monster, sources):
    disk_dir = (
        CONTENT_ROOT
        / "ReEcho"
        / "Art"
        / "Animation2D"
        / "Enemies"
        / monster
        / "Transform"
        / "Textures"
    )
    disk_dir.mkdir(parents=True, exist_ok=True)
    destination_path = f"{ANIMATION_ROOT}/{monster}/Transform/Textures"
    tasks = []
    for index, source in enumerate(sources, start=1):
        destination = disk_dir / f"Transform_{index:02d}.png"
        if not destination.exists() or destination.read_bytes() != source.read_bytes():
            shutil.copy2(source, destination)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(destination))
        task.set_editor_property("destination_path", destination_path)
        task.set_editor_property("destination_name", f"Transform_{index:02d}")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    textures = []
    for index in range(1, len(sources) + 1):
        texture = required(f"{destination_path}/Transform_{index:02d}", unreal.Texture2D)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError(f"Failed to save texture: {texture.get_path_name()}")
        textures.append(texture)
    return textures


def create_or_update_sprites(monster, textures):
    sprite_dir = f"{ANIMATION_ROOT}/{monster}/Transform/Sprites"
    unreal.EditorAssetLibrary.make_directory(sprite_dir)
    sprites = []
    for index, texture in enumerate(textures, start=1):
        name = f"Transform_{index:02d}_Sprite"
        path = f"{sprite_dir}/{name}"
        sprite = unreal.EditorAssetLibrary.load_asset(path)
        if sprite is None:
            sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                name, sprite_dir, unreal.PaperSprite, unreal.PaperSpriteFactory()
            )
        if not isinstance(sprite, unreal.PaperSprite):
            raise RuntimeError(f"Could not create PaperSprite: {path}")
        sprite.set_editor_property("source_texture", texture)
        sprite.set_editor_property("source_uv", unreal.Vector2D(0.0, 0.0))
        sprite.set_editor_property(
            "source_dimension",
            unreal.Vector2D(texture.blueprint_get_size_x(), texture.blueprint_get_size_y()),
        )
        sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CENTER_CENTER)
        sprite.set_editor_property("pixels_per_unreal_unit", 1.0)
        if not unreal.ReEcho2DAnimationComponent.rebuild_sprite_asset(sprite):
            raise RuntimeError(f"PaperSprite rebuild failed: {path}")
        if not unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False):
            raise RuntimeError(f"Failed to save sprite: {path}")
        sprites.append(sprite)
    return sprites


def create_or_update_flipbook(monster, sprites, frames_per_second):
    flipbook_dir = f"{ANIMATION_ROOT}/{monster}/Flipbooks"
    flipbook_path = f"{flipbook_dir}/Transform"
    flipbook = unreal.EditorAssetLibrary.load_asset(flipbook_path)
    if flipbook is None:
        flipbook = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "Transform", flipbook_dir, unreal.PaperFlipbook, unreal.PaperFlipbookFactory()
        )
    if not isinstance(flipbook, unreal.PaperFlipbook):
        raise RuntimeError(f"Could not create PaperFlipbook: {flipbook_path}")
    key_frames = []
    for sprite in sprites:
        key_frame = unreal.PaperFlipbookKeyFrame()
        key_frame.set_editor_property("sprite", sprite)
        key_frame.set_editor_property("frame_run", 1)
        key_frames.append(key_frame)
    flipbook.set_editor_property("frames_per_second", float(frames_per_second))
    flipbook.set_editor_property("key_frames", key_frames)
    if not unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save flipbook: {flipbook_path}")
    return flipbook


def bind_profile(profile_name, flipbook):
    profile = required(
        f"{PROFILE_ROOT}/{profile_name}", unreal.ReEcho2DCharacterPresentationProfile
    )
    animation_sets = list(profile.get_editor_property("animation_sets"))
    phase2_indices = [
        index
        for index, animation_set in enumerate(animation_sets)
        if normalized_set_id(animation_set) == "Phase2"
    ]
    if len(phase2_indices) != 1:
        raise RuntimeError(
            f"Expected one Phase2 AnimationSet: {profile.get_path_name()} count={len(phase2_indices)}"
        )
    phase2_index = phase2_indices[0]
    phase2_set = animation_sets[phase2_index]
    clips = phase2_set.get_editor_property("clips")
    clips[semantic(TRANSFORM_TAG_NAME)] = make_clip(flipbook)
    phase2_set.set_editor_property("clips", clips)
    animation_sets[phase2_index] = phase2_set
    profile.set_editor_property("animation_sets", animation_sets)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save profile: {profile.get_path_name()}")


def main():
    results = []
    for monster, profile_name, source_dir, frame_count in SPECS:
        sources = validate_sources(source_dir, frame_count)
        textures = copy_and_import_textures(monster, sources)
        sprites = create_or_update_sprites(monster, textures)
        flipbook = create_or_update_flipbook(monster, sprites, frame_count)
        bind_profile(profile_name, flipbook)
        results.append((monster, frame_count, frame_count, flipbook.get_path_name()))
    unreal.log(f"PLAN120_IMPORT_RESULT imported={results} profiles=3")


main()
