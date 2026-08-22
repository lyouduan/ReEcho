"""Persist the human-approved Plan40 Paper2D collision mode through Unreal's asset API.

Run with UnrealEditor-Cmd/UnrealEditor and -ExecutePythonScript. This script changes only
the explicitly listed production Flipbooks; it never generates or guesses per-frame Sprite geometry.
"""

import unreal


FLIPBOOK_PATHS = (
    "/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Walk",
    "/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack",
    "/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default",
    "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Default",
    "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0",
    "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk1",
    "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Attack0",
    "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Attack1",
    "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Default",
    "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/Walk",
    "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/gfa",
    "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/gfar",
)


def main() -> None:
    paths = list(FLIPBOOK_PATHS)
    for path in dict.fromkeys(paths):
        flipbook = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(flipbook, unreal.PaperFlipbook):
            raise RuntimeError(f"Plan40 Flipbook is missing: {path}")
        key_frames = flipbook.get_editor_property("key_frames")
        if not key_frames:
            raise RuntimeError(f"Plan40 Flipbook has no key frames: {path}")
        missing_sprites = [
            index
            for index, frame in enumerate(key_frames)
            if frame.get_editor_property("sprite") is None
        ]
        if missing_sprites:
            raise RuntimeError(f"Plan40 Flipbook has empty Sprite frames {missing_sprites}: {path}")
        flipbook.set_editor_property(
            "collision_source", unreal.FlipbookCollisionMode.EACH_FRAME_COLLISION
        )
        if not unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=False):
            raise RuntimeError(f"Failed to save Plan40 Flipbook: {path}")
        unreal.log(f"Plan40 EachFrameCollision saved: {path} ({len(key_frames)} key frames)")


main()
