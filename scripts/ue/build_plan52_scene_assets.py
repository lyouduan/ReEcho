"""Build Plan52 map materials, scene profiles, and editable Arena Blueprints.

Run with UnrealEditor-Cmd and -ExecutePythonScript. This script is intentionally
non-destructive: it never deletes legacy assets or changes Level00.
"""

from pathlib import Path

import unreal


MAP_ROOT = "/Game/ReEcho/Art/Scene/Map"
MATERIAL_ROOT = f"{MAP_ROOT}/Materials"
PROFILE_ROOT = "/Game/ReEcho/Scene/Profiles"
PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
MASTER_MATERIAL_PATH = f"{MATERIAL_ROOT}/M_ArenaGround"
BASE_BLUEPRINT_PATH = f"{PREFAB_ROOT}/BP_ArenaScene"
SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")
PROFILE_TUNING = {
    "SC01": {"brightness": 1.0, "saturation": 0.95, "contrast": 1.0, "density": 0.28,
             "ground": (8, 16), "mid": (2, 11, 12, 15), "foreground": (5, 9, 10)},
    "SC02": {"brightness": 1.05, "saturation": 0.9, "contrast": 0.95, "density": 0.12,
             "ground": (13, 14), "mid": (4, 17), "foreground": ()},
    "SC03": {"brightness": 1.08, "saturation": 0.8, "contrast": 0.9, "density": 0.18,
             "ground": (13, 14), "mid": (4, 17), "foreground": (16,)},
    "SC04": {"brightness": 1.12, "saturation": 0.9, "contrast": 1.0, "density": 0.24,
             "ground": (8, 13), "mid": (2, 11, 12, 15), "foreground": (5, 9, 10)},
}


def fail(message):
    raise RuntimeError(f"[Plan52] {message}")


def load_required(path, expected_type=None):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        fail(f"Missing required asset: {path}")
    if expected_type is not None and not isinstance(asset, expected_type):
        fail(f"Unexpected asset type at {path}: {asset.get_class().get_name()}")
    return asset


def ensure_texture(scene_id):
    asset_name = scene_id.lower()
    asset_path = f"{MAP_ROOT}/{asset_name}"
    source = Path(unreal.Paths.project_content_dir()) / "ReEcho" / "Art" / "Scene" / "Map" / f"{asset_name}.png"
    if not source.is_file():
        fail(f"Missing map source: {source}")

    # The PNG files are the authoritative art sources. Always reimport them so an
    # existing Texture2D cannot silently retain pixels from an older source file.
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = MAP_ROOT
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        fail(f"Map is not a Texture2D: {asset_path}")
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)


def ensure_master_material():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = unreal.EditorAssetLibrary.load_asset(MASTER_MATERIAL_PATH)
    if material is None:
        material = asset_tools.create_asset(
            "M_ArenaGround", MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(material, unreal.Material):
        fail(f"Could not create master material: {MASTER_MATERIAL_PATH}")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    texture = expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -1100, 0)
    texture.set_editor_property("parameter_name", "MapTexture")
    tint = expression(material, unreal.MaterialExpressionVectorParameter, -1100, 260)
    tint.set_editor_property("parameter_name", "GroundTint")
    tint.set_editor_property("default_value", unreal.LinearColor.WHITE)
    brightness = expression(material, unreal.MaterialExpressionScalarParameter, -1100, 440)
    brightness.set_editor_property("parameter_name", "GroundBrightness")
    brightness.set_editor_property("default_value", 1.0)
    saturation = expression(material, unreal.MaterialExpressionScalarParameter, -1100, 600)
    saturation.set_editor_property("parameter_name", "GroundSaturation")
    saturation.set_editor_property("default_value", 1.0)
    contrast = expression(material, unreal.MaterialExpressionScalarParameter, -1100, 760)
    contrast.set_editor_property("parameter_name", "GroundContrast")
    contrast.set_editor_property("default_value", 1.0)

    one_minus = expression(material, unreal.MaterialExpressionOneMinus, -850, 600)
    desaturation = expression(material, unreal.MaterialExpressionDesaturation, -600, 0)
    subtract_half = expression(material, unreal.MaterialExpressionAdd, -380, 0)
    subtract_half.set_editor_property("const_b", -0.5)
    apply_contrast = expression(material, unreal.MaterialExpressionMultiply, -160, 0)
    restore_half = expression(material, unreal.MaterialExpressionAdd, 60, 0)
    restore_half.set_editor_property("const_b", 0.5)
    apply_tint = expression(material, unreal.MaterialExpressionMultiply, 280, 0)
    apply_brightness = expression(material, unreal.MaterialExpressionMultiply, 500, 0)

    unreal.MaterialEditingLibrary.connect_material_expressions(saturation, "", one_minus, "")
    # Desaturation's primary color pin is unnamed in the editor connector API.
    # Passing the reflected property name ("Input") silently leaves it unbound.
    unreal.MaterialEditingLibrary.connect_material_expressions(texture, "RGB", desaturation, "")
    unreal.MaterialEditingLibrary.connect_material_expressions(one_minus, "", desaturation, "Fraction")
    unreal.MaterialEditingLibrary.connect_material_expressions(desaturation, "", subtract_half, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(subtract_half, "", apply_contrast, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(contrast, "", apply_contrast, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(apply_contrast, "", restore_half, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(restore_half, "", apply_tint, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(tint, "", apply_tint, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(apply_tint, "", apply_brightness, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(brightness, "", apply_brightness, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        apply_brightness, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def ensure_material_instance(scene_id, parent, texture):
    name = f"MI_{scene_id}"
    path = f"{MATERIAL_ROOT}/{name}"
    instance = unreal.EditorAssetLibrary.load_asset(path)
    if instance is None:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, MATERIAL_ROOT, unreal.MaterialInstanceConstant, factory
        )
    if not isinstance(instance, unreal.MaterialInstanceConstant):
        fail(f"Could not create material instance: {path}")
    instance.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        instance, "MapTexture", texture
    )
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    return instance


def ensure_profile(scene_id, material):
    name = f"DA_ArenaScene_{scene_id}"
    path = f"{PROFILE_ROOT}/{name}"
    profile = unreal.EditorAssetLibrary.load_asset(path)
    profile_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneProfile")
    if profile_class is None:
        fail("Native ReEchoArenaSceneProfile is unavailable; build the Editor target first")
    if profile is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", profile_class)
        profile = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, PROFILE_ROOT, profile_class, factory
        )
    if profile is None or profile.get_class() != profile_class:
        fail(f"Could not create scene profile: {path}")
    profile.set_editor_property("scene_id", scene_id)
    profile.set_editor_property("map_material", material)
    profile.set_editor_property("decoration_seed", 52000 + int(scene_id[-2:]))
    tuning = PROFILE_TUNING[scene_id]
    profile.set_editor_property("ground_brightness", tuning["brightness"])
    profile.set_editor_property("ground_saturation", tuning["saturation"])
    profile.set_editor_property("ground_contrast", tuning["contrast"])
    profile.set_editor_property("decoration_density", tuning["density"])
    profile.set_editor_property("center_safe_zone_ratio", unreal.Vector2D(0.6, 0.6))
    profile.set_editor_property("maximum_landmarks", 1)
    for property_name, key in (
        ("ground_detail_palette", "ground"),
        ("mid_decoration_palette", "mid"),
        ("foreground_palette", "foreground"),
    ):
        palette = [
            load_required(f"/Game/ReEcho/Art/Scene/Plants/Materials/MI_Plant_{number:02d}")
            for number in tuning[key]
        ]
        profile.set_editor_property(property_name, palette)
    unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False)
    return profile


def create_blueprint(name, parent_class):
    path = f"{PREFAB_ROOT}/{name}"
    blueprint = unreal.EditorAssetLibrary.load_asset(path)
    if blueprint is None:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, PREFAB_ROOT, unreal.Blueprint, factory
        )
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Could not create Blueprint: {path}")
    return blueprint


def ensure_blueprints(profiles):
    arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
    if arena_class is None:
        fail("Native ReEchoArenaSceneActor is unavailable")
    base = create_blueprint("BP_ArenaScene", arena_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(base)
    unreal.EditorAssetLibrary.save_loaded_asset(base, only_if_is_dirty=False)
    base_class = base.generated_class()
    for scene_id, profile in profiles.items():
        child = create_blueprint(f"BP_ArenaScene_{scene_id}", base_class)
        child_cdo = unreal.get_default_object(child.generated_class())
        child_cdo.set_editor_property("scene_profile", profile)
        unreal.BlueprintEditorLibrary.compile_blueprint(child)
        unreal.EditorAssetLibrary.save_loaded_asset(child, only_if_is_dirty=False)


unreal.EditorAssetLibrary.make_directory(MATERIAL_ROOT)
unreal.EditorAssetLibrary.make_directory(PROFILE_ROOT)
unreal.EditorAssetLibrary.make_directory(PREFAB_ROOT)
master = ensure_master_material()
profiles = {}
for scene_id in SCENE_IDS:
    texture = ensure_texture(scene_id)
    material = ensure_material_instance(scene_id, master, texture)
    profiles[scene_id] = ensure_profile(scene_id, material)
ensure_blueprints(profiles)
unreal.log("[Plan52] Built four map materials, scene profiles, and editable Arena Blueprints")
