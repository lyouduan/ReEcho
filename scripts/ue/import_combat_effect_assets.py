import unreal


SOURCE_DIR = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_content_dir() + "SourceArt/Effects"
)
DESTINATION = "/Game/ReEcho/Textures/Effects"

ASSETS = {
    "StaffLightWave.png": "StaffLightWave",
    "MoonStaff.png": "MoonStaff",
    "CrescentWeapon.png": "CrescentWeapon",
    "SlashCrescent.png": "SlashCrescent",
}


def import_texture(filename: str, asset_name: str) -> None:
    task = unreal.AssetImportTask()
    task.filename = SOURCE_DIR + "/" + filename
    task.destination_path = DESTINATION
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(f"{DESTINATION}/{asset_name}")
    if not texture:
        raise RuntimeError(f"Failed to import {filename}")

    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


for source_filename, destination_name in ASSETS.items():
    import_texture(source_filename, destination_name)

unreal.log("Imported ReEcho crescent weapon and slash trail textures.")
