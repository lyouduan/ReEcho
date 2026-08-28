"""Import the reviewed Plan147 echo-storage popup textures."""

from pathlib import Path

import unreal


SOURCE_DIR = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InventoryShop"
    / "EchoStorage"
)
DESTINATION_DIR = "/Game/ReEcho/Textures/UI/InventoryShop/EchoStorage"
TEXTURES = {
    "PopupFrame.png": "T_UI_EchoStorage_PopupFrame",
    "ButtonLight.png": "T_UI_EchoStorage_ButtonLight",
    "ButtonDark.png": "T_UI_EchoStorage_ButtonDark",
}


def import_texture(source_name: str, asset_name: str) -> unreal.Texture2D:
    source = SOURCE_DIR / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing Plan147 source texture: {source}")

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION_DIR
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = f"{DESTINATION_DIR}/{asset_name}"
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Failed to import Plan147 texture: {asset_path}")
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        texture, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Failed to save Plan147 texture: {asset_path}")
    return texture


for source_name, asset_name in TEXTURES.items():
    import_texture(source_name, asset_name)

unreal.log(f"[Plan147EchoImport] imported {len(TEXTURES)} formal UI textures")
