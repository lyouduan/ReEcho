"""Import and bind Plan153 Sheep Boss Phase3 Paper2D animations.

The script is idempotent. It copies only the approved source PNGs into the
project, authors Texture2D/PaperSprite/PaperFlipbook assets through Unreal
Editor APIs, moves the Phase3 AnimationSet from the unused GoatPriest profile
to the TimeGuard profile selected by the production sheep PresentationId.
"""

from pathlib import Path
import shutil

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
CONTENT_ROOT = PROJECT_ROOT / "Content"
ASSET_ROOT = "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Phase3"
PROFILE_PATH = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_TimeGuard"
LEGACY_PROFILE_PATH = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_GoatPriest"
CONFIG_PATH = "/Game/ReEcho/DataAsset/Enemy/DA_SheepBossPhase3"
PHASE3_SET_ID = "Phase3"
SPECS = (
    (
        "Walk",
        Path(r"F:\黑羊Boss_行走序列帧_8帧_完整"),
        "BlackSheepBoss_Walk",
        8.0,
    ),
    (
        "GroundSlam",
        Path(r"F:\黑羊Boss_技能_右手拍地_8帧"),
        "BlackSheepBoss_GroundSlam",
        16.0,
    ),
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


def validate_sources(source_dir, prefix):
    expected = tuple(source_dir / f"{prefix}_{index:02d}.png" for index in range(1, 9))
    actual = tuple(sorted(source_dir.glob("*.png"), key=lambda path: path.name.lower()))
    if actual != expected:
        raise RuntimeError(f"Unexpected source frame set: {source_dir} actual={actual}")
    for source in expected:
        if not source.is_file() or source.stat().st_size <= 0:
            raise RuntimeError(f"Missing or empty source frame: {source}")
    return expected


def copy_and_import_textures(animation_name, sources):
    disk_dir = (
        CONTENT_ROOT
        / "ReEcho"
        / "Art"
        / "Animation2D"
        / "Enemies"
        / "Goat"
        / "Phase3"
        / animation_name
        / "Textures"
    )
    disk_dir.mkdir(parents=True, exist_ok=True)
    destination_path = f"{ASSET_ROOT}/{animation_name}/Textures"
    tasks = []
    for index, source in enumerate(sources, start=1):
        destination = disk_dir / f"{animation_name}_{index:02d}.png"
        if not destination.exists() or destination.read_bytes() != source.read_bytes():
            shutil.copy2(source, destination)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(destination))
        task.set_editor_property("destination_path", destination_path)
        task.set_editor_property("destination_name", f"{animation_name}_{index:02d}")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    textures = []
    for index in range(1, 9):
        texture = required(f"{destination_path}/{animation_name}_{index:02d}", unreal.Texture2D)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
        texture.set_editor_property("compression_no_alpha", False)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError(f"Failed to save texture: {texture.get_path_name()}")
        textures.append(texture)
    return textures


def create_or_update_sprites(animation_name, textures):
    sprite_dir = f"{ASSET_ROOT}/{animation_name}/Sprites"
    unreal.EditorAssetLibrary.make_directory(sprite_dir)
    sprites = []
    for index, texture in enumerate(textures, start=1):
        name = f"{animation_name}_{index:02d}_Sprite"
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


def create_or_update_flipbook(animation_name, sprites, frames_per_second):
    flipbook_dir = f"{ASSET_ROOT}/{animation_name}"
    flipbook_path = f"{flipbook_dir}/{animation_name}"
    flipbook = unreal.EditorAssetLibrary.load_asset(flipbook_path)
    if flipbook is None:
        flipbook = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            animation_name,
            flipbook_dir,
            unreal.PaperFlipbook,
            unreal.PaperFlipbookFactory(),
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


def make_clip(flipbook, looping, restart_on_request):
    clip = unreal.ReEcho2DAnimationClip()
    clip.set_editor_property("flipbook", flipbook)
    clip.set_editor_property("looping", looping)
    clip.set_editor_property("restart_on_request", restart_on_request)
    clip.set_editor_property("play_rate", 1.0)
    clip.set_editor_property("use_native_scale", False)
    clip.set_editor_property("mirror_horizontally", False)
    clip.set_editor_property("local_offset", unreal.Vector(0.0, 0.0, 0.0))
    clip.set_editor_property("translucent_sort_priority", 10)
    return clip


def bind_profile(walk, ground_slam):
    legacy_profile = required(LEGACY_PROFILE_PATH, unreal.ReEcho2DCharacterPresentationProfile)
    legacy_sets = [
        animation_set
        for animation_set in legacy_profile.get_editor_property("animation_sets")
        if normalized_set_id(animation_set) != PHASE3_SET_ID
    ]
    legacy_profile.set_editor_property("animation_sets", legacy_sets)
    if not unreal.EditorAssetLibrary.save_loaded_asset(legacy_profile, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to remove stale Phase3 set from: {LEGACY_PROFILE_PATH}")

    profile = required(PROFILE_PATH, unreal.ReEcho2DCharacterPresentationProfile)
    sets = [
        animation_set
        for animation_set in profile.get_editor_property("animation_sets")
        if normalized_set_id(animation_set) != PHASE3_SET_ID
    ]
    phase3 = unreal.ReEcho2DCompositeAnimationSet()
    phase3.set_editor_property("weapon_visual_set_id", PHASE3_SET_ID)
    phase3.set_editor_property(
        "clips",
        {
            semantic("Animation.Move"): make_clip(walk, True, False),
            semantic("Animation.Attack.Charge"): make_clip(ground_slam, False, True),
            semantic("Animation.Attack.Basic"): make_clip(ground_slam, False, True),
        },
    )
    sets.append(phase3)
    profile.set_editor_property("animation_sets", sets)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save profile: {PROFILE_PATH}")


def create_or_update_phase3_config():
    config = unreal.EditorAssetLibrary.load_asset(CONFIG_PATH)
    if config is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.ReEchoBossPhase3Config)
        config = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_SheepBossPhase3", "/Game/ReEcho/DataAsset/Enemy", None, factory
        )
    if not isinstance(config, unreal.ReEchoBossPhase3Config):
        raise RuntimeError(f"Expected ReEchoBossPhase3Config: {CONFIG_PATH}")

    config.set_editor_property("boss_enemy_id", "M_SHEEP")
    config.set_editor_property("existing_ability_max_phase_index", 2)
    config.set_editor_property("trigger_seconds", 15.0)
    config.set_editor_property("phase_max_health", 500.0)
    config.set_editor_property("ability_id", "M_SHEEP_BlinkSlam")
    config.set_editor_property("damage", 24.0)
    config.set_editor_property("windup_seconds", 0.9)
    config.set_editor_property("active_seconds", 0.5)
    config.set_editor_property("recovery_seconds", 0.5)
    config.set_editor_property("cooldown_seconds", 5.0)
    config.set_editor_property("max_range_cm", 1000.0)
    config.set_editor_property("radius_cm", 180.0)
    config.set_editor_property("teleport_offset_cm", 180.0)
    config.set_editor_property("combo_min", 1)
    config.set_editor_property("combo_max", 3)
    if not unreal.EditorAssetLibrary.save_loaded_asset(config, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save config: {CONFIG_PATH}")


def main():
    flipbooks = {}
    for animation_name, source_dir, prefix, frames_per_second in SPECS:
        sources = validate_sources(source_dir, prefix)
        textures = copy_and_import_textures(animation_name, sources)
        sprites = create_or_update_sprites(animation_name, textures)
        flipbooks[animation_name] = create_or_update_flipbook(
            animation_name, sprites, frames_per_second
        )
    bind_profile(flipbooks["Walk"], flipbooks["GroundSlam"])
    create_or_update_phase3_config()
    unreal.log(
        "PLAN153_PHASE3_IMPORT_OK textures=16 sprites=16 flipbooks=2 "
        "walk_fps=8 walk_looping=true ground_slam_fps=16 ground_slam_looping=false "
        "phase3_clips=3 profile=1 config=1"
    )


main()
