"""Read-only audit for the Plan144 Fox source, Texture and animation-reference contract."""

from pathlib import Path
import os

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
ASSET_ROOT = "/Game/ReEcho/Art/Animation2D/Enemies/Fox"
FRAME_SETS = (("Born", tuple(str(index) for index in range(1, 6))),
              ("Walk", tuple(f"2_{index:05d}" for index in range(24))))
EXPECTED_FLIPBOOK = {"Born": (5, 10.0), "Walk": (24, None)}
issues = []


def normalized(path):
    return os.path.normcase(os.path.normpath(str(Path(path).resolve()))) if path else ""


for animation, names in FRAME_SETS:
    expected_sprites = []
    for name in names:
        texture_path = f"{ASSET_ROOT}/{animation}/Textures/{name}"
        texture = unreal.EditorAssetLibrary.load_asset(texture_path)
        if not isinstance(texture, unreal.Texture2D):
            issues.append(f"missing Texture2D: {texture_path}")
            continue
        expected_source = PROJECT_ROOT / "Content/ReEcho/Art/Animation2D/Enemies/Fox" / animation / "Textures" / f"{name}.png"
        import_data = texture.get_editor_property("asset_import_data")
        actual_source = import_data.get_first_filename() if import_data else ""
        if normalized(actual_source) != normalized(expected_source):
            issues.append(f"wrong import source: {texture_path} -> {actual_source}")
        expected = (
            ("srgb", True),
            ("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
            ("address_x", unreal.TextureAddress.TA_CLAMP),
            ("address_y", unreal.TextureAddress.TA_CLAMP),
            ("filter", unreal.TextureFilter.TF_BILINEAR),
            ("compression_settings", unreal.TextureCompressionSettings.TC_BC7),
            ("compression_no_alpha", False),
            ("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI),
        )
        for property_name, expected_value in expected:
            actual_value = texture.get_editor_property(property_name)
            if actual_value != expected_value:
                issues.append(f"{texture_path} {property_name}={actual_value}, expected {expected_value}")
        sprite_path = f"{ASSET_ROOT}/{animation}/Sprites/{name}_Sprite"
        sprite = unreal.EditorAssetLibrary.load_asset(sprite_path)
        if not isinstance(sprite, unreal.PaperSprite):
            issues.append(f"missing PaperSprite: {sprite_path}")
        elif sprite.get_editor_property("source_texture") != texture:
            issues.append(f"Sprite source changed: {sprite_path}")
        expected_sprites.append(sprite)

    flipbook_path = f"{ASSET_ROOT}/Flipbooks/{animation}"
    flipbook = unreal.EditorAssetLibrary.load_asset(flipbook_path)
    if not isinstance(flipbook, unreal.PaperFlipbook):
        issues.append(f"missing PaperFlipbook: {flipbook_path}")
        continue
    expected_frames, expected_fps = EXPECTED_FLIPBOOK[animation]
    key_frames = list(flipbook.get_editor_property("key_frames"))
    actual_sprites = [key.get_editor_property("sprite") for key in key_frames]
    if flipbook.get_num_frames() != expected_frames or len(key_frames) != expected_frames:
        issues.append(f"{flipbook_path} frame count changed")
    if expected_fps is not None and abs(float(flipbook.get_editor_property("frames_per_second")) - expected_fps) > 0.0001:
        issues.append(f"{flipbook_path} FPS changed")
    if actual_sprites != expected_sprites:
        issues.append(f"{flipbook_path} Sprite order changed")

profile_path = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Fox"
profile = unreal.EditorAssetLibrary.load_asset(profile_path)
if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
    issues.append(f"missing Fox Profile: {profile_path}")
else:
    semantic_flipbooks = {}
    for animation_set in profile.get_editor_property("animation_sets"):
        for semantic, clip in animation_set.get_editor_property("clips").items():
            semantic_name = str(semantic.get_editor_property("tag_name"))
            semantic_flipbooks.setdefault(semantic_name, []).append(clip.get_editor_property("flipbook"))
    for semantic, animation in (("Animation.Born", "Born"), ("Animation.Move", "Walk")):
        expected = unreal.EditorAssetLibrary.load_asset(f"{ASSET_ROOT}/Flipbooks/{animation}")
        if expected not in semantic_flipbooks.get(semantic, []):
            issues.append(f"Fox Profile {semantic} binding changed")

if issues:
    for issue in issues:
        unreal.log_error(f"FOX_2D_TEXTURE_AUDIT_ISSUE {issue}")
    raise RuntimeError(f"Fox 2D texture audit failed with {len(issues)} issue(s)")

unreal.log("FOX_2D_TEXTURE_AUDIT_OK textures=29 sprites=29 flipbooks=2 profile=1")
