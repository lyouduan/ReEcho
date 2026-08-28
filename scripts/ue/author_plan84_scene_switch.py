"""Author Plan84 Arena registrations and SC01 direct editable edge components."""

from pathlib import Path

import unreal


ART_ROOT = "/Game/ReEcho/Art/Scene/SC01/EdgeInserts"
MATERIAL_ROOT = f"{ART_ROOT}/Materials"
PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
MASTER_MATERIAL_PATH = f"{MATERIAL_ROOT}/M_SC01EdgeInsert"
PLANE_PATH = "/Engine/BasicShapes/Plane.Plane"
SCENES = ("SC01", "SC02", "SC03", "SC04")
COLLISION_COMPONENTS = ("Floor", "WallNorth", "WallSouth", "WallEast", "WallWest")
INSERTS = (
    ("上叶子.png", "T_SC01Edge_TopLeaves", "MI_SC01Edge_TopLeaves", "Edge_TopLeaves_Mid", (4072, 917), (2380.0, 0.0, 35.0), (9.17, 40.72, 1.0), 55),
    ("下.png", "T_SC01Edge_BottomFrame", "MI_SC01Edge_BottomFrame", "Edge_BottomFrame_Foreground", (3423, 735), (-2450.0, 0.0, 75.0), (7.35, 34.23, 1.0), 90),
    ("下草1.png", "T_SC01Edge_BottomGrassLeft", "MI_SC01Edge_BottomGrassLeft", "Edge_BottomGrassLeft_Foreground", (831, 1043), (-2220.0, -3350.0, 85.0), (10.43, 8.31, 1.0), 92),
    ("下草2.png", "T_SC01Edge_BottomGrassRight", "MI_SC01Edge_BottomGrassRight", "Edge_BottomGrassRight_Foreground", (765, 980), (-2200.0, 3400.0, 85.0), (9.80, 7.65, 1.0), 93),
    ("中右.png", "T_SC01Edge_MidRight", "MI_SC01Edge_MidRight", "Edge_MidRight_Mid", (436, 820), (250.0, 4080.0, 45.0), (8.20, 4.36, 1.0), 58),
    ("中左插件.png", "T_SC01Edge_MidLeftInsert", "MI_SC01Edge_MidLeftInsert", "Edge_MidLeftInsert_Mid", (382, 862), (180.0, -4090.0, 45.0), (8.62, 3.82, 1.0), 57),
    ("中左大叶子.png", "T_SC01Edge_MidLeftLargeLeaves", "MI_SC01Edge_MidLeftLargeLeaves", "Edge_MidLeftLargeLeaves_Mid", (721, 658), (-520.0, -3900.0, 50.0), (6.58, 7.21, 1.0), 59),
)


def fail(message):
    raise RuntimeError(f"[Plan84] {message}")


def load_required(path, expected_type=None):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None or (expected_type is not None and not isinstance(asset, expected_type)):
        fail(f"Missing or invalid asset: {path}")
    return asset


def entries(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if obj is not None:
            name = str(obj.get_name()).removesuffix("_GEN_VARIABLE")
            result[name] = (handle, obj)
    return subsystem, result


def ensure_component(blueprint, component_class, name, parent_handle):
    subsystem, current = entries(blueprint)
    if name in current:
        if not isinstance(current[name][1], component_class):
            fail(f"{name} has unexpected type")
        return current[name][1], False
    params = unreal.AddNewSubobjectParams(
        parent_handle=parent_handle, new_class=component_class, blueprint_context=blueprint
    )
    handle, reason = subsystem.add_new_subobject(params)
    if not subsystem.rename_subobject(handle, unreal.Text(name)):
        fail(f"Could not add {name}: {reason}")
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if component is None:
        fail(f"Could not resolve {name}")
    return component, True


def ensure_texture(spec):
    source_name, texture_name, *_ = spec
    source = Path(unreal.Paths.project_content_dir()) / "ReEcho" / "Art" / "Scene" / "SC01" / "EdgeInserts" / source_name
    if not source.is_file():
        fail(f"Missing source PNG: {source}")
    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = ART_ROOT
    task.destination_name = texture_name
    task.filename = str(source)
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = load_required(f"{ART_ROOT}/{texture_name}", unreal.Texture2D)
    if (texture.blueprint_get_size_x(), texture.blueprint_get_size_y()) != spec[4]:
        fail(f"Unexpected source dimensions: {source_name}")
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def ensure_master_material():
    material = unreal.EditorAssetLibrary.load_asset(MASTER_MATERIAL_PATH)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_SC01EdgeInsert", MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew()
        )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -450, 0
    )
    sample.set_editor_property("parameter_name", "InsertTexture")
    unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(sample, "A", unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def ensure_material(spec, parent, texture):
    material_name = spec[2]
    path = f"{MATERIAL_ROOT}/{material_name}"
    instance = unreal.EditorAssetLibrary.load_asset(path)
    if instance is None:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            material_name, MATERIAL_ROOT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew()
        )
    instance.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(instance, "InsertTexture", texture)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    return instance


def author_direct_components(sc01, materials):
    _, current = entries(sc01)
    plane = load_required(PLANE_PATH, unreal.StaticMesh)
    for spec in INSERTS:
        parent_name = "Foreground" if spec[3].endswith("_Foreground") else "MidDecoration"
        if parent_name not in current:
            fail(f"SC01 missing inherited {parent_name}")
        component, created = ensure_component(
            sc01, unreal.StaticMeshComponent, spec[3], current[parent_name][0]
        )
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        component.set_cast_shadow(False)
        if created:
            component.set_static_mesh(plane)
            component.set_material(0, materials[spec[3]])
            component.set_translucent_sort_priority(spec[7])
            component.set_editor_property("relative_location", unreal.Vector(*spec[5]))
            component.set_editor_property("relative_rotation", unreal.Rotator(0.0, 90.0, 0.0))
            component.set_editor_property("relative_scale3d", unreal.Vector(*spec[6]))


def component_transform(component):
    return (
        component.get_editor_property("relative_location"),
        component.get_editor_property("relative_rotation"),
        component.get_editor_property("relative_scale3d"),
    )


def configure_arena_editor_defaults(blueprint, profile):
    _, before_entries = entries(blueprint)
    before_collision = {
        name: component_transform(before_entries[name][1]) for name in COLLISION_COMPONENTS
    }
    defaults = unreal.get_default_object(blueprint.generated_class())
    defaults.set_editor_property("auto_layout_collision", False)
    backdrop = before_entries.get("Backdrop")
    if backdrop is None:
        fail(f"{blueprint.get_name()} has no Backdrop component")
    map_material = profile.get_editor_property("map_material")
    if map_material is None:
        fail(f"{profile.get_name()} has no MapMaterial")
    backdrop[1].set_material(0, map_material)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    _, after_entries = entries(blueprint)
    for name, expected in before_collision.items():
        if component_transform(after_entries[name][1]) != expected:
            fail(f"Disabling collision auto-layout changed {blueprint.get_name()}.{name} transform")


def author_scene_registry(blueprints):
    registrations = []
    for scene_id in SCENES:
        registration = unreal.ReEchoArenaSceneRegistration(
            scene_id=unreal.Name(scene_id), arena_class=blueprints[scene_id].generated_class()
        )
        registrations.append(registration)
    for scene_id, blueprint in blueprints.items():
        defaults = unreal.get_default_object(blueprint.generated_class())
        profile = defaults.get_editor_property("scene_profile")
        if profile is None or str(profile.get_editor_property("scene_id")) != scene_id:
            fail(f"{blueprint.get_name()} has no matching SceneProfile")
        defaults.set_editor_property("scene_id", unreal.Name(scene_id))
        defaults.set_editor_property("scene_registry", registrations)
        configure_arena_editor_defaults(blueprint, profile)
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)


def main():
	fail(
		"Retired by Plan134: scene inserts, materials and layout are authored directly by artists "
		"inside BP_ArenaScene_SC01..04. Use the read-only Plan134 scene audit instead."
	)


if __name__ == "__main__":
    main()
