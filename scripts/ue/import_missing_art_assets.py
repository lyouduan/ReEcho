"""Import missing source images under /Game/ReEcho/Art and build Paper2D assets."""

from pathlib import Path
import re
import subprocess

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
ART_ROOT = PROJECT_ROOT / "Content" / "ReEcho" / "Art"
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".exr"}
ANIMATION_MARKER = "/Animation2D/"


def deleted_assets():
    result = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=D", "--", "Content/ReEcho/Art"],
        cwd=PROJECT_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return {line.strip().replace("\\", "/") for line in result.stdout.splitlines() if line.strip()}


def content_relative(path):
    return path.relative_to(PROJECT_ROOT / "Content").as_posix()


def sanitize_asset_name(name):
    return re.sub(r"[^0-9A-Za-z_\u0080-\uffff]", "_", name)


def asset_path_for_source(path):
    relative = path.relative_to(PROJECT_ROOT / "Content").with_suffix("")
    parts = list(relative.parts)
    parts[-1] = sanitize_asset_name(parts[-1])
    return "/Game/" + "/".join(parts)


def import_texture(source_path, deleted):
    asset_path = asset_path_for_source(source_path)
    disk_asset = "Content/" + content_relative(source_path.with_suffix(".uasset"))
    if disk_asset in deleted:
        unreal.log_warning(f"Skipping explicitly deleted asset: {disk_asset}")
        return None

    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture:
        return texture

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_path))
    task.set_editor_property("destination_path", asset_path.rsplit("/", 1)[0])
    task.set_editor_property("destination_name", sanitize_asset_name(source_path.stem))
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Texture import failed: {source_path} -> {asset_path}")

    texture.set_editor_property("srgb", True)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def sprite_path_for_texture(texture_path):
    texture_dir, texture_name = texture_path.rsplit("/", 1)
    if not texture_dir.endswith("/Textures"):
        return None
    return texture_dir[:-9] + f"/Sprites/{texture_name}_Sprite"


def create_sprite(texture):
    texture_path = texture.get_path_name().split(".", 1)[0]
    sprite_path = sprite_path_for_texture(texture_path)
    if not sprite_path:
        return None

    sprite = unreal.EditorAssetLibrary.load_asset(sprite_path)
    if not sprite:
        asset_name = sprite_path.rsplit("/", 1)[1]
        asset_dir = sprite_path.rsplit("/", 1)[0]
        unreal.EditorAssetLibrary.make_directory(asset_dir)
        sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, asset_dir, unreal.PaperSprite, unreal.PaperSpriteFactory()
        )
    if not isinstance(sprite, unreal.PaperSprite):
        raise RuntimeError(f"PaperSprite creation failed: {sprite_path}")

    width = texture.blueprint_get_size_x()
    height = texture.blueprint_get_size_y()
    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("source_uv", unreal.Vector2D(0.0, 0.0))
    sprite.set_editor_property("source_dimension", unreal.Vector2D(width, height))
    sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CENTER_CENTER)
    if not unreal.ReEcho2DAnimationComponent.rebuild_sprite_asset(sprite):
        raise RuntimeError(f"PaperSprite render-data rebuild failed: {sprite_path}")
    unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False)
    return sprite


def natural_key(path):
    return [int(piece) if piece.isdigit() else piece.lower() for piece in re.split(r"(\d+)", path.name)]


def sequence_group(source_path):
    stem = source_path.stem
    match = re.match(r"(.+?)_(\d{5})$", stem)
    if match:
        return re.sub(r"\s+", "_", match.group(1))
    return source_path.parents[1].name


def flipbook_path_for_sources(sources, group_name):
    sequence_root = sources[0].parents[1]
    character_root = sequence_root.parent
    return "/Game/" + str(
        (character_root / "Flipbooks" / group_name).relative_to(PROJECT_ROOT / "Content")
    ).replace("\\", "/")


def build_flipbook(sources, sprites, group_name):
    flipbook_path = flipbook_path_for_sources(sources, group_name)
    flipbook = unreal.EditorAssetLibrary.load_asset(flipbook_path)
    should_rebuild = not flipbook or "/Enemies/Slime/Flipbooks/Default" in flipbook_path
    if not flipbook:
        asset_name = flipbook_path.rsplit("/", 1)[1]
        asset_dir = flipbook_path.rsplit("/", 1)[0]
        unreal.EditorAssetLibrary.make_directory(asset_dir)
        flipbook = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, asset_dir, unreal.PaperFlipbook, unreal.PaperFlipbookFactory()
        )
    if not isinstance(flipbook, unreal.PaperFlipbook):
        raise RuntimeError(f"PaperFlipbook creation failed: {flipbook_path}")
    if not should_rebuild:
        return flipbook

    keyed = sorted(zip(sources, sprites), key=lambda pair: natural_key(pair[0]))
    key_frames = []
    for _, sprite in keyed:
        key_frame = unreal.PaperFlipbookKeyFrame()
        key_frame.set_editor_property("sprite", sprite)
        key_frame.set_editor_property("frame_run", 1)
        key_frames.append(key_frame)
    flipbook.set_editor_property("key_frames", key_frames)
    flipbook.set_editor_property("frames_per_second", 12.0)
    unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=False)
    return flipbook


def main():
    deleted = deleted_assets()
    sources = sorted(
        path for path in ART_ROOT.rglob("*") if path.is_file() and path.suffix.lower() in IMAGE_EXTENSIONS
    )
    imported = 0
    skipped_deleted = 0
    animation_groups = {}

    for source in sources:
        disk_asset = "Content/" + content_relative(source.with_suffix(".uasset"))
        if disk_asset in deleted:
            skipped_deleted += 1
            continue
        existed = unreal.EditorAssetLibrary.does_asset_exist(asset_path_for_source(source))
        texture = import_texture(source, deleted)
        if not texture:
            continue
        if not existed:
            imported += 1

        relative = "/" + content_relative(source)
        if ANIMATION_MARKER not in relative or source.parent.name != "Textures":
            continue
        sprite = create_sprite(texture)
        group = sequence_group(source)
        key = (source.parents[1], group)
        animation_groups.setdefault(key, []).append((source, sprite))

    flipbooks = 0
    for (_, group), pairs in animation_groups.items():
        group_sources = [pair[0] for pair in pairs]
        group_sprites = [pair[1] for pair in pairs]
        build_flipbook(group_sources, group_sprites, group)
        flipbooks += 1

    unreal.log(
        f"Art import complete: sources={len(sources)} imported_textures={imported} "
        f"animation_groups={flipbooks} explicitly_deleted_skipped={skipped_deleted}"
    )


main()
