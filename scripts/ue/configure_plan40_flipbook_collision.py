"""Persist the human-approved Plan40 Paper2D collision mode through Unreal's asset API.

Run with UnrealEditor-Cmd/UnrealEditor and -ExecutePythonScript. This script changes only
the three active animated Flipbooks; it never generates or guesses per-frame Sprite geometry.
"""

import unreal


FLIPBOOK_PATHS = (
    "/Game/2DAnim/Flipbook/walk",
    "/Game/2DAnim/Flipbook/attack",
    "/Game/2DAnim/Flipbook/Grount",
    "/Game/2DAnim/Flipbook/Goat",
    "/Game/2DAnim/Flipbook/Rabbit",
)


def main() -> None:
    paths = list(FLIPBOOK_PATHS)
    paths.extend(
        asset_path.split(".")[0]
        for asset_path in unreal.EditorAssetLibrary.list_assets(
            "/Game/2DAnim/Flipbook", recursive=True
        )
        if "fox" in asset_path.lower()
        and ("walk" in asset_path.lower() or "attack" in asset_path.lower())
    )
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
