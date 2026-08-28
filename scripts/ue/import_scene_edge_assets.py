"""Import Plan135 edge art and create only missing editable scene components."""

from pathlib import Path

import unreal


SCENES = {
    "SC02": (
        ("上.png", "Top"),
        ("下.png", "Bottom"),
        ("左.png", "Left"),
        ("右.png", "Right"),
    ),
    "SC03": (
        ("上边.png", "Top"),
        ("下边.png", "Bottom"),
        ("右边1.png", "Right01"),
        ("右2.png", "Right02"),
        ("右3.png", "Right03"),
        ("左1.png", "Left01"),
        ("左2.png", "Left02"),
        ("左3.png", "Left03"),
    ),
    "SC04": (
        ("上边.png", "Top"),
        ("下边.png", "Bottom"),
        ("时钟.png", "Clock"),
        ("月亮.png", "Moon"),
        ("右边草.png", "RightGrass01"),
        ("右边草2.png", "RightGrass02"),
        ("左边草 拷贝 2.png", "LeftGrass"),
        ("右边人树.png", "RightFigureTree"),
        ("左边人树.png", "LeftFigureTree"),
        ("左边人树-1.png", "LeftFigureTreeDetail"),
        ("左边花.png", "LeftFlowers"),
        ("左边蘑菇.png", "LeftMushrooms"),
    ),
}

PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
PLANE_PATH = "/Engine/BasicShapes/Plane.Plane"

# Component name, semantic material, parent layer, location, sort priority.
# The plane scale is derived from the source image size, matching SC01's
# height/100 by width/100 convention. Existing components are never mutated.
PLACEMENTS = {
    "SC02": (
        ("Edge_Top_Mid", "Top", "MidDecoration", (2380.0, 0.0, 35.0), 55),
        ("Edge_Bottom_Foreground", "Bottom", "Foreground", (-2450.0, 0.0, 75.0), 90),
        ("Edge_Left_Mid", "Left", "MidDecoration", (0.0, -4100.0, 45.0), 57),
        ("Edge_Right_Mid", "Right", "MidDecoration", (0.0, 4100.0, 45.0), 58),
    ),
    "SC03": (
        ("Edge_Top_Mid", "Top", "MidDecoration", (2380.0, 0.0, 35.0), 55),
        ("Edge_Bottom_Foreground", "Bottom", "Foreground", (-2450.0, 0.0, 75.0), 90),
        ("Edge_Right01_Mid", "Right01", "MidDecoration", (1050.0, 4100.0, 45.0), 58),
        ("Edge_Right02_Mid", "Right02", "MidDecoration", (100.0, 4100.0, 47.0), 59),
        ("Edge_Right03_Mid", "Right03", "MidDecoration", (-850.0, 4100.0, 49.0), 60),
        ("Edge_Left01_Mid", "Left01", "MidDecoration", (1050.0, -4100.0, 45.0), 57),
        ("Edge_Left02_Mid", "Left02", "MidDecoration", (100.0, -4100.0, 47.0), 58),
        ("Edge_Left03_Mid", "Left03", "MidDecoration", (-850.0, -4100.0, 49.0), 59),
    ),
    "SC04": (
        ("Edge_Top_Mid", "Top", "MidDecoration", (2380.0, 0.0, 35.0), 55),
        ("Edge_Bottom_Foreground", "Bottom", "Foreground", (-2450.0, 0.0, 75.0), 90),
        ("Edge_Clock_Mid", "Clock", "MidDecoration", (1550.0, -3350.0, 48.0), 58),
        ("Edge_Moon_Mid", "Moon", "MidDecoration", (1550.0, 3350.0, 48.0), 58),
        ("Edge_RightGrass01_Foreground", "RightGrass01", "Foreground", (-1750.0, 3650.0, 82.0), 92),
        ("Edge_RightGrass02_Foreground", "RightGrass02", "Foreground", (-1100.0, 4100.0, 84.0), 93),
        ("Edge_LeftGrass_Foreground", "LeftGrass", "Foreground", (-1750.0, -3650.0, 82.0), 92),
        ("Edge_RightFigureTree_Mid", "RightFigureTree", "MidDecoration", (150.0, 4000.0, 50.0), 60),
        ("Edge_LeftFigureTree_Mid", "LeftFigureTree", "MidDecoration", (100.0, -4000.0, 50.0), 60),
        ("Edge_LeftFigureTreeDetail_Mid", "LeftFigureTreeDetail", "MidDecoration", (850.0, -4100.0, 52.0), 61),
        ("Edge_LeftFlowers_Foreground", "LeftFlowers", "Foreground", (-1450.0, -3200.0, 86.0), 94),
        ("Edge_LeftMushrooms_Foreground", "LeftMushrooms", "Foreground", (-2050.0, -2850.0, 88.0), 95),
    ),
}


def fail(message):
    raise RuntimeError(f"[Plan135] {message}")


def load_typed(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None or not isinstance(asset, expected_type):
        fail(f"Missing or invalid asset: {path}")
    return asset


def import_new_texture(scene_id, source_name, semantic_name):
    art_root = f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts"
    texture_name = f"T_{scene_id}Edge_{semantic_name}"
    texture_path = f"{art_root}/{texture_name}"
    source_path = (
        Path(unreal.Paths.project_content_dir())
        / "ReEcho" / "Art" / "Scene" / scene_id / "EdgeInserts" / source_name
    )
    if not source_path.is_file():
        fail(f"Missing repository source PNG: {source_path}")
    if unreal.EditorAssetLibrary.does_asset_exist(texture_path):
        return load_typed(texture_path, unreal.Texture2D)

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = art_root
    task.destination_name = texture_name
    task.filename = str(source_path)
    task.replace_existing = False
    task.replace_existing_settings = False
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return load_typed(texture_path, unreal.Texture2D)


def ensure_master_material(scene_id):
    material_root = f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts/Materials"
    material_name = f"M_{scene_id}EdgeInsert"
    material_path = f"{material_root}/{material_name}"
    material = unreal.EditorAssetLibrary.load_asset(material_path)
    if material is not None:
        return load_typed(material_path, unreal.Material)

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        material_name, material_root, unreal.Material, unreal.MaterialFactoryNew()
    )
    if material is None:
        fail(f"Could not create {material_path}")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -450, 0
    )
    sample.set_editor_property("parameter_name", "InsertTexture")
    unreal.MaterialEditingLibrary.connect_material_property(
        sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        sample, "A", unreal.MaterialProperty.MP_OPACITY
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def ensure_material_instance(scene_id, semantic_name, parent, texture):
    material_root = f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts/Materials"
    instance_name = f"MI_{scene_id}Edge_{semantic_name}"
    instance_path = f"{material_root}/{instance_name}"
    instance = unreal.EditorAssetLibrary.load_asset(instance_path)
    if instance is None:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            instance_name,
            material_root,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
        if instance is None:
            fail(f"Could not create {instance_path}")
        instance.set_editor_property("parent", parent)
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            instance, "InsertTexture", texture
        )
        unreal.MaterialEditingLibrary.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    return load_typed(instance_path, unreal.MaterialInstanceConstant)


def component_entries(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if obj is not None:
            result[str(obj.get_name()).removesuffix("_GEN_VARIABLE")] = (handle, obj)
    return subsystem, result


def ensure_direct_component(blueprint, name, parent_name, material, location, scale, sort_priority):
    subsystem, current = component_entries(blueprint)
    if name in current:
        if not isinstance(current[name][1], unreal.StaticMeshComponent):
            fail(f"{blueprint.get_name()}.{name} has unexpected type")
        return False
    if parent_name not in current:
        fail(f"{blueprint.get_name()} missing parent layer {parent_name}")
    params = unreal.AddNewSubobjectParams(
        parent_handle=current[parent_name][0],
        new_class=unreal.StaticMeshComponent,
        blueprint_context=blueprint,
    )
    handle, reason = subsystem.add_new_subobject(params)
    if not subsystem.rename_subobject(handle, unreal.Text(name)):
        fail(f"Could not add {blueprint.get_name()}.{name}: {reason}")
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if component is None:
        fail(f"Could not resolve {blueprint.get_name()}.{name}")
    component.set_static_mesh(load_typed(PLANE_PATH, unreal.StaticMesh))
    component.set_material(0, material)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_cast_shadow(False)
    component.set_translucent_sort_priority(sort_priority)
    component.set_editor_property("relative_location", unreal.Vector(*location))
    component.set_editor_property("relative_rotation", unreal.Rotator(0.0, 90.0, 0.0))
    component.set_editor_property("relative_scale3d", unreal.Vector(*scale))
    return True


def author_scene_components(scene_id, materials):
    blueprint = load_typed(f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}", unreal.Blueprint)
    sizes = {
        semantic_name: (
            load_typed(f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts/T_{scene_id}Edge_{semantic_name}", unreal.Texture2D).blueprint_get_size_x(),
            load_typed(f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts/T_{scene_id}Edge_{semantic_name}", unreal.Texture2D).blueprint_get_size_y(),
        )
        for _, semantic_name in SCENES[scene_id]
    }
    created = 0
    for name, semantic_name, parent_name, location, sort_priority in PLACEMENTS[scene_id]:
        width, height = sizes[semantic_name]
        scale = (height / 100.0, width / 100.0, 1.0)
        if ensure_direct_component(
            blueprint, name, parent_name, materials[semantic_name], location, scale, sort_priority
        ):
            created += 1
    if created:
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return created


def main():
    created_or_verified = 0
    components_created = 0
    for scene_id, entries in SCENES.items():
        art_root = f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts"
        unreal.EditorAssetLibrary.make_directory(art_root)
        unreal.EditorAssetLibrary.make_directory(f"{art_root}/Materials")
        parent = ensure_master_material(scene_id)
        materials = {}
        for source_name, semantic_name in entries:
            texture = import_new_texture(scene_id, source_name, semantic_name)
            materials[semantic_name] = ensure_material_instance(scene_id, semantic_name, parent, texture)
            created_or_verified += 1
        components_created += author_scene_components(scene_id, materials)
    unreal.log(
        f"[Plan135] Imported or verified {created_or_verified} edge art entries; "
        f"created {components_created} missing editable Blueprint components"
    )


if __name__ == "__main__":
    main()
