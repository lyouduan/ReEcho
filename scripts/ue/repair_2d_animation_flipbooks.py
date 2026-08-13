"""Repair the imported player idle Flipbook through Unreal's asset API."""

import unreal


PLAYER_TEXTURE_ROOT = "/Game/ReEcho/Art/Animation2D/Players/Spade/Walk/Textures"
PLAYER_FLIPBOOK_PATH = "/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Idle_Legacy"
FRAME_COUNT = 12
FRAMES_PER_SECOND = 12.0


def load_required(path: str, expected_type):
    asset = unreal.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Expected {expected_type.__name__} at {path}, got {asset}")
    return asset


def create_or_update_sprite(index: int):
    texture_path = f"{PLAYER_TEXTURE_ROOT}/Idel_{index:02d}"
    sprite_path = f"{texture_path}_Sprite"
    texture = load_required(texture_path, unreal.Texture2D)
    sprite = unreal.load_asset(sprite_path)
    if sprite is None:
        factory = unreal.PaperSpriteFactory()
        sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            f"Idel_{index:02d}_Sprite",
            PLAYER_TEXTURE_ROOT,
            unreal.PaperSprite,
            factory,
        )
    if not isinstance(sprite, unreal.PaperSprite):
        raise RuntimeError(f"Could not create PaperSprite at {sprite_path}")

    width = texture.blueprint_get_size_x()
    height = texture.blueprint_get_size_y()
    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("source_uv", unreal.Vector2D(0.0, 0.0))
    sprite.set_editor_property("source_dimension", unreal.Vector2D(width, height))
    sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CENTER_CENTER)
    if not unreal.ReEcho2DAnimationComponent.rebuild_sprite_asset(sprite):
        raise RuntimeError(f"PaperSprite rebuild failed: {sprite_path}")
    unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False)
    return sprite


def main() -> None:
    sprites = [create_or_update_sprite(index) for index in range(1, FRAME_COUNT + 1)]
    flipbook = load_required(PLAYER_FLIPBOOK_PATH, unreal.PaperFlipbook)
    key_frames = []
    for sprite in sprites:
        key_frame = unreal.PaperFlipbookKeyFrame()
        key_frame.set_editor_property("sprite", sprite)
        key_frame.set_editor_property("frame_run", 1)
        key_frames.append(key_frame)
    flipbook.set_editor_property("frames_per_second", FRAMES_PER_SECOND)
    flipbook.set_editor_property("key_frames", key_frames)
    unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=False)
    unreal.log(f"Repaired {PLAYER_FLIPBOOK_PATH} with {len(sprites)} frames")


main()
