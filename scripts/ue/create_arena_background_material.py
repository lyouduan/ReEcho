import unreal

TEXTURE_PATH = "/Game/ReEcho/Textures/Scenes/ArenaGround3D"
MATERIAL_PATH = "/Game/ReEcho/Materials"
MATERIAL_NAME = "M_ArenaBackground"

texture = unreal.load_asset(TEXTURE_PATH)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Missing arena Texture2D: {TEXTURE_PATH}")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset(f"{MATERIAL_PATH}/{MATERIAL_NAME}")
if material is None:
    material = asset_tools.create_asset(
        MATERIAL_NAME, MATERIAL_PATH, unreal.Material, unreal.MaterialFactoryNew()
    )
if not isinstance(material, unreal.Material):
    raise RuntimeError(f"Could not create arena material: {MATERIAL_PATH}/{MATERIAL_NAME}")

material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
sample = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionTextureSample, -300, 0
)
sample.set_editor_property("texture", texture)
unreal.MaterialEditingLibrary.connect_material_property(
    sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
unreal.log(f"Created arena background material: {material.get_path_name()}")
