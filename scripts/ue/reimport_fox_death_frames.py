"""Reimport the authored Fox and BadFox death frames without changing asset identities."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
FRAME_SETS = (
    (
        "Fox",
        "/Game/ReEcho/Art/Animation2D/Enemies/Fox",
        (3,),
        ((160.5, 697.0), (301.5, 396.0), (268.5, 456.0), (245.5, 297.0),
         (273.0, 280.0), (238.0, 270.0), (260.0, 307.0), (263.5, 299.0)),
    ),
    (
        "BadFox",
        "/Game/ReEcho/Art/Animation2D/Enemies/BadFox",
        (5, 7, 8),
        ((116.0, 562.0), (156.5, 513.0), (199.0, 464.0), (239.0, 346.0),
         (182.0, 237.0), (235.5, 266.0), (224.5, 221.0), (217.0, 232.0)),
    ),
)
FRAME_COUNT = 8


def required(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Expected {expected_type.__name__}: {path}")
    return asset


def reimport_texture(enemy_name, asset_root, frame_index):
    texture_path = f"{asset_root}/Death/Textures/d{frame_index}"
    source_file = (
        PROJECT_ROOT
        / "Content"
        / "ReEcho"
        / "Art"
        / "Animation2D"
        / "Enemies"
        / enemy_name
        / "Death"
        / "Textures"
        / f"d{frame_index}.png"
    )
    if not source_file.is_file():
        raise RuntimeError(f"Missing authored source frame: {source_file}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", texture_path.rsplit("/", 1)[0])
    task.set_editor_property("destination_name", texture_path.rsplit("/", 1)[1])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = required(texture_path, unreal.Texture2D)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save texture: {texture_path}")

    sprite_path = f"{texture_path}_Sprite"
    sprite = required(sprite_path, unreal.PaperSprite)
    if not unreal.ReEcho2DAnimationComponent.rebuild_sprite_asset(sprite):
        raise RuntimeError(f"Failed to rebuild sprite render data: {sprite_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save sprite: {sprite_path}")
    return sprite


def verify_flipbook(asset_root, expected_sprites):
    flipbook_path = f"{asset_root}/Flipbooks/Death"
    flipbook = required(flipbook_path, unreal.PaperFlipbook)
    key_frames = list(flipbook.get_editor_property("key_frames"))
    if len(key_frames) != FRAME_COUNT:
        raise RuntimeError(f"{flipbook_path} expected {FRAME_COUNT} keys, found {len(key_frames)}")
    for frame_index, (key_frame, expected_sprite) in enumerate(zip(key_frames, expected_sprites), start=1):
        actual_sprite = key_frame.get_editor_property("sprite")
        if actual_sprite != expected_sprite:
            raise RuntimeError(
                f"{flipbook_path} frame {frame_index} uses {actual_sprite}, expected {expected_sprite}"
            )
    unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=True)


def author_ground_pivot(sprite, pivot):
    sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CUSTOM)
    sprite.set_editor_property("custom_pivot_point", unreal.Vector2D(*pivot))
    if not unreal.ReEcho2DAnimationComponent.rebuild_sprite_asset(sprite):
        raise RuntimeError(f"Failed to rebuild custom-pivot sprite: {sprite.get_path_name()}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save custom-pivot sprite: {sprite.get_path_name()}")


for enemy_name, asset_root, changed_frames, ground_pivots in FRAME_SETS:
    for frame_index in changed_frames:
        reimport_texture(enemy_name, asset_root, frame_index)
    sprites = [
        required(f"{asset_root}/Death/Textures/d{frame_index}_Sprite", unreal.PaperSprite)
        for frame_index in range(1, FRAME_COUNT + 1)
    ]
    for sprite, pivot in zip(sprites, ground_pivots):
        author_ground_pivot(sprite, pivot)
    verify_flipbook(asset_root, sprites)
    unreal.log(
        f"FOX_DEATH_REIMPORT_OK enemy={enemy_name} changed={list(changed_frames)} "
        f"authored_ground_pivots={len(ground_pivots)} flipbook_frames={FRAME_COUNT}"
    )
