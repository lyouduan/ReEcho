"""Reimport Fox Born/Walk textures and apply the Plan144 Paper2D sampling contract."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
ASSET_ROOT = "/Game/ReEcho/Art/Animation2D/Enemies/Fox"
FRAME_SETS = (("Born", tuple(str(index) for index in range(1, 6))),
              ("Walk", tuple(f"2_{index:05d}" for index in range(24))))


def load_texture(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(asset, unreal.Texture2D):
        raise RuntimeError(f"Expected Texture2D: {asset_path}")
    return asset


configured = []
for animation, names in FRAME_SETS:
    for name in names:
        asset_path = f"{ASSET_ROOT}/{animation}/Textures/{name}"
        source_path = PROJECT_ROOT / "Content/ReEcho/Art/Animation2D/Enemies/Fox" / animation / "Textures" / f"{name}.png"
        if not source_path.is_file():
            raise RuntimeError(f"Missing source PNG: {source_path}")

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source_path))
        task.set_editor_property("destination_path", asset_path.rsplit("/", 1)[0])
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("replace_existing_settings", False)
        task.set_editor_property("save", False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

        texture = load_texture(asset_path)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
        texture.set_editor_property("compression_no_alpha", False)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("defer_compression", False)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError(f"Failed to save texture: {asset_path}")
        configured.append(asset_path)

unreal.log(f"FOX_2D_TEXTURE_CONFIG_OK textures={len(configured)}")
