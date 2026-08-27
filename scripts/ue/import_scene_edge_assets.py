"""Import Plan135 SC02-SC04 edge art without authoring Blueprint components."""

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


def main():
    created_or_verified = 0
    for scene_id, entries in SCENES.items():
        art_root = f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts"
        unreal.EditorAssetLibrary.make_directory(art_root)
        unreal.EditorAssetLibrary.make_directory(f"{art_root}/Materials")
        parent = ensure_master_material(scene_id)
        for source_name, semantic_name in entries:
            texture = import_new_texture(scene_id, source_name, semantic_name)
            ensure_material_instance(scene_id, semantic_name, parent, texture)
            created_or_verified += 1
    unreal.log(f"[Plan135] Imported or verified {created_or_verified} edge art entries; no Blueprint assets touched")


if __name__ == "__main__":
    main()
