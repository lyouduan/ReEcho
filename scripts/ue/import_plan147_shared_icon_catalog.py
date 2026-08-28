"""Import the character and element icons delivered with the Plan147 catalog."""

from pathlib import Path

import unreal


CONTENT_DIR = Path(unreal.Paths.project_content_dir())
SPECS = (
    ("IconCatalog/Elements/T_UI_Element_Flame.png", "/Game/ReEcho/Textures/UI/IconCatalog/Elements"),
    ("IconCatalog/Elements/T_UI_Element_Lightning.png", "/Game/ReEcho/Textures/UI/IconCatalog/Elements"),
    ("IconCatalog/Elements/T_UI_Element_Grass.png", "/Game/ReEcho/Textures/UI/IconCatalog/Elements"),
    ("IconCatalog/Elements/T_UI_Element_Water.png", "/Game/ReEcho/Textures/UI/IconCatalog/Elements"),
    ("IconCatalog/Characters/T_UI_CharacterIcon_J_DIAMOND.png", "/Game/ReEcho/Textures/UI/IconCatalog/Characters"),
    ("IconCatalog/Characters/T_UI_CharacterIcon_J_SPADE.png", "/Game/ReEcho/Textures/UI/IconCatalog/Characters"),
    ("IconCatalog/Characters/T_UI_CharacterIcon_J_HEART.png", "/Game/ReEcho/Textures/UI/IconCatalog/Characters"),
    ("IconCatalog/Characters/T_UI_CharacterIcon_J_CLOVER.png", "/Game/ReEcho/Textures/UI/IconCatalog/Characters"),
)


def configure(texture):
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


for relative_source, destination in SPECS:
    source = CONTENT_DIR / "SourceArt" / "UI" / relative_source
    if not source.is_file():
        raise RuntimeError(f"Missing shared icon source: {source}")
    task = unreal.AssetImportTask()
    task.automated = True
    task.filename = str(source)
    task.destination_path = destination
    task.destination_name = source.stem
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f"{destination}/{source.stem}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Failed to import shared icon: {source.stem}")
    configure(texture)

unreal.log(f"[Plan147SharedIconImport] imported={len(SPECS)}")
