import unreal


FOX_WALK = "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/Walk"
FOX_WALK_FIRST_SPRITE = "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Walk/Sprites/01_Sprite"


flipbook = unreal.EditorAssetLibrary.load_asset(FOX_WALK)
sprite = unreal.EditorAssetLibrary.load_asset(FOX_WALK_FIRST_SPRITE)
if not isinstance(flipbook, unreal.PaperFlipbook):
    raise RuntimeError(f"Missing Fox Walk Flipbook: {FOX_WALK}")
if not isinstance(sprite, unreal.PaperSprite):
    raise RuntimeError(f"Missing Fox Walk frame-01 Sprite: {FOX_WALK_FIRST_SPRITE}")

key_frames = list(flipbook.get_editor_property("key_frames"))
if len(key_frames) != 12:
    raise RuntimeError(f"Fox Walk must contain 12 key frames, found {len(key_frames)}")
key_frames[0].set_editor_property("sprite", sprite)
flipbook.set_editor_property("key_frames", key_frames)

if not unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Fox Walk Flipbook: {FOX_WALK}")

saved_frames = flipbook.get_editor_property("key_frames")
saved_first = saved_frames[0].get_editor_property("sprite")
if saved_first != sprite:
    raise RuntimeError("Fox Walk frame-01 reference did not persist")

unreal.log(f"Fixed {FOX_WALK} frame 0 -> {FOX_WALK_FIRST_SPRITE}")
